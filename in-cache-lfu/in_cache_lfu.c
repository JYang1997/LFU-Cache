
#include "lfu_cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


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
	
#elif
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
	HASH_ADD(list_lfu_hh, cache->HashItems, addrKey, sizeof(uint64_t), item);

#endif /*PERFECT_LFU*/

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

#ifdef PERFECT_LFU
		HASH_ADD(evict_hh, cache->Evicted_HashItems, addrKey, sizeof(uint64_t), del);
#elif
		free(del);
#endif	

		cache->FreqList->size--;
		if(cache->FreqList->size <= 0) {
			List_LFU_Freq_Node_t* tmp = cache->FreqList;
			DL_DELETE(cache->FreqList, tmp); //head node
			
			free(tmp);
			cache->totUniqueFreq--;

		}
	}

	if (cache->totUniqueFreq == 0 && cache->currSize+newItemSize < cache->capacity) {
		perror("cache eviction: not enough memory error!\n")
		exit(-1);
	}

}

List_LFU_Item_t* getEvictedItem(LFU_Cache_t* cache, uint64_t key) {

#ifdef PERFECT_LFU
	List_LFU_Item_t *ret;
	HASH_FIND(evict_hh, cache->Evicted_HashItems, &key, sizeof(uint64_t), ret);
	HASH_DELETE(evict_hh, cache->Evicted_HashItems, ret);
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
	 			perror("cache too small to fit item.\n")
	 			exit(-1)
	 		}
			evictItem(cache, item->size);
	 	}
		
		addItem(cache, item);
		return CACHE_MISS;
	}
}