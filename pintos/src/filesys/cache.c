#include <stdio.h>
#include <string.h>
#include "filesys/cache.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "threads/thread.h"

void cache_init (void) // initializes buffer cache
{
	list_init(&cache);
	lock_init(&global_cache_lock);
	cache_size = 0;
}

struct cache_entry * cache_get_entry (block_sector_t sector)
{
	lock_acquire(&global_cache_lock);

	struct cache_entry *entry;
	struct list_elem *elem = list_begin(&cache);

	/* This loop checks for cache hit */
	while (elem != list_end(&cache)) {
		entry = list_entry(elem, struct cache_entry, cache_list_elem);
		if (entry->block_sector == sector) {
			if (entry->pin == 0) {
				entry->pin = 1;
			}
			// printf("%d what1\n", thread_current()->tid);
			lock_release(&global_cache_lock);
			// printf("%d yeah1\n", thread_current()->tid);
			return entry;
		}
		elem = list_next(elem);
	}
	
	/* Cache miss */

	/* if cache size < max possible:
		init new cache_entry and append to list
		copy from disk to new entry
		cache_size++ */
	struct cache_entry *return_entry = NULL;
	if (cache_size < 64) {
		struct cache_entry *new_entry = malloc(sizeof(struct cache_entry));
		block_read(fs_device, sector, new_entry->data);
		new_entry->block_sector = sector;
		new_entry->pin = 1;
		new_entry->dirty = false;
		new_entry->threads_reading = 0;
		// lock_init(&(new_entry->cache_entry_lock));
		list_push_back(&cache, &(new_entry->cache_list_elem));
		return_entry = new_entry;
		cache_size++;
		if (clock_hand_elem == NULL) { 		// only happens on the first time;
			clock_hand_elem = list_end(&cache);
		}
	} else {
		return_entry = cache_clock_evict_and_replace(sector);
	}
	printf("%d what2\n", thread_current()->tid);
	lock_release(&global_cache_lock);
	printf("%d yeah2\n", thread_current()->tid);
	return return_entry;
}


/* 
Clock algorithm: searches for next cache entry to evict, while advancing clockhand 
replaces it with new_sector, which is read from disk 
returns cache_entry that was evicted/replaced
*/
struct cache_entry * cache_clock_evict_and_replace (block_sector_t new_sector) {

	struct cache_entry *temp_cache_entry; 
	while (true) {
		// access cache entry that clock is pointing at
		temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);

		if (temp_cache_entry->pin == 0) {
			// writeback if dirty
			if (temp_cache_entry->dirty == true) {
				cache_flush_clock_entry();
			}
			// read new sector from disk
			cache_read_sector_to_clock_sector(new_sector);
			break;
		} else {
			temp_cache_entry->pin = 0;
		}
		clock_hand_elem = wrapping_list_next(clock_hand_elem);
	}
	return temp_cache_entry;
}


/* 
advances through cache and wraps around from end to beginning of list
*/
struct list_elem * wrapping_list_next (struct list_elem *elem) {
	if (elem == list_end(&cache)) {
		elem = list_begin(&cache);
	} else {
		elem = list_next(elem);
	}
	return elem;
}


/* 
writes cache_entry indicated by clock_hand_elem to disk
*/
void cache_flush_clock_entry (void) {
	struct cache_entry *temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
	block_write(fs_device, temp_cache_entry->block_sector, temp_cache_entry->data);
}

/*
flush all entries in the cache
*/
void cache_flush_all (void) {
	struct cache_entry *entry;
	struct list_elem *elem = list_begin(&cache);

	/* This loop checks for cache hit */
	while (elem != list_end(&cache)) {
		entry = list_entry(elem, struct cache_entry, cache_list_elem);
		if (entry->dirty == true) {
			block_write(fs_device, entry->block_sector, entry->data);
		}
		free(entry);
		elem = list_next(elem);
	}
}


/* 
reads cache_entry from new_sector on disk to clock_hand_elem
*/
void cache_read_sector_to_clock_sector(block_sector_t new_sector) {
	struct cache_entry *temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
	block_read(fs_device, new_sector, temp_cache_entry->data);
	temp_cache_entry->dirty = false;
	temp_cache_entry->pin = 1;
	temp_cache_entry->block_sector = new_sector;
}















// /*
// don't need write
// */
// void cache_write_sector (block_sector_t sector, void* buffer) { // buffer cache is always writeback, don't need separate method for writeback
	
// 	lock_acquire(&global_cache_lock);

// 	struct cache_entry *entry;
// 	struct list_elem *elem = list_begin(&cache);

// 	/* This loop checks for cache hit */
// 	while (elem != list_end(&cache)) {
// 		entry = list_entry(elem, struct cache_entry, cache_list_elem);
// 		if (entry->block_sector == sector) {
// 			memcpy(entry->data, buffer, BLOCK_SECTOR_SIZE);
// 			entry->pin = 1;
// 			return;
// 		}
// 		elem = list_next(elem);
// 	}

// 	if (cache_size < 64) {
// 		struct cache_entry *new_entry = malloc(sizeof(struct cache_entry));
// 		// block_read(fs_device, sector, new_entry->data);
// 		new_entry->block_sector = sector;
// 		new_entry->pin = 1;
// 		new_entry->dirty = true;
// 		new_entry->threads_reading = 0;
// 		memcpy(new_entry->data, buffer, BLOCK_SECTOR_SIZE);
// 		lock_init(&(new_entry->cache_entry_lock));
// 		list_push_back(&cache, &(new_entry->cache_list_elem));
// 		cache_size++;
// 		// write data to entry now
// 		if (clock_hand_elem == NULL) { 		// only happens on the first time;
// 			clock_hand_elem = list_end(&cache);
// 		}
// 	} else {
// 		// evict entry (flush, clock alg)
// 		struct cache_entry *write_entry = cache_clock_evict_and_replace(sector);
// 		// write data to entry just flushed and update sector # 
// 		memcpy(write_entry->data, buffer, BLOCK_SECTOR_SIZE);
// 		write_entry->pin = 1;

// 	}

// 	lock_release(&global_cache_lock);

// }













// void cache_allocate (block_sector_t sector) // not sure if we need this. use case: initialize an entry in the cache table
// {

// }

// void cache_add (block_sector_t sector) // adding in a new sector, will use evict to evict a sector if necessary
// {

// }

// void cache_evict () // second chance algorithm! evicts a block.
// {

// }

// void cache_flush_all () // destroys cache table and writes every sector back to disk. flushes all dirty buffers to disk
// {

// }

// void cache_load_entry (block_sector_t sector, struct cache_entry* entry) // used in second chance algorithm. loads an entry from disk
// {

// }

// void cache_find (block_sector_t sector)
// {

// }

// void cache_readahead (block_sector_t sector)
// {

// }

