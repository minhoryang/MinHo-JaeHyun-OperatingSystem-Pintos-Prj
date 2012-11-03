#ifndef USERPROG_FILESYS_TYPE_H
#define USERPROG_FILESYS_TYPE_H

// XXX copied from "filesys/inode.h"
#include "filesys/off_t.h"
#include "devices/block.h"

// XXX copied from "filesys/file.c"
/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

// XXX copied from "filesys/inode.c"
/* On-disk inode.
   Must be exactly BLOCK_SECTOR_SIZE bytes long. */
struct inode_disk
  {
    block_sector_t start;               /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    uint32_t unused[125];               /* Not used. */
  };

/* In-memory inode. */
struct inode 
  {
    struct list_elem elem;              /* Element in inode list. */
    block_sector_t sector;              /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
	// XXX : Add Lock in here.
	struct lock lock;
	// XXX
  };

#endif /* userprog/filesys_type.h */
