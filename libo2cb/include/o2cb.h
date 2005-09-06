/* -*- mode: c; c-basic-offset: 8; -*-
 * vim: noexpandtab sw=8 ts=8 sts=0:
 *
 * o2cb.h
 *
 * Routines for accessing the o2cb configuration.
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
 */

#ifndef _O2CB_H
#define _O2CB_H

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

#include <linux/types.h>

#include <et/com_err.h>

#if O2CB_FLAT_INCLUDES

#include "sparse_endian_types.h"

#include "o2cb_err.h"

#include "ocfs2_nodemanager.h"
#include "ocfs2_heartbeat.h"

#else

#include <o2cb/sparse_endian_types.h>

#include <o2cb/o2cb_err.h>

#include <o2cb/ocfs2_nodemanager.h>
#include <o2cb/ocfs2_heartbeat.h>

#endif

errcode_t o2cb_init(void);

errcode_t o2cb_create_cluster(const char *cluster_name);
errcode_t o2cb_remove_cluster(const char *cluster_name);

errcode_t o2cb_add_node(const char *cluster_name,
			const char *node_name, const char *node_num,
			const char *ip_address, const char *ip_port,
			const char *local);
errcode_t o2cb_del_node(const char *cluster_name, const char *node_name);

errcode_t o2cb_list_clusters(char ***clusters);
void o2cb_free_cluster_list(char **clusters);

errcode_t o2cb_list_nodes(char *cluster_name, char ***nodes);
void o2cb_free_nodes_list(char **nodes);

struct o2cb_region_desc {
	char		*r_name;
	char		*r_device_name;
	int		r_block_bytes;
	uint64_t	r_start_block;
	uint64_t	r_blocks;
};

/* Expected use case for the region descriptor is to allocate it on
 * the stack and completely fill it before calling
 * start_heartbeat_region. */
errcode_t o2cb_start_heartbeat_region(const char *cluster_name,
				      struct o2cb_region_desc *desc);
errcode_t o2cb_stop_heartbeat_region(const char *cluster_name,
				     const char *region_name);
errcode_t o2cb_start_heartbeat_region_perm(const char *cluster_name,
					   struct o2cb_region_desc *desc);
errcode_t o2cb_stop_heartbeat_region_perm(const char *cluster_name,
					  const char *region_name);

errcode_t o2cb_get_region_ref(const char *region_name,
			      int undo);
errcode_t o2cb_put_region_ref(const char *region_name,
			      int undo);
errcode_t o2cb_num_region_refs(const char *region_name,
			       int *num_refs);

errcode_t o2cb_get_node_num(const char *cluster_name,
			    const char *node_name,
			    uint16_t *node_num);

errcode_t o2cb_get_hb_ctl_path(char *buf, int count);

#endif  /* _O2CB_H */
