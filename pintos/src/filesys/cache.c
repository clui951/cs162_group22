#include <stdio.h>
#include <string.h>
#include "filesys/cache.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"

void cache_init (void)
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
			entry->pin = 1;
			lock_release(&global_cache_lock);
			return entry;
		}
		elem = list_next(elem);
	}

	/* Cache miss

		if cache size < max possible:
		init new cache_entry and append to list
		copy from disk to new entry
		cache_size++ */
	struct cache_entry *return_entry;
	if (cache_size < 64) {
		return_entry = malloc(sizeof(struct cache_entry));
		block_read(fs_device, sector, &return_entry->data);
		return_entry->block_sector = sector;
		return_entry->pin = 1;
		return_entry->dirty = false;
		return_entry->threads_reading = 0;
		list_push_back(&cache, &return_entry->cache_list_elem);
		cache_size++;
		if (clock_hand_elem == NULL) {
			clock_hand_elem = list_back(&cache);
		}
	} else {
		return_entry = cache_clock_evict_and_replace(sector);
	}
	lock_release(&global_cache_lock);
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
		temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);

		if (temp_cache_entry->pin == 0) {
			if (temp_cache_entry->dirty == true) {
				cache_flush_clock_entry ();
			}
			cache_read_sector_to_clock_sector (new_sector);
			break;
		} else {
			temp_cache_entry->pin = 0;
		}
		increment_clock ();
	}
	increment_clock ();
	return temp_cache_entry;
}

/*
advances through cache and wraps around from end to beginning of list
*/
struct list_elem * wrapping_list_next (struct list_elem *elem) {
	if (elem == list_end(&cache)) {
		struct list_elem *start = list_begin(&cache);
		return start;
	}
	elem = list_next(elem);
	return elem;
}

void increment_clock (void) {
	if (clock_hand_elem == list_back(&cache)) {
		clock_hand_elem = list_begin(&cache);
	} else {
		clock_hand_elem = list_next(clock_hand_elem);
	}
}

/*
writes cache_entry indicated by clock_hand_elem to disk
*/
void cache_flush_clock_entry (void) {
	struct cache_entry *temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
	block_write(fs_device, temp_cache_entry->block_sector, &temp_cache_entry->data);
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
		elem = list_next(elem);
	}
	while (!list_empty(&cache)) {
		struct list_elem *removed = list_pop_front(&cache);
		free(list_entry(removed, struct cache_entry, cache_list_elem));
	}
}

/*
reads cache_entry from new_sector on disk to clock_hand_elem
*/
void cache_read_sector_to_clock_sector(block_sector_t new_sector) {
	struct cache_entry *temp_cache_entry = list_entry(clock_hand_elem, struct cache_entry, cache_list_elem);
	block_read(fs_device, new_sector, &temp_cache_entry->data);
	temp_cache_entry->dirty = false;
	temp_cache_entry->pin = 1;
	temp_cache_entry->block_sector = new_sector;
}
