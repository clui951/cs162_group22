#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"
#include "threads/synch.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44
#define MAX_FILESIZE 8388608
#define DIR_PTRS 122
#define NUM_INDIR_PTRS 1
#define NUM_DOUBLE_INDIR_PTRS 1
#define SIZE_INDIR_ARRAY 128

struct inode_index *inode_index (off_t pos);

/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    // block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    bool is_dir;                        /* True if directory. */
    block_sector_t direct[DIR_PTRS];
    block_sector_t indirect;
    block_sector_t doubly_indirect;
    block_sector_t parent;
  };

struct inode_index
  {
    int indirection;
    off_t index;
    off_t double_index;
    bool valid;
  };

struct block_of_pointers
  {
    block_sector_t pointers[128];
  };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, BLOCK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
    struct lock inode_lock;             /* Lock in order to operate on inode. */
  };

/* Returns the block device sector that contains byte offset POS
   within INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static block_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  if (pos < inode->data.length) {
    struct inode_index *index = inode_index(pos);
    int ret = -1;
    if (index->valid) {
      if (index->indirection == 0) {
        ret = inode->data.direct[index->index];
      } else if (index->indirection == 1) {
        struct block_of_pointers *pointers;
        pointers = (struct block_of_pointers *) cache_get_entry(inode->data.indirect)->data;
        ret = pointers->pointers[index->index];
       } else { // double indirect
        struct block_of_pointers *pointers;
        pointers = (struct block_of_pointers *) cache_get_entry(inode->data.doubly_indirect)->data;
        struct block_of_pointers *pointers2;
        pointers2 = (struct block_of_pointers *) cache_get_entry(pointers2->pointers[index->double_index])->data;
        ret = pointers2->pointers[index->index];
       }
      }
  free(index);
  return ret;
  }
  else
    return -1;

  // ASSERT (inode != NULL);
  // block_sector_t index_num = -1;
  // if (pos < inode->data.length) {
  //   struct inode_index *index = inode_index(pos);
  //   if (index->valid) {
  //     if (index->indirection == 0) {
  //       index_num = inode->data.direct[index->index];
  //     } else if (index->indirection == 1) {
  //       struct block_of_pointers *pointers;
  //       pointers = (struct block_of_pointers *) cache_get_entry(inode->data.indirect)->data;
  //       index_num = pointers->pointers[index->index];
  //     } else {
  //       struct cache_entry *entry = cache_get_entry(inode->data.doubly_indirect);
  //       struct block_of_pointers *double_pointers = (struct block_of_pointers *) entry->data;
  //       struct cache_entry *double_entry = cache_get_entry(double_pointers->pointers[index->double_index]);
  //       struct block_of_pointers *final_pointers = (struct block_of_pointers *) double_entry->data;
  //       index_num = final_pointers->pointers[index->index];
  //     }
  //   }
  //   free(index);
  // }
  // // printf("index_num %d\n", index_num);
  // return index_num;
}

struct inode_index *
inode_index (off_t pos)
{
  struct inode_index *inode_index = malloc(sizeof(inode_index));
  int num = pos / BLOCK_SECTOR_SIZE;
  inode_index->valid = false;
  if (num < DIR_PTRS) {
    inode_index->indirection = 0;
    inode_index->index = num;
    inode_index->valid = true;
  }
  else if (num < DIR_PTRS + 128) {
    inode_index->indirection = 1;
    inode_index->index = num - DIR_PTRS;
    inode_index->valid = true;
  }
  else {
    inode_index->indirection = 2;
    num = num - (DIR_PTRS + 128);
    inode_index->double_index = num / 128;
    inode_index->index = num % 128;
    inode_index->valid = true;
  }
  return inode_index;
}

struct lock inode_lock;

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   device.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
// bool
// inode_create (block_sector_t sector, off_t length, bool is_dir)
// {
//   struct inode_disk *disk_inode = NULL;
//   bool success = false;

//   ASSERT (length >= 0);

//   /* If this assertion fails, the inode structure is not exactly
//      one sector in size, and you should fix that. */
//   ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

//   disk_inode = calloc (1, sizeof *disk_inode);

//   if (disk_inode == NULL)
//     return false;
//   disk_inode->length = length;
//   disk_inode->magic = INODE_MAGIC;
//   disk_inode->is_dir = is_dir;
//   memset(disk_inode->direct, 0, DIR_PTRS);
//   disk_inode->indirect = 0;
//   disk_inode->doubly_indirect = 0;
//   int i;
//   if (length > 0) {
//     size_t sectors = bytes_to_sectors(length);
//     block_sector_t inode_sector;
//     for (i = 0; i < sectors; i++) {
//       if (i < DIR_PTRS) {
//           free_map_allocate(1, &inode_sector);
//           disk_inode->direct[i] = inode_sector;
//         }
//       else if (i < DIR_PTRS + 128) {
//         free_map_allocate(1, &inode_sector);
//         disk_inode->indirect = inode_sector;
//         struct block_of_pointers *pointers;
//         struct cache_entry *entry = cache_get_entry(inode_sector);
//         pointers = (struct block_of_pointers *) entry->data;
//         int j;
//         for (j = 0; j < sectors - DIR_PTRS -i && j < 128; j++) {
//           free_map_allocate(1, &inode_sector);
//           pointers->pointers[j] = inode_sector;
//           i++;
//         }
//         i--; // because the for loop will increment once for us
//         entry->dirty = true;
//       } else { // we should handle when the sectors don't fit in an inode, but whatever
//         free_map_allocate(1, &inode_sector);
//         disk_inode->indirect = inode_sector;
//         struct block_of_pointers *pointers;
//         struct cache_entry *entry = cache_get_entry(inode_sector);
//         pointers = (struct block_of_pointers *) entry->data;
//         int j;
//         for (j = 0; j < sectors - DIR_PTRS - i; j++) {
//             pointers->pointers[j] = inode_sector;
//             struct block_of_pointers *pointers2;
//             struct cache_entry *entry2 = cache_get_entry(inode_sector);
//             pointers2 = (struct block_of_pointers *) entry2->data;
//             int k;
//             for (k = 0; k < sectors - DIR_PTRS -i && j < 128; k++) {
//               free_map_allocate(1, &inode_sector);
//               pointers2->pointers[j] = inode_sector;
//               i++;
//             }
//             entry2->dirty = true;
//         }
//         entry->dirty = true;
//       }
//     }
//   }
//   // if (length > 0) {
//   //   size_t sectors = bytes_to_sectors(length);
//   //   block_sector_t inode_sector;
//   //   for (i = 0; i < sectors; i++) {
//   //     if (i < DIR_PTRS) {
//   //       free_map_allocate(1, &inode_sector);
//   //       disk_inode->direct[i] = inode_sector;
//   //     } else if (i < DIR_PTRS + 128) {
//   //       free_map_allocate(1, &inode_sector);
//   //       disk_inode->indirect = inode_sector;
//   //       struct cache_entry *indirect_entry = cache_get_entry(inode_sector);
//   //       struct block_of_pointers *pointers = (struct block_of_pointers *) indirect_entry->data;
//   //       int j;
//   //       for (j = 0; j < sectors - i && j < 128; j++) {
//   //         free_map_allocate(1, &inode_sector);
//   //         pointers->pointers[j] = inode_sector;
//   //         i++;
//   //       }
//   //       i--;
//   //       indirect_entry->dirty = true;
//   //     } else {
//   //       free_map_allocate(1, &inode_sector);
//   //       disk_inode->doubly_indirect = inode_sector;
//   //       struct cache_entry *indirect_entry = cache_get_entry(inode_sector);
//   //       struct block_of_pointers *pointers = (struct block_of_pointers *) indirect_entry->data;
//   //       int j;
//   //       int k;
//   //       for (j = 0; j < sectors - (DIR_PTRS + 128); j += k) {
//   //         free_map_allocate(1, &inode_sector);
//   //         pointers->pointers[j] = inode_sector;
//   //         struct cache_entry *doubly_indirect_entry = cache_get_entry(inode_sector);
//   //         struct block_of_pointers *double_pointers = (struct block_of_pointers *) doubly_indirect_entry->data;
//   //         for (k = 0; k < 128 && k < sectors - (DIR_PTRS + 128*(k + 1)); k++) {
//   //           free_map_allocate(1, &inode_sector);
//   //           double_pointers->pointers[k] = inode_sector;
//   //         }
//   //         doubly_indirect_entry->dirty = true;
//   //       }
//   //       indirect_entry->dirty = true;
//   //     }
//   //   }
//   // }
//   struct cache_entry *entry = cache_get_entry (sector);
//   memcpy(&entry->data, disk_inode, BLOCK_SECTOR_SIZE);
//   entry->dirty = true;
//   free (disk_inode);
//   return true;
// }
bool
inode_create (block_sector_t sector, off_t length, bool is_dir)
{
 struct inode_disk *disk_inode = NULL;
 bool success = false;

 ASSERT (length >= 0);

 /* If this assertion fails, the inode structure is not exactly
    one sector in size, and you should fix that. */
 ASSERT (sizeof *disk_inode == BLOCK_SECTOR_SIZE);

 disk_inode = calloc (1, sizeof *disk_inode);

 if (disk_inode == NULL)
   return false;
 disk_inode->length = length;
 disk_inode->magic = INODE_MAGIC;
 disk_inode->is_dir = is_dir;
 memset(disk_inode->direct, 0, DIR_PTRS);
 disk_inode->indirect = 0;
 disk_inode->doubly_indirect = 0;
 int i;
 if (length > 0) {
   size_t sectors = bytes_to_sectors(length);
   block_sector_t inode_sector;
   for (i = 0; i < sectors; i++) {
     if (i < DIR_PTRS) {
         free_map_allocate(1, &inode_sector);
         disk_inode->direct[i] = inode_sector;
       }
     else if (i < DIR_PTRS + 128) {
       free_map_allocate(1, &inode_sector);
       disk_inode->indirect = inode_sector;
       struct block_of_pointers *pointers;
       struct cache_entry *entry = cache_get_entry(inode_sector);
       pointers = (struct block_of_pointers *) entry->data;
       int j;
       for (j = 0; j < sectors - DIR_PTRS -i && j < 128; j++) {
         free_map_allocate(1, &inode_sector);
         pointers->pointers[j] = inode_sector;
         i++;
       }
       i--; // because the for loop will increment once for us
       entry->dirty = true;
     } else { // we should handle when the sectors don't fit in an inode, but whatever
       free_map_allocate(1, &inode_sector);
       disk_inode->indirect = inode_sector;
       struct block_of_pointers *pointers;
       struct cache_entry *entry = cache_get_entry(inode_sector);
       pointers = (struct block_of_pointers *) entry->data;
       int j;
       for (j = 0; j < sectors - DIR_PTRS - i; j++) {
           pointers->pointers[j] = inode_sector;
           struct block_of_pointers *pointers2;
           struct cache_entry *entry2 = cache_get_entry(inode_sector);
           pointers2 = (struct block_of_pointers *) entry2->data;
           int k;
           for (k = 0; k < sectors - DIR_PTRS -i && j < 128; k++) {
             free_map_allocate(1, &inode_sector);
             pointers2->pointers[j] = inode_sector;
             i++;
           }
           entry2->dirty = true;
       }
       entry->dirty = true;
     }
   }
 }
   // deal with doubly indirect later
 struct cache_entry *entry = cache_get_entry (sector);
 memcpy(&entry->data, disk_inode, BLOCK_SECTOR_SIZE);
 entry->dirty = true;
 free (disk_inode);
 return true;

 // if (disk_inode != NULL)
 //   {
 //     size_t sectors = bytes_to_sectors (length);
 //     disk_inode->length = length;
 //     disk_inode->magic = INODE_MAGIC;
 //     disk_inode->is_dir = is_dir;
 //     disk_inode->parent = ROOT_DIR_SECTOR;
 //     if (free_map_allocate (sectors, &disk_inode->start))
 //       {
 //         block_write (fs_device, sector, disk_inode);
 //         if (sectors > 0)
 //           {
 //             static char zeros[BLOCK_SECTOR_SIZE];
 //             size_t i;

 //             for (i = 0; i < sectors; i++)
 //               block_write (fs_device, disk_inode->start + i, zeros);
 //           }
 //         success = true;
 //       }
 //     free (disk_inode);
 //   }
 // return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (block_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  lock_init(&inode->inode_lock);
  struct cache_entry *entry = cache_get_entry (sector);
  memcpy(&inode->data, &entry->data, BLOCK_SECTOR_SIZE);
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
block_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      // if (inode->removed)
      //   {
      //     int i;
      //     size_t sectors = bytes_to_sectors(inode->data.length);
      //     for (i = 0; i < sectors; i++) {
      //       if (i < DIR_PTRS) {
      //         free_map_release(inode->data.direct[i], 1);
      //       } else if (i < DIR_PTRS + 128) {
      //         struct cache_entry *indirect_entry = cache_get_entry(inode->sector);
      //         struct block_of_pointers *pointers = (struct block_of_pointers *) indirect_entry->data;
      //         free_map_release(pointers->pointers[i - 128], 1);
      //       } else {
      //         struct cache_entry *doubly_indirect_entry = cache_get_entry(inode->sector);

      //       }
      //     }
      //     free_map_release (inode->sector, 1);
      //   }
      // free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  printf("size %d\n", size);
  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      printf("sector_idx %d\n", sector_idx);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      struct cache_entry *entry = cache_get_entry (sector_idx);
      memcpy(buffer + bytes_read, (uint8_t *) &entry->data + sector_ofs, chunk_size);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{
  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;

  if (inode->deny_write_cnt)
    return 0;
  // while (size > 0)
  //   {
  //     /* Sector to write, starting byte offset within sector. */
  //     block_sector_t sector_idx = byte_to_sector (inode, offset);
  //     int sector_ofs = offset % BLOCK_SECTOR_SIZE;

  //     /* Bytes left in inode, bytes left in sector, lesser of the two. */
  //     off_t inode_left = inode_length (inode) - offset;
  //     int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
  //     int min_left = inode_left < sector_left ? inode_left : sector_left;

  //     /* Number of bytes to actually write into this sector. */
  //     int chunk_size = size < min_left ? size : min_left;
  //     if (chunk_size <= 0)
  //       break;

  //     struct cache_entry *entry = cache_get_entry (sector_idx);
  //     memcpy((uint8_t *) &entry->data + sector_ofs, buffer + bytes_written, chunk_size);
  //     entry->dirty = true;

  //     /* Advance. */
  //     size -= chunk_size;
  //     offset += chunk_size;
  //     bytes_written += chunk_size;
  //   }
  if (inode->data.length < offset+size) {
    int i;
    int sectors = bytes_to_sectors(offset+size);
    struct inode_disk *disk_inode = &(inode->data);
    block_sector_t inode_sector;
    for (i = (disk_inode->length%BLOCK_SECTOR_SIZE==0 ? disk_inode->length :disk_inode->length/BLOCK_SECTOR_SIZE+1); i < sectors; i++) {
      if (i < DIR_PTRS) {
          free_map_allocate(1, &inode_sector);
          disk_inode->direct[i] = inode_sector;
        }
      else if (i < DIR_PTRS + 128) {
        free_map_allocate(1, &inode_sector);
        disk_inode->indirect = inode_sector;
        struct block_of_pointers *pointers;
        struct cache_entry *entry = cache_get_entry(inode_sector);
        pointers = (struct block_of_pointers *) entry->data;
        int j;
        for (j = 0; j < sectors - DIR_PTRS -i && j < 128; j++) {
          free_map_allocate(1, &inode_sector);
          pointers->pointers[j] = inode_sector;
          i++;
        }
        i--; // because the for loop will increment once for us
        entry->dirty = true;
      } else { // we should handle when the sectors don't fit in an inode, but whatever
        free_map_allocate(1, &inode_sector);
        disk_inode->indirect = inode_sector;
        struct block_of_pointers *pointers;
        struct cache_entry *entry = cache_get_entry(inode_sector);
        pointers = (struct block_of_pointers *) entry->data;
        int j;
        for (j = 0; j < sectors - DIR_PTRS - i; j++) {
            pointers->pointers[j] = inode_sector;
            struct block_of_pointers *pointers2;
            struct cache_entry *entry2 = cache_get_entry(inode_sector);
            pointers2 = (struct block_of_pointers *) entry2->data;
            int k;
            for (k = 0; k < sectors - DIR_PTRS -i && j < 128; k++) {
              free_map_allocate(1, &inode_sector);
              pointers2->pointers[j] = inode_sector;
              i++;
            }
            entry2->dirty = true;
        }
        entry->dirty = true;
      }
    }
    disk_inode->length = offset+size;
    struct cache_entry *entry = cache_get_entry(inode->sector);
    memcpy(entry->data, disk_inode, 512);
    entry->dirty = true;
  }
  while (size > 0)
    {
      /* Sector to write, starting byte offset within sector. */
      block_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % BLOCK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = BLOCK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;

      struct cache_entry *entry = cache_get_entry (sector_idx);
      memcpy((uint8_t *) &entry->data + sector_ofs, buffer + bytes_written, chunk_size);
      entry->dirty = true;

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
    }

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}

bool
inode_is_dir (struct inode *inode)
{
  return inode->data.is_dir;
}

void
inode_lock_acquire (struct inode *inode)
{
  lock_acquire(&inode->inode_lock);
}

void
inode_lock_release (struct inode *inode)
{
  lock_release(&inode->inode_lock);
}

block_sector_t
inode_get_parent (struct inode *inode)
{
  return inode->data.parent;
}

bool
inode_add_parent (block_sector_t child_sector, block_sector_t parent_sector)
{
  struct inode *child = inode_open(child_sector);
  if (!child)
    return false;
  inode_close(child);
  child->data.parent = parent_sector;
  return true;
}

bool
inode_still_open (struct inode *inode)
{
  return inode->open_cnt > 0;
}
