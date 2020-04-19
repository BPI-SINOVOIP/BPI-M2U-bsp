/*
 *  arch/arm/mach-sunxi/pm/ccmu-sun3iw1p1.h
 *
 * Copyright 2012 (c) njubietech.
 * gq.yang (yanggq@njubietech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __MEM_CCMU_SUN3IW1P1_H__
#define __MEM_CCMU_SUN3IW1P1_H__

typedef union {
	__u32 dwval;
	struct {
		__u32 FactorM:2;	/*bit0,  PLL1 Factor M */
		__u32 reserved0:2;	/*bit2,  reserved */
		__u32 FactorK:2;	/*bit4,  PLL1 factor K */
		__u32 reserved1:2;	/*bit6,  reserved */
		__u32 FactorN:5;	/*bit8,  PLL1 Factor N */
		__u32 reserved2:3;	/*bit13, reserved */
		__u32 FactorP:2;	/*bit16, PLL1 Factor P */
		__u32 reserved3:10;	/*bit18, reserved */
		__u32 Lock:1;	/*bit28, pll is stable flag,1-pll has stabled */
		__u32 reserved5:2;	/*bit29, reserved */
		__u32 PLLEn:1;	/*bit31,0-disable,1-enable, (24Mhz*N*K)/(M*P) */

	} bits;
} __ccmu_pll1_reg0000_t;

typedef struct __CCMU_PLL2_REG0008 {
	__u32 FactorM:5;	/*bit0,  PLL2 prev division M */
	__u32 reserved0:3;	/*bit5,  reserved */
	__u32 FactorN:7;	/*bit8,  PLL2 factor N */
	__u32 reserved1:9;	/*bit15, reserved */
	__u32 SdmEn:1;		/*bit24, pll sdm enable,
				factorN only low 4 bits valid when enable */
	__u32 reserved3:3;	/*bit25, reserved */
	__u32 Lock:1;		/*bit28, pll stable flag */
	__u32 reserved4:2;	/*bit29, reserved */
	__u32 PLLEn:1;		/*bit31, PLL2 enable */
} __ccmu_pll2_reg0008_t;

typedef struct __CCMU_PLL3_REG0010 {
	__u32 FactorM:4;	/*bit0,  PLL3 FactorM */
	__u32 reserved0:4;	/*bit4,  reserved */
	__u32 FactorN:7;	/*bit8,  PLL factor N */
	__u32 reserved1:5;	/*bit15, reserved */
	__u32 SdmEn:1;		/*bit20, sdm enable */
	__u32 reserved2:3;	/*bit21, reserved */
	__u32 ModeSel:1;	/*bit24, PLL mode select */
	__u32 FracMod:1;	/*bit25, PLL out is 0:270Mhz, 1:297Mhz */
	__u32 reserved3:2;	/*bit26, reserved */
	__u32 Lock:1;		/*bit28, lock flag */
	__u32 reserved4:1;	/*bit29, reserved */
	__u32 CtlMode:1;	/*bit30, control mode,
				0-controled by cpu, 1-control by DE */
	__u32 PLLEn:1;		/*bit31, PLL3 enable */
} __ccmu_pll3_reg0010_t;

typedef struct __CCMU_PLL4_REG0018 {
	__u32 FactorM:4;	/*bit0,  PLL3 FactorM */
	__u32 reserved0:4;	/*bit4,  reserved */
	__u32 FactorN:7;	/*bit8,  PLL factor N */
	__u32 reserved1:9;	/*bit15, reserved */
	__u32 ModeSel:1;	/*bit24, PLL mode select */
	__u32 FracMod:1;	/*bit25, PLL out is 0:270Mhz, 1:297Mhz */
	__u32 reserved3:2;	/*bit26, reserved */
	__u32 Lock:1;		/*bit28, lock flag */
	__u32 reserved4:2;	/*bit29, reserved */
	__u32 PLLEn:1;		/*bit31, PLL3 enable */
} __ccmu_pll4_reg0018_t;

typedef struct __CCMU_MEDIA_PLL {
	__u32 FactorM:4;	/*bit0,  PLL3 FactorM */
	__u32 reserved0:4;	/*bit4,  reserved */
	__u32 FactorN:7;	/*bit8,  PLL factor N */
	__u32 reserved1:5;	/*bit15, reserved */
	__u32 SdmEn:1;		/*bit20, sdm enable */
	__u32 reserved2:3;	/*bit21, reserved */
	__u32 ModeSel:1;	/*bit24, PLL mode select */
	__u32 FracMod:1;	/*bit25, PLL out is 0:270Mhz, 1:297Mhz */
	__u32 reserved3:2;	/*bit26, reserved */
	__u32 Lock:1;		/*bit28, lock flag */
	__u32 reserved4:1;	/*bit29, reserved */
	__u32 CtlMode:1;	/*bit30, control mode,
				0-controled by cpu, 1-control by DE */
	__u32 PLLEn:1;		/*bit31, PLL3 enable */
} __ccmu_media_pll_t;

typedef struct __CCMU_PLL5_REG0020 {
	__u32 FactorM:2;	/*bit0,  PLL5 factor M */
	__u32 reserved0:2;	/*bit2,  reserved */
	__u32 FactorK:2;	/*bit4,  PLL5 factor K */
	__u32 reserved1:2;	/*bit6,  reserved */
	__u32 FactorN:5;	/*bit8,  PLL5 factor N */
	__u32 reserved2:7;	/*bit13,  reserved */
	__u32 CfgUpdate:1;	/*bit20, Configuration Update */
	__u32 reserved3:3;	/*bit21,  reserved */
	__u32 SigmaDeltaEn:1;	/*bit24,  Sigma Delta En */
	__u32 reserved4:3;	/*bit25,  reserved */
	__u32 Lock:1;		/*bit28, lock flag */
	__u32 reserved5:2;	/*bit29, reserved */
	__u32 PLLEn:1;		/*bit31, PLL5 Enable */
} __ccmu_pll5_reg0020_t;

typedef struct __CCMU_PLL6_REG0028 {
	__u32 FactorM:2;	/*bit0,  PLL6 factor M */
	__u32 reserved0:2;	/*bit2,  reserved */
	__u32 FactorK:2;	/*bit4,  PLL6 factor K */
	__u32 reserved1:2;	/*bit6,  reserved */
	__u32 FactorN:5;	/*bit8,  PLL6 factor N */
	__u32 reserved2:3;	/*bit13, reserved */
	__u32 Pll24MPdiv:2;	/*bit16, PLL 24M output clock post divider */
	__u32 Pll24MOutEn:1;	/*bit18, PLL 24M output enable */
	__u32 reserved3:9;	/*bit19, reserved */
	__u32 Lock:1;		/*bit28, lock flag */
	__u32 reserved5:2;	/*bit29, reserved */
	__u32 PLLEn:1;		/*bit31, PLL6 enable */
} __ccmu_pll6_reg0028_t;

#define AC320_CLKSRC_LOSC   (0)
#define AC320_CLKSRC_HOSC   (1)
#define AC320_CLKSRC_PLL1   (2)
typedef union {
	__u32 dwval;
	struct {
		__u32 reserved0:16;	/*bit0,  reserved */
		__u32 CpuClkSrc:2;	/*bit16, CPU1/2/3/4 clock source select,
					00-internal LOSC,01-OSC24M, 10/11-PLL1*/
		__u32 reserved2:14;	/*bit18, reserved */
	} bits;
} __ccmu_sysclk_ratio_reg0050_t;

#define AHB1_CLKSRC_LOSC    (0)
#define AHB1_CLKSRC_HOSC    (1)
#define AHB1_CLKSRC_AXI     (2)
#define AHB1_CLKSRC_PLL6    (3)
typedef union {
	__u32 dwval;
	struct {
		__u32 reserved0:4;	/*bit0,  reserved */
		__u32 Ahb1Div:2;	/*bit4, ahb1 clk div ratio, 1/2/4/8 */
		__u32 Ahb1PreDiv:2;	/*bit6, ahb1 clk pre-div ratio 1/2/3/4*/
		__u32 Apb1Div:2;	/*bit8, apb1 clk div ratio 2/2/4/8,
					source is ahb1 */
		__u32 reserved1:2;	/*bit10, reserved */
		__u32 Ahb1ClkSrc:2;	/*bit12, ahb1 clk source, 00-LOSC, 01-
					OSC24M,10-CPUCLK, 11-PLL6/ahb1_pre_div*/
		__u32 reserved2:2;	/*bit14, reserved */
		__u32 HclkcDiv:2;	/*bit16, HCLKC clk div ratio. 00: /1,
					01: /2, 10: /3, 11: /4*/
		__u32 reserved3:14;	/*bit18, reserved */
	} bits;
} __ccmu_ahb1_ratio_reg0054_t;

typedef struct __CCMU_PLLLOCK_REG0200 {
	__u32 LockTime:16;	/*bit0,  PLL lock time, based on us */
	__u32 reserved:16;	/*bit16, reserved */
} __ccmu_plllock_reg0200_t;

typedef struct __CCMU_REG_LIST {
	volatile __ccmu_pll1_reg0000_t Pll1Ctl;	/*0x0000, PLL1 control, cpux */
	volatile __u32 reserved0;	/*0x0004, reserved */
	volatile __ccmu_pll2_reg0008_t Pll2Ctl;	/*0x0008, PLL2 control, audio */
	volatile __u32 reserved1;	/*0x000c, reserved */
	volatile __ccmu_pll3_reg0010_t Pll3Ctl;	/*0x0010, PLL3 control, video */
	volatile __u32 reserved2;	/*0x0014, reserved */
	volatile __ccmu_pll4_reg0018_t Pll4Ctl;	/*0x0018, PLL4 control, ve */
	volatile __u32 reserved4;	/*0x001c, reserved */
	volatile __ccmu_pll5_reg0020_t Pll5Ctl;	/*0x0020, PLL5 control, ddr0 */
	volatile __u32 reserved5;	/*0x0024, reserved */
	volatile __ccmu_pll6_reg0028_t Pll6Ctl;	/*0x0028, PLL6 control, periph*/
	volatile __u32 reserved6[9];	/*0x002c - 0x004c, reserved */
	volatile __ccmu_sysclk_ratio_reg0050_t SysClkDiv;
					/*0x0050, system clock divide ratio */
	volatile __ccmu_ahb1_ratio_reg0054_t Ahb1Div;
					/*0x0054, ahb1/apb1 clk divide ratio */
	volatile __u32 reserved7[2];	/*0x0058 - 0x005c, reserved*/
	volatile __u32 AhbGate0;	/*0x0060, bus gating reg0 */
	volatile __u32 AhbGate1;	/*0x0064, bus gating reg1 */
	volatile __u32 Apb1Gate;	/*0x0068, bus gating reg2 */
	volatile __u32 reserved8[7];	/*0x006c - 0x0084, reserved */
	volatile __u32 Sd0;	/*0x0088, sd/mmc controller 0 clock */
	volatile __u32 Sd1;	/*0x008c, sd/mmc controller 1 clock */
	volatile __u32 reserved9[8];	/*0x0090 - 0x00ac, reserved */
	volatile __u32 I2s0;	/*0x00b0, daudio-0 clock? */
	volatile __u32 SpdifClk;	/*0x00b4, spdif clock*/
	volatile __u32 CirClk;	/*0x00b8, cir clock */
	volatile __u32 reserved10[4];	/*0x00bc - 0x00c8, reserved */
	volatile __u32 Usb;	/*0x00cc, usb phy clock */
	volatile __u32 reserved11[12];	/*0x00d0 - 0x00fc, reserved */
	volatile __u32 DramGate;	/*0x0100, dram module clock */
	volatile __u32 Be0;	/*0x0104, BE module clk */
	volatile __u32 reserved12;	/*0x0108, reserved*/
	volatile __u32 Fe0;	/*0x010c, FE0 module clock */
	volatile __u32 reserved13[2];	/*0x0110 - 0x00114, reserved */
	volatile __u32 Lcd0Ch0;	/*0x0118, LCD0 CH0 module clock */
	volatile __u32 DeinterlaceClk;	/*0x011c, De-interface clock */
	volatile __u32 Tve;	/*0x0120, TVE clock*/
	volatile __u32 Tvd;	/*0x0124, TVD clock*/
	volatile __u32 reserved14[3];	/*0x0128-0x0130, reserved */
	volatile __u32 Csi0;	/*0x0134, csi0 module clock */
	volatile __u32 reserved15;	/*0x0138, reserved */
	volatile __u32 Ve;	/*0x013c, VE clock*/
	volatile __u32 AudioCodecClk;	/*0x0140, Audio Codec clock register */
	volatile __u32 Avs143;	/*0x0144, avs module clock */
	volatile __u32 reserved16[46];	/*0x0148 - 0x1fc, reserved */
	volatile __ccmu_plllock_reg0200_t PllLock;/*0x0200, pll lock time */
	volatile __u32 Pll1Lock;	/*0x0204, pll1 cpu lock time */
	volatile __u32 reserved17[6];	/*0x0208-0x21c, reserved */
	volatile __u32 PllxBias[1];	/*0x220,  pll cpux  bias reg */
	volatile __u32 PllAudioBias;	/*0x224,  pll audio bias reg */
	volatile __u32 PllVideo0Bias;	/*0x228,  pll vedio bias reg */
	volatile __u32 reserved201;	/*0x22c,  pll ve    bias reg */
	volatile __u32 PllDram0Bias;	/*0x230,  pll dram0 bias reg */
	volatile __u32 PllPeriphBias;	/*0x234,  pll periph bias reg */
	volatile __u32 reserved18[6];	/*0x238 - 0x24c,  pll gpu   bias reg */
	volatile __u32 Pll1Tun;	/*0x250, pll1 tun reg */
	volatile __u32 reserved203[3];	/*0x254-0x25c, reserved */
	volatile __u32 Pll5Tun;	/*0x260, pll5 tun reg */
	volatile __u32 reserved19[8];	/*0x264-0x280, reserved */
	volatile __u32 PllAudioPattern;	/*0x284,  pll audio pattern reg */
	volatile __u32 PllVedio0Pattern;/*0x288,  pll vedio pattern reg */
	volatile __u32 reserved20;	/*0x28c, reserved */
	volatile __u32 PllDram0PatternReg;/*0x290,  pll dram pattern reg0 */
	volatile __u32 reserved21[11];	/*0x294-0x2bc, reserved */
	volatile __u32 AhbReset0;	/*0x2c0, AHB1 module reset register 0 */
	volatile __u32 AhbReset1;	/*0x2c4, AHB1 module reset register 1 */
	volatile __u32 reserved22[2];	/*0x2c8-0x2cc*/
	volatile __u32 Apb1Reset;	/*0x2d0, APB1 module reset register */
} __ccmu_reg_list_t;

#endif				/* #ifndef __MEM_CCMU_SUN3IW1P1_H__ */
