/*************************************************************************/ /*!
@Title          System Description Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    This header provides system-specific declarations and macros
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

#if !defined(__SOCCONFIG_H__)
#define __SOCCONFIG_H__

#define VS_PRODUCT_NAME	"SGX Emulator"

/* this system has a SGX host port */
#if defined(SGX535)
#define SGX_FEATURE_HOST_PORT
#endif


#if defined(SGX_FEATURE_MP)
/* One block for each core plus one for the all-cores bank and one for the master bank.*/
#define SGX_REG_SIZE 		(0x4000 * (SGX_FEATURE_MP_CORE_COUNT_3D + 2))
#define SGX_EMUREG_OFFSET	(0x20000)
#else
#define SGX_REG_SIZE 		(0x4000)
#define SGX_EMUREG_OFFSET	(0x8000)
#endif /* SGX_FEATURE_MP */
#define SGX_REG_REGION_SIZE (0x10000)
#define SGXEMU_REG_SIZE		(0x10000)


#define PCI_BASEREG_OFFSET_DWORDS			4

#if defined(EMULATE_ATLAS_3BAR)

	/* Atlas reg on base register 0 */
	#define EMULATOR_ATLAS_REG_PCI_BASENUM		    0
	#define EMULATOR_ATLAS_REG_PCI_OFFSET		    (EMULATOR_ATLAS_REG_PCI_BASENUM + PCI_BASEREG_OFFSET_DWORDS)

	#define EMULATOR_ATLAS_REG_OFFSET                0x0
	#define EMULATOR_ATLAS_REG_SIZE                  0x4000


	#define EMULATOR_ATLAS_REG_REGION_SIZE			0x10000

	/* SGX reg on base register 1 */
	#define EMULATOR_SGX_REG_PCI_BASENUM				1
	#define EMULATOR_SGX_REG_PCI_OFFSET		        (EMULATOR_SGX_REG_PCI_BASENUM + PCI_BASEREG_OFFSET_DWORDS)

	#define EMULATOR_SGX_REG_OFFSET                  0x0
	#define EMULATOR_SGX_REG_SIZE 				    0x4000

	#define EMULATOR_SGX_REG_REGION_SIZE				0x8000

	/* SGX mem (including HP mapping) on base register 2 */
	#define EMULATOR_SGX_MEM_PCI_BASENUM				2
	#define EMULATOR_SGX_MEM_PCI_OFFSET		        (EMULATOR_SGX_MEM_PCI_BASENUM + PCI_BASEREG_OFFSET_DWORDS)

	#define EMULATOR_SGX_MEM_REGION_SIZE				0x20000000

#else /* EMULATE_ATLAS_3BAR */

	/* PCI config register to get the phys base address of SGX SocIF */
	#define EMULATOR_SGX_PCI_BASENUM			2
	#define EMULATOR_SGX_PCI_OFFSET				(PCI_BASEREG_OFFSET_DWORDS + EMULATOR_SGX_PCI_BASENUM)

#endif /* EMULATE_ATLAS_3BAR */

#define DEVICE_SGX_INTERRUPT		(1UL<<0)
#define DEVICE_MSVDX_INTERRUPT		(1UL<<1)
#define DEVICE_DISP_INTERRUPT		(1UL<<2)

#define EMULATOR_CR_BASE			0x0

/* Emulator interrupt related registers */
#define EMULATOR_CR_INT_STATUS				(EMULATOR_CR_BASE + 0x1418)
#define EMULATOR_CR_INT_MASK				(EMULATOR_CR_BASE + 0x141C)		/* it IS a mask so zero is enable */
#define EMULATOR_CR_INT_CLEAR				(EMULATOR_CR_BASE + 0x1420)

/* Interrupt bits */
#define	EMULATOR_INT_ENABLE_MASTER_ENABLE	0x8000UL
#define	EMULATOR_INT_ENABLE_MASTER_SHIFT	15

#define	EMULATOR_INT_MASK_MASTER_MASK		0x8000UL
#define	EMULATOR_INT_MASK_MASTER_SHIFT		15
#define	EMULATOR_INT_MASK_THALIA_MASK		0x0001UL
#define	EMULATOR_INT_MASK_THALIA_SHIFT		0

#define	EMULATOR_INT_CLEAR_MASTER_MASK		0x8000UL
#define	EMULATOR_INT_CLEAR_MASTER_SHIFT		15
#define	EMULATOR_INT_CLEAR_THALIA_MASK		0x0001UL
#define	EMULATOR_INT_CLEAR_THALIA_SHIFT		0


/* Emulator ID */
#define VENDOR_ID_EMULATOR                  0x1010

#define DEVICE_ID_EMULATOR                  0x1CE0
#define DEVICE_ID_EMULATOR_PCIE             0x1CE3
#define DEVICE_ID_TEST_CHIP                 0x1CF0
#define DEVICE_ID_TEST_CHIP_PCIE            0x1CF3

#define SYS_SGX_DEV_VENDOR_ID               VENDOR_ID_EMULATOR 
#define SYS_SGX_DEV_DEVICE_ID               DEVICE_ID_EMULATOR
#define SYS_SGX_DEV1_DEVICE_ID              DEVICE_ID_EMULATOR_PCIE

/* NOTE: this value must be kept in sync with PVRPDP_EMULATOR_SYSSURFACE_OFFSET */
#define SYS_LOCALMEM_SACRIFICE_SIZE	(64 * 1024 * 1024)

#if !defined(SGX545) || defined(EMULATE_ATLAS)
#define SYS_LOCALMEM_FOR_SGX_RESERVE_SIZE   ((256 * 1024 * 1024) - SYS_LOCALMEM_SACRIFICE_SIZE)
#else
#define SYS_LOCALMEM_FOR_SGX_RESERVE_SIZE   ((512 * 1024 * 1024) - SYS_LOCALMEM_SACRIFICE_SIZE)
#endif

/* Map no more than MEMTEST_MAP_SIZE into kernel space at any one time during the mem test */
#define MEMTEST_MAP_SIZE					(1024*1024)

#if defined(SGX_FEATURE_HOST_PORT)
	/* FIXME: change these dummy values if host port is needed */
	#define SYS_SGX_HP_SIZE					0x0
	/* device virtual address of host port base */
	#define SYS_SGX_HOSTPORT_BASE_DEVVADDR	0x0
#endif

#define JDBASE_ADDR                   (EMULATOR_CR_BASE + 0x1400)

// Original register compatable with driver
#define JDISPLAY_FRAME_BASE_ADD       (JDBASE_ADDR+0x0000)
#define JDISPLAY_FRAME_STRIDE_ADD     (JDBASE_ADDR+0x0004)
#define JDISPLAY_FBCTRL_ADD           (JDBASE_ADDR+0x0008)
#define JDISPLAY_SPGKICK_ADD          (JDBASE_ADDR+0x000C)
#define JDISPLAY_SPGSTATUS_ADD        (JDBASE_ADDR+0x0010)

#define JDISPLAY_FPGA_SOFT_RESETS_ADD (JDBASE_ADDR+0x0014)
//#define JDISPLAY_FPGA_IRQ_STAT_ADD    (JDBASE_ADDR+0x0018)
//#define JDISPLAY_FPGA_IRQ_MASK_ADD    (JDBASE_ADDR+0x001C)
//#define JDISPLAY_FPGA_IRQ_CLEAR_ADD   (JDBASE_ADDR+0x0020)
#define JDISPLAY_TRAS_ADD             (JDBASE_ADDR+0x0024)
#define JDISPLAY_TRP_ADD              (JDBASE_ADDR+0x0028)
#define JDISPLAY_TRCD_ADD             (JDBASE_ADDR+0x002C)
#define JDISPLAY_TRC_ADD              (JDBASE_ADDR+0x0030)
#define JDISPLAY_TREF_ADD             (JDBASE_ADDR+0x0034)
#define JDISPLAY_RFEN_REG_ADD         (JDBASE_ADDR+0x0038)
#define JDISPLAY_MEM_CONTROL_ADD      (JDBASE_ADDR+0x003C)

#define JDISPLAY_CURSOR_EN_ADD        (JDBASE_ADDR+0x0040)
#define JDISPLAY_CURSOR_XY_ADD        (JDBASE_ADDR+0x0044)
#define JDISPLAY_CURSOR_ADDR_ADD      (JDBASE_ADDR+0x0048)
#define JDISPLAY_CURSOR_DATA_ADD      (JDBASE_ADDR+0x004C)

//***********************
// New registers
//***********************
#define JDISPLAY_FRAME_SIZE_ADD       (JDBASE_ADDR+0x0050)
#define JDISPLAY_CURSOR_POSI_ADD      (JDBASE_ADDR+0x0058)
#define JDISPLAY_CURSRAM_CTRL_ADD     (JDBASE_ADDR+0x005C)
#define JDISPLAY_STATUS_ADD           (JDBASE_ADDR+0x0060)
#define JDISPLAY_GPIOREG_IN_ADD       (JDBASE_ADDR+0x0064)
#define JDISPLAY_GPIOREG_OUT_ADD      (JDBASE_ADDR+0x0068)
// jdisplay control register is the mirror of the signal in the top level of lcdfeed
#define JDISPLAY_CONTROL_ADD          (JDBASE_ADDR+0x0054)
// the next six register brakedown jdisplay control in to correct register chunks
#define JDISPLAY_FRAME_PACK_ADD       (JDBASE_ADDR+0x00B4)
#define JDISPLAY_STATIC_COLOUR_ADD    (JDBASE_ADDR+0x00B8)
#define JDISPLAY_MISC_CTRL_ADD        (JDBASE_ADDR+0x00BC)
#define JDISPLAY_SYNC_TRIG_ADD        (JDBASE_ADDR+0x00C0)
#define JDISPLAY_START_SYNC_ADD       (JDBASE_ADDR+0x00C4)
#define JDISPLAY_FETCH_FRAME_ADD      (JDBASE_ADDR+0x00C8)

// Syncgen timing register
#define JDISPLAY_TIM_VBPS_ADD         (JDBASE_ADDR+0x0080)
#define JDISPLAY_TIM_VTBS_ADD         (JDBASE_ADDR+0x0084)
#define JDISPLAY_TIM_VAS_ADD          (JDBASE_ADDR+0x0088)
#define JDISPLAY_TIM_VBBS_ADD         (JDBASE_ADDR+0x008C)
#define JDISPLAY_TIM_VFPS_ADD         (JDBASE_ADDR+0x0090)
#define JDISPLAY_TIM_VT_ADD           (JDBASE_ADDR+0x0094)
#define JDISPLAY_TIM_HBPS_ADD         (JDBASE_ADDR+0x0098)
#define JDISPLAY_TIM_HLBS_ADD         (JDBASE_ADDR+0x009C)
#define JDISPLAY_TIM_HAS_ADD          (JDBASE_ADDR+0x00A0)
#define JDISPLAY_TIM_HRBS_ADD         (JDBASE_ADDR+0x00A4)
#define JDISPLAY_TIM_HFPS_ADD         (JDBASE_ADDR+0x00A8)
#define JDISPLAY_TIM_HT_ADD           (JDBASE_ADDR+0x00AC)
#define JDISPLAY_TIM_VOFF_ADD         (JDBASE_ADDR+0x00B0)
/*****************************************************************************
 * system specific data structure
 *****************************************************************************/

/*!
 *****************************************************************************
	 PCI Config space mapping structures
 ****************************************************************************/
typedef struct
{
	union
	{
		IMG_UINT8	aui8PCISpace[256];
		IMG_UINT16	aui16PCISpace[128];
		IMG_UINT32	aui32PCISpace[64];
		struct
		{
			IMG_UINT16	ui16VenID;
			IMG_UINT16	ui16DevID;
			IMG_UINT16	ui16PCICmd;
			IMG_UINT16	ui16PCIStatus;
		}s;
	}u;
	
	IMG_UINT32 aui32BAROffset[6];
	IMG_UINT32 aui32BARSize[6];
} PCICONFIG_SPACE, *PPCICONFIG_SPACE;

#endif	/* __SOCCONFIG_H__ */
