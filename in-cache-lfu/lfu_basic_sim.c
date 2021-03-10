
#include "lfu_basic_sim.h"
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

	cache->currSize = 0;
	cache->capacity = cap;
	cache->HashItems = NULL; //required for UThash
	cache->FreqList= NULL; //required for UTlist

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

List_LFU_Item_t* newLFUListItem(uint64_t addrKey) {
	List_LFU_Item_t* ret;
	ret = malloc(sizeof(List_LFU_Item_t));
	if (ret == NULL) {
		perror("item allocation failed");
		exit(-1);
	}

	ret->addrKey = addrKey;
	ret->freqNode = NULL;
	return ret;
}




List_LFU_Item_t* createItem(uint64_t key) {

	return newLFUListItem(key);
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
	//begin insert item into frequency 1 node
	//if such node does not exist create one
	List_LFU_Freq_Node_t * freq1Node = cache->FreqList;
	if (cache->FreqList == NULL || cache->FreqList->freq != 1) {
		freq1Node = newFreqListNode(1);
		//prepend the node to the beginning of the freqlist
		DL_PREPEND(cache->FreqList, freq1Node);
	}
	//point to the parent freqNode
	item->freqNode = freq1Node;
	//from here, insert the item to freq list
	DL_APPEND(freq1Node->head, item);
	freq1Node->size++;
	HASH_ADD(list_lfu_hh, cache->HashItems, addrKey, sizeof(uint64_t), item);

}

void evictItem(LFU_Cache_t* cache) {
	//the evicted item should be item in the head of item list
	//in the head of freq list
	assert(cache != NULL);
	
	assert(cache->FreqList != NULL);
	assert(cache->FreqList->head != NULL);
	//consider merge this part
	List_LFU_Item_t* del = cache->FreqList->head;
	DL_DELETE(cache->FreqList->head, del);
	HASH_DELETE(list_lfu_hh, cache->HashItems, del);
	free(del);

	cache->FreqList->size--;
	if(cache->FreqList->size <= 0) {
		List_LFU_Freq_Node_t* tmp = cache->FreqList;
		DL_DELETE(cache->FreqList, tmp); //head node
		free(tmp);
	}

}

uint8_t access(LFU_Cache_t* cache, uint64_t key) {
	assert(cache != NULL);
	cache->totRef++;

	List_LFU_Item_t* curr = NULL;
	curr = findItem(cache, key);
	if (curr != NULL) {
		updateItem(cache, curr);
		return CACHE_HIT;
	} else {
		curr = createItem(key); //increment totKEY
		cache->totKey++;
		if (cache->currSize < cache->capacity)
			cache->currSize++;
	 	else 
			evictItem(cache);
		
		addItem(cache, curr);
		return CACHE_MISS;
	}
}