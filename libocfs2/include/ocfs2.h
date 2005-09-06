/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * ocfs2.h
 *
 * Filesystem object routines for the OCFS2 userspace library.
 *
 * Copyright (C) 2004 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License, version 2,  as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Authors: Joel Becker
 */

#ifndef _FILESYS_H
#define _FILESYS_H

#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE 600
#endif
#ifndef _LARGEFILE64_SOURCE
# define _LARGEFILE64_SOURCE
#endif

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#include <limits.h>

#include <linux/types.h>

#include <et/com_err.h>

#include "byteorder.h"

#include <kernel-list.h>
#include <kernel-rbtree.h>

#include <o2dlm.h>
#include <o2cb.h>

#if OCFS2_FLAT_INCLUDES
#include "ocfs2_err.h"
#include "ocfs2_fs.h"
#include "jbd.h"
#else
#include <ocfs2/ocfs2_err.h>
#include <ocfs2/ocfs2_fs.h>
#include <ocfs2/jbd.h>
#endif

#define OCFS2_LIB_FEATURE_INCOMPAT_SUPP		(OCFS2_FEATURE_INCOMPAT_SUPP | OCFS2_FEATURE_INCOMPAT_HEARTBEAT_DEV)
#define OCFS2_LIB_FEATURE_RO_COMPAT_SUPP	OCFS2_FEATURE_RO_COMPAT_SUPP

/* Flags for the ocfs2_filesys structure */
#define OCFS2_FLAG_RO			0x00
#define OCFS2_FLAG_RW			0x01
#define OCFS2_FLAG_CHANGED		0x02
#define OCFS2_FLAG_DIRTY		0x04
#define OCFS2_FLAG_SWAP_BYTES		0x08
#define OCFS2_FLAG_BUFFERED		0x10
#define OCFS2_FLAG_NO_REV_CHECK		0x20	/* Do not check the OCFS
						   vol_header structure
						   for revision info */
#define OCFS2_FLAG_HEARTBEAT_DEV_OK	0x40

/* Return flags for the extent iterator functions */
#define OCFS2_EXTENT_CHANGED	0x01
#define OCFS2_EXTENT_ABORT	0x02
#define OCFS2_EXTENT_ERROR	0x04

/*
 * Extent iterate flags
 *
 * OCFS2_EXTENT_FLAG_APPEND indicates that the iterator function should
 * be called on extents past the leaf next_free_rec.  This is used by
 * ocfs2_expand_dir() to add a new extent to a directory (via
 * OCFS2_BLOCK_FLAG_APPEND and the block iteration functions).
 *
 * OCFS2_EXTENT_FLAG_DEPTH_TRAVERSE indicates that the iterator
 * function for tree_depth > 0 records (ocfs2_extent_blocks, iow)
 * should be called after all of the extents contained in the
 * extent_block are processed.  This is useful if you are going to be
 * deallocating extents.
 *
 * OCFS2_EXTENT_FLAG_DATA_ONLY indicates that the iterator function
 * should be called for data extents (depth == 0) only.
 */
#define OCFS2_EXTENT_FLAG_APPEND		0x01
#define OCFS2_EXTENT_FLAG_DEPTH_TRAVERSE	0x02
#define OCFS2_EXTENT_FLAG_DATA_ONLY		0x04


/* Return flags for the block iterator functions */
#define OCFS2_BLOCK_CHANGED	0x01
#define OCFS2_BLOCK_ABORT	0x02
#define OCFS2_BLOCK_ERROR	0x04

/*
 * Block iterate flags
 *
 * In OCFS2, block iteration runs through the blocks contained in an
 * inode's data extents.  As such, "DATA_ONLY" and "DEPTH_TRAVERSE"
 * can't really apply.
 * 
 * OCFS2_BLOCK_FLAG_APPEND is as OCFS2_EXTENT_FLAG_APPEND, except on a
 * blocksize basis.  This may mean that the underlying extent already
 * contains the space for a new block, and i_size is updated
 * accordingly.
 */
#define OCFS2_BLOCK_FLAG_APPEND		0x01


/* Return flags for the directory iterator functions */
#define OCFS2_DIRENT_CHANGED	0x01
#define OCFS2_DIRENT_ABORT	0x02
#define OCFS2_DIRENT_ERROR	0x04

/* Directory iterator flags */
#define OCFS2_DIRENT_FLAG_INCLUDE_EMPTY		0x01
#define OCFS2_DIRENT_FLAG_INCLUDE_REMOVED	0x02
#define OCFS2_DIRENT_FLAG_EXCLUDE_DOTS		0x04

/* Return flags for the chain iterator functions */
#define OCFS2_CHAIN_CHANGED	0x01
#define OCFS2_CHAIN_ABORT	0x02
#define OCFS2_CHAIN_ERROR	0x04

/* Directory constants */
#define OCFS2_DIRENT_DOT_FILE		1
#define OCFS2_DIRENT_DOT_DOT_FILE	2
#define OCFS2_DIRENT_OTHER_FILE		3
#define OCFS2_DIRENT_DELETED_FILE	4

/* Directory scan flags */
#define OCFS2_DIR_SCAN_FLAG_EXCLUDE_DOTS	0x01

/* Check if mounted flags */
#define OCFS2_MF_MOUNTED		1
#define OCFS2_MF_ISROOT			2
#define OCFS2_MF_READONLY		4
#define OCFS2_MF_SWAP			8
#define OCFS2_MF_BUSY			16
#define OCFS2_MF_MOUNTED_CLUSTER	32

/* check_heartbeats progress states */
#define OCFS2_CHB_START		1
#define OCFS2_CHB_WAITING	2
#define OCFS2_CHB_COMPLETE	3

typedef void (*ocfs2_chb_notify)(int state, char *progress, void *data);

typedef struct _ocfs2_filesys ocfs2_filesys;
typedef struct _ocfs2_cached_inode ocfs2_cached_inode;
typedef struct _io_channel io_channel;
typedef struct _ocfs2_extent_map ocfs2_extent_map;
typedef struct _ocfs2_inode_scan ocfs2_inode_scan;
typedef struct _ocfs2_dir_scan ocfs2_dir_scan;
typedef struct _ocfs2_bitmap ocfs2_bitmap;
typedef struct _ocfs2_devices ocfs2_devices;

struct _ocfs2_filesys {
	char *fs_devname;
	uint32_t fs_flags;
	io_channel *fs_io;
	ocfs2_dinode *fs_super;
	ocfs2_dinode *fs_orig_super;
	unsigned int fs_blocksize;
	unsigned int fs_clustersize;
	uint32_t fs_clusters;
	uint64_t fs_blocks;
	uint32_t fs_umask;
	uint64_t fs_root_blkno;
	uint64_t fs_sysdir_blkno;
	uint64_t fs_first_cg_blkno;
	char uuid_str[OCFS2_VOL_UUID_LEN * 2 + 1];

	/* Allocators */
	ocfs2_cached_inode *fs_cluster_alloc;
	ocfs2_cached_inode **fs_inode_allocs;
	ocfs2_cached_inode *fs_system_inode_alloc;
	ocfs2_cached_inode **fs_eb_allocs;
	ocfs2_cached_inode *fs_system_eb_alloc;

	struct o2dlm_ctxt *fs_dlm_ctxt;

	/* Reserved for the use of the calling application. */
	void *fs_private;
};

struct _ocfs2_cached_inode {
	struct _ocfs2_filesys *ci_fs;
	uint64_t ci_blkno;
	ocfs2_dinode *ci_inode;
	ocfs2_extent_map *ci_map;
	ocfs2_bitmap *ci_chains;
};

struct _ocfs2_devices {
	struct list_head list;
	char dev_name[100];
	uint8_t label[64];
	uint8_t uuid[16];
	int mount_flags;
	int fs_type;			/* 0=unknown, 1=ocfs, 2=ocfs2 */
	uint32_t maj_num;		/* major number of the device */
	uint32_t min_num;		/* minor number of the device */
	errcode_t errcode;		/* error encountered reading device */
	void *private;
	uint16_t max_slots;
	uint8_t *node_nums;		/* list of mounted nodes */
};

errcode_t ocfs2_malloc(unsigned long size, void *ptr);
errcode_t ocfs2_malloc0(unsigned long size, void *ptr);
errcode_t ocfs2_free(void *ptr);
errcode_t ocfs2_realloc(unsigned long size, void *ptr);
errcode_t ocfs2_realloc0(unsigned long size, void *ptr,
			 unsigned long old_size);
errcode_t ocfs2_malloc_blocks(io_channel *channel, int num_blocks,
			      void *ptr);
errcode_t ocfs2_malloc_block(io_channel *channel, void *ptr);

errcode_t io_open(const char *name, int flags, io_channel **channel);
errcode_t io_close(io_channel *channel);
int io_get_error(io_channel *channel);
errcode_t io_set_blksize(io_channel *channel, int blksize);
int io_get_blksize(io_channel *channel);
errcode_t io_read_block(io_channel *channel, int64_t blkno, int count,
			char *data);
errcode_t io_write_block(io_channel *channel, int64_t blkno, int count,
			 const char *data);

errcode_t ocfs2_write_super(ocfs2_filesys *fs);
errcode_t ocfs2_open(const char *name, int flags,
		     unsigned int superblock, unsigned int blksize,
		     ocfs2_filesys **ret_fs);
errcode_t ocfs2_flush(ocfs2_filesys *fs);
errcode_t ocfs2_close(ocfs2_filesys *fs);
void ocfs2_freefs(ocfs2_filesys *fs);

void ocfs2_swap_inode_from_cpu(ocfs2_dinode *di);
void ocfs2_swap_inode_to_cpu(ocfs2_dinode *di);
errcode_t ocfs2_read_inode(ocfs2_filesys *fs, uint64_t blkno,
			   char *inode_buf);
errcode_t ocfs2_write_inode(ocfs2_filesys *fs, uint64_t blkno,
			    char *inode_buf);
errcode_t ocfs2_check_directory(ocfs2_filesys *fs, uint64_t dir);

errcode_t ocfs2_read_cached_inode(ocfs2_filesys *fs, uint64_t blkno,
				  ocfs2_cached_inode **ret_ci);
errcode_t ocfs2_write_cached_inode(ocfs2_filesys *fs,
				   ocfs2_cached_inode *cinode);
errcode_t ocfs2_free_cached_inode(ocfs2_filesys *fs,
				  ocfs2_cached_inode *cinode);

void ocfs2_swap_extent_list_from_cpu(ocfs2_extent_list *el);
void ocfs2_swap_extent_list_to_cpu(ocfs2_extent_list *el);
errcode_t ocfs2_extent_map_init(ocfs2_filesys *fs,
				ocfs2_cached_inode *cinode);
void ocfs2_extent_map_free(ocfs2_cached_inode *cinode);
errcode_t ocfs2_extent_map_insert(ocfs2_cached_inode *cinode,
				  ocfs2_extent_rec *rec,
				  int tree_depth);
errcode_t ocfs2_extent_map_drop(ocfs2_cached_inode *cinode,
				 uint32_t new_clusters);
errcode_t ocfs2_extent_map_trunc(ocfs2_cached_inode *cinode,
				 uint32_t new_clusters);
errcode_t ocfs2_extent_map_get_rec(ocfs2_cached_inode *cinode,
				   uint32_t cpos,
				   ocfs2_extent_rec **rec);
errcode_t ocfs2_extent_map_get_clusters(ocfs2_cached_inode *cinode,
					uint32_t v_cpos, int count,
					uint32_t *p_cpos,
					int *ret_count);
errcode_t ocfs2_extent_map_get_blocks(ocfs2_cached_inode *cinode,
				      uint64_t v_blkno, int count,
				      uint64_t *p_blkno,
				      int *ret_count);
errcode_t ocfs2_load_extent_map(ocfs2_filesys *fs,
				ocfs2_cached_inode *cinode);

void ocfs2_swap_journal_superblock(journal_superblock_t *jsb);
errcode_t ocfs2_init_journal_superblock(ocfs2_filesys *fs, char *buf,
					int buflen, uint32_t jrnl_size);
errcode_t ocfs2_create_journal_superblock(ocfs2_filesys *fs,
					  uint32_t size, int flags,
					  char **ret_jsb);
errcode_t ocfs2_read_journal_superblock(ocfs2_filesys *fs, uint64_t blkno,
					char *jsb_buf);
errcode_t ocfs2_write_journal_superblock(ocfs2_filesys *fs, uint64_t blkno,
					 char *jsb_buf);

errcode_t ocfs2_read_extent_block(ocfs2_filesys *fs, uint64_t blkno,
       				  char *eb_buf);
errcode_t ocfs2_read_extent_block_nocheck(ocfs2_filesys *fs, uint64_t blkno,
					  char *eb_buf);
errcode_t ocfs2_write_extent_block(ocfs2_filesys *fs, uint64_t blkno,
       				   char *eb_buf);
errcode_t ocfs2_extent_iterate(ocfs2_filesys *fs,
			       uint64_t blkno,
			       int flags,
			       char *block_buf,
			       int (*func)(ocfs2_filesys *fs,
					   ocfs2_extent_rec *rec,
					   int tree_depth,
					   uint32_t ccount,
					   uint64_t ref_blkno,
					   int ref_recno,
					   void *priv_data),
			       void *priv_data);
errcode_t ocfs2_extent_iterate_inode(ocfs2_filesys *fs,
				     ocfs2_dinode *inode,
				     int flags,
				     char *block_buf,
				     int (*func)(ocfs2_filesys *fs,
					         ocfs2_extent_rec *rec,
					         int tree_depth,
					         uint32_t ccount,
					         uint64_t ref_blkno,
					         int ref_recno,
					         void *priv_data),
					         void *priv_data);
errcode_t ocfs2_block_iterate(ocfs2_filesys *fs,
			      uint64_t blkno,
			      int flags,
			      int (*func)(ocfs2_filesys *fs,
					  uint64_t blkno,
					  uint64_t bcount,
					  void *priv_data),
			      void *priv_data);
errcode_t ocfs2_block_iterate_inode(ocfs2_filesys *fs,
				    ocfs2_dinode *inode,
				    int flags,
				    int (*func)(ocfs2_filesys *fs,
						uint64_t blkno,
						uint64_t bcount,
						void *priv_data),
				    void *priv_data);

errcode_t ocfs2_swap_dir_entries_from_cpu(void *buf, uint64_t bytes);
errcode_t ocfs2_swap_dir_entries_to_cpu(void *buf, uint64_t bytes);
errcode_t ocfs2_read_dir_block(ocfs2_filesys *fs, uint64_t block,
			       void *buf);
errcode_t ocfs2_write_dir_block(ocfs2_filesys *fs, uint64_t block,
				void *buf);

errcode_t ocfs2_dir_iterate2(ocfs2_filesys *fs,
			     uint64_t dir,
			     int flags,
			     char *block_buf,
			     int (*func)(uint64_t	dir,
					 int		entry,
					 struct ocfs2_dir_entry *dirent,
					 int	offset,
					 int	blocksize,
					 char	*buf,
					 void	*priv_data),
			     void *priv_data);
extern errcode_t ocfs2_dir_iterate(ocfs2_filesys *fs, 
				   uint64_t dir,
				   int flags,
				   char *block_buf,
				   int (*func)(struct ocfs2_dir_entry *dirent,
					       int	offset,
					       int	blocksize,
					       char	*buf,
					       void	*priv_data),
				   void *priv_data);

errcode_t ocfs2_lookup(ocfs2_filesys *fs, uint64_t dir,
		       const char *name, int namelen, char *buf,
		       uint64_t *inode);

errcode_t ocfs2_lookup_system_inode(ocfs2_filesys *fs, int type,
				    int slot_num, uint64_t *blkno);

errcode_t ocfs2_link(ocfs2_filesys *fs, uint64_t dir, const char *name,
		     uint64_t ino, int flags);

errcode_t ocfs2_unlink(ocfs2_filesys *fs, uint64_t dir,
		       const char *name, uint64_t ino, int flags);

errcode_t ocfs2_open_inode_scan(ocfs2_filesys *fs,
				ocfs2_inode_scan **ret_scan);
void ocfs2_close_inode_scan(ocfs2_inode_scan *scan);
errcode_t ocfs2_get_next_inode(ocfs2_inode_scan *scan,
			       uint64_t *blkno, char *inode);

errcode_t ocfs2_open_dir_scan(ocfs2_filesys *fs, uint64_t dir, int flags,
			      ocfs2_dir_scan **ret_scan);
void ocfs2_close_dir_scan(ocfs2_dir_scan *scan);
errcode_t ocfs2_get_next_dir_entry(ocfs2_dir_scan *scan,
				   struct ocfs2_dir_entry *dirent);

errcode_t ocfs2_cluster_bitmap_new(ocfs2_filesys *fs,
				   const char *description,
				   ocfs2_bitmap **ret_bitmap);
errcode_t ocfs2_block_bitmap_new(ocfs2_filesys *fs,
				 const char *description,
				 ocfs2_bitmap **ret_bitmap);
void ocfs2_bitmap_free(ocfs2_bitmap *bitmap);
errcode_t ocfs2_bitmap_set(ocfs2_bitmap *bitmap, uint64_t bitno,
			   int *oldval);
errcode_t ocfs2_bitmap_clear(ocfs2_bitmap *bitmap, uint64_t bitno,
			     int *oldval);
errcode_t ocfs2_bitmap_test(ocfs2_bitmap *bitmap, uint64_t bitno,
			    int *val);
errcode_t ocfs2_bitmap_find_next_set(ocfs2_bitmap *bitmap,
				     uint64_t start, uint64_t *found);
errcode_t ocfs2_bitmap_find_next_clear(ocfs2_bitmap *bitmap,
				       uint64_t start, uint64_t *found);
errcode_t ocfs2_bitmap_read(ocfs2_bitmap *bitmap);
errcode_t ocfs2_bitmap_write(ocfs2_bitmap *bitmap);
uint64_t ocfs2_bitmap_get_set_bits(ocfs2_bitmap *bitmap);
errcode_t ocfs2_bitmap_alloc_range(ocfs2_bitmap *bitmap, uint64_t len, 
				   uint64_t *first_bit);
errcode_t ocfs2_bitmap_clear_range(ocfs2_bitmap *bitmap, uint64_t len, 
				   uint64_t first_bit);

errcode_t ocfs2_get_device_size(const char *file, int blocksize,
				uint64_t *retblocks);

errcode_t ocfs2_get_device_sectsize(const char *file, int *sectsize);

errcode_t ocfs2_check_if_mounted(const char *file, int *mount_flags);
errcode_t ocfs2_check_mount_point(const char *device, int *mount_flags,
		                  char *mtpt, int mtlen);

errcode_t ocfs2_read_whole_file(ocfs2_filesys *fs, uint64_t blkno,
				char **buf, int *len);

void ocfs2_swap_disk_heartbeat_block(struct o2hb_disk_heartbeat_block *hb);
errcode_t ocfs2_check_heartbeat(char *device, int *mount_flags,
				struct list_head *nodes_list);

errcode_t ocfs2_check_heartbeats(struct list_head *dev_list);

errcode_t ocfs2_get_ocfs1_label(char *device, uint8_t *label, uint16_t label_len,
				uint8_t *uuid, uint16_t uuid_len);

void ocfs2_swap_group_desc(ocfs2_group_desc *gd);
errcode_t ocfs2_read_group_desc(ocfs2_filesys *fs, uint64_t blkno,
				char *gd_buf);

errcode_t ocfs2_write_group_desc(ocfs2_filesys *fs, uint64_t blkno,
				 char *gd_buf);

errcode_t ocfs2_chain_iterate(ocfs2_filesys *fs,
			      uint64_t blkno,
			      int (*func)(ocfs2_filesys *fs,
					  uint64_t gd_blkno,
					  int chain_num,
					  void *priv_data),
			      void *priv_data);

errcode_t ocfs2_load_chain_allocator(ocfs2_filesys *fs,
				     ocfs2_cached_inode *cinode);
errcode_t ocfs2_write_chain_allocator(ocfs2_filesys *fs,
				      ocfs2_cached_inode *cinode);
errcode_t ocfs2_chain_alloc(ocfs2_filesys *fs,
			    ocfs2_cached_inode *cinode,
			    uint64_t *gd_blkno,
			    uint64_t *bitno);
errcode_t ocfs2_chain_free(ocfs2_filesys *fs,
			   ocfs2_cached_inode *cinode,
			   uint64_t bitno);
errcode_t ocfs2_chain_alloc_range(ocfs2_filesys *fs,
				  ocfs2_cached_inode *cinode,
				  uint64_t requested,
				  uint64_t *start_bit);
errcode_t ocfs2_chain_free_range(ocfs2_filesys *fs,
				 ocfs2_cached_inode *cinode,
				 uint64_t len,
				 uint64_t start_bit);
errcode_t ocfs2_chain_test(ocfs2_filesys *fs,
			   ocfs2_cached_inode *cinode,
			   uint64_t bitno,
			   int *oldval);
errcode_t ocfs2_chain_force_val(ocfs2_filesys *fs,
				ocfs2_cached_inode *cinode,
				uint64_t blkno, 
				int newval,
				int *oldval);
errcode_t ocfs2_chain_add_group(ocfs2_filesys *fs,
				ocfs2_cached_inode *cinode);

errcode_t ocfs2_expand_dir(ocfs2_filesys *fs,
			   uint64_t dir,
			   uint64_t parent_dir);

errcode_t ocfs2_test_inode_allocated(ocfs2_filesys *fs, uint64_t blkno,
				     int *is_allocated);
void ocfs2_init_group_desc(ocfs2_filesys *fs, ocfs2_group_desc *gd,
			   uint64_t blkno, uint32_t generation,
			   uint64_t parent_inode, uint16_t bits,
			   uint16_t chain);

errcode_t ocfs2_new_dir_block(ocfs2_filesys *fs, uint64_t dir_ino,
			      uint64_t parent_ino, char **block);

errcode_t ocfs2_insert_extent(ocfs2_filesys *fs, uint64_t ino,
			      uint64_t c_blkno, uint32_t clusters);
errcode_t ocfs2_extent_allocation(ocfs2_filesys *fs, uint64_t ino,
				  uint32_t new_clusters);

errcode_t ocfs2_new_inode(ocfs2_filesys *fs, uint64_t *ino, int mode);
errcode_t ocfs2_new_system_inode(ocfs2_filesys *fs, uint64_t *ino, int mode, int flags);
errcode_t ocfs2_delete_inode(ocfs2_filesys *fs, uint64_t ino);
errcode_t ocfs2_new_extent_block(ocfs2_filesys *fs, uint64_t *blkno);
errcode_t ocfs2_delete_extent_block(ocfs2_filesys *fs, uint64_t blkno);
errcode_t ocfs2_extend_allocation(ocfs2_filesys *fs, uint64_t ino,
				  uint32_t new_clusters);
errcode_t ocfs2_truncate(ocfs2_filesys *fs, uint64_t ino, uint64_t new_i_size);
errcode_t ocfs2_new_clusters(ocfs2_filesys *fs,
			     uint32_t requested,
			     uint64_t *start_blkno);
errcode_t ocfs2_free_clusters(ocfs2_filesys *fs,
			      uint32_t len,
			      uint64_t start_blkno);

errcode_t ocfs2_lookup(ocfs2_filesys *fs, uint64_t dir, const char *name,
		       int namelen, char *buf, uint64_t *inode);

errcode_t ocfs2_namei(ocfs2_filesys *fs, uint64_t root, uint64_t cwd,
		      const char *name, uint64_t *inode);

errcode_t ocfs2_namei_follow(ocfs2_filesys *fs, uint64_t root, uint64_t cwd,
			     const char *name, uint64_t *inode);

errcode_t ocfs2_follow_link(ocfs2_filesys *fs, uint64_t root, uint64_t cwd,
			    uint64_t inode, uint64_t *res_inode);

errcode_t ocfs2_file_read(ocfs2_cached_inode *ci, void *buf, uint32_t count,
			  uint64_t offset, uint32_t *got);

errcode_t ocfs2_file_write(ocfs2_cached_inode *ci, void *buf, uint32_t count,
			   uint64_t offset, uint32_t *wrote);

errcode_t ocfs2_fill_heartbeat_desc(ocfs2_filesys *fs,
				    struct o2cb_region_desc *desc);

errcode_t ocfs2_start_heartbeat(ocfs2_filesys *fs);

errcode_t ocfs2_stop_heartbeat(ocfs2_filesys *fs);

errcode_t ocfs2_lock_down_cluster(ocfs2_filesys *fs);

errcode_t ocfs2_release_cluster(ocfs2_filesys *fs);

errcode_t ocfs2_initialize_dlm(ocfs2_filesys *fs);

errcode_t ocfs2_shutdown_dlm(ocfs2_filesys *fs);

errcode_t ocfs2_super_lock(ocfs2_filesys *fs);

errcode_t ocfs2_super_unlock(ocfs2_filesys *fs);

errcode_t ocfs2_meta_lock(ocfs2_filesys *fs, ocfs2_cached_inode *inode,
			  enum o2dlm_lock_level level, int flags);

errcode_t ocfs2_meta_unlock(ocfs2_filesys *fs, ocfs2_cached_inode *ci);

void ocfs2_swap_slot_map(int16_t *map, loff_t num_slots);

/* 
 * ${foo}_to_${bar} is a floor function.  blocks_to_clusters will
 * returns the cluster that contains a block, not the number of clusters
 * that hold a given number of blocks.
 *
 * ${foo}_in_${bar} is a ceiling function.  clusters_in_blocks will give
 * the number of clusters needed to hold a given number of blocks.
 */

static inline uint64_t ocfs2_clusters_to_blocks(ocfs2_filesys *fs,
						uint32_t clusters)
{
	int c_to_b_bits =
		OCFS2_RAW_SB(fs->fs_super)->s_clustersize_bits -
		OCFS2_RAW_SB(fs->fs_super)->s_blocksize_bits;

	return (uint64_t)clusters << c_to_b_bits;
}

static inline uint32_t ocfs2_blocks_to_clusters(ocfs2_filesys *fs,
						uint64_t blocks)
{
	int b_to_c_bits =
		OCFS2_RAW_SB(fs->fs_super)->s_clustersize_bits -
		OCFS2_RAW_SB(fs->fs_super)->s_blocksize_bits;

	return (uint32_t)(blocks >> b_to_c_bits);
}

static inline uint64_t ocfs2_blocks_in_bytes(ocfs2_filesys *fs, uint64_t bytes)
{
	uint64_t ret = bytes + fs->fs_blocksize - 1;

	if (ret < bytes) /* deal with wrapping */
		return UINT64_MAX;

	return ret >> OCFS2_RAW_SB(fs->fs_super)->s_blocksize_bits;
}

static inline uint32_t ocfs2_clusters_in_blocks(ocfs2_filesys *fs, 
						uint64_t blocks)
{
	int c_to_b_bits = OCFS2_RAW_SB(fs->fs_super)->s_clustersize_bits -
		          OCFS2_RAW_SB(fs->fs_super)->s_blocksize_bits;
	uint64_t ret = blocks + ((1 << c_to_b_bits) - 1); 

	if (ret < blocks) /* deal with wrapping */
		ret = UINT64_MAX;

	return (uint32_t)(ret >> c_to_b_bits);
}

static inline int ocfs2_block_out_of_range(ocfs2_filesys *fs, uint64_t block)
{
	return (block < OCFS2_SUPER_BLOCK_BLKNO) || (block > fs->fs_blocks);
}

struct ocfs2_cluster_group_sizes {
	uint16_t	cgs_cpg;
	uint16_t	cgs_tail_group_bits;
	uint32_t	cgs_cluster_groups;
};
static inline void ocfs2_calc_cluster_groups(uint64_t clusters, 
					     uint64_t blocksize,
				     struct ocfs2_cluster_group_sizes *cgs)
{
	uint16_t max_bits = 8 * ocfs2_group_bitmap_size(blocksize);

	cgs->cgs_cpg = max_bits;
	if (max_bits > clusters)
		cgs->cgs_cpg = clusters;

	cgs->cgs_cluster_groups = (clusters + cgs->cgs_cpg - 1) / 
				  cgs->cgs_cpg;

	cgs->cgs_tail_group_bits = clusters % cgs->cgs_cpg;
	if (cgs->cgs_tail_group_bits == 0)
		cgs->cgs_tail_group_bits = cgs->cgs_cpg;
}

/*
 * shamelessly lifted from the kernel
 *
 * min()/max() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define ocfs2_min(x,y) ({ \
	const typeof(x) _x = (x);       \
	const typeof(y) _y = (y);       \
	(void) (&_x == &_y);            \
	_x < _y ? _x : _y; })
                                                                                
#define ocfs2_max(x,y) ({ \
	const typeof(x) _x = (x);       \
	const typeof(y) _y = (y);       \
	(void) (&_x == &_y);            \
	_x > _y ? _x : _y; })

#endif  /* _FILESYS_H */
