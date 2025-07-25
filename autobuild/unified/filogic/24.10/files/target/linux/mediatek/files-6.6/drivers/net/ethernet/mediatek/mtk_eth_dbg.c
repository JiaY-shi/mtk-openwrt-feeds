/*
 *   Copyright (C) 2018 MediaTek Inc.
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; version 2 of the License
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   Copyright (C) 2009-2016 John Crispin <blogic@openwrt.org>
 *   Copyright (C) 2009-2016 Felix Fietkau <nbd@openwrt.org>
 *   Copyright (C) 2013-2016 Michael Lee <igvtee@gmail.com>
 */

#include <linux/trace_seq.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/u64_stats_sync.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/of_mdio.h>
#include <linux/of_address.h>

#include "mtk_eth_soc.h"
#include "mtk_eth_dbg.h"
#include "mtk_wed_regs.h"

enum mt753x_presence {
	MT753X_ABSENT = 0,
	MT753X_PRESENT = 1,
	MT753X_UNKNOWN = 0xffff,
};

enum mt753x_presence mt753x_presence = MT753X_UNKNOWN;
u32 hw_lro_agg_num_cnt[MTK_MAX_RX_RING_NUM][MTK_HW_LRO_MAX_AGG_CNT + 1];
u32 hw_lro_agg_size_cnt[MTK_MAX_RX_RING_NUM][16];
u32 hw_lro_tot_agg_cnt[MTK_MAX_RX_RING_NUM];
u32 hw_lro_tot_flush_cnt[MTK_MAX_RX_RING_NUM];
u32 hw_lro_agg_flush_cnt[MTK_MAX_RX_RING_NUM];
u32 hw_lro_age_flush_cnt[MTK_MAX_RX_RING_NUM];
u32 hw_lro_seq_flush_cnt[MTK_MAX_RX_RING_NUM];
u32 hw_lro_timestamp_flush_cnt[MTK_MAX_RX_RING_NUM];
u32 hw_lro_norule_flush_cnt[MTK_MAX_RX_RING_NUM];
u32 mtk_hwlro_stats_ebl;
u32 dbg_show_level;

static struct proc_dir_entry *proc_hw_lro_stats, *proc_hw_lro_auto_tlb;
typedef int (*mtk_lro_dbg_func) (int par);

struct mtk_eth_debug {
	struct dentry *root;
	void __iomem *base;
	int direct_access;
};

struct mtk_eth *g_eth;

struct mtk_eth_debug eth_debug;

static int qdma_pppq_show(struct seq_file *m, void *v)
{
	struct mtk_eth *eth = m->private;
	int i;

	seq_puts(m, "Usage of the QDMA PPPQ for the HW path:\n");
	for (i = 0; i < MTK_QDMA_NUM_QUEUES; i++)
		seq_printf(m, "qdma_txq%d:	%5d Mbps %8d refcnt\n",
			   i, eth->qdma_shaper.speed[i],
			   atomic_read(&eth->qdma_shaper.refcnt[i]));
	seq_printf(m, "qdma_thres:	%5d Mbps\n", eth->qdma_shaper.threshold);

	return 0;
}

static int qdma_pppq_open(struct inode *inode, struct file *file)
{
	return single_open(file, qdma_pppq_show, inode->i_private);
}

static ssize_t qdma_pppq_write(struct file *file, const char __user *buffer,
			       size_t count, loff_t *data)
{
	struct mtk_eth *eth = file_inode(file)->i_private;
	char buf[8] = {0};
	u32 threshold;
	int len = count;

	if ((len > 8) || copy_from_user(buf, buffer, len))
		return -EFAULT;

	if (sscanf(buf, "%6d", &threshold) != 1)
		return -EFAULT;

	if (threshold > 10000 || threshold < 0) {
		pr_warn("Threshold must be between 0 and 10000 Mbps\n");
		return -EINVAL;
	}

	eth->qdma_shaper.threshold = threshold;

	return len;
}

static const struct file_operations qdma_pppq_fops = {
	.open = qdma_pppq_open,
	.read = seq_read,
	.write = qdma_pppq_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t qdma_sched_show(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_soc_data *soc = eth->soc;
	long id = (long)file->private_data;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	u32 qdma_tx_sch, sch_reg;
	int enable, scheduling, max_rate, scheduler, i;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (mtk_is_netsys_v2_or_greater(eth))
		qdma_tx_sch = mtk_r32(eth, soc->reg_map->qdma.tx_sch_rate + (id >> 1) * 0x4);
	else
		qdma_tx_sch = mtk_r32(eth, soc->reg_map->qdma.tx_sch_rate);

	if (id & 0x1)
		qdma_tx_sch >>= 16;

	qdma_tx_sch &= MTK_QDMA_TX_SCH;
	enable = FIELD_GET(MTK_QDMA_TX_SCH_RATE_EN, qdma_tx_sch);
	scheduling = FIELD_GET(MTK_QDMA_TX_SCH_MAX_WFQ, qdma_tx_sch);
	max_rate = FIELD_GET(MTK_QDMA_TX_SCH_RATE_MAN, qdma_tx_sch);
	qdma_tx_sch = FIELD_GET(MTK_QDMA_TX_SCH_RATE_EXP, qdma_tx_sch);
	while (qdma_tx_sch--)
		max_rate *= 10;

	len += scnprintf(buf + len, buf_len - len,
			 "EN\tScheduling\tMAX\tQueue#\n%d\t%s%16d\t", enable,
			 (scheduling == 1) ? "WRR" : "SP", max_rate);

	for (i = 0; i < MTK_QDMA_NUM_QUEUES; i++) {
		mtk_w32(eth, (i / MTK_QTX_PER_PAGE), soc->reg_map->qdma.page);
		sch_reg = mtk_r32(eth, soc->reg_map->qdma.qtx_sch +
				       (i % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);
		if (mtk_is_netsys_v2_or_greater(eth))
			scheduler = FIELD_GET(MTK_QTX_SCH_TX_SEL_V2, sch_reg);
		else
			scheduler = FIELD_GET(MTK_QTX_SCH_TX_SEL, sch_reg);
		if (id == scheduler)
			len += scnprintf(buf + len, buf_len - len, "%d  ", i);
	}

	len += scnprintf(buf + len, buf_len - len, "\n");
	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
}

static ssize_t qdma_sched_write(struct file *file, const char __user *buf,
				size_t length, loff_t *offset)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_soc_data *soc = eth->soc;
	long id = (long)file->private_data;
	char line[64] = {0}, scheduling[32];
	int enable, rate, exp = 0, shift = 0;
	size_t size;
	u32 qdma_tx_sch, val = 0;

	if (length >= sizeof(line))
		return -EINVAL;

	if (copy_from_user(line, buf, length))
		return -EFAULT;

	if (sscanf(line, "%1d %3s %9d", &enable, scheduling, &rate) != 3)
		return -EFAULT;

	if (mtk_is_netsys_v3_or_greater(eth)) {
		if (rate > 10000000 || rate < 0)
			return -EINVAL;
	} else {
		if (rate > 1000000 || rate < 0)
			return -EINVAL;
	}

	while (rate > 127) {
		rate /= 10;
		exp++;
	}

	line[length] = '\0';

	if (enable)
		val |= MTK_QDMA_TX_SCH_RATE_EN;
	if (strcmp(scheduling, "sp") != 0)
		val |= MTK_QDMA_TX_SCH_MAX_WFQ;
	val |= FIELD_PREP(MTK_QDMA_TX_SCH_RATE_MAN, rate);
	val |= FIELD_PREP(MTK_QDMA_TX_SCH_RATE_EXP, exp);
	if (id & 0x1)
		shift = 16;

	if (mtk_is_netsys_v2_or_greater(eth))
		qdma_tx_sch = mtk_r32(eth, soc->reg_map->qdma.tx_sch_rate + (id >> 1) * 0x4);
	else
		qdma_tx_sch = mtk_r32(eth, soc->reg_map->qdma.tx_sch_rate);

	qdma_tx_sch &= ~(MTK_QDMA_TX_SCH << shift);
	qdma_tx_sch |= val << shift;
	if (mtk_is_netsys_v2_or_greater(eth))
		mtk_w32(eth, qdma_tx_sch, soc->reg_map->qdma.tx_sch_rate + (id >> 1) * 0x4);
	else
		mtk_w32(eth, qdma_tx_sch, soc->reg_map->qdma.tx_sch_rate);

	size = strlen(line);
	*offset += size;

	return length;
}

static const struct file_operations qdma_sched_fops = {
	.open = simple_open,
	.read = qdma_sched_show,
	.write = qdma_sched_write,
	.llseek = default_llseek,
};

static ssize_t qdma_queue_show(struct file *file, char __user *user_buf,
			       size_t count, loff_t *ppos)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_soc_data *soc = eth->soc;
	long id = (long)file->private_data;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	u32 qtx_sch, qtx_cfg;
	int scheduler;
	int min_rate_en, min_rate, min_rate_exp;
	int max_rate_en, max_weight, max_rate, max_rate_exp;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	mtk_w32(eth, (id / MTK_QTX_PER_PAGE), soc->reg_map->qdma.page);
	qtx_cfg = mtk_r32(eth, soc->reg_map->qdma.qtx_cfg +
			       (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);
	qtx_sch = mtk_r32(eth, soc->reg_map->qdma.qtx_sch +
			       (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);
	if (mtk_is_netsys_v2_or_greater(eth))
		scheduler = FIELD_GET(MTK_QTX_SCH_TX_SEL_V2, qtx_sch);
	else
		scheduler = FIELD_GET(MTK_QTX_SCH_TX_SEL, qtx_sch);

	min_rate_en = FIELD_GET(MTK_QTX_SCH_MIN_RATE_EN, qtx_sch);
	if (mtk_is_netsys_v3_or_greater(eth) && (eth->soc->caps != MT7988_CAPS)) {
		min_rate = FIELD_GET(MTK_QTX_SCH_MIN_RATE_MAN_V3, qtx_sch);
		min_rate_exp = FIELD_GET(MTK_QTX_SCH_MIN_RATE_EXP_V3, qtx_sch);
		max_rate_en = FIELD_GET(MTK_QTX_SCH_MAX_RATE_EN_V3, qtx_sch);
		max_weight = FIELD_GET(MTK_QTX_SCH_MAX_RATE_WEIGHT_V3, qtx_sch);
		max_rate = FIELD_GET(MTK_QTX_SCH_MAX_RATE_MAN_V3, qtx_sch);
		max_rate_exp = FIELD_GET(MTK_QTX_SCH_MAX_RATE_EXP_V3, qtx_sch);
	} else {
		min_rate = FIELD_GET(MTK_QTX_SCH_MIN_RATE_MAN, qtx_sch);
		min_rate_exp = FIELD_GET(MTK_QTX_SCH_MIN_RATE_EXP, qtx_sch);
		max_rate_en = FIELD_GET(MTK_QTX_SCH_MAX_RATE_EN, qtx_sch);
		max_weight = FIELD_GET(MTK_QTX_SCH_MAX_RATE_WEIGHT, qtx_sch);
		max_rate = FIELD_GET(MTK_QTX_SCH_MAX_RATE_MAN, qtx_sch);
		max_rate_exp = FIELD_GET(MTK_QTX_SCH_MAX_RATE_EXP, qtx_sch);
	}

	while (min_rate_exp--)
		min_rate *= 10;

	while (max_rate_exp--)
		max_rate *= 10;

	len += scnprintf(buf + len, buf_len - len,
			 "scheduler: %d\nhw resv: %d\nsw resv: %d\n", scheduler,
			 (qtx_cfg >> 8) & 0xff, qtx_cfg & 0xff);

	if (mtk_is_netsys_v2_or_greater(eth)) {
		/* Switch to debug mode */
		mtk_m32(eth, MTK_MIB_ON_QTX_CFG, MTK_MIB_ON_QTX_CFG, soc->reg_map->qdma.qtx_mib_if);
		mtk_m32(eth, MTK_VQTX_MIB_EN, MTK_VQTX_MIB_EN, soc->reg_map->qdma.qtx_mib_if);
		qtx_cfg = mtk_r32(eth, soc->reg_map->qdma.qtx_cfg +
				       (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);
		qtx_sch = mtk_r32(eth, soc->reg_map->qdma.qtx_sch +
				       (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);
		len += scnprintf(buf + len, buf_len - len,
				 "packet count: %u\n", qtx_cfg);
		len += scnprintf(buf + len, buf_len - len,
				 "packet drop: %u\n\n", qtx_sch);

		/* Recover to normal mode */
		mtk_m32(eth, MTK_MIB_ON_QTX_CFG, 0, soc->reg_map->qdma.qtx_mib_if);
		mtk_m32(eth, MTK_VQTX_MIB_EN, 0, soc->reg_map->qdma.qtx_mib_if);
	}

	len += scnprintf(buf + len, buf_len - len,
			 "      EN     RATE     WEIGHT\n");
	len += scnprintf(buf + len, buf_len - len,
			 "----------------------------\n");
	len += scnprintf(buf + len, buf_len - len,
			 "max%5d%9d%9d\n", max_rate_en, max_rate, max_weight);
	len += scnprintf(buf + len, buf_len - len,
			 "min%5d%9d        -\n", min_rate_en, min_rate);

	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
}

static ssize_t qdma_queue_write(struct file *file, const char __user *buf,
				size_t length, loff_t *offset)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_soc_data *soc = eth->soc;
	long id = (long)file->private_data;
	char line[64] = {0};
	int max_enable, max_rate, max_exp = 0;
	int min_enable, min_rate, min_exp = 0;
	int weight;
	int resv;
	int scheduler;
	size_t size;
	u32 qtx_sch = 0, qtx_cfg = 0;

	mtk_w32(eth, (id / MTK_QTX_PER_PAGE), soc->reg_map->qdma.page);
	if (length >= sizeof(line))
		return -EINVAL;

	if (copy_from_user(line, buf, length))
		return -EFAULT;

	if (sscanf(line, "%d %d %d %d %d %d %d", &scheduler, &min_enable, &min_rate,
		   &max_enable, &max_rate, &weight, &resv) != 7)
		return -EFAULT;

	line[length] = '\0';

	if (mtk_is_netsys_v3_or_greater(eth)) {
		if (max_rate > 10000000 || max_rate < 0 ||
		    min_rate > 10000000 || min_rate < 0)
			return -EINVAL;
	} else {
		if (max_rate > 1000000 || max_rate < 0 ||
		    min_rate > 1000000 || min_rate < 0)
			return -EINVAL;
	}

	while (max_rate > 127) {
		max_rate /= 10;
		max_exp++;
	}

	while (min_rate > 127) {
		min_rate /= 10;
		min_exp++;
	}

	if (mtk_is_netsys_v2_or_greater(eth))
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_TX_SEL_V2, scheduler);
	else
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_TX_SEL, scheduler);

	qtx_sch |= FIELD_PREP(MTK_QTX_SCH_LEAKY_BUCKET_SIZE, 3);

	if (min_enable)
		qtx_sch |= MTK_QTX_SCH_MIN_RATE_EN;
	if (mtk_is_netsys_v3_or_greater(eth) && (eth->soc->caps != MT7988_CAPS)) {
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_MAN_V3, min_rate);
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_EXP_V3, min_exp);
		if (max_enable)
			qtx_sch |= MTK_QTX_SCH_MAX_RATE_EN_V3;
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_WEIGHT_V3, weight);
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_MAN_V3, max_rate);
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_EXP_V3, max_exp);
	} else {
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_MAN, min_rate);
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MIN_RATE_EXP, min_exp);
		if (max_enable)
			qtx_sch |= MTK_QTX_SCH_MAX_RATE_EN;
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_WEIGHT, weight);
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_MAN, max_rate);
		qtx_sch |= FIELD_PREP(MTK_QTX_SCH_MAX_RATE_EXP, max_exp);
	}
	mtk_w32(eth, qtx_sch, soc->reg_map->qdma.qtx_sch +
			      (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);

	qtx_cfg = mtk_r32(eth, soc->reg_map->qdma.qtx_cfg +
			       (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);
	qtx_cfg &= 0xffff0000;
	qtx_cfg |= FIELD_PREP(MTK_QTX_CFG_HW_RESV, resv);
	qtx_cfg |= FIELD_PREP(MTK_QTX_CFG_SW_RESV, resv);
	mtk_w32(eth, qtx_cfg, soc->reg_map->qdma.qtx_cfg +
			      (id % MTK_QTX_PER_PAGE) * MTK_QTX_OFFSET);

	size = strlen(line);
	*offset += size;

	return length;
}

static const struct file_operations qdma_queue_fops = {
	.open = simple_open,
	.read = qdma_queue_show,
	.write = qdma_queue_write,
	.llseek = default_llseek,
};

int mt798x_iomap(void)
{
	struct device_node *np = NULL;

	np = of_find_compatible_node(NULL, NULL, "mediatek,mt7988-switch");
	if (np) {
		eth_debug.base = of_iomap(np, 0);
		if (!eth_debug.base) {
			pr_err("of_iomap failed\n");
			of_node_put(np);
			return -ENOMEM;
		}

		of_node_put(np);
		eth_debug.direct_access = 1;
	}

	return 0;
}

int mt798x_iounmap(void)
{
	eth_debug.direct_access = 0;
	if (eth_debug.base)
		iounmap(eth_debug.base);

	return 0;
}

void mt7530_mdio_w32(struct mtk_eth *eth, u16 reg, u32 val)
{
	mutex_lock(&eth->mii_bus->mdio_lock);

	if (eth_debug.direct_access)
		__raw_writel(val, eth_debug.base + reg);
	else {
		_mtk_mdio_write_c22(eth, 0x1f, 0x1f, (reg >> 6) & 0x3ff);
		_mtk_mdio_write_c22(eth, 0x1f, (reg >> 2) & 0xf, val & 0xffff);
		_mtk_mdio_write_c22(eth, 0x1f, 0x10, val >> 16);
	}

	mutex_unlock(&eth->mii_bus->mdio_lock);
}

u32 mt7530_mdio_r32(struct mtk_eth *eth, u32 reg)
{
	u16 high, low;
	u32 ret;

	mutex_lock(&eth->mii_bus->mdio_lock);

	if (eth_debug.direct_access) {
		ret = __raw_readl(eth_debug.base + reg);
		mutex_unlock(&eth->mii_bus->mdio_lock);
		return ret;
	}
	_mtk_mdio_write_c22(eth, 0x1f, 0x1f, (reg >> 6) & 0x3ff);
	low = _mtk_mdio_read_c22(eth, 0x1f, (reg >> 2) & 0xf);
	high = _mtk_mdio_read_c22(eth, 0x1f, 0x10);

	mutex_unlock(&eth->mii_bus->mdio_lock);

	return (high << 16) | (low & 0xffff);
}

static enum mt753x_presence mt753x_sw_detect(struct mtk_eth *eth)
{
	struct device_node *np;
	u32 sw_id;
	u32 rev;

	/* mt7988 with built-in 7531 */
	np = of_find_compatible_node(NULL, NULL, "mediatek,mt7988-switch");
	if (np) {
		of_node_put(np);
		return MT753X_PRESENT;
	}
	/* external 753x */
	rev = mt7530_mdio_r32(eth, 0x781c);
	sw_id = (rev & 0xffff0000) >> 16;
	if (sw_id == 0x7530 || sw_id == 0x7531)
		return MT753X_PRESENT;

	return MT753X_ABSENT;
}

static enum mt753x_presence mt7530_exist(struct mtk_eth *eth)
{
	if (mt753x_presence == MT753X_UNKNOWN)
		mt753x_presence = mt753x_sw_detect(eth);

	return mt753x_presence;
}

void mtk_switch_w32(struct mtk_eth *eth, u32 val, unsigned int reg)
{
	mtk_w32(eth, val, reg + 0x10000);
}
EXPORT_SYMBOL(mtk_switch_w32);

u32 mtk_switch_r32(struct mtk_eth *eth, unsigned int reg)
{
	return mtk_r32(eth, reg + 0x10000);
}
EXPORT_SYMBOL(mtk_switch_r32);

static int mtketh_debug_show(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;
	struct mtk_mac *mac = 0;
	int  i = 0;

	for (i = 0 ; i < MTK_MAX_DEVS ; i++) {
		if (!eth->mac[i] ||
		    of_phy_is_fixed_link(eth->mac[i]->of_node))
			continue;
		mac = eth->mac[i];
/* FIXME */
/*
		while (j < 30) {
			d =  _mtk_mdio_read_c22(eth, mac->phy_dev->addr, j);

			seq_printf(m, "phy=%d, reg=0x%08x, data=0x%08x\n",
				   mac->phy_dev->addr, j, d);
			j++;
		}
*/
	}
	return 0;
}

static int mtketh_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtketh_debug_show, inode->i_private);
}

static const struct file_operations mtketh_debug_fops = {
	.owner = THIS_MODULE,
	.open = mtketh_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mtketh_mt7530sw_debug_show(struct seq_file *m, void *private)
{
	struct mtk_eth *eth = m->private;
	u32  offset, data;
	int i;
	struct mt7530_ranges {
		u32 start;
		u32 end;
	} ranges[] = {
		{0x0, 0xac},
		{0x1000, 0x10e0},
		{0x1100, 0x1140},
		{0x1200, 0x1240},
		{0x1300, 0x1340},
		{0x1400, 0x1440},
		{0x1500, 0x1540},
		{0x1600, 0x1640},
		{0x1800, 0x1848},
		{0x1900, 0x1948},
		{0x1a00, 0x1a48},
		{0x1b00, 0x1b48},
		{0x1c00, 0x1c48},
		{0x1d00, 0x1d48},
		{0x1e00, 0x1e48},
		{0x1f60, 0x1ffc},
		{0x2000, 0x212c},
		{0x2200, 0x222c},
		{0x2300, 0x232c},
		{0x2400, 0x242c},
		{0x2500, 0x252c},
		{0x2600, 0x262c},
		{0x3000, 0x3014},
		{0x30c0, 0x30f8},
		{0x3100, 0x3114},
		{0x3200, 0x3214},
		{0x3300, 0x3314},
		{0x3400, 0x3414},
		{0x3500, 0x3514},
		{0x3600, 0x3614},
		{0x4000, 0x40d4},
		{0x4100, 0x41d4},
		{0x4200, 0x42d4},
		{0x4300, 0x43d4},
		{0x4400, 0x44d4},
		{0x4500, 0x45d4},
		{0x4600, 0x46d4},
		{0x4f00, 0x461c},
		{0x7000, 0x7038},
		{0x7120, 0x7124},
		{0x7800, 0x7804},
		{0x7810, 0x7810},
		{0x7830, 0x7830},
		{0x7a00, 0x7a7c},
		{0x7b00, 0x7b04},
		{0x7e00, 0x7e04},
		{0x7ffc, 0x7ffc},
	};

	if (!mt7530_exist(eth))
		return -EOPNOTSUPP;

	if ((!eth->mac[0] || !of_phy_is_fixed_link(eth->mac[0]->of_node)) &&
	    (!eth->mac[1] || !of_phy_is_fixed_link(eth->mac[1]->of_node))) {
		seq_puts(m, "no switch found\n");
		return 0;
	}

	mt798x_iomap();
	for (i = 0 ; i < ARRAY_SIZE(ranges) ; i++) {
		for (offset = ranges[i].start;
		     offset <= ranges[i].end; offset += 4) {
			data =  mt7530_mdio_r32(eth, offset);
			seq_printf(m, "mt7530 switch reg=0x%08x, data=0x%08x\n",
				   offset, data);
		}
	}
	mt798x_iounmap();

	return 0;
}

static int mtketh_debug_mt7530sw_open(struct inode *inode, struct file *file)
{
	return single_open(file, mtketh_mt7530sw_debug_show, inode->i_private);
}

static const struct file_operations mtketh_debug_mt7530sw_fops = {
	.owner = THIS_MODULE,
	.open = mtketh_debug_mt7530sw_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static ssize_t mtketh_mt7530sw_debugfs_write(struct file *file,
					     const char __user *ptr,
					     size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	char buf[32], *token, *p = buf;
	unsigned long reg, value, phy;
	int ret;

	if (!mt7530_exist(eth))
		return -EOPNOTSUPP;

	if (*off != 0)
		return 0;

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, ptr, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&phy))
		return -EINVAL;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&reg))
		return -EINVAL;

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&value))
		return -EINVAL;

	mt798x_iomap();
	pr_info("%s:phy=%d, reg=0x%lx, val=0x%lx\n", __func__,
		0x1f, reg, value);
	mt7530_mdio_w32(eth, reg, value);
	pr_info("%s:phy=%d, reg=0x%lx, val=0x%x confirm..\n", __func__,
		0x1f, reg, mt7530_mdio_r32(eth, reg));
	mt798x_iounmap();

	return len;
}

static ssize_t mtketh_debugfs_write(struct file *file, const char __user *ptr,
				    size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	char buf[32], *token, *p = buf;
	unsigned long reg, value, phy;
	int ret;

	if (*off != 0)
		return 0;

	if (len > sizeof(buf) - 1)
		len = sizeof(buf) - 1;

	ret = strncpy_from_user(buf, ptr, len);
	if (ret < 0)
		return ret;
	buf[len] = '\0';

	token = strsep(&p, " ");
	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&phy))
		return -EINVAL;

	token = strsep(&p, " ");

	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&reg))
		return -EINVAL;

	token = strsep(&p, " ");

	if (!token)
		return -EINVAL;
	if (kstrtoul(token, 16, (unsigned long *)&value))
		return -EINVAL;

	pr_info("%s:phy=%ld, reg=0x%lx, val=0x%lx\n", __func__,
		phy, reg, value);

	_mtk_mdio_write_c22(eth, phy,  reg, value);

	pr_info("%s:phy=%ld, reg=0x%lx, val=0x%x confirm..\n", __func__,
		phy, reg, _mtk_mdio_read_c22(eth, phy, reg));

	return len;
}

static ssize_t mtketh_debugfs_reset(struct file *file, const char __user *ptr,
				    size_t len, loff_t *off)
{
	struct mtk_eth *eth = file->private_data;
	unsigned long dbg_level = 0;
	char buf[8] = "";
	int count = len;

	len = min((size_t)count, sizeof(buf) - 1);
	if (copy_from_user(buf, ptr, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtoul(buf, 0, &dbg_level))
		return -EINVAL;

	switch (dbg_level) {
	case 0:
		atomic_set(&eth->reset.force, 0);
		break;
	case 1:
		if (atomic_read(&eth->reset.force) && !test_bit(MTK_RESETTING, &eth->state))
			schedule_work(&eth->pending_work);
		else
			pr_info(" stat:disable\n");
		break;
	case 2:
		atomic_set(&eth->reset.force, 1);
		break;
	default:
		pr_info("Usage: echo [level] > /sys/kernel/debug/mtketh/reset\n");
		pr_info("Commands:   [level]\n");
		pr_info("		0	disable FE force reset\n");
		pr_info("		1	trigger FE and WDMA force reset\n");
		pr_info("		2	enable FE force reset\n");
		break;
	}

	return count;
}

static const struct file_operations fops_reg_w = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_debugfs_write,
	.llseek = noop_llseek,
};

static const struct file_operations fops_eth_reset = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_debugfs_reset,
	.llseek = noop_llseek,
};

static const struct file_operations fops_mt7530sw_reg_w = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = mtketh_mt7530sw_debugfs_write,
	.llseek = noop_llseek,
};

void mtketh_debugfs_exit(struct mtk_eth *eth)
{
	debugfs_remove_recursive(eth_debug.root);
}

int mtketh_debugfs_init(struct mtk_eth *eth)
{
	char name[16];
	int i, ret = 0;

	eth_debug.root = debugfs_create_dir("mtketh", NULL);
	if (!eth_debug.root) {
		dev_notice(eth->dev, "%s:err at %d\n", __func__, __LINE__);
		ret = -ENOMEM;
	}

	debugfs_create_file("phy_regs", 0444,
			    eth_debug.root, eth, &mtketh_debug_fops);
	debugfs_create_file("phy_reg_w", 0200,
			    eth_debug.root, eth, &fops_reg_w);
	debugfs_create_file("reset", 0444,
			    eth_debug.root, eth, &fops_eth_reset);
	if (mt7530_exist(eth)) {
		debugfs_create_file("mt7530sw_regs", 0444,
				    eth_debug.root, eth,
				    &mtketh_debug_mt7530sw_fops);
		debugfs_create_file("mt7530sw_reg_w", 0200,
				    eth_debug.root, eth,
				    &fops_mt7530sw_reg_w);
	}

	if (mtk_is_netsys_v3_or_greater(eth))
		debugfs_create_file("qdma_pppq", 0444, eth_debug.root, eth,
				    &qdma_pppq_fops);

	for (i = 0; i < (mtk_is_netsys_v2_or_greater(eth) ? 4 : 2); i++) {
		ret = snprintf(name, sizeof(name), "qdma_sch%d", i);
		if (ret != strlen(name)) {
			ret = -ENOMEM;
			goto err;
		}
		debugfs_create_file(name, 0444, eth_debug.root, (void *)(uintptr_t)i,
				    &qdma_sched_fops);
	}

	for (i = 0; i < MTK_QDMA_NUM_QUEUES; i++) {
		ret = snprintf(name, sizeof(name), "qdma_txq%d", i);
		if (ret != strlen(name)) {
			ret = -ENOMEM;
			goto err;
		}
		debugfs_create_file(name, 0444, eth_debug.root, (void *)(uintptr_t)i,
				    &qdma_queue_fops);
	}

	return 0;

err:
	debugfs_remove_recursive(eth_debug.root);
	return ret;
}

void mii_mgr_read_combine(struct mtk_eth *eth, u32 phy_addr, u32 phy_register,
			  u32 *read_data)
{
	if (mt7530_exist(eth) && phy_addr == 31)
		*read_data = mt7530_mdio_r32(eth, phy_register);

	else
		*read_data = mdiobus_read(eth->mii_bus, phy_addr, phy_register);
}

void mii_mgr_write_combine(struct mtk_eth *eth, u16 phy_addr, u16 phy_register,
			   u32 write_data)
{
	if (mt7530_exist(eth) && phy_addr == 31)
		mt7530_mdio_w32(eth, phy_register, write_data);

	else
		mdiobus_write(eth->mii_bus, phy_addr, phy_register, write_data);
}

static void mii_mgr_read_cl45(struct mtk_eth *eth, u16 port, u16 devad, u16 reg, u16 *data)
{
	*data = mdiobus_c45_read(eth->mii_bus, port, devad, reg);
}

static void mii_mgr_write_cl45(struct mtk_eth *eth, u16 port, u16 devad, u16 reg, u16 data)
{
	mdiobus_c45_write(eth->mii_bus, port, devad, reg, data);
}

int mtk_do_priv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct mtk_mac *mac = netdev_priv(dev);
	struct mtk_eth *eth = mac->hw;
	struct mtk_mii_ioctl_data mii;
	struct mtk_esw_reg reg;
	u16 val;

	switch (cmd) {
	case MTKETH_MII_READ:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_read_combine(eth, mii.phy_id, mii.reg_num,
				     &mii.val_out);
		if (copy_to_user(ifr->ifr_data, &mii, sizeof(mii)))
			goto err_copy;

		return 0;
	case MTKETH_MII_WRITE:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_write_combine(eth, mii.phy_id, mii.reg_num,
				      mii.val_in);
		return 0;
	case MTKETH_MII_READ_CL45:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		mii_mgr_read_cl45(eth,
				  mdio_phy_id_prtad(mii.phy_id),
				  mdio_phy_id_devad(mii.phy_id),
				  mii.reg_num,
				  &val);
		mii.val_out = val;
		if (copy_to_user(ifr->ifr_data, &mii, sizeof(mii)))
			goto err_copy;

		return 0;
	case MTKETH_MII_WRITE_CL45:
		if (copy_from_user(&mii, ifr->ifr_data, sizeof(mii)))
			goto err_copy;
		val = mii.val_in;
		mii_mgr_write_cl45(eth,
				  mdio_phy_id_prtad(mii.phy_id),
				  mdio_phy_id_devad(mii.phy_id),
				  mii.reg_num,
				  val);
		return 0;
	case MTKETH_ESW_REG_READ:
		if (!mt7530_exist(eth))
			return -EOPNOTSUPP;
		if (copy_from_user(&reg, ifr->ifr_data, sizeof(reg)))
			goto err_copy;
		if (reg.off > REG_ESW_MAX)
			return -EINVAL;
		reg.val = mtk_switch_r32(eth, reg.off);

		if (copy_to_user(ifr->ifr_data, &reg, sizeof(reg)))
			goto err_copy;

		return 0;
	case MTKETH_ESW_REG_WRITE:
		if (!mt7530_exist(eth))
			return -EOPNOTSUPP;
		if (copy_from_user(&reg, ifr->ifr_data, sizeof(reg)))
			goto err_copy;
		if (reg.off > REG_ESW_MAX)
			return -EINVAL;
		mtk_switch_w32(eth, reg.val, reg.off);

		return 0;
	default:
		break;
	}

	return -EOPNOTSUPP;
err_copy:
	return -EFAULT;
}

static void gdm_reg_dump_v3(struct seq_file *seq, struct mtk_eth *eth,
			    u32 gdm_id, u32 mib_base)
{
	seq_printf(seq, "| GDMA%d_RX_GBCNT  : %010u (Rx Good Bytes)	|\n",
		   gdm_id, mtk_r32(eth, mib_base));
	seq_printf(seq, "| GDMA%d_RX_GPCNT  : %010u (Rx Good Pkts)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x08));
	seq_printf(seq, "| GDMA%d_RX_OERCNT : %010u (overflow error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x10));
	seq_printf(seq, "| GDMA%d_RX_FERCNT : %010u (FCS error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x14));
	seq_printf(seq, "| GDMA%d_RX_SERCNT : %010u (too short)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x18));
	seq_printf(seq, "| GDMA%d_RX_LERCNT : %010u (too long)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x1C));
	seq_printf(seq, "| GDMA%d_RX_CERCNT : %010u (checksum error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x20));
	seq_printf(seq, "| GDMA%d_RX_FCCNT  : %010u (flow control)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x24));
	seq_printf(seq, "| GDMA%d_RX_VDPCNT : %010u (VID drop)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x28));
	seq_printf(seq, "| GDMA%d_RX_PFCCNT : %010u (priority flow control)\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x2C));
	seq_printf(seq, "| GDMA%d_TX_GBCNT  : %010u (Tx Good Bytes)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x40));
	seq_printf(seq, "| GDMA%d_TX_GPCNT  : %010u (Tx Good Pkts)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x48));
	seq_printf(seq, "| GDMA%d_TX_SKIPCNT: %010u (abort count)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x50));
	seq_printf(seq, "| GDMA%d_TX_COLCNT : %010u (collision count)|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x54));
	seq_printf(seq, "| GDMA%d_TX_OERCNT : %010u (overflow error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x58));
	seq_printf(seq, "| GDMA%d_TX_FCCNT  : %010u (flow control)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x60));
	seq_printf(seq, "| GDMA%d_TX_PFCCNT : %010u (priority flow control)\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x64));
	seq_puts(seq, "|						|\n");
}

static void gdm_reg_dump_v2(struct seq_file *seq, struct mtk_eth *eth,
			    u32 gdm_id, u32 mib_base)
{
	seq_printf(seq, "| GDMA%d_RX_GBCNT  : %010u (Rx Good Bytes)	|\n",
		   gdm_id, mtk_r32(eth, mib_base));
	seq_printf(seq, "| GDMA%d_RX_GPCNT  : %010u (Rx Good Pkts)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x08));
	seq_printf(seq, "| GDMA%d_RX_OERCNT : %010u (overflow error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x10));
	seq_printf(seq, "| GDMA%d_RX_FERCNT : %010u (FCS error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x14));
	seq_printf(seq, "| GDMA%d_RX_SERCNT : %010u (too short)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x18));
	seq_printf(seq, "| GDMA%d_RX_LERCNT : %010u (too long)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x1C));
	seq_printf(seq, "| GDMA%d_RX_CERCNT : %010u (checksum error)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x20));
	seq_printf(seq, "| GDMA%d_RX_FCCNT  : %010u (flow control)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x24));
	seq_printf(seq, "| GDMA%d_TX_SKIPCNT: %010u (abort count)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x28));
	seq_printf(seq, "| GDMA%d_TX_COLCNT : %010u (collision count)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x2C));
	seq_printf(seq, "| GDMA%d_TX_GBCNT  : %010u (Tx Good Bytes)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x30));
	seq_printf(seq, "| GDMA%d_TX_GPCNT  : %010u (Tx Good Pkts)	|\n",
		   gdm_id, mtk_r32(eth, mib_base + 0x38));
	seq_puts(seq, "|						|\n");
}

static void gdm_cnt_read(struct seq_file *seq, struct mtk_eth *eth)
{
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	struct mtk_hw_stats *hw_stats;
	u32 i, mib_base;

	seq_puts(seq, "\n			<<CPU>>\n");
	seq_puts(seq, "			   |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "|		  <<PSE>>		        |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "			   |\n");
	seq_puts(seq, "+-----------------------------------------------+\n");
	seq_puts(seq, "|		  <<GDMA>>		        |\n");

	for (i = 0; i < MTK_MAX_DEVS; i++) {
		if (!eth->mac[i] || !eth->mac[i]->hw_stats)
			continue;

		hw_stats = eth->mac[i]->hw_stats;

		mib_base = reg_map->gdm1_cnt + hw_stats->reg_offset * i;

		if (mtk_is_netsys_v3_or_greater(eth))
			gdm_reg_dump_v3(seq, eth, i + 1, mib_base);
		else
			gdm_reg_dump_v2(seq, eth, i + 1, mib_base);
	}

	seq_puts(seq, "+-----------------------------------------------+\n");
}

void dump_each_port(struct seq_file *seq, struct mtk_eth *eth, u32 base)
{
	u32 pkt_cnt = 0;
	int i = 0;

	for (i = 0; i < 7; i++) {
		if (mtk_is_netsys_v3_or_greater(eth)) {
			if ((base == 0x402C) && (i == 6))
				base = 0x408C;
			else if ((base == 0x408C) && (i == 6))
				base = 0x402C;
			else
				;
		}
		pkt_cnt = mt7530_mdio_r32(eth, (base) + (i * 0x100));
		seq_printf(seq, "%8u ", pkt_cnt);
	}
	seq_puts(seq, "\n");
}

int esw_cnt_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;

	gdm_cnt_read(seq, eth);

	if (!mt7530_exist(eth))
		return 0;

	mt798x_iomap();

	seq_printf(seq, "===================== %8s %8s %8s %8s %8s %8s %8s\n",
		   "Port0", "Port1", "Port2", "Port3", "Port4", "Port5",
		   "Port6");
	seq_puts(seq, "Tx Drop Packet      :");
	dump_each_port(seq, eth, 0x4000);
	seq_puts(seq, "Tx CRC Error        :");
	dump_each_port(seq, eth, 0x4004);
	seq_puts(seq, "Tx Unicast Packet   :");
	dump_each_port(seq, eth, 0x4008);
	seq_puts(seq, "Tx Multicast Packet :");
	dump_each_port(seq, eth, 0x400C);
	seq_puts(seq, "Tx Broadcast Packet :");
	dump_each_port(seq, eth, 0x4010);
	seq_puts(seq, "Tx Collision Event  :");
	dump_each_port(seq, eth, 0x4014);
	seq_puts(seq, "Tx Pause Packet     :");
	dump_each_port(seq, eth, 0x402C);
	seq_puts(seq, "Rx Drop Packet      :");
	dump_each_port(seq, eth, 0x4060);
	seq_puts(seq, "Rx Filtering Packet :");
	dump_each_port(seq, eth, 0x4064);
	seq_puts(seq, "Rx Unicast Packet   :");
	dump_each_port(seq, eth, 0x4068);
	seq_puts(seq, "Rx Multicast Packet :");
	dump_each_port(seq, eth, 0x406C);
	seq_puts(seq, "Rx Broadcast Packet :");
	dump_each_port(seq, eth, 0x4070);
	seq_puts(seq, "Rx Alignment Error  :");
	dump_each_port(seq, eth, 0x4074);
	seq_puts(seq, "Rx CRC Error	    :");
	dump_each_port(seq, eth, 0x4078);
	seq_puts(seq, "Rx Undersize Error  :");
	dump_each_port(seq, eth, 0x407C);
	seq_puts(seq, "Rx Fragment Error   :");
	dump_each_port(seq, eth, 0x4080);
	seq_puts(seq, "Rx Oversize Error   :");
	dump_each_port(seq, eth, 0x4084);
	seq_puts(seq, "Rx Jabber Error     :");
	dump_each_port(seq, eth, 0x4088);
	seq_puts(seq, "Rx Pause Packet     :");
	dump_each_port(seq, eth, 0x408C);
	mt7530_mdio_w32(eth, 0x4fe0, 0xf0);
	mt7530_mdio_w32(eth, 0x4fe0, 0x800000f0);

	seq_puts(seq, "\n");

	mt798x_iounmap();

	return 0;
}

static int switch_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, esw_cnt_read, 0);
}

static const struct proc_ops switch_count_fops = {
	.proc_open = switch_count_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

void mac_mib_dump(struct seq_file *seq, u32 gdm_id)
{
	struct mtk_eth *eth = g_eth;

	PRINT_FORMATTED_MAC_MIB64(seq, TX_UC_PKT_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, TX_UC_BYTE_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, TX_MC_PKT_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, TX_MC_BYTE_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, TX_BC_PKT_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, TX_BC_BYTE_CNT);

	PRINT_FORMATTED_MAC_MIB64(seq, RX_UC_PKT_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, RX_UC_BYTE_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, RX_MC_PKT_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, RX_MC_BYTE_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, RX_BC_PKT_CNT);
	PRINT_FORMATTED_MAC_MIB64(seq, RX_BC_BYTE_CNT);
}

int mac_cnt_read(struct seq_file *seq, void *v)
{
	int i;

	seq_puts(seq, "+------------------------------------+\n");
	seq_puts(seq, "|              <<GMAC>>              |\n");

	for (i = MTK_GMAC1_ID; i < MTK_GMAC_ID_MAX; i++) {
		mac_mib_dump(seq, i);
		seq_puts(seq, "|                                    |\n");
	}

	seq_puts(seq, "+------------------------------------+\n");

	return 0;
}

static int mac_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, mac_cnt_read, 0);
}

static const struct proc_ops mac_count_fops = {
	.proc_open = mac_count_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

void xfi_mib_dump(struct seq_file *seq, u32 gdm_id)
{
	struct mtk_eth *eth = g_eth;

	PRINT_FORMATTED_XFI_MIB(seq, TX_PKT_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, TX_ETH_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, TX_PAUSE_CNT, GENMASK(15, 0));
	PRINT_FORMATTED_XFI_MIB(seq, TX_BYTE_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB64(seq, TX_UC_PKT_CNT);
	PRINT_FORMATTED_XFI_MIB64(seq, TX_MC_PKT_CNT);
	PRINT_FORMATTED_XFI_MIB64(seq, TX_BC_PKT_CNT);

	PRINT_FORMATTED_XFI_MIB(seq, RX_PKT_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_ETH_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_PAUSE_CNT, GENMASK(15, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_LEN_ERR_CNT, GENMASK(15, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_CRC_ERR_CNT, GENMASK(15, 0));
	PRINT_FORMATTED_XFI_MIB64(seq, RX_UC_PKT_CNT);
	PRINT_FORMATTED_XFI_MIB64(seq, RX_MC_PKT_CNT);
	PRINT_FORMATTED_XFI_MIB64(seq, RX_BC_PKT_CNT);
	PRINT_FORMATTED_XFI_MIB(seq, RX_UC_DROP_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_BC_DROP_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_MC_DROP_CNT, GENMASK(31, 0));
	PRINT_FORMATTED_XFI_MIB(seq, RX_ALL_DROP_CNT, GENMASK(31, 0));
}

int xfi_cnt_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	int i;

	seq_puts(seq, "+------------------------------------+\n");
	seq_puts(seq, "|             <<XFI MAC>>            |\n");

	for (i = MTK_GMAC2_ID; i < MTK_GMAC_ID_MAX; i++) {
		xfi_mib_dump(seq, i);
		mtk_m32(eth, 0x1, 0x1, MTK_XFI_MIB_BASE(i) + MTK_XFI_CNT_CTRL);
		seq_puts(seq, "|                                    |\n");
	}

	seq_puts(seq, "+------------------------------------+\n");

	return 0;
}

static int xfi_count_open(struct inode *inode, struct file *file)
{
	return single_open(file, xfi_cnt_read, 0);
}

static const struct proc_ops xfi_count_fops = {
	.proc_open = xfi_count_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

static struct proc_dir_entry *proc_tx_ring, *proc_hwtx_ring, *proc_rx_ring;

int tx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	struct mtk_tx_ring *ring = &g_eth->tx_ring;
	struct mtk_tx_dma_v2 *tx_ring;
	int i = 0;

	seq_printf(seq, "free count = %d\n", (int)atomic_read(&ring->free_count));
	seq_printf(seq, "cpu next free: %d\n",
		   (int)(ring->next_free - (struct mtk_tx_dma *)ring->dma));
	seq_printf(seq, "cpu last free: %d\n",
		   (int)(ring->last_free - (struct mtk_tx_dma *)ring->dma));
	for (i = 0; i < eth->soc->tx.dma_size; i++) {
		dma_addr_t tmp = ring->phys +
				 i * (dma_addr_t)eth->soc->tx.desc_size;

		tx_ring = ring->dma + i * eth->soc->tx.desc_size;

		seq_printf(seq, "%d (%pad): %08x %08x %08x %08x", i, &tmp,
			   tx_ring->txd1, tx_ring->txd2,
			   tx_ring->txd3, tx_ring->txd4);

		if (mtk_is_netsys_v2_or_greater(eth)) {
			seq_printf(seq, " %08x %08x %08x %08x",
				   tx_ring->txd5, tx_ring->txd6,
				   tx_ring->txd7, tx_ring->txd8);
		}

		seq_puts(seq, "\n");
	}

	return 0;
}

static int tx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, tx_ring_read, NULL);
}

static const struct proc_ops tx_ring_fops = {
	.proc_open = tx_ring_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

int hwtx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	struct mtk_tx_dma_v2 *hwtx_ring;
	int i = 0;

	for (i = 0; i < MTK_QDMA_RING_SIZE; i++) {
		dma_addr_t addr = eth->fq_ring.phy_scratch_ring +
				  i * (dma_addr_t)eth->soc->tx.desc_size;

		hwtx_ring = eth->fq_ring.scratch_ring + i * eth->soc->tx.desc_size;

		seq_printf(seq, "%d (%pad): %08x %08x %08x %08x", i, &addr,
			   hwtx_ring->txd1, hwtx_ring->txd2,
			   hwtx_ring->txd3, hwtx_ring->txd4);

		if (mtk_is_netsys_v2_or_greater(eth)) {
			seq_printf(seq, " %08x %08x %08x %08x",
				   hwtx_ring->txd5, hwtx_ring->txd6,
				   hwtx_ring->txd7, hwtx_ring->txd8);
		}

		seq_puts(seq, "\n");
	}

	return 0;
}

static int hwtx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, hwtx_ring_read, NULL);
}

static const struct proc_ops hwtx_ring_fops = {
	.proc_open = hwtx_ring_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

int rx_ring_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	struct mtk_rx_ring *ring;
	struct mtk_rx_dma_v2 *rx_ring;
	int i = 0, j = 0;

	for (j = 0; j < MTK_MAX_RX_RING_NUM; j++) {
		ring = &eth->rx_ring[j];
		if (!ring->dma)
			continue;

		seq_printf(seq, "[Ring%d] next to read: %d\n", j,
			   NEXT_DESP_IDX(ring->calc_idx, eth->soc->rx.dma_size));
		for (i = 0; i < ring->dma_size; i++) {
			rx_ring = ring->dma + i * eth->soc->rx.desc_size;

			seq_printf(seq, "%04d: %08x %08x %08x %08x", i,
				   rx_ring->rxd1, rx_ring->rxd2,
				   rx_ring->rxd3, rx_ring->rxd4);

			if (mtk_is_netsys_v3_or_greater(eth)) {
				seq_printf(seq, " %08x %08x %08x %08x",
					   rx_ring->rxd5, rx_ring->rxd6,
					   rx_ring->rxd7, rx_ring->rxd8);
			}

			seq_puts(seq, "\n");
		}
	}

	return 0;
}

static int rx_ring_open(struct inode *inode, struct file *file)
{
	return single_open(file, rx_ring_read, NULL);
}

static const struct proc_ops rx_ring_fops = {
	.proc_open = rx_ring_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

static inline u32 mtk_dbg_r32(u32 reg)
{
	void __iomem *virt_reg;
	u32 val;

	virt_reg = ioremap(reg, 32);
	val = __raw_readl(virt_reg);
	iounmap(virt_reg);

	return val;
}

int dbg_regs_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	u32 i;

	seq_puts(seq, "   <<DEBUG REG DUMP>>\n");

	seq_printf(seq, "| FE_INT_STA	: %08x |\n",
		   mtk_r32(eth, MTK_FE_INT_STATUS));
	if (mtk_is_netsys_v2_or_greater(eth))
		seq_printf(seq, "| FE_INT_STA2	: %08x |\n",
			   mtk_r32(eth, MTK_FE_INT_STATUS2));

	seq_printf(seq, "| PSE_FQFC_CFG	: %08x |\n",
		   mtk_r32(eth, MTK_PSE_FQFC_CFG));
	seq_printf(seq, "| PSE_IQ_STA1	: %08x |\n",
		   mtk_r32(eth, reg_map->pse_iq_sta + 0x00));
	seq_printf(seq, "| PSE_IQ_STA2	: %08x |\n",
		   mtk_r32(eth, reg_map->pse_iq_sta + 0x04));

	if (mtk_is_netsys_v2_or_greater(eth)) {
		seq_printf(seq, "| PSE_IQ_STA3	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_iq_sta + 0x08));
		seq_printf(seq, "| PSE_IQ_STA4	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_iq_sta + 0x0c));
		seq_printf(seq, "| PSE_IQ_STA5	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_iq_sta + 0x10));
		seq_printf(seq, "| PSE_IQ_STA6	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_iq_sta + 0x14));
		seq_printf(seq, "| PSE_IQ_STA7	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_iq_sta + 0x18));
		seq_printf(seq, "| PSE_IQ_STA8	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_iq_sta + 0x1c));
	}

	seq_printf(seq, "| PSE_OQ_STA1	: %08x |\n",
		   mtk_r32(eth, reg_map->pse_oq_sta + 0x00));
	seq_printf(seq, "| PSE_OQ_STA2	: %08x |\n",
		   mtk_r32(eth, reg_map->pse_oq_sta + 0x04));

	if (mtk_is_netsys_v2_or_greater(eth)) {
		seq_printf(seq, "| PSE_OQ_STA3	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_oq_sta + 0x08));
		seq_printf(seq, "| PSE_OQ_STA4	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_oq_sta + 0x0c));
		seq_printf(seq, "| PSE_OQ_STA5	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_oq_sta + 0x10));
		seq_printf(seq, "| PSE_OQ_STA6	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_oq_sta + 0x14));
		seq_printf(seq, "| PSE_OQ_STA7	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_oq_sta + 0x18));
		seq_printf(seq, "| PSE_OQ_STA8	: %08x |\n",
			   mtk_r32(eth, reg_map->pse_oq_sta + 0x1c));
	}

	seq_printf(seq, "| PDMA_CRX_IDX	: %08x |\n",
		   mtk_r32(eth, reg_map->pdma.pcrx_ptr));
	seq_printf(seq, "| PDMA_DRX_IDX	: %08x |\n",
		   mtk_r32(eth, reg_map->pdma.pdrx_ptr));
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_RSS)) {
		for (i = 1; i < eth->soc->rss_num; i++) {
			seq_printf(seq, "| PDMA_CRX_IDX%d	: %08x |\n", i,
				   mtk_r32(eth, reg_map->pdma.pcrx_ptr +
					   i * MTK_QRX_OFFSET));
			seq_printf(seq, "| PDMA_DRX_IDX%d	: %08x |\n", i,
				   mtk_r32(eth, reg_map->pdma.pdrx_ptr +
					   i * MTK_QRX_OFFSET));
		}
	}
	if (MTK_HAS_CAPS(eth->soc->caps, MTK_HWLRO)) {
		for (i = 0; i < MTK_HW_LRO_RING_NUM; i++) {
			seq_printf(seq, "| PDMA_CRX_IDX%d	: %08x |\n",
				   MTK_HW_LRO_RING(i),
				   mtk_r32(eth, reg_map->pdma.pcrx_ptr +
					   MTK_HW_LRO_RING(i) * MTK_QRX_OFFSET));
			seq_printf(seq, "| PDMA_DRX_IDX%d	: %08x |\n",
				   MTK_HW_LRO_RING(i),
				   mtk_r32(eth, reg_map->pdma.pdrx_ptr +
					   MTK_HW_LRO_RING(i) * MTK_QRX_OFFSET));
		}
	}
	seq_printf(seq, "| QDMA_CTX_IDX	: %08x |\n",
		   mtk_r32(eth, reg_map->qdma.ctx_ptr));
	seq_printf(seq, "| QDMA_DTX_IDX	: %08x |\n",
		   mtk_r32(eth, reg_map->qdma.dtx_ptr));
	seq_printf(seq, "| QDMA_FQ_CNT	: %08x |\n",
		   mtk_r32(eth, reg_map->qdma.fq_count));
	seq_printf(seq, "| QDMA_FWD_CNT	: %08x |\n",
		   mtk_r32(eth, reg_map->qdma.fwd_count));
	seq_printf(seq, "| QDMA_FSM	: %08x |\n",
		   mtk_r32(eth, reg_map->qdma.fsm));
	seq_printf(seq, "| FE_PSE_FREE	: %08x |\n",
		   mtk_r32(eth, MTK_FE_PSE_FREE));
	seq_printf(seq, "| FE_DROP_FQ	: %08x |\n",
		   mtk_r32(eth, MTK_FE_DROP_FQ));
	seq_printf(seq, "| FE_DROP_FC	: %08x |\n",
		   mtk_r32(eth, MTK_FE_DROP_FC));
	seq_printf(seq, "| FE_DROP_PPE	: %08x |\n",
		   mtk_r32(eth, MTK_FE_DROP_PPE));
	seq_printf(seq, "| GDM1_IG_CTRL	: %08x |\n",
		   mtk_r32(eth, MTK_GDMA_FWD_CFG(0)));
	seq_printf(seq, "| GDM2_IG_CTRL	: %08x |\n",
		   mtk_r32(eth, MTK_GDMA_FWD_CFG(1)));
	if (mtk_is_netsys_v3_or_greater(eth)) {
		seq_printf(seq, "| GDM3_IG_CTRL	: %08x |\n",
			   mtk_r32(eth, MTK_GDMA_FWD_CFG(2)));
	}
	seq_printf(seq, "| MAC_P1_MCR	: %08x |\n",
		   mtk_r32(eth, MTK_MAC_MCR(0)));
	seq_printf(seq, "| MAC_P2_MCR	: %08x |\n",
		   mtk_r32(eth, MTK_MAC_MCR(1)));
	if (mtk_is_netsys_v3_or_greater(eth)) {
		seq_printf(seq, "| MAC_P3_MCR	: %08x |\n",
			   mtk_r32(eth, MTK_MAC_MCR(2)));
	}
	seq_printf(seq, "| MAC_P1_FSM	: %08x |\n",
		   mtk_r32(eth, MTK_MAC_FSM(0)));
	seq_printf(seq, "| MAC_P2_FSM	: %08x |\n",
		   mtk_r32(eth, MTK_MAC_FSM(1)));
	if (mtk_is_netsys_v3_or_greater(eth)) {
		seq_printf(seq, "| MAC_P3_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_MAC_FSM(2)));
	}

	if (mtk_is_netsys_v3_or_greater(eth)) {
		seq_printf(seq, "| XMAC_P1_MCR	: %08x |\n",
			   mtk_r32(eth, MTK_XMAC_MCR(1)));
		seq_printf(seq, "| XMAC_P1_STS	: %08x |\n",
			   mtk_r32(eth, MTK_HAS_CAPS(eth->soc->caps, MTK_XGMAC_V2) ?
					MTK_XMAC_STS(1) : MTK_XGMAC_STS(1)));
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_GMAC3_USXGMII)) {
			seq_printf(seq, "| XMAC_P2_MCR	: %08x |\n",
				   mtk_r32(eth, MTK_XMAC_MCR(2)));
			seq_printf(seq, "| XMAC_P2_STS	: %08x |\n",
				   mtk_r32(eth, MTK_HAS_CAPS(eth->soc->caps, MTK_XGMAC_V2) ?
						MTK_XMAC_STS(2) : MTK_XGMAC_STS(2)));
		}
	}

	if (mtk_is_netsys_v2_or_greater(eth)) {
		seq_printf(seq, "| FE_CDM1_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM1_FSM));
		seq_printf(seq, "| FE_CDM2_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM2_FSM));
		seq_printf(seq, "| FE_CDM3_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM3_FSM));
		seq_printf(seq, "| FE_CDM4_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM4_FSM));
		seq_printf(seq, "| FE_CDM5_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM5_FSM));
		seq_printf(seq, "| FE_CDM6_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM6_FSM));
		seq_printf(seq, "| FE_CDM7_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_CDM7_FSM));
		seq_printf(seq, "| FE_GDM1_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_GDM1_FSM));
		seq_printf(seq, "| FE_GDM2_FSM	: %08x |\n",
			   mtk_r32(eth, MTK_FE_GDM2_FSM));
		if (mtk_is_netsys_v3_or_greater(eth)) {
			seq_printf(seq, "| FE_GDM3_FSM	: %08x |\n",
				   mtk_r32(eth, MTK_FE_GDM3_FSM));
		}
		seq_printf(seq, "| SGMII_EFUSE	: %08x |\n",
			   mtk_dbg_r32(MTK_SGMII_EFUSE));
		seq_printf(seq, "| SGMII0_RX_CNT : %08x |\n",
			   mtk_dbg_r32(MTK_SGMII_FALSE_CARRIER_CNT(0)));
		seq_printf(seq, "| SGMII1_RX_CNT : %08x |\n",
			   mtk_dbg_r32(MTK_SGMII_FALSE_CARRIER_CNT(1)));
		seq_printf(seq, "| WED_RTQM_GLO	: %08x |\n",
			   mtk_dbg_r32(MTK_WED_RTQM_GLO_CFG));
	}

	mtk_w32(eth, 0xffffffff, MTK_FE_INT_STATUS);
	if (mtk_is_netsys_v2_or_greater(eth))
		mtk_w32(eth, 0xffffffff, MTK_FE_INT_STATUS2);

	return 0;
}

static int dbg_regs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dbg_regs_read, 0);
}

static const struct proc_ops dbg_regs_fops = {
	.proc_open = dbg_regs_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};

void hw_lro_stats_update(u32 ring_no, struct mtk_rx_dma_v2 *rxd)
{
	struct mtk_eth *eth = g_eth;
	u32 idx, agg_cnt, agg_size;

	if (mtk_is_netsys_v3_or_greater(eth)) {
		idx = ring_no - 4;
		agg_cnt = FIELD_GET(RX_DMA_GET_AGG_CNT_V2, rxd->rxd6);
	} else {
		idx = ring_no - 1;
		agg_cnt = FIELD_GET(RX_DMA_GET_AGG_CNT, rxd->rxd2);
	}

	if (idx >= MTK_HW_LRO_RING_NUM)
		return;

	agg_size = RX_DMA_GET_PLEN0(rxd->rxd2);

	hw_lro_agg_size_cnt[idx][agg_size / 5000]++;
	hw_lro_agg_num_cnt[idx][agg_cnt]++;
	hw_lro_tot_flush_cnt[idx]++;
	hw_lro_tot_agg_cnt[idx] += agg_cnt;
}

void hw_lro_flush_stats_update(u32 ring_no, struct mtk_rx_dma_v2 *rxd)
{
	struct mtk_eth *eth = g_eth;
	u32 idx, flush_reason;

	if (mtk_is_netsys_v3_or_greater(eth)) {
		idx = ring_no - 4;
		flush_reason = FIELD_GET(RX_DMA_GET_FLUSH_RSN_V2, rxd->rxd6);
	} else {
		idx = ring_no - 1;
		flush_reason = FIELD_GET(RX_DMA_GET_REV, rxd->rxd2);
	}

	if (idx >= MTK_HW_LRO_RING_NUM)
		return;

	if ((flush_reason & 0x7) == MTK_HW_LRO_AGG_FLUSH)
		hw_lro_agg_flush_cnt[idx]++;
	else if ((flush_reason & 0x7) == MTK_HW_LRO_AGE_FLUSH)
		hw_lro_age_flush_cnt[idx]++;
	else if ((flush_reason & 0x7) == MTK_HW_LRO_NOT_IN_SEQ_FLUSH)
		hw_lro_seq_flush_cnt[idx]++;
	else if ((flush_reason & 0x7) == MTK_HW_LRO_TIMESTAMP_FLUSH)
		hw_lro_timestamp_flush_cnt[idx]++;
	else if ((flush_reason & 0x7) == MTK_HW_LRO_NON_RULE_FLUSH)
		hw_lro_norule_flush_cnt[idx]++;
}

ssize_t hw_lro_stats_write(struct file *file, const char __user *buffer,
			   size_t count, loff_t *data)
{
	memset(hw_lro_agg_num_cnt, 0, sizeof(hw_lro_agg_num_cnt));
	memset(hw_lro_agg_size_cnt, 0, sizeof(hw_lro_agg_size_cnt));
	memset(hw_lro_tot_agg_cnt, 0, sizeof(hw_lro_tot_agg_cnt));
	memset(hw_lro_tot_flush_cnt, 0, sizeof(hw_lro_tot_flush_cnt));
	memset(hw_lro_agg_flush_cnt, 0, sizeof(hw_lro_agg_flush_cnt));
	memset(hw_lro_age_flush_cnt, 0, sizeof(hw_lro_age_flush_cnt));
	memset(hw_lro_seq_flush_cnt, 0, sizeof(hw_lro_seq_flush_cnt));
	memset(hw_lro_timestamp_flush_cnt, 0,
	       sizeof(hw_lro_timestamp_flush_cnt));
	memset(hw_lro_norule_flush_cnt, 0, sizeof(hw_lro_norule_flush_cnt));

	pr_info("clear hw lro cnt table\n");

	return count;
}

int hw_lro_stats_read_v1(struct seq_file *seq, void *v)
{
	int i;

	seq_puts(seq, "HW LRO statistic dump:\n");

	/* Agg number count */
	seq_puts(seq, "Cnt:   RING1 | RING2 | RING3 | Total\n");
	for (i = 0; i <= MTK_HW_LRO_MAX_AGG_CNT; i++) {
		seq_printf(seq, " %d :      %d        %d        %d        %d\n",
			   i, hw_lro_agg_num_cnt[0][i],
			   hw_lro_agg_num_cnt[1][i], hw_lro_agg_num_cnt[2][i],
			   hw_lro_agg_num_cnt[0][i] + hw_lro_agg_num_cnt[1][i] +
			   hw_lro_agg_num_cnt[2][i]);
	}

	/* Total agg count */
	seq_puts(seq, "Total agg:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   hw_lro_tot_agg_cnt[0], hw_lro_tot_agg_cnt[1],
		   hw_lro_tot_agg_cnt[2],
		   hw_lro_tot_agg_cnt[0] + hw_lro_tot_agg_cnt[1] +
		   hw_lro_tot_agg_cnt[2]);

	/* Total flush count */
	seq_puts(seq, "Total flush:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   hw_lro_tot_flush_cnt[0], hw_lro_tot_flush_cnt[1],
		   hw_lro_tot_flush_cnt[2],
		   hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		   hw_lro_tot_flush_cnt[2]);

	/* Avg agg count */
	seq_puts(seq, "Avg agg:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "                %d      %d      %d      %d\n",
		   (hw_lro_tot_flush_cnt[0]) ?
		    hw_lro_tot_agg_cnt[0] / hw_lro_tot_flush_cnt[0] : 0,
		   (hw_lro_tot_flush_cnt[1]) ?
		    hw_lro_tot_agg_cnt[1] / hw_lro_tot_flush_cnt[1] : 0,
		   (hw_lro_tot_flush_cnt[2]) ?
		    hw_lro_tot_agg_cnt[2] / hw_lro_tot_flush_cnt[2] : 0,
		   (hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		    hw_lro_tot_flush_cnt[2]) ?
		    ((hw_lro_tot_agg_cnt[0] + hw_lro_tot_agg_cnt[1] +
		      hw_lro_tot_agg_cnt[2]) / (hw_lro_tot_flush_cnt[0] +
		      hw_lro_tot_flush_cnt[1] + hw_lro_tot_flush_cnt[2])) : 0);

	/*  Statistics of aggregation size counts */
	seq_puts(seq, "HW LRO flush pkt len:\n");
	seq_puts(seq, " Length  | RING1  | RING2  | RING3  | Total\n");
	for (i = 0; i < 15; i++) {
		seq_printf(seq, "%d~%d: %d      %d      %d      %d\n", i * 5000,
			   (i + 1) * 5000, hw_lro_agg_size_cnt[0][i],
			   hw_lro_agg_size_cnt[1][i], hw_lro_agg_size_cnt[2][i],
			   hw_lro_agg_size_cnt[0][i] +
			   hw_lro_agg_size_cnt[1][i] +
			   hw_lro_agg_size_cnt[2][i]);
	}

	seq_puts(seq, "Flush reason:   RING1 | RING2 | RING3 | Total\n");
	seq_printf(seq, "AGG timeout:      %d      %d      %d      %d\n",
		   hw_lro_agg_flush_cnt[0], hw_lro_agg_flush_cnt[1],
		   hw_lro_agg_flush_cnt[2],
		   (hw_lro_agg_flush_cnt[0] + hw_lro_agg_flush_cnt[1] +
		    hw_lro_agg_flush_cnt[2]));

	seq_printf(seq, "AGE timeout:      %d      %d      %d      %d\n",
		   hw_lro_age_flush_cnt[0], hw_lro_age_flush_cnt[1],
		   hw_lro_age_flush_cnt[2],
		   (hw_lro_age_flush_cnt[0] + hw_lro_age_flush_cnt[1] +
		    hw_lro_age_flush_cnt[2]));

	seq_printf(seq, "Not in-sequence:  %d      %d      %d      %d\n",
		   hw_lro_seq_flush_cnt[0], hw_lro_seq_flush_cnt[1],
		   hw_lro_seq_flush_cnt[2],
		   (hw_lro_seq_flush_cnt[0] + hw_lro_seq_flush_cnt[1] +
		    hw_lro_seq_flush_cnt[2]));

	seq_printf(seq, "Timestamp:        %d      %d      %d      %d\n",
		   hw_lro_timestamp_flush_cnt[0],
		   hw_lro_timestamp_flush_cnt[1],
		   hw_lro_timestamp_flush_cnt[2],
		   (hw_lro_timestamp_flush_cnt[0] +
		    hw_lro_timestamp_flush_cnt[1] +
		    hw_lro_timestamp_flush_cnt[2]));

	seq_printf(seq, "No LRO rule:      %d      %d      %d      %d\n",
		   hw_lro_norule_flush_cnt[0],
		   hw_lro_norule_flush_cnt[1],
		   hw_lro_norule_flush_cnt[2],
		   (hw_lro_norule_flush_cnt[0] +
		    hw_lro_norule_flush_cnt[1] +
		    hw_lro_norule_flush_cnt[2]));

	return 0;
}

int hw_lro_stats_read_v2(struct seq_file *seq, void *v)
{
	int i;

	seq_puts(seq, "HW LRO statistic dump:\n");

	/* Agg number count */
	seq_puts(seq, "Cnt:   RING4 | RING5 | RING6 | RING7 Total\n");
	for (i = 0; i <= MTK_HW_LRO_MAX_AGG_CNT; i++) {
		seq_printf(seq,
			   " %d :      %d        %d        %d        %d        %d\n",
			   i, hw_lro_agg_num_cnt[0][i], hw_lro_agg_num_cnt[1][i],
			   hw_lro_agg_num_cnt[2][i], hw_lro_agg_num_cnt[3][i],
			   hw_lro_agg_num_cnt[0][i] + hw_lro_agg_num_cnt[1][i] +
			   hw_lro_agg_num_cnt[2][i] + hw_lro_agg_num_cnt[3][i]);
	}

	/* Total agg count */
	seq_puts(seq, "Total agg:   RING4 | RING5 | RING6 | RING7 Total\n");
	seq_printf(seq, "                %d      %d      %d      %d      %d\n",
		   hw_lro_tot_agg_cnt[0], hw_lro_tot_agg_cnt[1],
		   hw_lro_tot_agg_cnt[2], hw_lro_tot_agg_cnt[3],
		   hw_lro_tot_agg_cnt[0] + hw_lro_tot_agg_cnt[1] +
		   hw_lro_tot_agg_cnt[2] + hw_lro_tot_agg_cnt[3]);

	/* Total flush count */
	seq_puts(seq, "Total flush:   RING4 | RING5 | RING6 | RING7 Total\n");
	seq_printf(seq, "                %d      %d      %d      %d      %d\n",
		   hw_lro_tot_flush_cnt[0], hw_lro_tot_flush_cnt[1],
		   hw_lro_tot_flush_cnt[2], hw_lro_tot_flush_cnt[3],
		   hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		   hw_lro_tot_flush_cnt[2] + hw_lro_tot_flush_cnt[3]);

	/* Avg agg count */
	seq_puts(seq, "Avg agg:   RING4 | RING5 | RING6 | RING7 Total\n");
	seq_printf(seq, "                %d      %d      %d      %d      %d\n",
		   (hw_lro_tot_flush_cnt[0]) ?
		    hw_lro_tot_agg_cnt[0] / hw_lro_tot_flush_cnt[0] : 0,
		   (hw_lro_tot_flush_cnt[1]) ?
		    hw_lro_tot_agg_cnt[1] / hw_lro_tot_flush_cnt[1] : 0,
		   (hw_lro_tot_flush_cnt[2]) ?
		    hw_lro_tot_agg_cnt[2] / hw_lro_tot_flush_cnt[2] : 0,
		   (hw_lro_tot_flush_cnt[3]) ?
		    hw_lro_tot_agg_cnt[3] / hw_lro_tot_flush_cnt[3] : 0,
		   (hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		    hw_lro_tot_flush_cnt[2] + hw_lro_tot_flush_cnt[3]) ?
		    ((hw_lro_tot_agg_cnt[0] + hw_lro_tot_agg_cnt[1] +
		      hw_lro_tot_agg_cnt[2] + hw_lro_tot_agg_cnt[3]) /
		     (hw_lro_tot_flush_cnt[0] + hw_lro_tot_flush_cnt[1] +
		      hw_lro_tot_flush_cnt[2] + hw_lro_tot_flush_cnt[3])) : 0);

	/*  Statistics of aggregation size counts */
	seq_puts(seq, "HW LRO flush pkt len:\n");
	seq_puts(seq, " Length  | RING4  | RING5  | RING6  | RING7 Total\n");
	for (i = 0; i < 15; i++) {
		seq_printf(seq, "%d~%d: %d      %d      %d      %d      %d\n",
			   i * 5000, (i + 1) * 5000,
			   hw_lro_agg_size_cnt[0][i], hw_lro_agg_size_cnt[1][i],
			   hw_lro_agg_size_cnt[2][i], hw_lro_agg_size_cnt[3][i],
			   hw_lro_agg_size_cnt[0][i] +
			   hw_lro_agg_size_cnt[1][i] +
			   hw_lro_agg_size_cnt[2][i] +
			   hw_lro_agg_size_cnt[3][i]);
	}

	seq_puts(seq, "Flush reason:   RING4 | RING5 | RING6 | RING7 Total\n");
	seq_printf(seq, "AGG timeout:      %d      %d      %d      %d      %d\n",
		   hw_lro_agg_flush_cnt[0], hw_lro_agg_flush_cnt[1],
		   hw_lro_agg_flush_cnt[2], hw_lro_agg_flush_cnt[3],
		   (hw_lro_agg_flush_cnt[0] + hw_lro_agg_flush_cnt[1] +
		    hw_lro_agg_flush_cnt[2] + hw_lro_agg_flush_cnt[3]));

	seq_printf(seq, "AGE timeout:      %d      %d      %d      %d      %d\n",
		   hw_lro_age_flush_cnt[0], hw_lro_age_flush_cnt[1],
		   hw_lro_age_flush_cnt[2], hw_lro_age_flush_cnt[3],
		   (hw_lro_age_flush_cnt[0] + hw_lro_age_flush_cnt[1] +
		    hw_lro_age_flush_cnt[2] + hw_lro_age_flush_cnt[3]));

	seq_printf(seq, "Not in-sequence:  %d      %d      %d      %d      %d\n",
		   hw_lro_seq_flush_cnt[0], hw_lro_seq_flush_cnt[1],
		   hw_lro_seq_flush_cnt[2], hw_lro_seq_flush_cnt[3],
		   (hw_lro_seq_flush_cnt[0] + hw_lro_seq_flush_cnt[1] +
		    hw_lro_seq_flush_cnt[2] + hw_lro_seq_flush_cnt[3]));

	seq_printf(seq, "Timestamp:        %d      %d      %d      %d      %d\n",
		   hw_lro_timestamp_flush_cnt[0],
		   hw_lro_timestamp_flush_cnt[1],
		   hw_lro_timestamp_flush_cnt[2],
		   hw_lro_timestamp_flush_cnt[3],
		   (hw_lro_timestamp_flush_cnt[0] +
		    hw_lro_timestamp_flush_cnt[1] +
		    hw_lro_timestamp_flush_cnt[2] +
		    hw_lro_timestamp_flush_cnt[3]));

	seq_printf(seq, "No LRO rule:      %d      %d      %d      %d      %d\n",
		   hw_lro_norule_flush_cnt[0],
		   hw_lro_norule_flush_cnt[1],
		   hw_lro_norule_flush_cnt[2],
		   hw_lro_norule_flush_cnt[3],
		   (hw_lro_norule_flush_cnt[0] +
		    hw_lro_norule_flush_cnt[1] +
		    hw_lro_norule_flush_cnt[2] +
		    hw_lro_norule_flush_cnt[3]));

	return 0;
}

int hw_lro_stats_read_wrapper(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;

	if (mtk_is_netsys_v3_or_greater(eth))
		hw_lro_stats_read_v2(seq, v);
	else
		hw_lro_stats_read_v1(seq, v);

	return 0;
}

static int hw_lro_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_lro_stats_read_wrapper, NULL);
}

static const struct proc_ops hw_lro_stats_fops = {
	.proc_open = hw_lro_stats_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_write = hw_lro_stats_write,
	.proc_release = single_release
};

int hwlro_agg_cnt_ctrl(int cnt)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	int i;

	for (i = 1; i <= MTK_HW_LRO_RING_NUM; i++)
		SET_PDMA_RXRING_MAX_AGG_CNT(eth, i, cnt);

	return 0;
}

int hwlro_agg_time_ctrl(int time)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	int i;

	for (i = 1; i <= MTK_HW_LRO_RING_NUM; i++)
		SET_PDMA_RXRING_AGG_TIME(eth, i, time);

	return 0;
}

int hwlro_age_time_ctrl(int time)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	int i;

	for (i = 1; i <= MTK_HW_LRO_RING_NUM; i++)
		SET_PDMA_RXRING_AGE_TIME(eth, i, time);
	return 0;
}

int hwlro_threshold_ctrl(int bandwidth)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;

	SET_PDMA_LRO_BW_THRESHOLD(eth, bandwidth);

	return 0;
}

int hwlro_ring_enable_ctrl(int enable)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	int i;

	pr_info("[%s] %s HW LRO rings\n", __func__, (enable) ? "Enable" : "Disable");

	for (i = 1; i <= MTK_HW_LRO_RING_NUM; i++)
		SET_PDMA_RXRING_VALID(eth, i, enable);

	return 0;
}

int hwlro_stats_enable_ctrl(int enable)
{
	pr_info("[%s] %s HW LRO statistics\n", __func__, (enable) ? "Enable" : "Disable");
	mtk_hwlro_stats_ebl = enable;

	return 0;
}

static const mtk_lro_dbg_func lro_dbg_func[] = {
	[0] = hwlro_agg_cnt_ctrl,
	[1] = hwlro_agg_time_ctrl,
	[2] = hwlro_age_time_ctrl,
	[3] = hwlro_threshold_ctrl,
	[4] = hwlro_ring_enable_ctrl,
	[5] = hwlro_stats_enable_ctrl,
};

ssize_t hw_lro_auto_tlb_write(struct file *file, const char __user *buffer,
			      size_t count, loff_t *data)
{
	char buf[32];
	char *p_buf;
	char *p_token = NULL;
	char *p_delimiter = " \t";
	long x = 0, y = 0;
	u32 len = count;
	int ret;

	if (len >= sizeof(buf)) {
		pr_info("Input handling fail!\n");
		return -1;
	}

	if (copy_from_user(buf, buffer, len))
		return -EFAULT;

	buf[len] = '\0';

	p_buf = buf;
	p_token = strsep(&p_buf, p_delimiter);
	if (!p_token)
		x = 0;
	else
		ret = kstrtol(p_token, 10, &x);

	p_token = strsep(&p_buf, "\t\n ");
	if (p_token)
		ret = kstrtol(p_token, 10, &y);

	if (lro_dbg_func[x] && (ARRAY_SIZE(lro_dbg_func) > x))
		(*lro_dbg_func[x]) (y);

	return count;
}

void hw_lro_auto_tlb_dump_v1(struct seq_file *seq, u32 index)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	struct mtk_lro_alt_v1 alt;
	__be32 addr;
	u32 tlb_info[9];
	u32 dw_len, cnt, priority;
	u32 entry;
	int i;

	if (index > 4)
		index = index - 1;
	entry = (index * 9) + 1;

	/* read valid entries of the auto-learn table */
	mtk_w32(eth, entry, MTK_FE_ALT_CF8);

	for (i = 0; i < 9; i++)
		tlb_info[i] = mtk_r32(eth, MTK_FE_ALT_SEQ_CFC);

	memcpy(&alt, tlb_info, sizeof(struct mtk_lro_alt_v1));

	dw_len = alt.alt_info7.dw_len;
	cnt = alt.alt_info6.cnt;

	if (mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0) & MTK_LRO_ALT_PKT_CNT_MODE)
		priority = cnt;		/* packet count */
	else
		priority = dw_len;	/* byte count */

	/* dump valid entries of the auto-learn table */
	if (index >= 4)
		seq_printf(seq, "\n===== TABLE Entry: %d (Act) =====\n", index);
	else
		seq_printf(seq, "\n===== TABLE Entry: %d (LRU) =====\n", index);

	if (alt.alt_info8.ipv4) {
		addr = htonl(alt.alt_info1.sip0);
		seq_printf(seq, "SIP = %pI4 (IPv4)\n", &addr);
	} else {
		seq_printf(seq, "SIP = %08X:%08X:%08X:%08X (IPv6)\n",
			   alt.alt_info4.sip3, alt.alt_info3.sip2,
			   alt.alt_info2.sip1, alt.alt_info1.sip0);
	}

	seq_printf(seq, "DIP_ID = %d\n", alt.alt_info8.dip_id);
	seq_printf(seq, "TCP SPORT = %d | TCP DPORT = %d\n",
		   alt.alt_info0.stp, alt.alt_info0.dtp);
	seq_printf(seq, "VLAN_VID_VLD = %d\n", alt.alt_info6.vlan_vid_vld);
	seq_printf(seq, "VLAN1 = %d | VLAN2 = %d | VLAN3 = %d | VLAN4 =%d\n",
		   (alt.alt_info5.vlan_vid0 & 0xfff),
		   ((alt.alt_info5.vlan_vid0 >> 12) & 0xfff),
		   ((alt.alt_info6.vlan_vid1 << 8) |
		   ((alt.alt_info5.vlan_vid0 >> 24) & 0xfff)),
		   ((alt.alt_info6.vlan_vid1 >> 4) & 0xfff));
	seq_printf(seq, "TPUT = %d | FREQ = %d\n", dw_len, cnt);
	seq_printf(seq, "PRIORITY = %d\n", priority);
}

void hw_lro_auto_tlb_dump_v2(struct seq_file *seq, u32 index)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	struct mtk_lro_alt_v2 alt;
	u32 score = 0, ipv4 = 0;
	u32 ipv6[4] = { 0 };
	u32 tlb_info[12];
	int i;

	/* read valid entries of the auto-learn table */
	mtk_w32(eth, index << MTK_LRO_ALT_INDEX_OFFSET, reg_map->pdma.lro_alt_dbg);

	for (i = 0; i < 11; i++)
		tlb_info[i] = mtk_r32(eth, reg_map->pdma.lro_alt_dbg_data);

	memcpy(&alt, tlb_info, sizeof(struct mtk_lro_alt_v2));

	if (mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW0) & MTK_LRO_ALT_PKT_CNT_MODE)
		score = 1;	/* packet count */
	else
		score = 0;	/* byte count */

	/* dump valid entries of the auto-learn table */
	if (alt.alt_info0.valid) {
		if (index < 5)
			seq_printf(seq,
				   "\n===== TABLE Entry: %d (onging) =====\n",
				   index);
		else
			seq_printf(seq,
				   "\n===== TABLE Entry: %d (candidate) =====\n",
				   index);

		if (alt.alt_info1.v4_valid) {
			ipv4 = (alt.alt_info4.sip0_h << 23) |
				alt.alt_info5.sip0_l;
			seq_printf(seq, "SIP = 0x%x: (IPv4)\n", ipv4);

			ipv4 = (alt.alt_info8.dip0_h << 23) |
				alt.alt_info9.dip0_l;
			seq_printf(seq, "DIP = 0x%x: (IPv4)\n", ipv4);
		} else if (alt.alt_info1.v6_valid) {
			ipv6[3] = (alt.alt_info1.sip3_h << 23) |
				   (alt.alt_info2.sip3_l << 9);
			ipv6[2] = (alt.alt_info2.sip2_h << 23) |
				   (alt.alt_info3.sip2_l << 9);
			ipv6[1] = (alt.alt_info3.sip1_h << 23) |
				   (alt.alt_info4.sip1_l << 9);
			ipv6[0] = (alt.alt_info4.sip0_h << 23) |
				   (alt.alt_info5.sip0_l << 9);
			seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",
				   ipv6[3], ipv6[2], ipv6[1], ipv6[0]);

			ipv6[3] = (alt.alt_info5.dip3_h << 23) |
				   (alt.alt_info6.dip3_l << 9);
			ipv6[2] = (alt.alt_info6.dip2_h << 23) |
				   (alt.alt_info7.dip2_l << 9);
			ipv6[1] = (alt.alt_info7.dip1_h << 23) |
				   (alt.alt_info8.dip1_l << 9);
			ipv6[0] = (alt.alt_info8.dip0_h << 23) |
				   (alt.alt_info9.dip0_l << 9);
			seq_printf(seq, "DIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",
				   ipv6[3], ipv6[2], ipv6[1], ipv6[0]);
		}

		seq_printf(seq, "TCP SPORT = %d | TCP DPORT = %d\n",
			   (alt.alt_info9.sp_h << 7) | (alt.alt_info10.sp_l),
			   alt.alt_info10.dp);
	}
}

void hw_lro_auto_tlb_dump_v3(struct seq_file *seq, u32 index)
{
	struct mtk_eth *eth = g_eth;
	u32 val, sport, dport, vld, dip_idx, mode;
	u32 ipv6[4] = { 0 };
	u32 ipv4 = 0;
	int ret;

	/* dump the LRO_DATA table for the specific entry */
	val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
	val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DATA_BASE + index);
	val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
	mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
	/* check if the GLO_MEM access is successful */
	ret = FIELD_GET(MTK_GLO_MEM_CMD, mtk_r32(eth, MTK_GLO_MEM_CTRL));
	if (ret != 0) {
		pr_warn("GLO_MEM read/write error\n");
		return;
	}

	/* dump the valid entries of the auto-learn table */
	vld = FIELD_GET(MTK_LRO_DATA_VLD, mtk_r32(eth, MTK_GLO_MEM_DATA(6)));
	if (vld) {
		if (index < 5)
			seq_printf(seq,
				   "\n===== TABLE Entry: %d (onging) =====\n",
				   index);
		else
			seq_printf(seq,
				   "\n===== TABLE Entry: %d (candidate) =====\n",
				   index);

		/* determine the DIP index for the specific entry */
		dip_idx = FIELD_GET(MTK_LRO_DATA_DIP_IDX, mtk_r32(eth, MTK_GLO_MEM_DATA(5)));
		/* switch to the LRO_DIP table for the specific DIP index */
		val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
		val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DIP_BASE + dip_idx);
		val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
		mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
		/* determine the mode for the specific DIP index */
		mode = FIELD_GET(MTK_LRO_DIP_MODE, mtk_r32(eth, MTK_GLO_MEM_DATA(4)));
		/* dump the SIP and DIP */
		if (mode == MTK_LRO_DIP_IPV4) {
			val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
			val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DATA_BASE + index);
			val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
			mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
			ipv4 = mtk_r32(eth, MTK_GLO_MEM_DATA(1));
			seq_printf(seq, "SIP = 0x%x: (IPv4)\n", ipv4);
			val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
			val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DIP_BASE + dip_idx);
			val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
			mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
			ipv4 = mtk_r32(eth, MTK_GLO_MEM_DATA(0));
			seq_printf(seq, "DIP = 0x%x: (IPv4)\n", ipv4);
		} else if (mode == MTK_LRO_DIP_IPV6) {
			val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
			val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DATA_BASE + index);
			val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
			mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
			ipv6[3] = mtk_r32(eth, MTK_GLO_MEM_DATA(4));
			ipv6[2] = mtk_r32(eth, MTK_GLO_MEM_DATA(3));
			ipv6[1] = mtk_r32(eth, MTK_GLO_MEM_DATA(2));
			ipv6[0] = mtk_r32(eth, MTK_GLO_MEM_DATA(1));
			seq_printf(seq, "SIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",
				   ipv6[3], ipv6[2], ipv6[1], ipv6[0]);
			val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
			val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DIP_BASE + dip_idx);
			val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
			mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
			ipv6[3] = mtk_r32(eth, MTK_GLO_MEM_DATA(3));
			ipv6[2] = mtk_r32(eth, MTK_GLO_MEM_DATA(2));
			ipv6[1] = mtk_r32(eth, MTK_GLO_MEM_DATA(1));
			ipv6[0] = mtk_r32(eth, MTK_GLO_MEM_DATA(0));
			seq_printf(seq, "DIP = 0x%x:0x%x:0x%x:0x%x (IPv6)\n",
				   ipv6[3], ipv6[2], ipv6[1], ipv6[0]);
		}

		/* dump the SPORT and DPORT */
		val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
		val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_DATA_BASE + index);
		val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
		mtk_w32(eth, val, MTK_GLO_MEM_CTRL);
		sport = FIELD_GET(MTK_LRO_DATA_SPORT, mtk_r32(eth, MTK_GLO_MEM_DATA(0)));
		dport = FIELD_GET(MTK_LRO_DATA_DPORT, mtk_r32(eth, MTK_GLO_MEM_DATA(0)));
		seq_printf(seq, "TCP SPORT = %d | TCP DPORT = %d\n", sport, dport);
	}
}

int hw_lro_auto_tlb_read(struct seq_file *seq, void *v)
{
	struct mtk_eth *eth = g_eth;
	const struct mtk_reg_map *reg_map = eth->soc->reg_map;
	int i;
	u32 reg_val;
	u32 reg_op1, reg_op2, reg_op3, reg_op4;
	u32 agg_cnt, agg_time, age_time;

	seq_puts(seq, "Usage of /proc/mtketh/hw_lro_auto_tlb:\n");
	seq_puts(seq, "echo [function] [setting] > /proc/mtketh/hw_lro_auto_tlb\n");
	seq_puts(seq, "Functions:\n");
	seq_puts(seq, "[0] = hwlro_agg_cnt_ctrl\n");
	seq_puts(seq, "[1] = hwlro_agg_time_ctrl\n");
	seq_puts(seq, "[2] = hwlro_age_time_ctrl\n");
	seq_puts(seq, "[3] = hwlro_threshold_ctrl\n");
	seq_puts(seq, "[4] = hwlro_ring_enable_ctrl\n");
	seq_puts(seq, "[5] = hwlro_stats_enable_ctrl\n\n");

	if (mtk_is_netsys_v3_or_greater(eth)) {
		for (i = 1; i <= 8; i++) {
			if (MTK_HAS_CAPS(eth->soc->caps, MTK_GLO_MEM_ACCESS))
				hw_lro_auto_tlb_dump_v3(seq, i);
			else
				hw_lro_auto_tlb_dump_v2(seq, i);
		}
	} else {
		/* Read valid entries of the auto-learn table */
		mtk_w32(eth, 0, MTK_FE_ALT_CF8);
		reg_val = mtk_r32(eth, MTK_FE_ALT_SEQ_CFC);

		seq_printf(seq,
			   "HW LRO Auto-learn Table: (MTK_FE_ALT_SEQ_CFC=0x%x)\n",
			   reg_val);

		for (i = 7; i >= 0; i--) {
			if (reg_val & (1 << i))
				hw_lro_auto_tlb_dump_v1(seq, i);
		}
	}

	/* Read the agg_time/age_time/agg_cnt of LRO rings */
	seq_puts(seq, "\nHW LRO Ring Settings\n");

	for (i = 1; i <= MTK_HW_LRO_RING_NUM; i++) {
		if (MTK_HAS_CAPS(eth->soc->caps, MTK_GLO_MEM_ACCESS)) {
			reg_val = FIELD_PREP(MTK_GLO_MEM_IDX, MTK_LRO_MEM_IDX);
			reg_val |= FIELD_PREP(MTK_GLO_MEM_ADDR, MTK_LRO_MEM_CFG_BASE + i);
			reg_val |= FIELD_PREP(MTK_GLO_MEM_CMD, MTK_GLO_MEM_READ);
			mtk_w32(eth, reg_val, MTK_GLO_MEM_CTRL);
			agg_cnt = FIELD_GET(MTK_RING_MAX_AGG_CNT,
					    mtk_r32(eth, MTK_GLO_MEM_DATA(1)));
			agg_time = FIELD_GET(MTK_RING_MAX_AGG_TIME_V2,
					     mtk_r32(eth, MTK_GLO_MEM_DATA(0)));
			age_time = FIELD_GET(MTK_RING_AGE_TIME,
					     mtk_r32(eth, MTK_GLO_MEM_DATA(0)));
		} else {
			reg_op1 = mtk_r32(eth, MTK_LRO_CTRL_DW1_CFG(i));
			reg_op2 = mtk_r32(eth, MTK_LRO_CTRL_DW2_CFG(i));
			reg_op3 = mtk_r32(eth, MTK_LRO_CTRL_DW3_CFG(i));

			agg_cnt = ((reg_op3 & 0x3) << 6) |
				   ((reg_op2 >> MTK_LRO_RING_AGG_CNT_L_OFFSET) & 0x3f);
			agg_time = (reg_op2 >> MTK_LRO_RING_AGG_TIME_OFFSET) & 0xffff;
			age_time = ((reg_op2 & 0x3f) << 10) |
				    ((reg_op1 >> MTK_LRO_RING_AGE_TIME_L_OFFSET) & 0x3ff);
		}
		reg_op4 = mtk_r32(eth, MTK_PDMA_LRO_CTRL_DW2);

		seq_printf(seq,
			   "Ring[%d]: MAX_AGG_CNT=%d, AGG_TIME=%d, AGE_TIME=%d, Threshold=%d\n",
			   MTK_HW_LRO_RING(i - 1), agg_cnt, agg_time, age_time, reg_op4);
	}

	seq_puts(seq, "\n");

	return 0;
}

static int hw_lro_auto_tlb_open(struct inode *inode, struct file *file)
{
	return single_open(file, hw_lro_auto_tlb_read, NULL);
}

static const struct proc_ops hw_lro_auto_tlb_fops = {
	.proc_open = hw_lro_auto_tlb_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_write = hw_lro_auto_tlb_write,
	.proc_release = single_release
};

struct proc_dir_entry *proc_reg_dir;
static struct proc_dir_entry *proc_esw_cnt, *proc_mac_cnt, *proc_xfi_cnt,
			     *proc_dbg_regs, *proc_reset_event;

int debug_proc_init(struct mtk_eth *eth)
{
	g_eth = eth;

	if (!proc_reg_dir)
		proc_reg_dir = proc_mkdir(PROCREG_DIR, NULL);

	proc_tx_ring =
	    proc_create(PROCREG_TXRING, 0, proc_reg_dir, &tx_ring_fops);
	if (!proc_tx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_TXRING);

	proc_hwtx_ring =
	    proc_create(PROCREG_HWTXRING, 0, proc_reg_dir, &hwtx_ring_fops);
	if (!proc_hwtx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_HWTXRING);

	proc_rx_ring =
	    proc_create(PROCREG_RXRING, 0, proc_reg_dir, &rx_ring_fops);
	if (!proc_rx_ring)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_RXRING);

	proc_esw_cnt =
	    proc_create(PROCREG_ESW_CNT, 0, proc_reg_dir, &switch_count_fops);
	if (!proc_esw_cnt)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_ESW_CNT);

	if (mtk_is_netsys_v3_or_greater(g_eth)) {
		proc_mac_cnt =
		    proc_create(PROCREG_MAC_CNT, 0,
				proc_reg_dir, &mac_count_fops);
		if (!proc_mac_cnt)
			pr_notice("!! FAIL to create %s PROC !!\n",
				  PROCREG_MAC_CNT);

		proc_xfi_cnt =
		    proc_create(PROCREG_XFI_CNT, 0,
				proc_reg_dir, &xfi_count_fops);
		if (!proc_xfi_cnt)
			pr_notice("!! FAIL to create %s PROC !!\n",
				  PROCREG_XFI_CNT);
	}

	proc_dbg_regs =
	    proc_create(PROCREG_DBG_REGS, 0, proc_reg_dir, &dbg_regs_fops);
	if (!proc_dbg_regs)
		pr_notice("!! FAIL to create %s PROC !!\n", PROCREG_DBG_REGS);

	if (g_eth->hwlro) {
		proc_hw_lro_stats =
			proc_create(PROCREG_HW_LRO_STATS, 0, proc_reg_dir,
				    &hw_lro_stats_fops);
		if (!proc_hw_lro_stats)
			pr_info("!! FAIL to create %s PROC !!\n", PROCREG_HW_LRO_STATS);

		proc_hw_lro_auto_tlb =
			proc_create(PROCREG_HW_LRO_AUTO_TLB, 0, proc_reg_dir,
				    &hw_lro_auto_tlb_fops);
		if (!proc_hw_lro_auto_tlb)
			pr_info("!! FAIL to create %s PROC !!\n",
				PROCREG_HW_LRO_AUTO_TLB);
	}

	dbg_show_level = 1;

	return 0;
}

void debug_proc_exit(void)
{
	if (proc_tx_ring)
		remove_proc_entry(PROCREG_TXRING, proc_reg_dir);

	if (proc_hwtx_ring)
		remove_proc_entry(PROCREG_HWTXRING, proc_reg_dir);

	if (proc_rx_ring)
		remove_proc_entry(PROCREG_RXRING, proc_reg_dir);

	if (proc_esw_cnt)
		remove_proc_entry(PROCREG_ESW_CNT, proc_reg_dir);

	if (proc_mac_cnt)
		remove_proc_entry(PROCREG_MAC_CNT, proc_reg_dir);

	if (proc_xfi_cnt)
		remove_proc_entry(PROCREG_XFI_CNT, proc_reg_dir);

	if (proc_reg_dir)
		remove_proc_entry(PROCREG_DIR, 0);

	if (proc_dbg_regs)
		remove_proc_entry(PROCREG_DBG_REGS, proc_reg_dir);

	if (g_eth->hwlro) {
		if (proc_hw_lro_stats)
			remove_proc_entry(PROCREG_HW_LRO_STATS, proc_reg_dir);

		if (proc_hw_lro_auto_tlb)
			remove_proc_entry(PROCREG_HW_LRO_AUTO_TLB, proc_reg_dir);
	}

	if (proc_reset_event)
		remove_proc_entry(PROCREG_RESET_EVENT, proc_reg_dir);
}
