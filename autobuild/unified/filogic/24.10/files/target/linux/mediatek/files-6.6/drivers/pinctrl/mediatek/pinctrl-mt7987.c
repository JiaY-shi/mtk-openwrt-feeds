// SPDX-License-Identifier: GPL-2.0
/*
 * The MT7987 driver based on Linux generic pinctrl binding.
 *
 * Copyright (C) 2020 MediaTek Inc.
 * Author: Tim.Kuo <Tim.Kuo@mediatek.com>
 */

#include "pinctrl-moore.h"

enum MT7987_PINCTRL_REG_PAGE {
	GPIO_BASE,
	IOCFG_RB_BASE,
	IOCFG_LB_BASE,
	IOCFG_RT1_BASE,
	IOCFG_RT2_BASE,
	IOCFG_TL_BASE,
};

#define MT7987_PIN(_number, _name) MTK_PIN(_number, _name, 0, _number, DRV_GRP4)

#define PIN_FIELD_BASE(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs, _s_bit,     \
		       _x_bits)                                                \
	PIN_FIELD_CALC(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs, _s_bit,     \
		       _x_bits, 32, 0)

#define PIN_FIELD_BASE(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs, _s_bit,     \
		       _x_bits)                                                \
	PIN_FIELD_CALC(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs, _s_bit,     \
		       _x_bits, 32, 0)


#define PINS_FIELD_BASE(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs, _s_bit,    \
			_x_bits)                                               \
	PIN_FIELD_CALC(_s_pin, _e_pin, _i_base, _s_addr, _x_addrs, _s_bit,     \
		       _x_bits, 32, 1)

static const struct mtk_pin_field_calc mt7987_pin_mode_range[] = {
	PIN_FIELD(0, 49, 0x300, 0x10, 0, 4),
};

static const struct mtk_pin_field_calc mt7987_pin_dir_range[] = {
	PIN_FIELD(0, 49, 0x0, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_di_range[] = {
	PIN_FIELD(0, 49, 0x200, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_do_range[] = {
	PIN_FIELD(0, 49, 0x100, 0x10, 0, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_ies_range[] = {
	PIN_FIELD_BASE(0, 0, IOCFG_RT2_BASE, 0x20, 0x10, 3, 1),
	PIN_FIELD_BASE(1, 1, IOCFG_RT2_BASE, 0x20, 0x10, 2, 1),
	PIN_FIELD_BASE(2, 2, IOCFG_RT2_BASE, 0x20, 0x10, 11, 1),
	PIN_FIELD_BASE(3, 3, IOCFG_TL_BASE, 0x20, 0x10, 2, 1),
	PIN_FIELD_BASE(4, 4, IOCFG_TL_BASE, 0x20, 0x10, 1, 1),
	PIN_FIELD_BASE(5, 5, IOCFG_TL_BASE, 0x20, 0x10, 3, 1),
	PIN_FIELD_BASE(6, 6, IOCFG_TL_BASE, 0x20, 0x10, 0, 1),
	PIN_FIELD_BASE(7, 7, IOCFG_TL_BASE, 0x20, 0x10, 4, 1),
	PIN_FIELD_BASE(8, 8, IOCFG_RB_BASE, 0x10, 0x10, 2, 1),
	PIN_FIELD_BASE(9, 9, IOCFG_RB_BASE, 0x10, 0x10, 1, 1),
	PIN_FIELD_BASE(10, 10, IOCFG_RB_BASE, 0x10, 0x10, 0, 1),
	PIN_FIELD_BASE(11, 11, IOCFG_RB_BASE, 0x10, 0x10, 3, 1),
	PIN_FIELD_BASE(12, 12, IOCFG_RB_BASE, 0x10, 0x10, 4, 1),
	PIN_FIELD_BASE(13, 13, IOCFG_RT1_BASE, 0x20, 0x10, 0, 1),
	PIN_FIELD_BASE(14, 14, IOCFG_RT1_BASE, 0x20, 0x10, 15, 1),
	PIN_FIELD_BASE(15, 15, IOCFG_RT1_BASE, 0x20, 0x10, 3, 1),
	PIN_FIELD_BASE(16, 16, IOCFG_RT1_BASE, 0x20, 0x10, 7, 1),
	PIN_FIELD_BASE(17, 17, IOCFG_RT1_BASE, 0x20, 0x10, 6, 1),
	PIN_FIELD_BASE(18, 18, IOCFG_RT1_BASE, 0x20, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, IOCFG_RT1_BASE, 0x20, 0x10, 5, 1),
	PIN_FIELD_BASE(20, 20, IOCFG_RT1_BASE, 0x20, 0x10, 8, 1),
	PIN_FIELD_BASE(21, 21, IOCFG_RT1_BASE, 0x20, 0x10, 9, 1),
	PIN_FIELD_BASE(22, 22, IOCFG_RT1_BASE, 0x20, 0x10, 12, 1),
	PIN_FIELD_BASE(23, 23, IOCFG_RT1_BASE, 0x20, 0x10, 11, 1),
	PIN_FIELD_BASE(24, 24, IOCFG_RT1_BASE, 0x20, 0x10, 10, 1),
	PIN_FIELD_BASE(25, 25, IOCFG_RT1_BASE, 0x20, 0x10, 13, 1),
	PIN_FIELD_BASE(26, 26, IOCFG_RT1_BASE, 0x20, 0x10, 14, 1),
	PIN_FIELD_BASE(27, 27, IOCFG_RT2_BASE, 0x20, 0x10, 9, 1),
	PIN_FIELD_BASE(28, 28, IOCFG_RT2_BASE, 0x20, 0x10, 7, 1),
	PIN_FIELD_BASE(29, 29, IOCFG_RT2_BASE, 0x20, 0x10, 8, 1),
	PIN_FIELD_BASE(30, 30, IOCFG_RT2_BASE, 0x20, 0x10, 10, 1),
	PIN_FIELD_BASE(31, 31, IOCFG_TL_BASE, 0x20, 0x10, 5, 1),
	PIN_FIELD_BASE(32, 32, IOCFG_TL_BASE, 0x20, 0x10, 6, 1),
	PIN_FIELD_BASE(33, 33, IOCFG_LB_BASE, 0x20, 0x10, 2, 1),
	PIN_FIELD_BASE(34, 34, IOCFG_LB_BASE, 0x20, 0x10, 0, 1),
	PIN_FIELD_BASE(35, 35, IOCFG_LB_BASE, 0x20, 0x10, 4, 1),
	PIN_FIELD_BASE(36, 36, IOCFG_LB_BASE, 0x20, 0x10, 3, 1),
	PIN_FIELD_BASE(37, 37, IOCFG_LB_BASE, 0x20, 0x10, 1, 1),
	PIN_FIELD_BASE(38, 38, IOCFG_LB_BASE, 0x20, 0x10, 5, 1),
	PIN_FIELD_BASE(39, 39, IOCFG_RT1_BASE, 0x20, 0x10, 1, 1),
	PIN_FIELD_BASE(40, 40, IOCFG_RT1_BASE, 0x20, 0x10, 2, 1),
	PIN_FIELD_BASE(41, 41, IOCFG_RT2_BASE, 0x20, 0x10, 0, 1),
	PIN_FIELD_BASE(42, 42, IOCFG_RT2_BASE, 0x20, 0x10, 1, 1),
	PIN_FIELD_BASE(43, 43, IOCFG_RT2_BASE, 0x20, 0x10, 4, 1),
	PIN_FIELD_BASE(44, 44, IOCFG_RT2_BASE, 0x20, 0x10, 5, 1),
	PIN_FIELD_BASE(45, 45, IOCFG_RT2_BASE, 0x20, 0x10, 6, 1),
	PIN_FIELD_BASE(46, 46, IOCFG_TL_BASE, 0x20, 0x10, 9, 1),
	PIN_FIELD_BASE(47, 47, IOCFG_TL_BASE, 0x20, 0x10, 10, 1),
	PIN_FIELD_BASE(48, 48, IOCFG_TL_BASE, 0x20, 0x10, 7, 1),
	PIN_FIELD_BASE(49, 49, IOCFG_TL_BASE, 0x20, 0x10, 8, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_smt_range[] = {
	PIN_FIELD_BASE(0, 0, IOCFG_RT2_BASE, 0x90, 0x10, 3, 1),
	PIN_FIELD_BASE(1, 1, IOCFG_RT2_BASE, 0x90, 0x10, 2, 1),
	PIN_FIELD_BASE(2, 2, IOCFG_RT2_BASE, 0x90, 0x10, 11, 1),
	PIN_FIELD_BASE(3, 3, IOCFG_TL_BASE, 0x90, 0x10, 2, 1),
	PIN_FIELD_BASE(4, 4, IOCFG_TL_BASE, 0x90, 0x10, 1, 1),
	PIN_FIELD_BASE(5, 5, IOCFG_TL_BASE, 0x90, 0x10, 3, 1),
	PIN_FIELD_BASE(6, 6, IOCFG_TL_BASE, 0x90, 0x10, 0, 1),
	PIN_FIELD_BASE(7, 7, IOCFG_TL_BASE, 0x90, 0x10, 4, 1),
	PIN_FIELD_BASE(8, 8, IOCFG_RB_BASE, 0x70, 0x10, 2, 1),
	PIN_FIELD_BASE(9, 9, IOCFG_RB_BASE, 0x70, 0x10, 1, 1),
	PIN_FIELD_BASE(10, 10, IOCFG_RB_BASE, 0x70, 0x10, 0, 1),
	PIN_FIELD_BASE(11, 11, IOCFG_RB_BASE, 0x70, 0x10, 3, 1),
	PIN_FIELD_BASE(12, 12, IOCFG_RB_BASE, 0x70, 0x10, 4, 1),
	PIN_FIELD_BASE(13, 13, IOCFG_RT1_BASE, 0xA0, 0x10, 0, 1),
	PIN_FIELD_BASE(14, 14, IOCFG_RT1_BASE, 0xA0, 0x10, 15, 1),
	PIN_FIELD_BASE(15, 15, IOCFG_RT1_BASE, 0xA0, 0x10, 3, 1),
	PIN_FIELD_BASE(16, 16, IOCFG_RT1_BASE, 0xA0, 0x10, 7, 1),
	PIN_FIELD_BASE(17, 17, IOCFG_RT1_BASE, 0xA0, 0x10, 6, 1),
	PIN_FIELD_BASE(18, 18, IOCFG_RT1_BASE, 0xA0, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, IOCFG_RT1_BASE, 0xA0, 0x10, 5, 1),
	PIN_FIELD_BASE(20, 20, IOCFG_RT1_BASE, 0xA0, 0x10, 8, 1),
	PIN_FIELD_BASE(21, 21, IOCFG_RT1_BASE, 0xA0, 0x10, 9, 1),
	PIN_FIELD_BASE(22, 22, IOCFG_RT1_BASE, 0xA0, 0x10, 12, 1),
	PIN_FIELD_BASE(23, 23, IOCFG_RT1_BASE, 0xA0, 0x10, 11, 1),
	PIN_FIELD_BASE(24, 24, IOCFG_RT1_BASE, 0xA0, 0x10, 10, 1),
	PIN_FIELD_BASE(25, 25, IOCFG_RT1_BASE, 0xA0, 0x10, 13, 1),
	PIN_FIELD_BASE(26, 26, IOCFG_RT1_BASE, 0xA0, 0x10, 14, 1),
	PIN_FIELD_BASE(27, 27, IOCFG_RT2_BASE, 0x90, 0x10, 9, 1),
	PIN_FIELD_BASE(28, 28, IOCFG_RT2_BASE, 0x90, 0x10, 7, 1),
	PIN_FIELD_BASE(29, 29, IOCFG_RT2_BASE, 0x90, 0x10, 8, 1),
	PIN_FIELD_BASE(30, 30, IOCFG_RT2_BASE, 0x90, 0x10, 10, 1),
	PIN_FIELD_BASE(31, 31, IOCFG_TL_BASE, 0x90, 0x10, 5, 1),
	PIN_FIELD_BASE(32, 32, IOCFG_TL_BASE, 0x90, 0x10, 6, 1),
	PIN_FIELD_BASE(33, 33, IOCFG_LB_BASE, 0x60, 0x10, 2, 1),
	PIN_FIELD_BASE(34, 34, IOCFG_LB_BASE, 0x60, 0x10, 0, 1),
	PIN_FIELD_BASE(35, 35, IOCFG_LB_BASE, 0x60, 0x10, 4, 1),
	PIN_FIELD_BASE(36, 36, IOCFG_LB_BASE, 0x60, 0x10, 3, 1),
	PIN_FIELD_BASE(37, 37, IOCFG_LB_BASE, 0x60, 0x10, 1, 1),
	PIN_FIELD_BASE(38, 38, IOCFG_LB_BASE, 0x60, 0x10, 5, 1),
	PIN_FIELD_BASE(39, 39, IOCFG_RT1_BASE, 0xA0, 0x10, 1, 1),
	PIN_FIELD_BASE(40, 40, IOCFG_RT1_BASE, 0xA0, 0x10, 2, 1),
	PIN_FIELD_BASE(41, 41, IOCFG_RT2_BASE, 0x90, 0x10, 0, 1),
	PIN_FIELD_BASE(42, 42, IOCFG_RT2_BASE, 0x90, 0x10, 1, 1),
	PIN_FIELD_BASE(43, 43, IOCFG_RT2_BASE, 0x90, 0x10, 4, 1),
	PIN_FIELD_BASE(44, 44, IOCFG_RT2_BASE, 0x90, 0x10, 5, 1),
	PIN_FIELD_BASE(45, 45, IOCFG_RT2_BASE, 0x90, 0x10, 6, 1),
	PIN_FIELD_BASE(46, 46, IOCFG_TL_BASE, 0x90, 0x10, 9, 1),
	PIN_FIELD_BASE(47, 47, IOCFG_TL_BASE, 0x90, 0x10, 10, 1),
	PIN_FIELD_BASE(48, 48, IOCFG_TL_BASE, 0x90, 0x10, 7, 1),
	PIN_FIELD_BASE(49, 49, IOCFG_TL_BASE, 0x90, 0x10, 8, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_pu_range[] = {
	PIN_FIELD_BASE(33, 33, IOCFG_LB_BASE, 0x40, 0x10, 2, 1),
	PIN_FIELD_BASE(34, 34, IOCFG_LB_BASE, 0x40, 0x10, 0, 1),
	PIN_FIELD_BASE(35, 35, IOCFG_LB_BASE, 0x40, 0x10, 4, 1),
	PIN_FIELD_BASE(36, 36, IOCFG_LB_BASE, 0x40, 0x10, 3, 1),
	PIN_FIELD_BASE(37, 37, IOCFG_LB_BASE, 0x40, 0x10, 1, 1),
	PIN_FIELD_BASE(38, 38, IOCFG_LB_BASE, 0x40, 0x10, 5, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_pd_range[] = {
	PIN_FIELD_BASE(33, 33, IOCFG_LB_BASE, 0x30, 0x10, 2, 1),
	PIN_FIELD_BASE(34, 34, IOCFG_LB_BASE, 0x30, 0x10, 0, 1),
	PIN_FIELD_BASE(35, 35, IOCFG_LB_BASE, 0x30, 0x10, 4, 1),
	PIN_FIELD_BASE(36, 36, IOCFG_LB_BASE, 0x30, 0x10, 3, 1),
	PIN_FIELD_BASE(37, 37, IOCFG_LB_BASE, 0x30, 0x10, 1, 1),
	PIN_FIELD_BASE(38, 38, IOCFG_LB_BASE, 0x30, 0x10, 5, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_drv_range[] = {
	PIN_FIELD_BASE(0, 0, IOCFG_RT2_BASE, 0x0, 0x10, 9, 3),
	PIN_FIELD_BASE(1, 1, IOCFG_RT2_BASE, 0x0, 0x10, 6, 3),
	PIN_FIELD_BASE(2, 2, IOCFG_RT2_BASE, 0x10, 0x10, 3, 3),
	PIN_FIELD_BASE(3, 3, IOCFG_TL_BASE, 0x0, 0x10, 6, 3),
	PIN_FIELD_BASE(4, 4, IOCFG_TL_BASE, 0x0, 0x10, 3, 3),
	PIN_FIELD_BASE(5, 5, IOCFG_TL_BASE, 0x0, 0x10, 9, 3),
	PIN_FIELD_BASE(6, 6, IOCFG_TL_BASE, 0x0, 0x10, 0, 3),
	PIN_FIELD_BASE(7, 7, IOCFG_TL_BASE, 0x0, 0x10, 12, 3),
	PIN_FIELD_BASE(8, 8, IOCFG_RB_BASE, 0x0, 0x10, 6, 3),
	PIN_FIELD_BASE(9, 9, IOCFG_RB_BASE, 0x0, 0x10, 3, 3),
	PIN_FIELD_BASE(10, 10, IOCFG_RB_BASE, 0x0, 0x10, 0, 3),
	PIN_FIELD_BASE(11, 11, IOCFG_RB_BASE, 0x0, 0x10, 9, 3),
	PIN_FIELD_BASE(12, 12, IOCFG_RB_BASE, 0x0, 0x10, 12, 3),
	PIN_FIELD_BASE(13, 13, IOCFG_RT1_BASE, 0x0, 0x10, 0, 3),
	PIN_FIELD_BASE(14, 14, IOCFG_RT1_BASE, 0x10, 0x10, 15, 3),
	PIN_FIELD_BASE(15, 15, IOCFG_RT1_BASE, 0x0, 0x10, 9, 3),
	PIN_FIELD_BASE(16, 16, IOCFG_RT1_BASE, 0x0, 0x10, 21, 3),
	PIN_FIELD_BASE(17, 17, IOCFG_RT1_BASE, 0x0, 0x10, 18, 3),
	PIN_FIELD_BASE(18, 18, IOCFG_RT1_BASE, 0x0, 0x10, 12, 3),
	PIN_FIELD_BASE(19, 19, IOCFG_RT1_BASE, 0x0, 0x10, 15, 3),
	PIN_FIELD_BASE(20, 20, IOCFG_RT1_BASE, 0x0, 0x10, 24, 3),
	PIN_FIELD_BASE(21, 21, IOCFG_RT1_BASE, 0x0, 0x10, 27, 3),
	PIN_FIELD_BASE(22, 22, IOCFG_RT1_BASE, 0x10, 0x10, 6, 3),
	PIN_FIELD_BASE(23, 23, IOCFG_RT1_BASE, 0x10, 0x10, 3, 3),
	PIN_FIELD_BASE(24, 24, IOCFG_RT1_BASE, 0x10, 0x10, 0, 3),
	PIN_FIELD_BASE(25, 25, IOCFG_RT1_BASE, 0x10, 0x10, 9, 3),
	PIN_FIELD_BASE(26, 26, IOCFG_RT1_BASE, 0x10, 0x10, 12, 3),
	PIN_FIELD_BASE(27, 27, IOCFG_RT2_BASE, 0x0, 0x10, 27, 3),
	PIN_FIELD_BASE(28, 28, IOCFG_RT2_BASE, 0x0, 0x10, 21, 3),
	PIN_FIELD_BASE(29, 29, IOCFG_RT2_BASE, 0x0, 0x10, 24, 3),
	PIN_FIELD_BASE(30, 30, IOCFG_RT2_BASE, 0x10, 0x10, 0, 3),
	PIN_FIELD_BASE(31, 31, IOCFG_TL_BASE, 0x0, 0x10, 15, 3),
	PIN_FIELD_BASE(32, 32, IOCFG_TL_BASE, 0x0, 0x10, 18, 3),
	PIN_FIELD_BASE(33, 33, IOCFG_LB_BASE, 0x0, 0x10, 6, 3),
	PIN_FIELD_BASE(34, 34, IOCFG_LB_BASE, 0x0, 0x10, 0, 3),
	PIN_FIELD_BASE(35, 35, IOCFG_LB_BASE, 0x0, 0x10, 12, 3),
	PIN_FIELD_BASE(36, 36, IOCFG_LB_BASE, 0x0, 0x10, 9, 3),
	PIN_FIELD_BASE(37, 37, IOCFG_LB_BASE, 0x0, 0x10, 3, 3),
	PIN_FIELD_BASE(38, 38, IOCFG_LB_BASE, 0x0, 0x10, 15, 3),
	PIN_FIELD_BASE(39, 39, IOCFG_RT1_BASE, 0x0, 0x10, 3, 3),
	PIN_FIELD_BASE(40, 40, IOCFG_RT1_BASE, 0x0, 0x10, 6, 3),
	PIN_FIELD_BASE(41, 41, IOCFG_RT2_BASE, 0x0, 0x10, 0, 3),
	PIN_FIELD_BASE(42, 42, IOCFG_RT2_BASE, 0x0, 0x10, 3, 3),
	PIN_FIELD_BASE(43, 43, IOCFG_RT2_BASE, 0x0, 0x10, 12, 3),
	PIN_FIELD_BASE(44, 44, IOCFG_RT2_BASE, 0x0, 0x10, 15, 3),
	PIN_FIELD_BASE(45, 45, IOCFG_RT2_BASE, 0x0, 0x10, 18, 3),
	PIN_FIELD_BASE(46, 46, IOCFG_TL_BASE, 0x0, 0x10, 27, 3),
	PIN_FIELD_BASE(47, 47, IOCFG_TL_BASE, 0x10, 0x10, 0, 3),
	PIN_FIELD_BASE(48, 48, IOCFG_TL_BASE, 0x0, 0x10, 21, 3),
	PIN_FIELD_BASE(49, 49, IOCFG_TL_BASE, 0x0, 0x10, 24, 3),
};

static const struct mtk_pin_field_calc mt7987_pin_pupd_range[] = {
	PIN_FIELD_BASE(0, 0, IOCFG_RT2_BASE, 0x30, 0x10, 3, 1),
	PIN_FIELD_BASE(1, 1, IOCFG_RT2_BASE, 0x30, 0x10, 2, 1),
	PIN_FIELD_BASE(2, 2, IOCFG_RT2_BASE, 0x30, 0x10, 11, 1),
	PIN_FIELD_BASE(3, 3, IOCFG_TL_BASE, 0x30, 0x10, 2, 1),
	PIN_FIELD_BASE(4, 4, IOCFG_TL_BASE, 0x30, 0x10, 1, 1),
	PIN_FIELD_BASE(5, 5, IOCFG_TL_BASE, 0x30, 0x10, 3, 1),
	PIN_FIELD_BASE(6, 6, IOCFG_TL_BASE, 0x30, 0x10, 0, 1),
	PIN_FIELD_BASE(7, 7, IOCFG_TL_BASE, 0x30, 0x10, 4, 1),
	PIN_FIELD_BASE(8, 8, IOCFG_RB_BASE, 0x20, 0x10, 2, 1),
	PIN_FIELD_BASE(9, 9, IOCFG_RB_BASE, 0x20, 0x10, 1, 1),
	PIN_FIELD_BASE(10, 10, IOCFG_RB_BASE, 0x20, 0x10, 0, 1),
	PIN_FIELD_BASE(11, 11, IOCFG_RB_BASE, 0x20, 0x10, 3, 1),
	PIN_FIELD_BASE(12, 12, IOCFG_RB_BASE, 0x20, 0x10, 4, 1),
	PIN_FIELD_BASE(13, 13, IOCFG_RT1_BASE, 0x30, 0x10, 0, 1),
	PIN_FIELD_BASE(14, 14, IOCFG_RT1_BASE, 0x30, 0x10, 15, 1),
	PIN_FIELD_BASE(15, 15, IOCFG_RT1_BASE, 0x30, 0x10, 3, 1),
	PIN_FIELD_BASE(16, 16, IOCFG_RT1_BASE, 0x30, 0x10, 7, 1),
	PIN_FIELD_BASE(17, 17, IOCFG_RT1_BASE, 0x30, 0x10, 6, 1),
	PIN_FIELD_BASE(18, 18, IOCFG_RT1_BASE, 0x30, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, IOCFG_RT1_BASE, 0x30, 0x10, 5, 1),
	PIN_FIELD_BASE(20, 20, IOCFG_RT1_BASE, 0x30, 0x10, 8, 1),
	PIN_FIELD_BASE(21, 21, IOCFG_RT1_BASE, 0x30, 0x10, 9, 1),
	PIN_FIELD_BASE(22, 22, IOCFG_RT1_BASE, 0x30, 0x10, 12, 1),
	PIN_FIELD_BASE(23, 23, IOCFG_RT1_BASE, 0x30, 0x10, 11, 1),
	PIN_FIELD_BASE(24, 24, IOCFG_RT1_BASE, 0x30, 0x10, 10, 1),
	PIN_FIELD_BASE(25, 25, IOCFG_RT1_BASE, 0x30, 0x10, 13, 1),
	PIN_FIELD_BASE(26, 26, IOCFG_RT1_BASE, 0x30, 0x10, 14, 1),
	PIN_FIELD_BASE(27, 27, IOCFG_RT2_BASE, 0x30, 0x10, 9, 1),
	PIN_FIELD_BASE(28, 28, IOCFG_RT2_BASE, 0x30, 0x10, 7, 1),
	PIN_FIELD_BASE(29, 29, IOCFG_RT2_BASE, 0x30, 0x10, 8, 1),
	PIN_FIELD_BASE(30, 30, IOCFG_RT2_BASE, 0x30, 0x10, 10, 1),
	PIN_FIELD_BASE(31, 31, IOCFG_TL_BASE, 0x30, 0x10, 5, 1),
	PIN_FIELD_BASE(32, 32, IOCFG_TL_BASE, 0x30, 0x10, 6, 1),

	PIN_FIELD_BASE(39, 39, IOCFG_RT1_BASE, 0x30, 0x10, 1, 1),
	PIN_FIELD_BASE(40, 40, IOCFG_RT1_BASE, 0x30, 0x10, 2, 1),
	PIN_FIELD_BASE(41, 41, IOCFG_RT2_BASE, 0x30, 0x10, 0, 1),
	PIN_FIELD_BASE(42, 42, IOCFG_RT2_BASE, 0x30, 0x10, 1, 1),
	PIN_FIELD_BASE(43, 43, IOCFG_RT2_BASE, 0x30, 0x10, 4, 1),
	PIN_FIELD_BASE(44, 44, IOCFG_RT2_BASE, 0x30, 0x10, 5, 1),
	PIN_FIELD_BASE(45, 45, IOCFG_RT2_BASE, 0x30, 0x10, 6, 1),
	PIN_FIELD_BASE(46, 46, IOCFG_TL_BASE, 0x30, 0x10, 9, 1),
	PIN_FIELD_BASE(47, 47, IOCFG_TL_BASE, 0x30, 0x10, 10, 1),
	PIN_FIELD_BASE(48, 48, IOCFG_TL_BASE, 0x30, 0x10, 7, 1),
	PIN_FIELD_BASE(49, 49, IOCFG_TL_BASE, 0x30, 0x10, 8, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_r0_range[] = {
	PIN_FIELD_BASE(0, 0, IOCFG_RT2_BASE, 0x40, 0x10, 3, 1),
	PIN_FIELD_BASE(1, 1, IOCFG_RT2_BASE, 0x40, 0x10, 2, 1),
	PIN_FIELD_BASE(2, 2, IOCFG_RT2_BASE, 0x40, 0x10, 11, 1),
	PIN_FIELD_BASE(3, 3, IOCFG_TL_BASE, 0x40, 0x10, 2, 1),
	PIN_FIELD_BASE(4, 4, IOCFG_TL_BASE, 0x40, 0x10, 1, 1),
	PIN_FIELD_BASE(5, 5, IOCFG_TL_BASE, 0x40, 0x10, 3, 1),
	PIN_FIELD_BASE(6, 6, IOCFG_TL_BASE, 0x40, 0x10, 0, 1),
	PIN_FIELD_BASE(7, 7, IOCFG_TL_BASE, 0x40, 0x10, 4, 1),
	PIN_FIELD_BASE(8, 8, IOCFG_RB_BASE, 0x30, 0x10, 2, 1),
	PIN_FIELD_BASE(9, 9, IOCFG_RB_BASE, 0x30, 0x10, 1, 1),
	PIN_FIELD_BASE(10, 10, IOCFG_RB_BASE, 0x30, 0x10, 0, 1),
	PIN_FIELD_BASE(11, 11, IOCFG_RB_BASE, 0x30, 0x10, 3, 1),
	PIN_FIELD_BASE(12, 12, IOCFG_RB_BASE, 0x30, 0x10, 4, 1),
	PIN_FIELD_BASE(13, 13, IOCFG_RT1_BASE, 0x40, 0x10, 0, 1),
	PIN_FIELD_BASE(14, 14, IOCFG_RT1_BASE, 0x40, 0x10, 15, 1),
	PIN_FIELD_BASE(15, 15, IOCFG_RT1_BASE, 0x40, 0x10, 3, 1),
	PIN_FIELD_BASE(16, 16, IOCFG_RT1_BASE, 0x40, 0x10, 7, 1),
	PIN_FIELD_BASE(17, 17, IOCFG_RT1_BASE, 0x40, 0x10, 6, 1),
	PIN_FIELD_BASE(18, 18, IOCFG_RT1_BASE, 0x40, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, IOCFG_RT1_BASE, 0x40, 0x10, 5, 1),
	PIN_FIELD_BASE(20, 20, IOCFG_RT1_BASE, 0x40, 0x10, 8, 1),
	PIN_FIELD_BASE(21, 21, IOCFG_RT1_BASE, 0x40, 0x10, 9, 1),
	PIN_FIELD_BASE(22, 22, IOCFG_RT1_BASE, 0x40, 0x10, 12, 1),
	PIN_FIELD_BASE(23, 23, IOCFG_RT1_BASE, 0x40, 0x10, 11, 1),
	PIN_FIELD_BASE(24, 24, IOCFG_RT1_BASE, 0x40, 0x10, 10, 1),
	PIN_FIELD_BASE(25, 25, IOCFG_RT1_BASE, 0x40, 0x10, 13, 1),
	PIN_FIELD_BASE(26, 26, IOCFG_RT1_BASE, 0x40, 0x10, 14, 1),
	PIN_FIELD_BASE(27, 27, IOCFG_RT2_BASE, 0x40, 0x10, 9, 1),
	PIN_FIELD_BASE(28, 28, IOCFG_RT2_BASE, 0x40, 0x10, 7, 1),
	PIN_FIELD_BASE(29, 29, IOCFG_RT2_BASE, 0x40, 0x10, 8, 1),
	PIN_FIELD_BASE(30, 30, IOCFG_RT2_BASE, 0x40, 0x10, 10, 1),
	PIN_FIELD_BASE(31, 31, IOCFG_TL_BASE, 0x40, 0x10, 5, 1),
	PIN_FIELD_BASE(32, 32, IOCFG_TL_BASE, 0x40, 0x10, 6, 1),

	PIN_FIELD_BASE(39, 39, IOCFG_RT1_BASE, 0x40, 0x10, 1, 1),
	PIN_FIELD_BASE(40, 40, IOCFG_RT1_BASE, 0x40, 0x10, 2, 1),
	PIN_FIELD_BASE(41, 41, IOCFG_RT2_BASE, 0x40, 0x10, 0, 1),
	PIN_FIELD_BASE(42, 42, IOCFG_RT2_BASE, 0x40, 0x10, 1, 1),
	PIN_FIELD_BASE(43, 43, IOCFG_RT2_BASE, 0x40, 0x10, 4, 1),
	PIN_FIELD_BASE(44, 44, IOCFG_RT2_BASE, 0x40, 0x10, 5, 1),
	PIN_FIELD_BASE(45, 45, IOCFG_RT2_BASE, 0x40, 0x10, 6, 1),
	PIN_FIELD_BASE(46, 46, IOCFG_TL_BASE, 0x40, 0x10, 9, 1),
	PIN_FIELD_BASE(47, 47, IOCFG_TL_BASE, 0x40, 0x10, 10, 1),
	PIN_FIELD_BASE(48, 48, IOCFG_TL_BASE, 0x40, 0x10, 7, 1),
	PIN_FIELD_BASE(49, 49, IOCFG_TL_BASE, 0x40, 0x10, 8, 1),
};

static const struct mtk_pin_field_calc mt7987_pin_r1_range[] = {
	PIN_FIELD_BASE(0, 0, IOCFG_RT2_BASE, 0x50, 0x10, 3, 1),
	PIN_FIELD_BASE(1, 1, IOCFG_RT2_BASE, 0x50, 0x10, 2, 1),
	PIN_FIELD_BASE(2, 2, IOCFG_RT2_BASE, 0x50, 0x10, 11, 1),
	PIN_FIELD_BASE(3, 3, IOCFG_TL_BASE, 0x50, 0x10, 2, 1),
	PIN_FIELD_BASE(4, 4, IOCFG_TL_BASE, 0x50, 0x10, 1, 1),
	PIN_FIELD_BASE(5, 5, IOCFG_TL_BASE, 0x50, 0x10, 3, 1),
	PIN_FIELD_BASE(6, 6, IOCFG_TL_BASE, 0x50, 0x10, 0, 1),
	PIN_FIELD_BASE(7, 7, IOCFG_TL_BASE, 0x50, 0x10, 4, 1),
	PIN_FIELD_BASE(8, 8, IOCFG_RB_BASE, 0x40, 0x10, 2, 1),
	PIN_FIELD_BASE(9, 9, IOCFG_RB_BASE, 0x40, 0x10, 1, 1),
	PIN_FIELD_BASE(10, 10, IOCFG_RB_BASE, 0x40, 0x10, 0, 1),
	PIN_FIELD_BASE(11, 11, IOCFG_RB_BASE, 0x40, 0x10, 3, 1),
	PIN_FIELD_BASE(12, 12, IOCFG_RB_BASE, 0x40, 0x10, 4, 1),
	PIN_FIELD_BASE(13, 13, IOCFG_RT1_BASE, 0x50, 0x10, 0, 1),
	PIN_FIELD_BASE(14, 14, IOCFG_RT1_BASE, 0x50, 0x10, 15, 1),
	PIN_FIELD_BASE(15, 15, IOCFG_RT1_BASE, 0x50, 0x10, 3, 1),
	PIN_FIELD_BASE(16, 16, IOCFG_RT1_BASE, 0x50, 0x10, 7, 1),
	PIN_FIELD_BASE(17, 17, IOCFG_RT1_BASE, 0x50, 0x10, 6, 1),
	PIN_FIELD_BASE(18, 18, IOCFG_RT1_BASE, 0x50, 0x10, 4, 1),
	PIN_FIELD_BASE(19, 19, IOCFG_RT1_BASE, 0x50, 0x10, 5, 1),
	PIN_FIELD_BASE(20, 20, IOCFG_RT1_BASE, 0x50, 0x10, 8, 1),
	PIN_FIELD_BASE(21, 21, IOCFG_RT1_BASE, 0x50, 0x10, 9, 1),
	PIN_FIELD_BASE(22, 22, IOCFG_RT1_BASE, 0x50, 0x10, 12, 1),
	PIN_FIELD_BASE(23, 23, IOCFG_RT1_BASE, 0x50, 0x10, 11, 1),
	PIN_FIELD_BASE(24, 24, IOCFG_RT1_BASE, 0x50, 0x10, 10, 1),
	PIN_FIELD_BASE(25, 25, IOCFG_RT1_BASE, 0x50, 0x10, 13, 1),
	PIN_FIELD_BASE(26, 26, IOCFG_RT1_BASE, 0x50, 0x10, 14, 1),
	PIN_FIELD_BASE(27, 27, IOCFG_RT2_BASE, 0x50, 0x10, 9, 1),
	PIN_FIELD_BASE(28, 28, IOCFG_RT2_BASE, 0x50, 0x10, 7, 1),
	PIN_FIELD_BASE(29, 29, IOCFG_RT2_BASE, 0x50, 0x10, 8, 1),
	PIN_FIELD_BASE(30, 30, IOCFG_RT2_BASE, 0x50, 0x10, 10, 1),
	PIN_FIELD_BASE(31, 31, IOCFG_TL_BASE, 0x50, 0x10, 5, 1),
	PIN_FIELD_BASE(32, 32, IOCFG_TL_BASE, 0x50, 0x10, 6, 1),

	PIN_FIELD_BASE(39, 39, IOCFG_RT1_BASE, 0x50, 0x10, 1, 1),
	PIN_FIELD_BASE(40, 40, IOCFG_RT1_BASE, 0x50, 0x10, 2, 1),
	PIN_FIELD_BASE(41, 41, IOCFG_RT2_BASE, 0x50, 0x10, 0, 1),
	PIN_FIELD_BASE(42, 42, IOCFG_RT2_BASE, 0x50, 0x10, 1, 1),
	PIN_FIELD_BASE(43, 43, IOCFG_RT2_BASE, 0x50, 0x10, 4, 1),
	PIN_FIELD_BASE(44, 44, IOCFG_RT2_BASE, 0x50, 0x10, 5, 1),
	PIN_FIELD_BASE(45, 45, IOCFG_RT2_BASE, 0x50, 0x10, 6, 1),
	PIN_FIELD_BASE(46, 46, IOCFG_TL_BASE, 0x50, 0x10, 9, 1),
	PIN_FIELD_BASE(47, 47, IOCFG_TL_BASE, 0x50, 0x10, 10, 1),
	PIN_FIELD_BASE(48, 48, IOCFG_TL_BASE, 0x50, 0x10, 7, 1),
	PIN_FIELD_BASE(49, 49, IOCFG_TL_BASE, 0x50, 0x10, 8, 1),
};

static const unsigned int mt7987_pull_type[] = {
	MTK_PULL_PUPD_R1R0_TYPE,/*0*/ MTK_PULL_PUPD_R1R0_TYPE,/*1*/
	MTK_PULL_PUPD_R1R0_TYPE,/*2*/ MTK_PULL_PUPD_R1R0_TYPE,/*3*/
	MTK_PULL_PUPD_R1R0_TYPE,/*4*/ MTK_PULL_PUPD_R1R0_TYPE,/*5*/
	MTK_PULL_PUPD_R1R0_TYPE,/*6*/ MTK_PULL_PUPD_R1R0_TYPE,/*7*/
	MTK_PULL_PUPD_R1R0_TYPE,/*8*/ MTK_PULL_PUPD_R1R0_TYPE,/*9*/
	MTK_PULL_PUPD_R1R0_TYPE,/*10*/ MTK_PULL_PUPD_R1R0_TYPE,/*11*/
	MTK_PULL_PUPD_R1R0_TYPE,/*12*/ MTK_PULL_PUPD_R1R0_TYPE,/*13*/
	MTK_PULL_PUPD_R1R0_TYPE,/*14*/ MTK_PULL_PUPD_R1R0_TYPE,/*15*/
	MTK_PULL_PUPD_R1R0_TYPE,/*16*/ MTK_PULL_PUPD_R1R0_TYPE,/*17*/
	MTK_PULL_PUPD_R1R0_TYPE,/*18*/ MTK_PULL_PUPD_R1R0_TYPE,/*19*/
	MTK_PULL_PUPD_R1R0_TYPE,/*20*/ MTK_PULL_PUPD_R1R0_TYPE,/*21*/
	MTK_PULL_PUPD_R1R0_TYPE,/*22*/ MTK_PULL_PUPD_R1R0_TYPE,/*23*/
	MTK_PULL_PUPD_R1R0_TYPE,/*24*/ MTK_PULL_PUPD_R1R0_TYPE,/*25*/
	MTK_PULL_PUPD_R1R0_TYPE,/*26*/ MTK_PULL_PUPD_R1R0_TYPE,/*27*/
	MTK_PULL_PUPD_R1R0_TYPE,/*28*/ MTK_PULL_PUPD_R1R0_TYPE,/*29*/
	MTK_PULL_PUPD_R1R0_TYPE,/*30*/ MTK_PULL_PUPD_R1R0_TYPE,/*31*/
	MTK_PULL_PUPD_R1R0_TYPE,/*32*/ MTK_PULL_PU_PD_TYPE,/*33*/
	MTK_PULL_PU_PD_TYPE,/*34*/ MTK_PULL_PU_PD_TYPE,/*35*/
	MTK_PULL_PU_PD_TYPE,/*36*/ MTK_PULL_PU_PD_TYPE,/*37*/
	MTK_PULL_PU_PD_TYPE,/*38*/ MTK_PULL_PUPD_R1R0_TYPE,/*39*/
	MTK_PULL_PUPD_R1R0_TYPE,/*40*/ MTK_PULL_PUPD_R1R0_TYPE,/*41*/
	MTK_PULL_PUPD_R1R0_TYPE,/*42*/ MTK_PULL_PUPD_R1R0_TYPE,/*43*/
	MTK_PULL_PUPD_R1R0_TYPE,/*44*/ MTK_PULL_PUPD_R1R0_TYPE,/*45*/
	MTK_PULL_PUPD_R1R0_TYPE,/*46*/ MTK_PULL_PUPD_R1R0_TYPE,/*47*/
	MTK_PULL_PUPD_R1R0_TYPE,/*48*/ MTK_PULL_PUPD_R1R0_TYPE,/*49*/
};

static const struct mtk_pin_reg_calc mt7987_reg_cals[] = {
	[PINCTRL_PIN_REG_MODE] = MTK_RANGE(mt7987_pin_mode_range),
	[PINCTRL_PIN_REG_DIR] = MTK_RANGE(mt7987_pin_dir_range),
	[PINCTRL_PIN_REG_DI] = MTK_RANGE(mt7987_pin_di_range),
	[PINCTRL_PIN_REG_DO] = MTK_RANGE(mt7987_pin_do_range),
	[PINCTRL_PIN_REG_SMT] = MTK_RANGE(mt7987_pin_smt_range),
	[PINCTRL_PIN_REG_IES] = MTK_RANGE(mt7987_pin_ies_range),
	[PINCTRL_PIN_REG_PU] = MTK_RANGE(mt7987_pin_pu_range),
	[PINCTRL_PIN_REG_PD] = MTK_RANGE(mt7987_pin_pd_range),
	[PINCTRL_PIN_REG_DRV] = MTK_RANGE(mt7987_pin_drv_range),
	[PINCTRL_PIN_REG_PUPD] = MTK_RANGE(mt7987_pin_pupd_range),
	[PINCTRL_PIN_REG_R0] = MTK_RANGE(mt7987_pin_r0_range),
	[PINCTRL_PIN_REG_R1] = MTK_RANGE(mt7987_pin_r1_range),
};

static const struct mtk_pin_desc mt7987_pins[] = {
	MT7987_PIN(0, "GPIO_WPS"),
	MT7987_PIN(1, "GPIO_RESET"),
	MT7987_PIN(2, "SYS_WATCHDOG"),
	MT7987_PIN(3, "JTAG_JTDO"),
	MT7987_PIN(4, "JTAG_JTDI"),
	MT7987_PIN(5, "JTAG_JTMS"),
	MT7987_PIN(6, "JTAG_JTCLK"),
	MT7987_PIN(7, "JTAG_JTRST_N"),
	MT7987_PIN(8, "PCM_DTX_I2S_DOUT"),
	MT7987_PIN(9, "PCM_DRX_I2S_DIN"),
	MT7987_PIN(10, "PCM_CLK_I2S_BCLK"),
	MT7987_PIN(11, "PCM_FS_I2S_LRCK"),
	MT7987_PIN(12, "PCM_MCK_I2S_MCLK"),
	MT7987_PIN(13, "PWM0"),
	MT7987_PIN(14, "USB_VBUS"),
	MT7987_PIN(15, "SPI0_CLK"),
	MT7987_PIN(16, "SPI0_MOSI"),
	MT7987_PIN(17, "SPI0_MISO"),
	MT7987_PIN(18, "SPI0_CS"),
	MT7987_PIN(19, "SPI0_HOLD"),
	MT7987_PIN(20, "SPI0_WP"),
	MT7987_PIN(21, "SPI1_CLK"),
	MT7987_PIN(22, "SPI1_MOSI"),
	MT7987_PIN(23, "SPI1_MISO"),
	MT7987_PIN(24, "SPI1_CS"),
	MT7987_PIN(25, "SPI2_CLK"),
	MT7987_PIN(26, "SPI2_MOSI"),
	MT7987_PIN(27, "SPI2_MISO"),
	MT7987_PIN(28, "SPI2_CS"),
	MT7987_PIN(29, "SPI2_HOLD"),
	MT7987_PIN(30, "SPI2_WP"),
	MT7987_PIN(31, "UART0_RXD"),
	MT7987_PIN(32, "UART0_TXD"),
	MT7987_PIN(33, "PCIE_PERESET_N_0"),
	MT7987_PIN(34, "PCIE_CLK_REQ_0"),
	MT7987_PIN(35, "PCIE_WAKE_N_0"),
	MT7987_PIN(36, "PCIE_PERESET_N_1"),
	MT7987_PIN(37, "PCIE_CLK_REQ_1"),
	MT7987_PIN(38, "PCIE_WAKE_N_1"),
	MT7987_PIN(39, "SMI_MDC"),
	MT7987_PIN(40, "SMI_MDIO"),
	MT7987_PIN(41, "GBE_INT"),
	MT7987_PIN(42, "GBE_RESET"),
	MT7987_PIN(43, "I2C_SCLK"),
	MT7987_PIN(44, "I2C_SDATA"),
	MT7987_PIN(45, "2P5G_LED0"),
	MT7987_PIN(46, "UART1_RXD"),
	MT7987_PIN(47, "UART1_TXD"),
	MT7987_PIN(48, "UART1_CTS"),
	MT7987_PIN(49, "UART1_RTS"),
};

/* watchdog */
static int mt7987_watchdog_pins[] = {2};
static int mt7987_watchdog_funcs[] = {1};

/* jtag */
static int mt7987_jtag_pins[] = {3, 4, 5, 6, 7};
static int mt7987_jtag_funcs[] = {1, 1, 1, 1, 1};

/* pcm */
static int mt7987_pcm0_0_pins[] = {3, 4, 5, 6, 7};
static int mt7987_pcm0_0_funcs[] = {2, 2, 2, 2, 2};

static int mt7987_pcm0_1_pins[] = {8, 9, 10, 11, 12};
static int mt7987_pcm0_1_funcs[] = {1, 1, 1, 1, 1};

/* uart */
static int mt7987_uart0_pins[] = {31, 32};
static int mt7987_uart0_funcs[] = {1, 1};

static int mt7987_uart1_0_pins[] = {3, 4, 5, 6};
static int mt7987_uart1_0_funcs[] = {3, 3, 3, 3};

static int mt7987_uart1_0_lite_pins[] = {3, 4};
static int mt7987_uart1_0_lite_funcs[] = {3, 3};

static int mt7987_uart1_1_pins[] = {21, 22, 23, 24};
static int mt7987_uart1_1_funcs[] = {3, 3, 3, 3};

static int mt7987_uart1_2_pins[] = {46, 47, 48, 49};
static int mt7987_uart1_2_funcs[] = {1, 1, 1, 1};

static int mt7987_uart2_0_pins[] = {8, 9, 10, 11};
static int mt7987_uart2_0_funcs[] = {2, 2, 2, 2};

static int mt7987_uart2_1_pins[] = {25, 26, 27, 28};
static int mt7987_uart2_1_funcs[] = {2, 2, 2, 2};

/* pwm */
static int mt7987_pwm0_pins[] = {13};
static int mt7987_pwm0_funcs[] = {1};

static int mt7987_pwm1_0_pins[] = {7};
static int mt7987_pwm1_0_funcs[] = {3};

static int mt7987_pwm1_1_pins[] = {43};
static int mt7987_pwm1_1_funcs[] = {2};

static int mt7987_pwm2_0_pins[] = {12};
static int mt7987_pwm2_0_funcs[] = {2};

static int mt7987_pwm2_1_pins[] = {44};
static int mt7987_pwm2_1_funcs[] = {2};

/* vbus */
static int mt7987_drv_vbus_p1_pins[] = {14};
static int mt7987_drv_vbus_p1_funcs[] = {1};

static int mt7987_drv_vbus_pins[] = {48};
static int mt7987_drv_vbus_funcs[] = {3};

/* 2p5gbe_led */
static int mt7987_2p5gbe_led0_pins[] = {45};
static int mt7987_2p5gbe_led0_funcs[] = {1};

static int mt7987_2p5gbe_led1_0_pins[] = {13};
static int mt7987_2p5gbe_led1_0_funcs[] = {2};

static int mt7987_2p5gbe_led1_1_pins[] = {49};
static int mt7987_2p5gbe_led1_1_funcs[] = {3};

/* mdc, mdio */
static int mt7987_2p5g_ext_mdc_mdio_pins[] = {23, 24};
static int mt7987_2p5g_ext_mdc_mdio_funcs[] = {4, 4};

static int mt7987_mdc_mdio_pins[] = {39, 40};
static int mt7987_mdc_mdio_funcs[] = {1, 1};

/* spi */
static int mt7987_spi0_pins[] = {15, 16, 17, 18};
static int mt7987_spi0_funcs[] = {1, 1, 1, 1};

static int mt7987_spi0_wp_hold_pins[] = {19, 20};
static int mt7987_spi0_wp_hold_funcs[] = {1, 1};

static int mt7987_spi1_pins[] = {21, 22, 23, 24};
static int mt7987_spi1_funcs[] = {1, 1, 1, 1};

static int mt7987_spi1_1_pins[] = {46, 47, 48, 49};
static int mt7987_spi1_1_funcs[] = {2, 2, 2, 2};

static int mt7987_spi2_pins[] = {25, 26, 27, 28};
static int mt7987_spi2_funcs[] = {1, 1, 1, 1};

static int mt7987_spi2_wp_hold_pins[] = {29, 30};
static int mt7987_spi2_wp_hold_funcs[] = {1, 1};

/* emmc */
static int mt7987_emmc_45_pins[] = {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
static int mt7987_emmc_45_funcs[] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

/* sd */
static int mt7987_sd_pins[] = {15, 16, 17, 18, 23, 24};
static int mt7987_sd_funcs[] = {2, 2, 2, 2, 2, 2};

/* i2c */
static int mt7987_i2c0_0_pins[] = {29, 30};
static int mt7987_i2c0_0_funcs[] = {2, 2};

static int mt7987_i2c0_1_pins[] = {39, 40};
static int mt7987_i2c0_1_funcs[] = {2, 2};

static int mt7987_i2c0_2_pins[] = {43, 44};
static int mt7987_i2c0_2_funcs[] = {1, 1};

/* pcie */
static int mt7987_pcie0_pereset_pins[] = {33};
static int mt7987_pcie0_pereset_funcs[] = {1};

static int mt7987_pcie0_clkreq_pins[] = {34};
static int mt7987_pcie0_clkreq_funcs[] = {1};

static int mt7987_pcie0_wake_pins[] = {35};
static int mt7987_pcie0_wake_funcs[] = {1};

static int mt7987_pcie1_pereset_pins[] = {36};
static int mt7987_pcie1_pereset_funcs[] = {1};

static int mt7987_pcie1_clkreq_pins[] = {37};
static int mt7987_pcie1_clkreq_funcs[] = {1};

static int mt7987_pcie1_wake_pins[] = {38};
static int mt7987_pcie1_wake_funcs[] = {1};

static int mt7987_pcie_phy_i2c_pins[] = {43, 44};
static int mt7987_pcie_phy_i2c_funcs[] = {3, 3};

static const struct group_desc mt7987_groups[] = {
	PINCTRL_PIN_GROUP("watchdog", mt7987_watchdog),
	PINCTRL_PIN_GROUP("jtag", mt7987_jtag),
	PINCTRL_PIN_GROUP("pcm0_0", mt7987_pcm0_0),
	PINCTRL_PIN_GROUP("pcm0_1", mt7987_pcm0_1),
	PINCTRL_PIN_GROUP("uart0", mt7987_uart0),
	PINCTRL_PIN_GROUP("uart1_0", mt7987_uart1_0),
	PINCTRL_PIN_GROUP("uart1_0_lite", mt7987_uart1_0_lite),
	PINCTRL_PIN_GROUP("uart1_1", mt7987_uart1_1),
	PINCTRL_PIN_GROUP("uart1_2", mt7987_uart1_2),
	PINCTRL_PIN_GROUP("uart2_0", mt7987_uart2_0),
	PINCTRL_PIN_GROUP("uart2_1", mt7987_uart2_1),
	PINCTRL_PIN_GROUP("pwm0", mt7987_pwm0),
	PINCTRL_PIN_GROUP("pwm1_0", mt7987_pwm1_0),
	PINCTRL_PIN_GROUP("pwm1_1", mt7987_pwm1_1),
	PINCTRL_PIN_GROUP("pwm2_0", mt7987_pwm2_0),
	PINCTRL_PIN_GROUP("pwm2_1", mt7987_pwm2_1),
	PINCTRL_PIN_GROUP("drv_vbus_p1", mt7987_drv_vbus_p1),
	PINCTRL_PIN_GROUP("drv_vbus", mt7987_drv_vbus),
	PINCTRL_PIN_GROUP("2p5gbe_led0", mt7987_2p5gbe_led0),
	PINCTRL_PIN_GROUP("2p5gbe_led1_0", mt7987_2p5gbe_led1_0),
	PINCTRL_PIN_GROUP("2p5gbe_led1_1", mt7987_2p5gbe_led1_1),
	PINCTRL_PIN_GROUP("2p5g_ext_mdc_mdio", mt7987_2p5g_ext_mdc_mdio),
	PINCTRL_PIN_GROUP("mdc_mdio", mt7987_mdc_mdio),
	PINCTRL_PIN_GROUP("spi0", mt7987_spi0),
	PINCTRL_PIN_GROUP("spi0_wp_hold", mt7987_spi0_wp_hold),
	PINCTRL_PIN_GROUP("spi1", mt7987_spi1),
	PINCTRL_PIN_GROUP("spi1_1", mt7987_spi1_1),
	PINCTRL_PIN_GROUP("spi2", mt7987_spi2),
	PINCTRL_PIN_GROUP("spi2_wp_hold", mt7987_spi2_wp_hold),
	PINCTRL_PIN_GROUP("emmc_45", mt7987_emmc_45),
	PINCTRL_PIN_GROUP("sd", mt7987_sd),
	PINCTRL_PIN_GROUP("i2c0_0", mt7987_i2c0_0),
	PINCTRL_PIN_GROUP("i2c0_1", mt7987_i2c0_1),
	PINCTRL_PIN_GROUP("i2c0_2", mt7987_i2c0_2),
	PINCTRL_PIN_GROUP("pcie0_pereset", mt7987_pcie0_pereset),
	PINCTRL_PIN_GROUP("pcie0_clkreq", mt7987_pcie0_clkreq),
	PINCTRL_PIN_GROUP("pcie0_wake", mt7987_pcie0_wake),
	PINCTRL_PIN_GROUP("pcie1_pereset", mt7987_pcie1_pereset),
	PINCTRL_PIN_GROUP("pcie1_clkreq", mt7987_pcie1_clkreq),
	PINCTRL_PIN_GROUP("pcie1_wake", mt7987_pcie1_wake),
	PINCTRL_PIN_GROUP("pcie1_pcie_phy_i2c", mt7987_pcie_phy_i2c),
};

static const char *const mt7987_wdt_groups[] = {"watchdog",};
static const char *const mt7987_jtag_groups[] = {"jtag",};
static const char *const mt7987_pcm_groups[] = {"pcm0_0", "pcm0_1"};
static const char *const mt7987_uart_groups[] = {"uart0", "uart1_0",
						 "uart1_0_lite", "uart1_1",
						 "uart1_2", "uart2_0",
						 "uart2_1",};
static const char *const mt7987_pwm_groups[] = {"pwm0", "pwm1_0", "pwm1_1", "pwm2_0",
					       "pwm2_1",};
static const char *const mt7987_usb_groups[] = {"drv_vbus_p1", "drv_vbus",};
static const char *const mt7987_led_groups[] = {"2p5gbe_led0", "2p5gbe_led1_0",
					       "2p5gbe_led1_1",};
static const char *const mt7987_ethernet_groups[] = {"2p5g_ext_mdc_mdio", "mdc_mdio",};
static const char *const mt7987_spi_groups[] = {"spi0", "spi0_wp_hold", "spi1",
						"spi1_1", "spi2", "spi2_wp_hold",};
static const char *const mt7987_flash_groups[] = {"emmc_45", "sd"};
static const char *const mt7987_i2c_groups[] = {"i2c0_0", "i2c0_1", "i2c0_2",};
static const char *const mt7987_pcie_groups[] = {"pcie_phy_i2c", "pcie0_pereset",
						 "pcie0_clkreq", "pcie0_wake",
						 "pcie1_pereset", "pcie1_clkreq",
						 "pcie1_wake",};
static const char *const mt7987_i2s_groups[] = {"pcm0_0", "pcm0_1"};


static const struct function_desc mt7987_functions[] = {
	{"wdt", mt7987_wdt_groups, ARRAY_SIZE(mt7987_wdt_groups)},
	{"jtag", mt7987_jtag_groups, ARRAY_SIZE(mt7987_jtag_groups)},
	{"pcm", mt7987_pcm_groups, ARRAY_SIZE(mt7987_pcm_groups)},
	{"uart", mt7987_uart_groups, ARRAY_SIZE(mt7987_uart_groups)},
	{"pwm", mt7987_pwm_groups, ARRAY_SIZE(mt7987_pwm_groups)},
	{"usb", mt7987_usb_groups, ARRAY_SIZE(mt7987_usb_groups)},
	{"led", mt7987_led_groups, ARRAY_SIZE(mt7987_led_groups)},
	{"eth", mt7987_ethernet_groups, ARRAY_SIZE(mt7987_ethernet_groups)},
	{"spi", mt7987_spi_groups, ARRAY_SIZE(mt7987_spi_groups)},
	{"flash", mt7987_flash_groups, ARRAY_SIZE(mt7987_flash_groups)},
	{"i2c", mt7987_i2c_groups, ARRAY_SIZE(mt7987_i2c_groups)},
	{"pcie", mt7987_pcie_groups, ARRAY_SIZE(mt7987_pcie_groups)},
	{"i2s", mt7987_i2s_groups, ARRAY_SIZE(mt7987_i2s_groups)},
};

static const struct mtk_eint_hw mt7987_eint_hw = {
	.port_mask = 7,
	.ports = 7,
	.ap_num = ARRAY_SIZE(mt7987_pins),
	.db_cnt = 16,
};

static const char * const mt7987_pinctrl_register_base_names[] = {
	"gpio", "iocfg_rb", "iocfg_lb", "iocfg_rt1", "iocfg_rt2", "iocfg_tl",
};

static struct mtk_pin_soc mt7987_data = {
	.reg_cal = mt7987_reg_cals,
	.pins = mt7987_pins,
	.npins = ARRAY_SIZE(mt7987_pins),
	.grps = mt7987_groups,
	.ngrps = ARRAY_SIZE(mt7987_groups),
	.funcs = mt7987_functions,
	.nfuncs = ARRAY_SIZE(mt7987_functions),
	.eint_hw = &mt7987_eint_hw,
	.gpio_m = 0,
	.ies_present = false,
	.base_names = mt7987_pinctrl_register_base_names,
	.nbase_names = ARRAY_SIZE(mt7987_pinctrl_register_base_names),
	.bias_disable_set = mtk_pinconf_bias_disable_set,
	.bias_disable_get = mtk_pinconf_bias_disable_get,
	.bias_set = mtk_pinconf_bias_set,
	.bias_get = mtk_pinconf_bias_get,
	.pull_type = mt7987_pull_type,
	.bias_set_combo = mtk_pinconf_bias_set_combo,
	.bias_get_combo = mtk_pinconf_bias_get_combo,
	.drive_set = mtk_pinconf_drive_set_rev1,
	.drive_get = mtk_pinconf_drive_get_rev1,
	.adv_pull_get = mtk_pinconf_adv_pull_get,
	.adv_pull_set = mtk_pinconf_adv_pull_set,
};

static const struct of_device_id mt7987_pinctrl_of_match[] = {
	{
		.compatible = "mediatek,mt7987-pinctrl",
	},
	{}
};

static int mt7987_pinctrl_probe(struct platform_device *pdev)
{
	return mtk_moore_pinctrl_probe(pdev, &mt7987_data);
}

static struct platform_driver mt7987_pinctrl_driver = {
	.driver = {
		.name = "mt7987-pinctrl",
		.of_match_table = mt7987_pinctrl_of_match,
	},
	.probe = mt7987_pinctrl_probe,
};

static int __init mt7987_pinctrl_init(void)
{
	return platform_driver_register(&mt7987_pinctrl_driver);
}
arch_initcall(mt7987_pinctrl_init);
