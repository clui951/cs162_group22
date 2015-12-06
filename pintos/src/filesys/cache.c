void cache_init (void) // initializes buffer cache
{

}

void cache_read (block_sector_t sector, int size, int offset, void* buffer)
{

}

void cache_write (block_sector_t sector, int size, int offset, void* buffer) // buffer cache is always writeback, don't need separate method for writeback
{

}

void cache_allocate (block_sector_t sector) // not sure if we need this. use case: initialize an entry in the cache table
{

}

void cache_add (block_sector_t sector) // adding in a new sector, will use evict to evict a sector if necessary
{

}

void cache_evict () // second chance algorithm! evicts a block.
{

}

void cache_flush_all () // destroys cache table and writes every sector back to disk. flushes all dirty buffers to disk
{

}

void cache_flush_entry (struct cache_entry* entry) // used in second chance algorithm. we could use this in cache_flush.
{

}

void cache_load_entry (block_sector_t sector, struct cache_entry* entry) // used in second chance algorithm. loads an entry from disk
{

}

void cache_find (block_sector_t sector)
{

}

void cache_readahead (block_sector_t sector)
{

}

