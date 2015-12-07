#include "filesys/cache.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"

void cache_init(void) // initializes buffer cache
{
	list_init(&cache);
	lock_init(&global_cache_lock);
}

struct cache_entry * cache_read_sector (block_sector_t sector)
{
	struct cache_entry *entry;
	struct list_elem *elem = list_begin(&cache);

	/* This loop checks for cache hit */
	while (elem != list_end(&cache)) {
		entry = list_entry(elem, struct cache_entry, cache_list_elem);
		if (entry->block_sector == sector) {
			entry->pin = 1;
			return entry;
		}
		elem = list_next(elem);
	}


	lock_acquire(&global_cache_lock);
	/* Cache miss */
	struct cache_entry *new_entry = malloc(sizeof(struct cache_entry));
	block_read(fs_device, sector, new_entry->data);
	
	/* if cache size < max possible:
		init new cache_entry and append to list
		copy from disk to new entry
		cache_size++ */
	if (cache_size < 64) {
		new_entry->block_sector = sector;
		new_entry->pin = 1;
		new_entry->dirty = false;
		new_entry->threads_reading = 0;
		lock_init(&(new_entry->cache_entry_lock));
		list_push_back(&cache, &(new_entry->cache_list_elem));
		cache_size++;
		if (clock_hand_elem == NULL) { 		// only happens on the first time;
			clock_hand_elem = list_end(&cache);
		}
	} else {

	/* 
	else:
		evict() with clock
		copy from disk to evicted entry space
	*/

	}

	/*
	return the sector read
	*/



	free(new_entry);
	lock_release(&global_cache_lock);
	return NULL;
}

void cache_clock_evict_and_replace (block_sector_t new_sector) {

	struct cache_entry *temp_cache_entry; 
	while (true) {
		// access cache entry that clock is pointing at
		temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
		// lock the cache_entry we are checking
		lock_acquire(&(temp_cache_entry->cache_entry_lock));

		if (temp_cache_entry->pin == 0) {
			// writeback if dirty
			if (temp_cache_entry->dirty == true) {
				cache_flush_clock_entry();
			}
			// read new sector from disk
			cache_read_sector_to_clock_sector(new_sector);
			// advance clock pointer
			clock_hand_elem = wrapping_list_next(clock_hand_elem);
			lock_release(&(temp_cache_entry->cache_entry_lock));
			break;
		} else {
			temp_cache_entry->pin = 0;
			// advance clock pointer
			clock_hand_elem = wrapping_list_next(clock_hand_elem);

			// unlock the cache_entry we are checking
			lock_release(&(temp_cache_entry->cache_entry_lock));
		}
	}
}


struct list_elem * wrapping_list_next(struct list_elem *elem) {
	elem = list_next(elem);
	if (elem == list_tail(&cache)) {
		elem = list_begin(&cache);
	}
	return elem;
}

// called & locked from cache_clock_evict_and_replace
void cache_flush_clock_entry(void) {
	struct cache_entry *temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
	block_write(fs_device, temp_cache_entry->block_sector, temp_cache_entry->data);
}

// called & locked from cache_clock_evict_and_replace
void cache_read_sector_to_clock_sector(block_sector_t new_sector) {
	struct cache_entry *temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
	block_read(fs_device, new_sector, temp_cache_entry->data);
	temp_cache_entry->dirty = false;
	temp_cache_entry->pin = 1;
	temp_cache_entry->block_sector = new_sector;
}

// void cache_write (block_sector_t sector, int size, int offset, void* buffer) // buffer cache is always writeback, don't need separate method for writeback
// {

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

