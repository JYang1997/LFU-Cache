
#include "lfu_cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#ifdef FAST_PERFECT_LFU
int cmp_func(struct avl_node *a, struct avl_node *b, void *aux) {
	List_LFU_Freq_Node_t *aa, *bb;
    aa = _get_entry(a, List_LFU_Freq_Node_t, avl);
    bb = _get_entry(b, List_LFU_Freq_Node_t, avl);

    if (aa->freq < bb->freq) return -1;
    else if (aa->freq > bb->freq) return 1;
    else return 0;
}
#endif

LFU_Cache_t* cacheInit(uint32_t cap) {

	LFU_Cache_t* cache = malloc(sizeof(LFU_Cache_t));
	if (cache == NULL) {
		perror("cache malloc failed!");
		exit(-1);
	}
	cache->totKey = 0;
	cache->totRef = 0;
	cache->totUniqueFreq = 0;
	cache->currSize = 0;
	cache->capacity = cap;
	cache->HashItems = NULL; //required for UThash
	cache->FreqList= NULL; //required for UTlist

#ifdef PERFECT_LFU
	cache->Evicted_HashItems = NULL; //required for hashtable for evicted items
#endif 

	assert(cache != NULL);
	return cache;
}


void cacheFree(LFU_Cache_t* cache) {
	//this will be last function to be implement
	List_LFU_Freq_Node_t *head, *elt, *tmp;
	List_LFU_Item_t *ihead, *ielt, *itmp; 

	head = cache->FreqList;

	DL_FOREACH_SAFE(head,elt,tmp) {
		ihead = elt->head;
		DL_FOREACH_SAFE(ihead,ielt, itmp) {

			DL_DELETE(ihead,ielt);
			HASH_DELETE(list_lfu_hh, cache->HashItems, ielt);
			free(ielt);
		}

		DL_DELETE(head, elt);
		free(elt);
	}

#ifdef PERFECT_LFU	
	ielt = NULL;
	itmp = NULL;
	HASH_ITER(evict_hh, cache->Evicted_HashItems, ielt, itmp) {
		HASH_DELETE(evict_hh, cache->Evicted_HashItems, ielt); /* delete; users advances to next */
		free(ielt);             /* optional- if you want to free  */
	}
#endif

	free(cache);

}


List_LFU_Freq_Node_t* newFreqListNode(uint32_t freq) {
	List_LFU_Freq_Node_t* ret;
	ret = malloc(sizeof(List_LFU_Freq_Node_t));
	if (ret == NULL) {
		perror("freq node allocation failed.");
		exit(-1);
	}
	ret->head = NULL;
	ret->size = 0;
	ret->freq = freq;
	return ret;
}

List_LFU_Item_t* newLFUListItem(uint64_t addrKey, uint32_t size) {
	List_LFU_Item_t* ret;
	ret = malloc(sizeof(List_LFU_Item_t));
	if (ret == NULL) {
		perror("item allocation failed");
		exit(-1);
	}

	ret->addrKey = addrKey;
	ret->size = size;
	ret->freqNode = NULL;
#if defined(PERFECT_LFU) || defined(FAST_PERFECT_LFU)
	ret->freq = 1;
#endif
	return ret;
}




List_LFU_Item_t* createItem(uint64_t key, uint32_t size) {

	return newLFUListItem(key, size);
}

List_LFU_Item_t* findItem(LFU_Cache_t* cache, uint64_t key) {
	List_LFU_Item_t *ret;
	HASH_FIND(list_lfu_hh, cache->HashItems, &key, sizeof(uint64_t), ret);
	return ret;
}


void updateItem(LFU_Cache_t* cache, List_LFU_Item_t* item) {
	assert(item != NULL);
	assert(item->freqNode != NULL);
	assert(cache != NULL);
	
	List_LFU_Freq_Node_t* prev_freqNode = item->freqNode;
	uint32_t new_freq = prev_freqNode->freq + 1;
	
	List_LFU_Freq_Node_t* new_freqNode = prev_freqNode->next;
	//if first statement is correct second should not be evaluated
	if(new_freqNode == NULL || (new_freqNode->freq != new_freq)) {
		//next node doesnt exist 
		new_freqNode = newFreqListNode(new_freq);
		DL_APPEND_ELEM(cache->FreqList, prev_freqNode, new_freqNode);
		cache->totUniqueFreq++;
	} 
	//next node already exist
	DL_DELETE(prev_freqNode->head, item);
	prev_freqNode->size--;
	if(prev_freqNode->size <= 0) {
		DL_DELETE(cache->FreqList, prev_freqNode);
		free(prev_freqNode);
	}
	//add item to the freq node's list's tail, 
	//(when evict, we start from head of the list to break tie by recency)
	DL_APPEND(new_freqNode->head, item);
	item->freqNode = new_freqNode;
	new_freqNode->size++;
}


void addItem(LFU_Cache_t* cache, List_LFU_Item_t* item) {
	assert(cache != NULL && item != NULL);


#ifdef PERFECT_LFU
	//this will be last function to be implement
	List_LFU_Freq_Node_t *head, *elt, *tmp;

	head = cache->FreqList;

	int flag = 1;

	DL_FOREACH_SAFE(head,elt,tmp) {
		if(elt->freq == item->freq) {
			DL_APPEND(elt->head, item); //add item to such node
			elt->size++;
			flag = 0;
			break;
		} else if(elt->freq > item->freq) {
			List_LFU_Freq_Node_t * freqNode = newFreqListNode(item->freq);
			DL_PREPEND_ELEM(cache->FreqList,elt ,freqNode);
			cache->totUniqueFreq++;

			DL_APPEND(freqNode->head, item);
			freqNode->size++;
			flag = 0;
			break;
		}
	}

	if (flag == 1) {
		List_LFU_Freq_Node_t * freqNode = newFreqListNode(item->freq);
			DL_APPEND(cache->FreqList, freqNode);
			cache->totUniqueFreq++;

			DL_APPEND(freqNode->head, item);
			freqNode->size++;
	}
#elif FAST_PERFECT_LFU
	//insert into avl tree
	//first you want to search see if it exist
	//if it does not exist than find smallest key greater than it
	//if still not exist append.
	struct avl_node *cur;
	List_LFU_Freq_Node_t *node, query;
	node = NULL;
	query.freq = item->freq;
	cur = avl_search_greater(&(cache->tree), &query.avl, cmp_func);
	if (cur != NULL) { 
		node = _get_entry(cur, List_LFU_Freq_Node_t, avl);
		if (node->freq == item->freq) {
			DL_APPEND(node->head, item); //add item to such node
			node->size++;
		} else {
			List_LFU_Freq_Node_t * freqNode = newFreqListNode(item->freq);
			DL_PREPEND_ELEM(cache->FreqList,node ,freqNode);
			avl_insert(&(cache->tree), &freqNode->avl, cmp_func);
			cache->totUniqueFreq++;

			DL_APPEND(freqNode->head, item);
			freqNode->size++;
		}
	} else {//freqNode is not in the cache
		//add new node to the end
		//inser it yo avl tree
		List_LFU_Freq_Node_t * freqNode = newFreqListNode(item->freq);
		DL_APPEND(cache->FreqList, freqNode);
		avl_insert(&(cache->tree), &freqNode->avl, cmp_func);
		cache->totUniqueFreq++;

		DL_APPEND(freqNode->head, item);
		freqNode->size++;
	}

#else
	//begin insert item into frequency 1 node
	//if such node does not exist create one
	List_LFU_Freq_Node_t * freq1Node = cache->FreqList;
	if (cache->FreqList == NULL || cache->FreqList->freq != 1) {
		freq1Node = newFreqListNode(1);
		//prepend the node to the beginning of the freqlist
		DL_PREPEND(cache->FreqList, freq1Node);
		cache->totUniqueFreq++;
	}
	//point to the parent freqNode
	item->freqNode = freq1Node;
	//from here, insert the item to freq list
	DL_APPEND(freq1Node->head, item);
	freq1Node->size++;
#endif /*PERFECT_LFU*/

	HASH_ADD(list_lfu_hh, cache->HashItems, addrKey, sizeof(uint64_t), item);
}

void evictItem(LFU_Cache_t* cache, uint32_t newItemSize) {
	//the evicted item should be item in the head of item list
	//in the head of freq list
	assert(cache != NULL);
	
	assert(cache->FreqList != NULL);
	assert(cache->FreqList->head != NULL);
	//consider merge this part
	while (cache->FreqList->head != NULL 
		&& cache->currSize+newItemSize < cache->capacity) {
		
		List_LFU_Item_t* del = cache->FreqList->head;
		DL_DELETE(cache->FreqList->head, del);
		HASH_DELETE(list_lfu_hh, cache->HashItems, del);

#if defined(PERFECT_LFU) || defined(FAST_PERFECT_LFU)
		HASH_ADD(evict_hh, cache->Evicted_HashItems, addrKey, sizeof(uint64_t), del);
#else
		free(del);
#endif	

		cache->FreqList->size--;
		if(cache->FreqList->size <= 0) {
			List_LFU_Freq_Node_t* tmp = cache->FreqList;
			DL_DELETE(cache->FreqList, tmp); //head node
#ifdef FAST_PERFECT_LFU
			//remove and rebalance freq node
			avl_remove(&(cache->tree), &(tmp->avl));
#endif
			free(tmp);
			cache->totUniqueFreq--;

		}
	}

	if (cache->totUniqueFreq == 0 && cache->currSize+newItemSize < cache->capacity) {
		perror("cache eviction: not enough memory error!\n");
		exit(-1);
	}

}

List_LFU_Item_t* getEvictedItem(LFU_Cache_t* cache, uint64_t key) {

#if defined(PERFECT_LFU) || defined(FAST_PERFECT_LFU)
	List_LFU_Item_t *ret;
	HASH_FIND(evict_hh, cache->Evicted_HashItems, &key, sizeof(uint64_t), ret);
	if (ret != NULL) {
		HASH_DELETE(evict_hh, cache->Evicted_HashItems, ret);
		ret->freq += 1;
	}
	return ret;
#endif
	return NULL;
}


uint8_t access(LFU_Cache_t* cache, uint64_t key, uint32_t size) {
	assert(cache != NULL);
	cache->totRef++;

	List_LFU_Item_t* item = NULL;
	item = findItem(cache, key);
	if (item != NULL) {
		updateItem(cache, item);
		return CACHE_HIT;
	} else {
		//check if item is in the evicted table
		item = getEvictedItem(cache, key);
		
		if(item == NULL) {
			item = createItem(key, size); //increment totKEY
			cache->totKey++;
		}
		
		if (cache->currSize+item->size < cache->capacity)
			cache->currSize += item->size;
	 	else {
	 		if(cache->capacity < size) {
	 			perror("cache too small to fit item.\n");
	 			exit(-1);
	 		}
			evictItem(cache, item->size);
	 	}
		
		addItem(cache, item);
		return CACHE_MISS;
	}
}


