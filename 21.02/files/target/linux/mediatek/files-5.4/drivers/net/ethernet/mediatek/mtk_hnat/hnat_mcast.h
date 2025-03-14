/*   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2014-2016 Zhiqiang Yang <zhiqiang.yang@mediatek.com>
 */

#ifndef NF_HNAT_MCAST_H
#define NF_HNAT_MCAST_H

#define RTMGRP_IPV4_MROUTE 0x20
#define RTMGRP_MDB 0x2000000

#define MAX_MCAST_ENTRY 64
#define MAX_MCAST_PORT	5

struct ppe_mcast_group {
	u32 mac_hi; /*multicast mac addr*/
	u16 mac_lo; /*multicast mac addr*/
	u16 vid;
	u8 mc_port; /*1:forward to cpu,2:forward to GDMA1,3:forward to GDMA2*/
	u8 if_num[MAX_MCAST_PORT]; /*num of if added to multi group. */
	bool valid;
};

struct ppe_mcast_table {
	struct workqueue_struct *queue;
	struct work_struct work;
	struct socket *msock;
	struct ppe_mcast_group mtbl[MAX_MCAST_ENTRY];
	u8 max_entry;
};

struct ppe_mcast_h {
	union {
		u32 value;
		struct {
			u32 mc_vid : 12;
			u32 mc_qos_qid64 : 3;
			u32 mc_px_en : 5;
			u32 mc_mpre_sel : 2; /* 0=01:00, 1=33:33 */
			u32 mc_vid_cmp: 1;
			u32 mc_px_qos_en : 5;
			u32 mc_qos_qid : 4;
		} info;
	} u;
};

struct ppe_mcast_l {
	u32 addr;
};

enum ppe_mcast_port {
	MCAST_TO_GDMA3,
	MCAST_TO_PDMA,
	MCAST_TO_GDMA1,
	MCAST_TO_GDMA2,
	MCAST_TO_QDMA,
};

int hnat_mcast_enable(u32 ppe_id);
int hnat_mcast_disable(void);

#endif
