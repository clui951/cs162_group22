#include "threads/synch.h"
#include "devices/block.h"
#include <list.h>


struct list cache;
struct lock global_cache_lock; 		/* Global lock for cache operations. */
int cache_size = 0;
struct list_elem *clock_hand_elem = NULL;		/* Sector that the current clock hand is pointing to */

struct cache_entry {
	int threads_reading;		/* Number of threads reading from this cache entry. */
	bool dirty; 			/* True if cache has been written to. */
	int pin;			/* Pin value for clock replacement. */
	block_sector_t block_sector;			/* The number of the cache entry’s sector. */
	struct lock cache_entry_lock;		/* Lock specific to each cache for synchronization. */
	struct list_elem cache_list_elem;		/* Used to make cache entry a member of a struct list */
	uint8_t data[BLOCK_SECTOR_SIZE];		/* List to hold the data of the cache entry */
};

void cache_init(void); // initializes buffer cache
uint8_t * cache_read_sector (block_sector_t sector);
void cache_write_sector (block_sector_t sector, void* buffer); // buffer cache is always writeback, don't need separate method for writeback
// void cache_allocate (block_sector_t sector); // not sure if we need this. use case: initialize an entry in the cache table
// void cache_add (block_sector_t sector); // adding in a new sector, will use evict to evict a sector if necessary
// void cache_evict (); // second chance algorithm! evicts a block.
// void cache_flush_all (); // destroys cache table and writes every sector back to disk. flushes all dirty buffers to disk
// void cache_flush_entry (struct cache_entry* entry); // used in second chance algorithm. we could use this in cache_flush.
// void cache_load_entry (block_sector_t sector, struct cache_entry* entry); // used in second chance algorithm. loads an entry from disk
// void cache_find (block_sector_t sector);
// void cache_readahead (block_sector_t sector);
struct cache_entry * cache_clock_evict_and_replace (block_sector_t new_sector);
struct list_elem * wrapping_list_next(struct list_elem *elem);
void cache_flush_clock_entry(void);
void cache_read_sector_to_clock_sector(block_sector_t new_sector);

// #endif /* filesys/cache.h */
