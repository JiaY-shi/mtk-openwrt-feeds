// SPDX-License-Identifier: ISC
/* Copyright (C) 2020 Felix Fietkau <nbd@nbd.name> */
#ifndef __MT76_VENDOR_H
#define __MT76_VENDOR_H

#include <errno.h>
#include <linux/nl80211.h>
#include <netlink/attr.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unl.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64, ktime_t;

#define MTK_NL80211_VENDOR_ID	0x0ce7

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

#ifndef DIV_ROUND_UP
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))
#endif

struct nl_msg;
struct nlattr;

enum mtk_nl80211_vendor_subcmds {
	MTK_NL80211_VENDOR_SUBCMD_AMNT_CTRL = 0xae,
	MTK_NL80211_VENDOR_SUBCMD_CSI_CTRL = 0xc2,
	MTK_NL80211_VENDOR_SUBCMD_RFEATURE_CTRL = 0xc3,
	MTK_NL80211_VENDOR_SUBCMD_WIRELESS_CTRL = 0xc4,
	MTK_NL80211_VENDOR_SUBCMD_MU_CTRL = 0xc5,
	MTK_NL80211_VENDOR_SUBCMD_PHY_CAPA_CTRL = 0xc6,
};

enum mtk_vendor_attr_csi_ctrl {
	MTK_VENDOR_ATTR_CSI_CTRL_UNSPEC,

	MTK_VENDOR_ATTR_CSI_CTRL_CFG,
	MTK_VENDOR_ATTR_CSI_CTRL_CFG_MODE,
	MTK_VENDOR_ATTR_CSI_CTRL_CFG_TYPE,
	MTK_VENDOR_ATTR_CSI_CTRL_CFG_VAL1,
	MTK_VENDOR_ATTR_CSI_CTRL_CFG_VAL2,
	MTK_VENDOR_ATTR_CSI_CTRL_MAC_ADDR,
	MTK_VENDOR_ATTR_CSI_CTRL_INTERVAL,
	MTK_VENDOR_ATTR_CSI_CTRL_STA_INTERVAL,

	MTK_VENDOR_ATTR_CSI_CTRL_DUMP_NUM,

	MTK_VENDOR_ATTR_CSI_CTRL_DATA,
	MTK_VENDOR_ATTR_CSI_CTRL_DUMP_MAC_FILTER,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_CSI_CTRL,
	MTK_VENDOR_ATTR_CSI_CTRL_MAX =
		NUM_MTK_VENDOR_ATTRS_CSI_CTRL - 1
};

enum mtk_vendor_attr_csi_data {
	MTK_VENDOR_ATTR_CSI_DATA_UNSPEC,
	MTK_VENDOR_ATTR_CSI_DATA_PAD,

	MTK_VENDOR_ATTR_CSI_DATA_VER,
	MTK_VENDOR_ATTR_CSI_DATA_TS,
	MTK_VENDOR_ATTR_CSI_DATA_RSSI,
	MTK_VENDOR_ATTR_CSI_DATA_SNR,
	MTK_VENDOR_ATTR_CSI_DATA_BW,
	MTK_VENDOR_ATTR_CSI_DATA_CH_IDX,
	MTK_VENDOR_ATTR_CSI_DATA_TA,
	MTK_VENDOR_ATTR_CSI_DATA_NUM,
	MTK_VENDOR_ATTR_CSI_DATA_I,
	MTK_VENDOR_ATTR_CSI_DATA_Q,
	MTK_VENDOR_ATTR_CSI_DATA_INFO,
	MTK_VENDOR_ATTR_CSI_DATA_RSVD1,
	MTK_VENDOR_ATTR_CSI_DATA_RSVD2,
	MTK_VENDOR_ATTR_CSI_DATA_RSVD3,
	MTK_VENDOR_ATTR_CSI_DATA_RSVD4,
	MTK_VENDOR_ATTR_CSI_DATA_TX_ANT,
	MTK_VENDOR_ATTR_CSI_DATA_RX_ANT,
	MTK_VENDOR_ATTR_CSI_DATA_MODE,
	MTK_VENDOR_ATTR_CSI_DATA_CHAIN_INFO,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_CSI_DATA,
	MTK_VENDOR_ATTR_CSI_DATA_MAX =
		NUM_MTK_VENDOR_ATTRS_CSI_DATA - 1
};

enum mtk_vendor_attr_csi_mac_filter {
	MTK_VENDOR_ATTR_CSI_MAC_FILTER_UNSPEC,

	MTK_VENDOR_ATTR_CSI_MAC_FILTER_MAC,
	MTK_VENDOR_ATTR_CSI_MAC_FILTER_INTERVAL,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_CSI_MAC_FILTER,
	MTK_VENDOR_ATTR_CSI_MAC_FILTER_MAX =
		NUM_MTK_VENDOR_ATTRS_CSI_MAC_FILTER - 1
};

enum mtk_vendor_attr_mnt_ctrl {
	MTK_VENDOR_ATTR_AMNT_CTRL_UNSPEC,

	MTK_VENDOR_ATTR_AMNT_CTRL_SET,
	MTK_VENDOR_ATTR_AMNT_CTRL_DUMP,
	/* keep last */
	NUM_MTK_VENDOR_ATTRS_AMNT_CTRL,
	MTK_VENDOR_ATTR_AMNT_CTRL_MAX =
		NUM_MTK_VENDOR_ATTRS_AMNT_CTRL - 1
};

enum mtk_vendor_attr_mnt_set {
	MTK_VENDOR_ATTR_AMNT_SET_UNSPEC,

	MTK_VENDOR_ATTR_AMNT_SET_INDEX,
	MTK_VENDOR_ATTR_AMNT_SET_MACADDR,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_AMNT_SET,
	MTK_VENDOR_ATTR_AMNT_SET_MAX =
		NUM_MTK_VENDOR_ATTRS_AMNT_SET - 1
};

enum mtk_vendor_attr_mnt_dump {
	MTK_VENDOR_ATTR_AMNT_DUMP_UNSPEC,

	MTK_VENDOR_ATTR_AMNT_DUMP_INDEX,
	MTK_VENDOR_ATTR_AMNT_DUMP_LEN,
	MTK_VENDOR_ATTR_AMNT_DUMP_RESULT,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_AMNT_DUMP,
	MTK_VENDOR_ATTR_AMNT_DUMP_MAX =
		NUM_MTK_VENDOR_ATTRS_AMNT_DUMP - 1
};

enum mtk_vendor_attr_wireless_ctrl {
	MTK_VENDOR_ATTR_WIRELESS_CTRL_UNSPEC,

	MTK_VENDOR_ATTR_WIRELESS_CTRL_FIXED_MCS,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_FIXED_OFDMA,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_PPDU_TX_TYPE,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_NUSERS_OFDMA,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_BA_BUFFER_SIZE,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_MIMO,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_AMPDU,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_AMSDU,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_CERT,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_RTS_SIGTA,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_WIRELESS_CTRL,
	MTK_VENDOR_ATTR_WIRELESS_CTRL_MAX =
		NUM_MTK_VENDOR_ATTRS_WIRELESS_CTRL - 1
};

enum mtk_vendor_attr_mu_ctrl {
	MTK_VENDOR_ATTR_MU_CTRL_UNSPEC,

	MTK_VENDOR_ATTR_MU_CTRL_ONOFF,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_MU_CTRL,
	MTK_VENDOR_ATTR_MU_CTRL_MAX =
		NUM_MTK_VENDOR_ATTRS_MU_CTRL - 1
};

enum mtk_vendor_attr_rfeature_ctrl {
	MTK_VENDOR_ATTR_RFEATURE_CTRL_UNSPEC,

	MTK_VENDOR_ATTR_RFEATURE_CTRL_HE_GI,
	MTK_VENDOR_ATTR_RFEATURE_CTRL_HE_LTF,
	MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_CFG,
	MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE_EN,
	MTK_VENDOR_ATTR_RFEATURE_CTRL_TRIG_TYPE,
	MTK_VENDOR_ATTR_RFEATURE_CTRL_ACK_PLCY,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_RFEATURE_CTRL,
	MTK_VENDOR_ATTR_RFEATURE_CTRL_MAX =
		NUM_MTK_VENDOR_ATTRS_RFEATURE_CTRL - 1
};

enum mtk_vendor_attr_phy_capa_ctrl {
	MTK_VENDOR_ATTR_PHY_CAPA_CTRL_UNSPEC,

	MTK_VENDOR_ATTR_PHY_CAPA_CTRL_SET,
	MTK_VENDOR_ATTR_PHY_CAPA_CTRL_DUMP,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_PHY_CAPA_CTRL,
	MTK_VENDOR_ATTR_PHY_CAPA_CTRL_MAX =
		NUM_MTK_VENDOR_ATTRS_PHY_CAPA_CTRL - 1
};

enum mtk_vendor_attr_phy_capa_dump {
	MTK_VENDOR_ATTR_PHY_CAPA_DUMP_UNSPEC,

	MTK_VENDOR_ATTR_PHY_CAPA_DUMP_MAX_SUPPORTED_BSS,
	MTK_VENDOR_ATTR_PHY_CAPA_DUMP_MAX_SUPPORTED_STA,

	/* keep last */
	NUM_MTK_VENDOR_ATTRS_PHY_CAPA_DUMP,
	MTK_VENDOR_ATTR_PHY_CAPA_DUMP_MAX =
		NUM_MTK_VENDOR_ATTRS_PHY_CAPA_DUMP - 1
};

#define CSI_BW20_DATA_COUNT	64
#define CSI_BW40_DATA_COUNT	128
#define CSI_BW80_DATA_COUNT	256
#define CSI_BW160_DATA_COUNT	512
#define ETH_ALEN 6

struct csi_data {
	u8 ch_bw;
	u16 data_num;
	s16 data_i[CSI_BW160_DATA_COUNT];
	s16 data_q[CSI_BW160_DATA_COUNT];
	u8 band;
	s8 rssi;
	u8 snr;
	u32 ts;
	u8 data_bw;
	u8 pri_ch_idx;
	u8 ta[ETH_ALEN];
	u32 ext_info;
	u8 rx_mode;
	u32 chain_info;
	u16 tx_idx;
	u16 rx_idx;
	u32 segment_num;
	u8 remain_last;
	u16 pkt_sn;
	u8 tr_stream;
};

struct amnt_data {
	u8 idx;
	u8 addr[ETH_ALEN];
	s8 rssi[4];
	u32 last_seen;
};

extern struct unl unl;

int mt76_csi_set(int idx, int argc, char **argv);
int mt76_csi_dump(int idx, int argc, char **argv);

int mt76_amnt_set(int idx, int argc, char **argv);
int mt76_amnt_dump(int idx, int argc, char **argv);

int mt76_ap_rfeatures_set(int idx, int argc, char **argv);
int mt76_ap_wireless_set(int idx, int argc, char **argv);

int mt76_mu_onoff_set(int idx, int argc, char **argv);

int mt76_phy_capa_dump(int idx, int argc, char **argv);
#endif
