/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 MediaTek Inc.
 */

#ifndef __LINUX_REGULATOR_MT6315_H
#define __LINUX_REGULATOR_MT6315_H

#define MT6315_SLAVE_ID_3	3
#define MT6315_SLAVE_ID_6	6
#define MT6315_SLAVE_ID_7	7

/* Register */
#define MT6315_SWCID_H				0xb
#define MT6315_TOP2_ELR7			0x139
#define MT6315_TOP_TMA_KEY			0x39f
#define MT6315_TOP_TMA_KEY_H			0x3a0
#define MT6315_BUCK_TOP_CON0			0x1440
#define MT6315_BUCK_TOP_CON1			0x1443
#define MT6315_BUCK_TOP_ELR0			0x1449
#define MT6315_BUCK_TOP_ELR2			0x144b
#define MT6315_BUCK_TOP_ELR4			0x144d
#define MT6315_BUCK_TOP_ELR6			0x144f
#define MT6315_VBUCK1_DBG0			0x1499
#define MT6315_VBUCK1_DBG4			0x149d
#define MT6315_VBUCK2_DBG0			0x1519
#define MT6315_VBUCK2_DBG4			0x151d
#define MT6315_VBUCK3_DBG0			0x1599
#define MT6315_VBUCK3_DBG4			0x159d
#define MT6315_VBUCK4_DBG0			0x1619
#define MT6315_VBUCK4_DBG4			0x161d
#define MT6315_BUCK_TOP_4PHASE_ANA_CON42	0x16b1

#define PROTECTION_KEY_H			0x9C
#define PROTECTION_KEY				0xEA

#endif /* __LINUX_REGULATOR_MT6315_H */
