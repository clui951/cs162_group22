#include "filesys/cache.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"

void cache_init(void) // initializes buffer cache
{
	list_init(&cache);
	lock_init(&global_cache_lock);
}

struct cache_entry * cache_read (block_sector_t sector, int size, int offset, void* buffer)
{
	lock_acquire(&global_cache_lock);
	struct cache_entry *entry;
	struct list_elem *elem = list_begin(&cache);

	/* This loop checks for cache hit */
	while (elem != list_end(&cache)) {
		entry = list_entry(elem, struct cache_entry, cache_list_elem);
		if (entry->block_sector == sector) {
			entry->pin = 1;
			lock_release(&global_cache_lock);
			return entry;
		}
		elem = list_next(elem);
	}

	/* Cache miss */
	struct cache_entry *new_entry = malloc(sizeof(struct cache_entry));
	block_read(fs_device, sector, new_entry->data);
	
	if (cache_size < 64) {
		new_entry->block_sector = sector;
		new_entry->pin = 1;
		new_entry->dirty = false;
		new_entry->threads_reading = 0;
		lock_init(&(new_entry->cache_lock));
		list_push_back(&cache, &(new_entry->cache_list_elem));
		cache_size++;
	} else if (cache_size == 64) {

	} else {
		printf("rekt\n");
	}
	/* 
	if cache size < max possible:
		init new cache_entry and append to list
		copy from disk to new entry
		cache_size++
	else:
		evict() with clock
		copy from disk to evicted entry space

	return the sector read
	*/
	free(new_entry);
	return NULL;
}

void clock_evict_and_replace (block_sector_t new_sector) {

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

// void cache_flush_entry (struct cache_entry* entry) // used in second chance algorithm. we could use this in cache_flush.
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

