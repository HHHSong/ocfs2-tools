/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * ocfs2ne_feature_sparse_files.c
 *
 * ocfs2 tune utility for enabling and disabling the sparse file feature.
 *
 * Copyright (C) 2004, 2008 Oracle.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include <inttypes.h>
#include <assert.h>

#include "ocfs2-kernel/kernel-list.h"
#include "ocfs2/ocfs2.h"

#include "libtunefs.h"
#include "libtunefs_err.h"


struct hole_list {
	struct list_head list;
	uint32_t start;
	uint32_t len;
};

/*
 * A sparse file may have many holes.  All the holes will be stored in
 * hole_list.  Since filling up a hole may need a new extent record and
 * lead to some new extent block, the total hole number in the sparse file
 * will also be recorded.
 *
 * Some sparse file may also have some clusters which exceed the limit of
 * i_size, and they should be truncated.
 */
struct sparse_file {
	struct list_head list;
	uint64_t blkno;
	struct list_head holes;
	uint32_t holes_num;
	uint32_t hole_clusters;
	int truncate;
};

struct fill_hole_context {
	errcode_t ret;
	uint32_t more_clusters;
	uint32_t more_ebs;
	struct list_head files;
};

/*
 * Truncate file according to i_size.
 * All allocated clusters which exceeds i_size will be released.
 */
static errcode_t truncate_to_i_size(ocfs2_filesys *fs,
				    struct ocfs2_dinode *di,
				    void *unused)
{
	errcode_t ret;
	uint32_t new_clusters;
	ocfs2_cached_inode *ci = NULL;

	ret = ocfs2_read_cached_inode(fs, di->i_blkno, &ci);
	if (ret)
		goto out;

	tunefs_block_signals();
	ret = ocfs2_zero_tail_and_truncate(fs, ci, di->i_size, &new_clusters);
	tunefs_unblock_signals();
	if (ret)
		goto out;

	if (new_clusters != ci->ci_inode->i_clusters) {
		ci->ci_inode->i_clusters = new_clusters;
		tunefs_block_signals();
		ret = ocfs2_write_cached_inode(fs, ci);
		tunefs_unblock_signals();
	}

out:
	if (ci)
		ocfs2_free_cached_inode(fs, ci);

	return ret;
}

static int enable_sparse_files(ocfs2_filesys *fs, int flags)
{
	errcode_t ret = 0;
	struct ocfs2_super_block *super = OCFS2_RAW_SB(fs->fs_super);

	if (ocfs2_sparse_alloc(super)) {
		verbosef(VL_APP,
			 "Sparse file feature is already enabled; "
			 "nothing to enable\n");
		goto out;
	}

	if (!tunefs_interact("Enable the sparse file feature on device "
			     "\"%s\"? ",
			     fs->fs_devname))
		goto out;

	ret = tunefs_foreach_inode(fs, S_IFREG, truncate_to_i_size, NULL);
	if (ret) {
		tcom_err(ret,
			 "while trying to remove any extraneous allocation");
		goto out;
	}

	OCFS2_SET_INCOMPAT_FEATURE(super,
				   OCFS2_FEATURE_INCOMPAT_SPARSE_ALLOC);
	tunefs_block_signals();
	ret = ocfs2_write_super(fs);
	tunefs_unblock_signals();
	if (ret)
		tcom_err(ret, "while writing out the superblock");

out:
	return ret;
}

static errcode_t add_hole(struct sparse_file *file, uint32_t hole_start,
			  uint32_t hole_len)
{
	errcode_t ret;
	struct hole_list *hole = NULL;

	ret = ocfs2_malloc0(sizeof(struct hole_list), &hole);
	if (ret)
		return ret;

	hole->start = hole_start;
	hole->len = hole_len;
	list_add_tail(&hole->list, &file->holes);

	file->hole_clusters += hole_len;
	file->holes_num++;

	return 0;
}

static void free_sparse_file(struct sparse_file *file)
{
	struct hole_list *hole;
	struct list_head *n, *pos;

	list_del(&file->list);

	list_for_each_safe(pos, n, &file->holes) {
		hole = list_entry(pos, struct hole_list, list);
		list_del(&hole->list);
		ocfs2_free(&hole);
	}

	ocfs2_free(&file);
}

static void empty_fill_hole_context(struct fill_hole_context *ctxt)
{
	struct sparse_file *file;
	struct list_head *n, *pos;

	list_for_each_safe(pos, n, &ctxt->files) {
		file = list_entry(pos, struct sparse_file, list);
		free_sparse_file(file);
	}
}


/*
 * Walk the allocations of a file, filling in the struct sparse_file.
 */
static errcode_t find_holes_in_file(ocfs2_filesys *fs,
				    struct ocfs2_dinode *di,
				    struct sparse_file *file)
{
	errcode_t ret;
	uint32_t clusters, v_cluster = 0, p_cluster, num_clusters;
	uint32_t last_v_cluster = 0;
	uint16_t extent_flags;
	ocfs2_cached_inode *ci = NULL;

	clusters = (di->i_size + fs->fs_clustersize -1 ) /
			fs->fs_clustersize;

	ret = ocfs2_read_cached_inode(fs, di->i_blkno, &ci);
	if (ret)
		goto bail;

	while (v_cluster < clusters) {
		ret = ocfs2_get_clusters(ci,
					 v_cluster, &p_cluster,
					 &num_clusters, &extent_flags);
		if (ret)
			goto bail;

		if (!p_cluster) {
			/*
			 * If the tail of the file is a hole, let the
			 * hole length only cover the last i_size.
			 */
			if (v_cluster + num_clusters == UINT32_MAX)
				num_clusters = clusters - v_cluster;

			ret = add_hole(file, v_cluster, num_clusters);
			if (ret)
				goto bail;
		}

		if (extent_flags & OCFS2_EXT_UNWRITTEN) {
			ret = TUNEFS_ET_UNWRITTEN_PRESENT;
			goto bail;
		}

		v_cluster += num_clusters;
	}

	/*
	 * If the last allocated cluster's virtual offset is greater
	 * than the clusters we calculated from i_size, this cluster
	 * must exceed the limit of i_size.
	 */
	ret = ocfs2_get_last_cluster_offset(fs, di, &last_v_cluster);
	if (ret)
		goto bail;

	if (last_v_cluster >= clusters)
		file->truncate = 1;

bail:
	if (ci)
		ocfs2_free_cached_inode(fs, ci);
	return ret;
}


static errcode_t hole_iterate(ocfs2_filesys *fs, struct ocfs2_dinode *di,
			      void *user_data)
{
	errcode_t ret = 0;
	uint64_t blk_num;
	uint32_t clusters;
	struct sparse_file *file = NULL;
	uint32_t recs_per_eb = ocfs2_extent_recs_per_eb(fs->fs_blocksize);
	struct fill_hole_context *ctxt = user_data;

	assert(S_ISREG(di->i_mode));

	ret = ocfs2_malloc0(sizeof(struct sparse_file), &file);
	if (ret)
		goto bail;

	file->blkno = di->i_blkno;
	INIT_LIST_HEAD(&file->holes);
	ret = find_holes_in_file(fs, di, file);
	if (ret)
		goto bail;

	/*
	 * If there is no hole or unwritten extents in the file and we don't
	 * need to truncate it, just free it.
	 */
	if (list_empty(&file->holes) && !file->truncate)
		goto bail;

	/*
	 * We have  "hole_num" holes, so more extent records are needed,
	 * and more extent blocks may needed here.
	 * In order to simplify the estimation process, we take it for
	 * granted that one hole need one extent record, so that we can
	 * calculate the extent block we need roughly.
	 */
	blk_num = (file->holes_num + recs_per_eb - 1) / recs_per_eb;
	clusters = ocfs2_clusters_in_blocks(fs, blk_num);
	ctxt->more_ebs += clusters;

	list_add_tail(&file->list, &ctxt->files);

	return 0;

bail:
	if (file)
		ocfs2_free(&file);
	return ret;
}

static errcode_t get_total_free_clusters(ocfs2_filesys *fs,
					 uint32_t *clusters)
{
	errcode_t ret;
	uint64_t blkno;
	char *buf = NULL;
	struct ocfs2_dinode *di = NULL;

	ret = ocfs2_malloc_block(fs->fs_io, &buf);
	if (ret)
		goto bail;

	ret = ocfs2_lookup_system_inode(fs, GLOBAL_BITMAP_SYSTEM_INODE,
					0, &blkno);
	if (ret)
		goto bail;

	ret = ocfs2_read_inode(fs, blkno, buf);
	if (ret)
		goto bail;

	di = (struct ocfs2_dinode *)buf;
	if (clusters)
		*clusters = di->id1.bitmap1.i_total - di->id1.bitmap1.i_used;
bail:
	if (buf)
		ocfs2_free(&buf);
	return ret;
}

static errcode_t find_sparse_files(ocfs2_filesys *fs,
				   struct fill_hole_context *ctxt)
{
	errcode_t ret;
	uint32_t free_clusters = 0;

	ret = tunefs_foreach_inode(fs, S_IFREG, hole_iterate, ctxt);
	if (ret)
		goto bail;

	ret = get_total_free_clusters(fs, &free_clusters);
	if (ret)
		goto bail;

	verbosef(VL_APP,
		 "We have %u clusters free, and need %u clusters to fill "
		 "every sparse file and %u clusters for more extent "
		 "blocks\n",
		 free_clusters, ctxt->more_clusters,
		 ctxt->more_ebs);

	if (free_clusters < (ctxt->more_clusters + ctxt->more_ebs))
		ret = OCFS2_ET_NO_SPACE;

bail:
	return ret;
}

static errcode_t empty_clusters(ocfs2_filesys *fs,
				uint64_t start_blk,
				uint32_t num_clusters)
{
	errcode_t ret;
	char *buf = NULL;
	uint16_t bpc = fs->fs_clustersize / fs->fs_blocksize;
	uint64_t total_blocks = bpc * num_clusters;
	uint64_t io_blocks = total_blocks;

	ret = ocfs2_malloc_blocks(fs->fs_io, io_blocks, &buf);
	if (ret == OCFS2_ET_NO_MEMORY) {
		io_blocks = bpc;
		ret = ocfs2_malloc_blocks(fs->fs_io, io_blocks, &buf);
	}
	if (ret)
		goto bail;

	memset(buf, 0, io_blocks * fs->fs_blocksize);

	while (total_blocks) {
		ret = io_write_block(fs->fs_io, start_blk, io_blocks, buf);
		if (ret)
			goto bail;

		total_blocks -= io_blocks;
		start_blk += io_blocks;
	}

bail:
	if (buf)
		ocfs2_free(&buf);

	return ret;
}


static errcode_t fill_one_hole(ocfs2_filesys *fs, struct sparse_file *file,
			       struct hole_list *hole)
{
	errcode_t ret = 0;
	uint32_t start = hole->start;
	uint32_t len = hole->len;
	uint32_t n_clusters;
	uint64_t p_start;

	while (len) {
		tunefs_block_signals();
		ret = ocfs2_new_clusters(fs, 1, len,
					 &p_start, &n_clusters);
		if ((!ret && (n_clusters == 0)) ||
		    (ret == OCFS2_ET_BIT_NOT_FOUND))
			ret = OCFS2_ET_NO_SPACE;
		if (ret)
			break;

		ret = empty_clusters(fs, p_start, n_clusters);
		if (ret)
			break;

		ret = ocfs2_insert_extent(fs, file->blkno,
					  start, p_start,
					  n_clusters, 0);
		if (ret)
			break;

		len -= n_clusters;
		start += n_clusters;
		tunefs_unblock_signals();
	}

	if (ret)
		tunefs_unblock_signals();

	return ret;
}

static errcode_t fill_one_file(ocfs2_filesys *fs, struct sparse_file *file)
{
	errcode_t ret = 0;
	struct hole_list *hole;
	struct list_head *pos;

	list_for_each(pos, &file->holes) {
		hole = list_entry(pos, struct hole_list, list);
		ret = fill_one_hole(fs, file, hole);
		if (ret)
			break;
	}

	return ret;
}

static errcode_t fill_sparse_files(ocfs2_filesys *fs,
				   struct fill_hole_context *ctxt)
{
	errcode_t ret = 0;
	char *buf = NULL;
	struct ocfs2_dinode *di = NULL;
	struct list_head *pos;
	struct sparse_file *file;

	ret = ocfs2_malloc_block(fs->fs_io, &buf);
	if (ret)
		goto out;

	/* Iterate all the holes and fill them. */
	list_for_each(pos, &ctxt->files) {
		file = list_entry(pos, struct sparse_file, list);
		ret = fill_one_file(fs, file);
		if (ret)
			break;

		if (!file->truncate)
			continue;

		ret = ocfs2_read_inode(fs, file->blkno, buf);
		if (ret)
			break;
		di = (struct ocfs2_dinode *)buf;
		ret = truncate_to_i_size(fs, di, NULL);
		if (ret)
			break;
	}

	ocfs2_free(&buf);

out:
	return ret;
}

static int disable_sparse_files(ocfs2_filesys *fs, int flags)
{
	errcode_t ret = 0;
	struct ocfs2_super_block *super = OCFS2_RAW_SB(fs->fs_super);
	struct fill_hole_context ctxt;

	if (!ocfs2_sparse_alloc(super)) {
		verbosef(VL_APP,
			 "Sparse file feature is not enabled; "
			 "nothing to disable\n");
		goto out;
	}

	if (ocfs2_writes_unwritten_extents(super)) {
		errorf("Unwritten extents are enabled on device \"%s\"; "
		       "sparse files cannot be disabled\n",
		       fs->fs_devname);
		ret = TUNEFS_ET_UNWRITTEN_PRESENT;
		goto out;
	}

	if (!tunefs_interact("Disable the sparse file feature on device "
			     "\"%s\"? ",
			     fs->fs_devname))
		goto out;

	memset(&ctxt, 0, sizeof(ctxt));
	INIT_LIST_HEAD(&ctxt.files);
	ret = find_sparse_files(fs, &ctxt);
	if (ret) {
		if (ret == OCFS2_ET_NO_SPACE)
			errorf("There is not enough space to fill all of "
			       "the sparse files on device \"%s\"\n",
			       fs->fs_devname);
		else
			tcom_err(ret, "while trying to find sparse files");
		goto out_cleanup;
	}

	ret = fill_sparse_files(fs, &ctxt);
	if (ret) {
		tcom_err(ret,
			 "while trying to fill the sparse files on device "
			 "\"%s\"",
			 fs->fs_devname);
		goto out_cleanup;
	}

	OCFS2_CLEAR_INCOMPAT_FEATURE(super,
				     OCFS2_FEATURE_INCOMPAT_SPARSE_ALLOC);
	tunefs_block_signals();
	ret = ocfs2_write_super(fs);
	tunefs_unblock_signals();
	if (ret)
		tcom_err(ret, "while writing out the superblock");

out_cleanup:
	empty_fill_hole_context(&ctxt);

out:
	return ret;
}

DEFINE_TUNEFS_FEATURE_INCOMPAT(sparse_files,
			       OCFS2_FEATURE_INCOMPAT_SPARSE_ALLOC,
			       TUNEFS_FLAG_RW | TUNEFS_FLAG_ALLOCATION,
			       enable_sparse_files,
			       disable_sparse_files);

int main(int argc, char *argv[])
{
	return tunefs_feature_main(argc, argv, &sparse_files_feature);
}

