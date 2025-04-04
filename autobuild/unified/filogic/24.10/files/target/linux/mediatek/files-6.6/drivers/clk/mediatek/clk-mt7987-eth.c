// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 MediaTek Inc.
 * Author: Lu Tang <Lu.Tang@mediatek.com>
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include "clk-mtk.h"
#include "clk-gate.h"
#include <dt-bindings/clock/mediatek,mt7987-clk.h>

static const struct mtk_gate_regs ethdma_cg_regs = {
	.set_ofs = 0x30,
	.clr_ofs = 0x30,
	.sta_ofs = 0x30,
};

#define GATE_ETHDMA(_id, _name, _parent, _shift)                  \
	{                                                         \
		.id = _id, .name = _name, .parent_name = _parent, \
		.regs = &ethdma_cg_regs, .shift = _shift,         \
		.ops = &mtk_clk_gate_ops_no_setclr_inv,           \
	}

static const struct mtk_gate ethdma_clks[] = {
	GATE_ETHDMA(CLK_ETHDMA_FE_EN, "ethdma_fe_en", "netsys_2x_sel", 6),
	GATE_ETHDMA(CLK_ETHDMA_GP2_EN, "ethdma_gp2_en", "netsys_500m_sel", 7),
	GATE_ETHDMA(CLK_ETHDMA_GP1_EN, "ethdma_gp1_en", "netsys_500m_sel", 8),
	GATE_ETHDMA(CLK_ETHDMA_GP3_EN, "ethdma_gp3_en", "netsys_500m_sel", 10),
};

static const struct mtk_clk_desc ethdma_desc = {
	.clks = ethdma_clks,
	.num_clks = ARRAY_SIZE(ethdma_clks),
};

static const struct mtk_gate_regs sgmii_cg_regs = {
	.set_ofs = 0xe4,
	.clr_ofs = 0xe4,
	.sta_ofs = 0xe4,
};

#define GATE_SGMII(_id, _name, _parent, _shift)                   \
	{                                                         \
		.id = _id, .name = _name, .parent_name = _parent, \
		.regs = &sgmii_cg_regs, .shift = _shift,          \
		.ops = &mtk_clk_gate_ops_no_setclr_inv,           \
	}

static const struct mtk_gate sgmii0_clks[] = {
	GATE_SGMII(CLK_SGM0_TX_EN, "sgm0_tx_en", "clkxtal", 2),
	GATE_SGMII(CLK_SGM0_RX_EN, "sgm0_rx_en", "clkxtal", 3),
};

static const struct mtk_clk_desc sgmii0_desc = {
	.clks = sgmii0_clks,
	.num_clks = ARRAY_SIZE(sgmii0_clks),
};

static const struct mtk_gate sgmii1_clks[] = {
	GATE_SGMII(CLK_SGM1_TX_EN, "sgm1_tx_en", "clkxtal", 2),
	GATE_SGMII(CLK_SGM1_RX_EN, "sgm1_rx_en", "clkxtal", 3),
};

static const struct mtk_clk_desc sgmii1_desc = {
	.clks = sgmii1_clks,
	.num_clks = ARRAY_SIZE(sgmii1_clks),
};

static const struct of_device_id of_match_clk_mt7987_eth[] = {
	{ .compatible = "mediatek,mt7987-ethsys", .data = &ethdma_desc },
	{ .compatible = "mediatek,mt7987-sgmiisys0", .data = &sgmii0_desc },
	{ .compatible = "mediatek,mt7987-sgmiisys1", .data = &sgmii1_desc },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, of_match_clk_mt7987_eth);

static struct platform_driver clk_mt7987_eth_drv = {
	.driver = {
		.name = "clk-mt7987-eth",
		.of_match_table = of_match_clk_mt7987_eth,
	},
	.probe = mtk_clk_simple_probe,
	.remove_new = mtk_clk_simple_remove,
};
module_platform_driver(clk_mt7987_eth_drv);

MODULE_DESCRIPTION("MediaTek MT7987 Ethernet clocks driver");
MODULE_LICENSE("GPL");
