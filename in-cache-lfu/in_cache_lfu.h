
#ifndef JY_LFU_CACHE_H
#define JY_LFU_CACHE_H

#include "uthash.h"
#include "utlist.h"
#include "avltree.h"

#define CACHE_HIT 1
#define CACHE_MISS 0


//@Junyao Yang 09/05/2020
//this header file specifies basic LFU implementation prototypes
//the LFU scheme implemented is based on (Ketan Shah, 2010) 
//all items assume to have logical size of 1.(naive LFU doesn't consider item size)
//we use utlist.h macro for DList implementation

struct _List_LFU_Freq_Node_t;
struct _List_LFU_Item_t; 

//Object structs
typedef struct _List_LFU_Item_t {
	uint64_t addrKey; //item's key
	uint32_t size; //size of current item
	struct _List_LFU_Freq_Node_t *freqNode; //all items are stored under freqNode
											//pointer to such Node
	struct _List_LFU_Item_t *next; //used in Utlist
	struct _List_LFU_Item_t *prev;
	//this is the hash handler for uthash, allow O(1) lookup
	UT_hash_handle list_lfu_hh; 
	

#if defined(PERFECT_LFU) || defined(FAST_PERFECT_LFU)
	uint64_t freq;
	UT_hash_handle evict_hh;
#endif

} List_LFU_Item_t;


typedef struct _List_LFU_Freq_Node_t {
	
	List_LFU_Item_t *head; //head of item list, for utlist must init to NULL
	uint32_t size; //number of item under curr node
	uint32_t freq; //frequency represented by curr node
	
	struct _List_LFU_Freq_Node_t *next; //UTlist pointer
	struct _List_LFU_Freq_Node_t *prev;

	/****add avl struct****/
#ifdef FAST_PERFECT_LFU
	struct avl_node avl;
#endif

} List_LFU_Freq_Node_t;


typedef struct _LFU_Cache_t {
	//these two parameter is not required, just additional stat about trace
	uint32_t totKey;//unqiue keys
	uint32_t totRef;//num of refs

	uint32_t totUniqueFreq; //total number of frequency nodes in the cache

	uint32_t currSize; //number of item in curr cache
	uint32_t capacity; // the size of cache

#if defined(PERFECT_LFU) || defined(FAST_PERFECT_LFU)
	List_LFU_Item_t *Evicted_HashItems; //use to store evicted item 
#endif

#ifdef FAST_PERFECT_LFU
	struct avl_tree tree; //use for fast non 1 frequency insertion
#endif /*FAST_PERFECT_LFU*/

	
	List_LFU_Item_t *HashItems; //head of Uthash, must init to NULL
	List_LFU_Freq_Node_t *FreqList; // head of freqList, must init to NULL for Utlist
} LFU_Cache_t;


//public interfaces

//return LFU_Cache pointer on sucess
//cap: cache capacity
LFU_Cache_t* cacheInit(uint32_t cap);

//free all allocated memory
void cacheFree(LFU_Cache_t* cache);

//return LFU_HIT if an access is hit
//return LFU_MISS if an access is miss
uint8_t access(LFU_Cache_t* cache, uint64_t key, uint32_t size);



//private helpers

void evictItem(LFU_Cache_t* cache);
//this method will add newly created item to the cache,
void addItem(LFU_Cache_t* cache, List_LFU_Item_t* item);
void updateItem(LFU_Cache_t* cache, List_LFU_Item_t* item);
//for now only need key, later, depend on the LFU variation, we might have to add more
List_LFU_Item_t* createItem(uint64_t key, uint32_t size);
List_LFU_Item_t* findItem(LFU_Cache_t* cache, uint64_t key);
List_LFU_Freq_Node_t* newFreqListNode(uint32_t freq);
List_LFU_Item_t* newLFUListItem(uint64_t addrKey, uint32_t size);


#endif /*JY_LFU_CACHE_H*/