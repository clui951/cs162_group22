#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/block.h"
#include "filesys/directory.h"

struct bitmap;

void inode_init (void);
bool inode_create (block_sector_t, off_t, bool is_dir);
struct inode *inode_open (block_sector_t);
struct inode *inode_reopen (struct inode *);
block_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);
bool inode_is_dir (struct inode *);
void inode_lock_acquire (struct inode *inode);
void inode_lock_release (struct inode *inode);
block_sector_t inode_get_parent (struct inode *inode);
bool inode_add_parent (block_sector_t child_sector, block_sector_t parent_sector);
// void inode_allocate ();
// void inode_deallocate ();
// void inode_expand (struct inode *inode, off_t length);
// void inode_expand_indirect (struct inode *inode);
// void inode_expand_doubly_indirect (struct inode *inode);

#endif /* filesys/inode.h */
