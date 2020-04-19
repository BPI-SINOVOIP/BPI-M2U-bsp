/*************************************************************************/ /*!
@Title          System Description
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    System Description functions
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

#include "sgxdefs.h"
#include "services_headers.h"
#include "kerneldisplay.h"
#include "oemfuncs.h"
#include "sgxinfo.h"
#include "sgxinfokm.h"


#define SYS_SGX_HWRECOVERY_TIMEOUT_FREQ		(1) // 1s (1hz)
#define SYS_SGX_PDS_TIMER_FREQ				(5) // 0.1Hz

#if !defined(__linux__)
//#define FORCE_RESET_SGX
//#define DO_2D_VIDMEM_TEST
#define MEMTEST
#endif

/* top level system data anchor point*/
SYS_DATA* gpsSysData = (SYS_DATA*)IMG_NULL;
SYS_DATA  gsSysData;


/* SGX structures */
static IMG_UINT32       gui32SGXDeviceID;
static SGX_DEVICE_MAP   gsSGXDeviceMap;
static IMG_CPU_PHYADDR  gsSOCRegsCpuPBase;

IMG_UINT32 PVRSRV_BridgeDispatchKM( IMG_UINT32  Ioctl,
									IMG_BYTE   *pInBuf,
									IMG_UINT32  InBufLen,
									IMG_BYTE   *pOutBuf,
									IMG_UINT32  OutBufLen,
									IMG_UINT32 *pdwBytesTransferred);

#if defined(MEMTEST)
static PVRSRV_ERROR MemTest(IMG_VOID);
#endif

// Globals
IMG_BOOL	bTestChipBoard=IMG_FALSE;
IMG_UINT32	ui32AtlasRev=0;
IMG_UINT32	ui32CoreID=0;
IMG_UINT32	ui32CoreRev=0;



#ifdef __linux__
// static PVRSRV_PCI_DEV_HANDLE hSGXPCI;
static IMG_HANDLE	hSGXPCI;

/*!
******************************************************************************

 @Function		PCIInitDev

 @Description

 Initialise the PCI device for subsequent use.

 @Return	PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR PCIInitDev(IMG_VOID)
{
	/* Old Test chip ID and Emulator board */
	hSGXPCI = OSPCIAcquireDev(VENDOR_ID_EMULATOR, DEVICE_ID_EMULATOR, 0);
	if (!hSGXPCI)
	{
		/* New Test Chip ID (From 1st Nov 2006) */
		hSGXPCI = OSPCIAcquireDev(VENDOR_ID_EMULATOR, DEVICE_ID_TEST_CHIP, 0);
	}
	if (!hSGXPCI)
	{
		/* PCI Express Emulator board ID */
		hSGXPCI = OSPCIAcquireDev(VENDOR_ID_EMULATOR, DEVICE_ID_EMULATOR_PCIE, HOST_PCI_INIT_FLAG_BUS_MASTER);
	}

	if (!hSGXPCI)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Failed to acquire PCI device"));
		return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
	}

#if defined(EMULATE_ATLAS_3BAR)

	PVR_TRACE(("Emulator Atlas Reg PCI memory region: %x to %x", OSPCIAddrRangeStart(hSGXPCI, EMULATOR_ATLAS_REG_PCI_BASENUM), OSPCIAddrRangeEnd(hSGXPCI, EMULATOR_ATLAS_REG_PCI_BASENUM)));

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(hSGXPCI, EMULATOR_ATLAS_REG_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available (atlas reg)"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}

	PVR_TRACE(("Emulator SGX Reg PCI memory region: %x to %x", OSPCIAddrRangeStart(hSGXPCI, EMULATOR_SGX_REG_PCI_BASENUM), OSPCIAddrRangeEnd(hSGXPCI, EMULATOR_SGX_REG_PCI_BASENUM)));

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(hSGXPCI, EMULATOR_SGX_REG_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available (SGX reg) "));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}

	PVR_TRACE(("Emulator SGX Mem PCI memory region: %x to %x", OSPCIAddrRangeStart(hSGXPCI, EMULATOR_SGX_MEM_PCI_BASENUM), OSPCIAddrRangeEnd(hSGXPCI, EMULATOR_SGX_MEM_PCI_BASENUM)));

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(hSGXPCI, EMULATOR_SGX_MEM_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: Device memory region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}

#else
	PVR_TRACE(("SGX Emulator PCI region: %x to %x", OSPCIAddrRangeStart(hSGXPCI, EMULATOR_SGX_PCI_BASENUM), OSPCIAddrRangeEnd(hSGXPCI, EMULATOR_SGX_PCI_BASENUM)));

	/* Reserve the address range */
	if (OSPCIRequestAddrRange(hSGXPCI, EMULATOR_SGX_PCI_BASENUM) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"PCIInitDev: SGX Emulator PCI region not available"));
		return PVRSRV_ERROR_PCI_REGION_UNAVAILABLE;
	}
#endif

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function		PCIDeInitDev

 @Description

 Uninitialise the PCI device when it is no loger required

 @Return	none

******************************************************************************/
static IMG_VOID PCIDeInitDev(IMG_VOID)
{
#if defined(EMULATE_ATLAS_3BAR)
	OSPCIReleaseAddrRange(hSGXPCI, EMULATOR_ATLAS_REG_PCI_BASENUM);
	OSPCIReleaseAddrRange(hSGXPCI, EMULATOR_SGX_REG_PCI_BASENUM);
	OSPCIReleaseAddrRange(hSGXPCI, EMULATOR_SGX_MEM_PCI_BASENUM);
#else
	OSPCIReleaseAddrRange(hSGXPCI, EMULATOR_SGX_PCI_BASENUM);
#endif

	OSPCIReleaseDev(hSGXPCI);
}
#else	/* __linux__ */
/*!
******************************************************************************

 @Function		FindPCIDevice

 @Description	Scans the PCI bus for ui16DevID/ui16VenID and if found,
				enumerates the PCI config registers.

 @Input			ui16VenID
 @Input			ui16DevID
 @Output		psPCISpace

 @Return		PVRSRV_ERROR

******************************************************************************/
static PVRSRV_ERROR FindPCIDevice(IMG_UINT16 ui16VenID, IMG_UINT16 ui16DevID, PCICONFIG_SPACE *psPCISpace)
{
	IMG_UINT32 ui32BusNum;
	IMG_UINT32 ui32DevNum;
	IMG_UINT32 ui32VenDevID;
	
	/* Check all the busses */
	for (ui32BusNum=0; ui32BusNum < 255; ui32BusNum++)
	{
		/* Check all devices on that bus */
		for (ui32DevNum=0; ui32DevNum < 32; ui32DevNum++)
		{
			/* Get the deviceID and vendor ID */
			ui32VenDevID=OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 0);
			
			/* Check if it is the required device */
			if (ui32VenDevID == (IMG_UINT32)((ui16DevID<<16)+ui16VenID))
			{
				IMG_UINT32 ui32Idx;
				IMG_BOOL   bLastBARWas64Bit = IMG_FALSE;
				
				/* Ensure Access to FB memory is enabled */
				OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, 4, OSPCIReadDword(ui32BusNum, ui32DevNum, 0, 4) | 0x02);
				
				/* Now copy PCI config space to users buffer */
				for (ui32Idx = 0; ui32Idx < 64; ui32Idx++)
				{
					IMG_UINT32 ui32RegValue = OSPCIReadDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4);
					psPCISpace->u.aui32PCISpace[ui32Idx] = ui32RegValue;
					
					if(!bLastBARWas64Bit && (ui32Idx >= PCI_BASEREG_OFFSET_DWORDS) && (ui32Idx < (PCI_BASEREG_OFFSET_DWORDS + 6)))
					{
						/*
							Read the 6 BAR registers..
						*/
						if (
								((ui32RegValue & 0x1) == 0x0) && // Memory BAR
								((((ui32RegValue >> 1) & 0x3) == 0x0))) // 32bit
						{
							/* Found a 32bit memory BAR. Attempt to disover the memory size.. */
							OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4, 0xFFFFFFFF);
							psPCISpace->aui32BARSize[ui32Idx - PCI_BASEREG_OFFSET_DWORDS] = ~OSPCIReadDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4) + 1;
							psPCISpace->aui32BAROffset[ui32Idx - PCI_BASEREG_OFFSET_DWORDS] = ui32RegValue & ~0xF;
							OSPCIWriteDword(ui32BusNum, ui32DevNum, 0, ui32Idx*4, ui32RegValue);
							
							bLastBARWas64Bit = IMG_FALSE;
						}
						else if (
								((ui32RegValue & 0x1) == 0x0) && // Memory BAR
								(((ui32RegValue >> 1) & 0x3) == 0x2) // 64bit
							)
						{
							/* Found a 64bit memory BAR. Ignore these for now. */
							PVR_DPF((PVR_DBG_ERROR,"Warning: 64bit BAR found."));
							bLastBARWas64Bit = IMG_TRUE;
						}
						else
						{
							// Ignore IO BARs and unwanted memory BARs..
							bLastBARWas64Bit = IMG_FALSE;
						}
					}
					else
					{
						bLastBARWas64Bit = IMG_FALSE;
					}
					
					if (ui32Idx < 16)
					{
						PVR_DPF((PVR_DBG_VERBOSE, "%08X\n", ui32RegValue));
					}
				}
				
				/* Request the memory size of the device */
				
				return PVRSRV_OK;
			}
			
		}/*End loop for each device*/
		
	}/*End loop for each bus*/
	
	PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));
	
	return PVRSRV_ERROR_PCI_DEVICE_NOT_FOUND;
}
#endif	/* __linux__ */

/*!
******************************************************************************

 @Function	SysLocateDevices

 @Description specifies devices in the systems memory map

 @Input    psSysData - sys data

 @Return   PVRSRV_ERROR  :

******************************************************************************/
static PVRSRV_ERROR SysLocateDevices(SYS_DATA *psSysData)
{
	IMG_CPU_PHYADDR	sRegRegion;
	IMG_UINT32		ui32FPGARegsPAddr, ui32SGXRegsPAddr, ui32SGXEMURegsPAddr, ui32SGXMemPAddr;
	IMG_VOID		*pvSGXRegsLinAddr, *pvSGXEMURegsLinAddr;
	IMG_UINT32		ui32IRQ;
	IMG_UINT32		ui32SGXMemSize = 0;
#if !defined(__linux__)
	IMG_UINT16 ui16DevIDs[] =
	{
		DEVICE_ID_EMULATOR,
		DEVICE_ID_EMULATOR_PCIE,
		DEVICE_ID_TEST_CHIP,
		DEVICE_ID_TEST_CHIP_PCIE
	};
	PVRSRV_ERROR	eError = PVRSRV_ERROR_INVALID_DEVICE;
	PCICONFIG_SPACE	sPCISpace = {0};
	IMG_UINT32 i;
#endif /* __linux__ */
	
	PVR_UNREFERENCED_PARAMETER(psSysData);
	
#if !defined(__linux__)
	for(i = 0; i < (sizeof(ui16DevIDs) / sizeof(IMG_UINT16)); i++)
	{
		eError = FindPCIDevice(VENDOR_ID_EMULATOR, ui16DevIDs[i], &sPCISpace);
		if (eError == PVRSRV_OK)
		{
			// Found a valid device so stop searching..
			break;
		}
	}
	
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"Couldn't find PCI device"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	
	if(i > 1)
	{
		bTestChipBoard = IMG_TRUE;
	}
#endif /* __linux__ */


#if defined(EMULATE_ATLAS_3BAR)
	/*
		Get the regions from base address register 0, 1, 2
	*/
#if defined(__linux__)
	ui32FPGARegsPAddr	= OSPCIAddrRangeStart(hSGXPCI, EMULATOR_ATLAS_REG_PCI_BASENUM);
	ui32SGXRegsPAddr	= OSPCIAddrRangeStart(hSGXPCI, EMULATOR_SGX_REG_PCI_BASENUM);
	ui32SGXMemPAddr		= OSPCIAddrRangeStart(hSGXPCI, EMULATOR_SGX_MEM_PCI_BASENUM);
#else
	ui32FPGARegsPAddr	= sPCISpace.aui32BAROffset[EMULATOR_ATLAS_REG_PCI_BASENUM];
	ui32SGXRegsPAddr	= sPCISpace.aui32BAROffset[EMULATOR_SGX_REG_PCI_BASENUM];
	ui32SGXMemPAddr		= sPCISpace.aui32BAROffset[EMULATOR_SGX_MEM_PCI_BASENUM];
	ui32SGXMemSize		= sPCISpace.aui32BARSize[EMULATOR_SGX_MEM_PCI_BASENUM];
#endif /* __linux__ */

#else
	/*
		Get the regions from base address register 0
	*/
#if defined(__linux__)
	ui32SGXRegsPAddr	= OSPCIAddrRangeStart(hSGXPCI, EMULATOR_SGX_PCI_BASENUM);
#else
	ui32SGXRegsPAddr	= sPCISpace.u.aui32PCISpace[EMULATOR_SGX_PCI_OFFSET];
#endif /* __linux__ */

	ui32SGXMemPAddr	= ui32SGXRegsPAddr + SGX_REG_REGION_SIZE;
	ui32FPGARegsPAddr = 0;

#endif /* EMULATE_ATLAS_3BAR */

	ui32SGXEMURegsPAddr = ui32SGXRegsPAddr + SGX_EMUREG_OFFSET;

	/*
		Get the IRQ.
	*/
#if defined(__linux__)
	if (OSPCIIRQ(hSGXPCI, &ui32IRQ) != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Couldn't get IRQ"));
		return PVRSRV_ERROR_INVALID_DEVICE;
	}
	PVR_TRACE(("IRQ: %d", ui32IRQ));
#else	/* __linux__ */
	ui32IRQ = (IMG_UINT32)sPCISpace.u.aui8PCISpace[0x3C];//FIXME: need defines in sysconfig.h

	PVR_DPF((PVR_DBG_ERROR,"Found - DevID %04X Ven Id 0x%04X SSDevId %04X SSVenId %04X", sPCISpace.u.aui16PCISpace[1], sPCISpace.u.aui16PCISpace[1], sPCISpace.u.aui16PCISpace[23], sPCISpace.u.aui16PCISpace[22]));
#endif	/* __linux__ */

	/*
		Set up the device map.
	*/

	/* Emulator registers */
	gsSOCRegsCpuPBase.uiAddr = ui32SGXEMURegsPAddr; // Should this be ui32FPGARegsPAddr?

	/* SGX Device: */
	gsSGXDeviceMap.ui32Flags = 0x0;

	/* Registers */
	gsSGXDeviceMap.sRegsSysPBase.uiAddr = ui32SGXRegsPAddr;
	gsSGXDeviceMap.sRegsCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sRegsSysPBase);
	gsSGXDeviceMap.ui32RegsSize = SGX_REG_SIZE;

	/* Local Device Memory Region: (present) */
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr = ui32SGXMemPAddr;
	gsSGXDeviceMap.sLocalMemDevPBase = SysSysPAddrToDevPAddr(PVRSRV_DEVICE_TYPE_SGX, gsSGXDeviceMap.sLocalMemSysPBase);
	gsSGXDeviceMap.sLocalMemCpuPBase = SysSysPAddrToCpuPAddr(gsSGXDeviceMap.sLocalMemSysPBase);
	
	if(ui32SGXMemSize == 0)
	{
		/*
			If we couldn't auto-discover the memory size then resort to the old
			method of using pre-defined sizes.
		*/
		gsSGXDeviceMap.ui32LocalMemSize = SYS_LOCALMEM_FOR_SGX_RESERVE_SIZE;
		PVR_DPF((PVR_DBG_ERROR, "Using hard-coded local memory size: 0x%08X (%d MB)", gsSGXDeviceMap.ui32LocalMemSize, gsSGXDeviceMap.ui32LocalMemSize / 1024 / 1024));
	}
	else
	{
		PVR_ASSERT(ui32SGXMemSize > SYS_LOCALMEM_SACRIFICE_SIZE);
		gsSGXDeviceMap.ui32LocalMemSize = ui32SGXMemSize - SYS_LOCALMEM_SACRIFICE_SIZE;
		PVR_DPF((PVR_DBG_ERROR, "Detected local memory size: 0x%08X (%d MB)", gsSGXDeviceMap.ui32LocalMemSize, gsSGXDeviceMap.ui32LocalMemSize / 1024 / 1024));
	}

#if defined(SGX_FEATURE_HOST_PORT)
	/* HostPort: */
	gsSGXDeviceMap.sHPSysPBase.uiAddr = 0;
	gsSGXDeviceMap.sHPCpuPBase.uiAddr = 0;
	gsSGXDeviceMap.ui32HPSize = 0;
#endif

	/* device interrupt IRQ */
	gsSGXDeviceMap.ui32IRQ = ui32IRQ;

#if defined(PDUMP)
	{
		/* initialise memory region name for pdumping */
		static IMG_CHAR pszPDumpDevName[] = "SGXMEM";
		gsSGXDeviceMap.pszPDumpDevName = pszPDumpDevName;
	}
#endif

	/*
		Map the SGX EMU registers.
	*/
	sRegRegion.uiAddr = ui32SGXEMURegsPAddr;
	pvSGXEMURegsLinAddr = OSMapPhysToLin(sRegRegion,
										 SGXEMU_REG_SIZE,
										 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
										 IMG_NULL);
	if (pvSGXEMURegsLinAddr == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Failed to map SGX EMU registers"));
		return 1;
	}

	/*
		Initialise SGX EMU memory configuration.
	*/
	OSWriteHWReg(pvSGXEMURegsLinAddr, 0x143C, 0x000200cb); // setup the memory configuration
	OSWriteHWReg(pvSGXEMURegsLinAddr, 0x1500, 0x00000001); // This is the bypass register, if set to 1 it will allow the host to access the memory directly without going through host_bif

	/*
		Map the SGX registers.
	*/
	sRegRegion.uiAddr = gsSGXDeviceMap.sRegsSysPBase.uiAddr;
	pvSGXRegsLinAddr = OSMapPhysToLin(sRegRegion,
									  SGX_REG_SIZE,
									  PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
									  IMG_NULL);
	if (pvSGXRegsLinAddr == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysLocateDevices: Failed to map SGX registers"));
		return 1;
	}

#ifdef SGX_FEATURE_HOST_PORT
	OSWriteHWReg(pvSGXRegsLinAddr, 0xC00, 0x00010000); // Set BIF control 'MMU_BYPASS_HOST' bit
#endif

	if (bTestChipBoard)
	{
		// new test chip board
		ui32AtlasRev = 110;

		#if defined(FORCE_RESET_SGX)
			/*
				The Atlas reset regs are defined as:

				register CLK_AND_RST_CTRL at 0x1C, split
					field sgx535_resetn [0 .. 0], default 1 endf
					field ddr_resetn    [1 .. 1], default 1 endf
					field pdp1_resetn   [2 .. 2], default 1 endf
					field pdp2_resetn   [3 .. 3], default 1 endf
					-- registers for clk gate control
					field clk_gate_cntl [15 .. 15], default 0 endf
					field glb_clkg_en   [16 .. 16], default 1 endf
				endr

				So to reset sgx without messing with the clk gates etc:
				write  0001000E  to assert reset
				then   0001000F  to release reset
			*/

			// Reset SGX, DDR/IF and PDP from Altlas
			OSWriteHWReg(pvSGXEMURegsLinAddr, 0x0080, 0x00020000); // assert reset
			OSWaitus(100);
			OSWriteHWReg(pvSGXEMURegsLinAddr, 0x0080, 0x0002000F); // release reset
			OSWaitus(1000);

		#endif//#if FORCE_RESET_SGX
	}

	/*
		Unmap the SGX registers.
	*/
	OSUnMapPhysToLin(pvSGXRegsLinAddr,
                     SGX_REG_SIZE,
                     PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
                     IMG_NULL);
	pvSGXRegsLinAddr = 0;

	/*
		Unmap the SGX EMU registers.
	*/
	OSUnMapPhysToLin(pvSGXEMURegsLinAddr,
	                 SGXEMU_REG_SIZE,
	                 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
	                 IMG_NULL);
	pvSGXEMURegsLinAddr = 0;

	#if defined(MEMTEST)

		eError = MemTest();
		return eError;
	#else

		return PVRSRV_OK;

	#endif//MEMTEST
}


#if defined(MEMTEST)
/*
	MemTest

	Tests memory on 4096 byte page boundries
	Maps no more than MEMTEST_MAP_SIZE into kernel space at any one time.

	Uses globals :
	gsSGXDeviceMap.sLocalMemSysPBase.uiAddr
	gsSGXDeviceMap.ui32LocalMemSize
*/

static PVRSRV_ERROR MemTest(IMG_VOID)
{
	IMG_VOID *pvLinAddr=0;
	IMG_CPU_PHYADDR sMemRegion;
	IMG_UINT32 ui32MemTestData=0;
	IMG_UINT32 *pui32PageAddr=0;
	IMG_UINT32 *pui32PageAddrMax;
	IMG_UINT32 ui32MapSize;
	IMG_UINT32 ui32Overrun;
	IMG_UINT32 ui32HighestDevicePhysAddr;
	IMG_UINT32 ui32HighestMappedPhysAddr;
	IMG_UINT32 ui32DataExpected = 0x00000000;
	PVRSRV_ERROR eError = PVRSRV_OK;

	sMemRegion.uiAddr = gsSGXDeviceMap.sLocalMemSysPBase.uiAddr;
	ui32HighestDevicePhysAddr = gsSGXDeviceMap.sLocalMemSysPBase.uiAddr + (gsSGXDeviceMap.ui32LocalMemSize-4);

	PVR_DPF((PVR_DBG_ERROR,"Testing SGX board memory from host"));

	while (sMemRegion.uiAddr <= ui32HighestDevicePhysAddr)
	{
		ui32MapSize = MEMTEST_MAP_SIZE;

		// Deal with the case where the PCI bios has put the device right at the end of physical space (unlikely)
		if (ui32HighestDevicePhysAddr == 0xfffffffc)
		{
			if (sMemRegion.uiAddr >= (0xfffffffc - ui32MapSize))
			{
				ui32MapSize = 0xfffffffc - sMemRegion.uiAddr;
			}
		}

		ui32HighestMappedPhysAddr = sMemRegion.uiAddr + ui32MapSize;

		// Check for overrun when the device memory size is not an exact multiple of MAP_SIZE
		if (ui32HighestMappedPhysAddr >= ui32HighestDevicePhysAddr)
		{
			ui32Overrun = ui32HighestMappedPhysAddr - ui32HighestDevicePhysAddr;

			if (ui32Overrun < ui32MapSize)
			{
				// Just test the remainder
				ui32MapSize -= ui32Overrun;
			}
			else
			{
				// No more left to test.
				break;
			}
		}

		// Map into kernel space
		pvLinAddr = OSMapPhysToLin( sMemRegion,
									ui32MapSize,
									PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
									IMG_NULL);

		if (!pvLinAddr)
		{
			eError = PVRSRV_ERROR_INIT_FAILURE;
			PVR_DBG_BREAK;
			break;
		}

		pui32PageAddr = (IMG_UINT32 *)pvLinAddr;
		pui32PageAddrMax = pui32PageAddr + (ui32MapSize>>2);

		while (pui32PageAddr < pui32PageAddrMax)
		{
			// Now do the memtest
			ui32DataExpected = 0xFFFFFFFF;
			*pui32PageAddr = ui32DataExpected;
			ui32MemTestData = *pui32PageAddr;

			if (ui32MemTestData != ui32DataExpected)
			{
				eError = PVRSRV_ERROR_INIT_FAILURE;
				break;
			}

			ui32DataExpected = 0x00000000;
			*pui32PageAddr = ui32DataExpected;
			ui32MemTestData = *pui32PageAddr;
			if (ui32MemTestData != ui32DataExpected)
			{
				eError = PVRSRV_ERROR_INIT_FAILURE;
				break;
			}

			pui32PageAddr += (4096>>2); // Next page
		}

		// Unmap
		OSUnMapPhysToLin(pvLinAddr,
						 ui32MapSize,
						 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
						 IMG_NULL);

		if (eError != PVRSRV_OK)
		{
			break;
		}

		// Next chunk
		sMemRegion.uiAddr += MEMTEST_MAP_SIZE;
	}

	if (eError == PVRSRV_OK)
	{
		// Memory is good
		PVR_DPF((PVR_DBG_ERROR,"*** %d MEG of SGX device mem Tested and OK ***", (gsSGXDeviceMap.ui32LocalMemSize+1)/1024/1024));
	}
	else
	{
		// Memory is bad.
		if (pvLinAddr)
		{
			PVR_DPF((PVR_DBG_ERROR,"Memtest failed at base + 0x%08X. Got %08X, expected %08X",
					(IMG_UINT32)pui32PageAddr - (IMG_UINT32)pvLinAddr, ui32MemTestData, ui32DataExpected));
		}
		PVR_DBG_BREAK;
	}

	return  eError;
}
#endif//#if MEMTEST



#if defined(SYS_USING_INTERRUPTS)

/*!
******************************************************************************

 @Function	SysEnableInterrupts

 @Description 	Enables system interrupts

 @Input    pointer to system specific data

 @Return   none

******************************************************************************/
static IMG_VOID SysEnableInterrupts(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32InterruptMask = OSReadHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_MASK);
	ui32InterruptMask &= ~(EMULATOR_INT_MASK_THALIA_MASK | EMULATOR_INT_MASK_MASTER_MASK);
	OSWriteHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_MASK, ui32InterruptMask);
}


/*!
******************************************************************************

 @Function	SysDisableInterrupts

 @Description 	Disables all system interrupts

 @Input    pointer to system specific data

 @Return   none

******************************************************************************/
static IMG_VOID SysDisableInterrupts(SYS_DATA *psSysData)
{
	IMG_UINT32 ui32InterruptMask = OSReadHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_MASK);
	ui32InterruptMask |= (EMULATOR_INT_MASK_THALIA_MASK | EMULATOR_INT_MASK_MASTER_MASK);
	OSWriteHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_MASK, ui32InterruptMask);
}

#endif /* defined(SYS_USING_INTERRUPTS) */

/*!
******************************************************************************

 @Function	SysInitialise

 @Description Initialises kernel services at 'driver load' time

 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysInitialise(IMG_VOID)
{
	IMG_UINT32			i;
	PVRSRV_ERROR 		eError;
	PVRSRV_DEVICE_NODE	*psDeviceNode;
	SGX_TIMING_INFORMATION*	psTimingInfo;

	gpsSysData = &gsSysData;
	OSMemSet(gpsSysData, 0, sizeof(SYS_DATA));

	eError = OSInitEnvData(&gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to setup env structure"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	gpsSysData->ui32NumDevices = SYS_DEVICE_COUNT;
	/* Set up timing information*/
	psTimingInfo = &gsSGXDeviceMap.sTimingInfo;
	psTimingInfo->ui32CoreClockSpeed = SYS_SGX_CLOCK_SPEED;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = 0;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;

#ifdef __linux__
	eError = PCIInitDev();
	if (eError != PVRSRV_OK)
	{
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}
#endif
	/* init device ID's */
	for(i=0; i<SYS_DEVICE_COUNT; i++)
	{
		gpsSysData->sDeviceID[i].uiID = i;
		gpsSysData->sDeviceID[i].bInUse = IMG_FALSE;
	}

	gpsSysData->psDeviceNodeList = IMG_NULL;
	gpsSysData->psQueueList = IMG_NULL;

	eError = SysInitialiseCommon(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed in SysInitialiseCommon"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Locate the devices within the system, specifying
		the physical addresses of each devices components
		(regs, mem, ports etc.)
	*/
	eError = SysLocateDevices(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to locate devices"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Register devices with the system
		This also sets up their memory maps/heaps
	*/
	eError = PVRSRVRegisterDevice(gpsSysData, SGXRegisterDevice,
								  DEVICE_SGX_INTERRUPT, &gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to register device!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		A given SOC may have 0 to n local device memory blocks.
		It is assumed device local memory blocks will be either
		specific to each device or shared among one or more services
		managed devices.
		Note: no provision is made for a single device having more
		than one local device memory blocks. If required we must
		add new types,
		e.g.PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG0
			PVRSRV_BACKINGSTORE_LOCALMEM_NONCONTIG1
	*/

	/*
		Create Local Device Page Managers for each device memory block.
		We'll use an RA to do the management
		(only one for the emulator)
	*/
	gpsSysData->apsLocalDevMemArena[0] = RA_Create ("EmulatorDeviceMemory",
													gsSGXDeviceMap.sLocalMemSysPBase.uiAddr,
													gsSGXDeviceMap.ui32LocalMemSize,
													IMG_NULL,
													HOST_PAGESIZE(),
													IMG_NULL,
													IMG_NULL,
													IMG_NULL,
													IMG_NULL);

	if (gpsSysData->apsLocalDevMemArena[0] == IMG_NULL)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to create local dev mem allocator!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	/*
		Once all devices are registered, specify the backing store
		and, if required, customise the memory heap config
	*/
	psDeviceNode = gpsSysData->psDeviceNodeList;
	while(psDeviceNode)
	{
		/* perform any OEM SOC address space customisations here */
		switch(psDeviceNode->sDevId.eDeviceType)
		{
			case PVRSRV_DEVICE_TYPE_SGX:
			{
				DEVICE_MEMORY_INFO *psDevMemoryInfo;
				DEVICE_MEMORY_HEAP_INFO *psDeviceMemoryHeap;

				/* specify the backing store to use for the devices MMU PT/PDs */
				psDeviceNode->psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];

				/* useful pointers */
				psDevMemoryInfo = &psDeviceNode->sDevMemoryInfo;
				psDeviceMemoryHeap = psDevMemoryInfo->psDeviceMemoryHeap;

				/* specify the backing store for all SGX heaps */
				for(i=0; i<psDevMemoryInfo->ui32HeapCount; i++)
				{
					/*
						specify the backing store type
						(all local device mem noncontig) for emulator
					*/
					psDeviceMemoryHeap[i].ui32Attribs |= PVRSRV_BACKINGSTORE_LOCALMEM_CONTIG;

					/*
						map the device memory allocator(s) onto
						the device memory heaps as required
					*/
					psDeviceMemoryHeap[i].psLocalDevMemArena = gpsSysData->apsLocalDevMemArena[0];

#ifdef OEM_CUSTOMISE
					/* if required, modify the memory config */
#endif
				}
				break;
			}
			default:
				break;
		}

		/* advance to next device */
		psDeviceNode = psDeviceNode->psNext;
	}

	/*
		Initialise all devices 'managed' by services:
	*/
	eError = PVRSRVInitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysInitialise: Failed to initialise device!"));
		(IMG_VOID)SysDeinitialise(gpsSysData);
		gpsSysData = IMG_NULL;
		return eError;
	}

	// Map emulator registers
	gpsSysData->pvSOCRegsBase = OSMapPhysToLin (gsSOCRegsCpuPBase,
												0x10000,
												PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
												IMG_NULL);

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysFinalise

 @Description Final part of initialisation


 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysFinalise(IMG_VOID)
{
	PVRSRV_ERROR eError;

	eError = OSInstallMISR(gpsSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallMISR: Failed to install MISR"));
		return eError;
	}

#if defined(SYS_USING_INTERRUPTS)
	/* install a system ISR */
	eError = OSInstallSystemLISR(gpsSysData, gsSGXDeviceMap.ui32IRQ);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"OSInstallSystemLISR: Failed to install ISR"));
		return eError;
	}
#endif /* defined(SYS_USING_INTERRUPTS) */

#if  defined(SYS_USING_INTERRUPTS)
	SysEnableInterrupts(gpsSysData);
#endif

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDeinitialise

 @Description De-initialises kernel services at 'driver unload' time


 @Return   PVRSRV_ERROR  :

******************************************************************************/
PVRSRV_ERROR SysDeinitialise (SYS_DATA *psSysData)
{
	PVRSRV_ERROR eError;

#if  defined(SYS_USING_INTERRUPTS)
	SysDisableInterrupts(psSysData);
#endif

	eError = OSUninstallMISR(psSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallMISR failed"));
		return eError;
	}

#if defined(SYS_USING_INTERRUPTS)
	eError = OSUninstallSystemLISR(psSysData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: OSUninstallSystemLISR failed"));
		return eError;
	}
#endif /* #if defined(SYS_USING_INTERRUPTS) */

	/* de-initialise all services managed devices */
	eError = PVRSRVDeinitialiseDevice (gui32SGXDeviceID);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init the device"));
		return eError;
	}

	if (gpsSysData->pvSOCRegsBase)
	{
		OSUnMapPhysToLin(gpsSysData->pvSOCRegsBase,
						 0x10000,
						 PVRSRV_HAP_UNCACHED|PVRSRV_HAP_KERNEL_ONLY,
						 IMG_NULL);
		gpsSysData->pvSOCRegsBase = IMG_NULL;
	}

	SysDeinitialiseCommon(gpsSysData);

	/*
		Destroy the local memory arena.
	*/
	RA_Delete(gpsSysData->apsLocalDevMemArena[0]);
	gpsSysData->apsLocalDevMemArena[0] = IMG_NULL;

#ifdef __linux__
	PCIDeInitDev();
#endif

	eError = OSDeInitEnvData(gpsSysData->pvEnvSpecificData);
	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR,"SysDeinitialise: failed to de-init env structure"));
		return eError;
	}

	gpsSysData = IMG_NULL;

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function      SysGetInterruptSource

 @Description 	Returns System specific information about the device(s) that
                generated the interrupt in the system

 @Input         psSysData
 @Input         psDeviceNode

 @Return        System specific information indicating which device(s)
                generated the interrupt

******************************************************************************/
IMG_UINT32 SysGetInterruptSource(SYS_DATA* psSysData,
								 PVRSRV_DEVICE_NODE *psDeviceNode)
{
	IMG_UINT32 u32InterruptBits;
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);

#if defined (SIM_EMULATOR_INTS)
	u32InterruptBits  = OSReadHWReg(psSysData->pvSOCRegsBase, EUR_CR_EVENT_STATUS);
	u32InterruptBits &= OSReadHWReg(psSysData->pvSOCRegsBase, EUR_CR_EVENT_HOST_ENABLE);

	return ((u32InterruptBits) ? DEVICE_SGX_INTERRUPT : 0);
#else
#if defined (PDP_EMULATOR_USING_INTERRUPTS)
#error PDP_EMULATOR_USING_INTERRUPTS not supported by sgx_emulator system config
#else
	u32InterruptBits  =   OSReadHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_STATUS);
	u32InterruptBits &= ~(OSReadHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_MASK));

	return ((u32InterruptBits) ? DEVICE_SGX_INTERRUPT : 0);
#endif /* PDP_EMULATOR_USING_INTERRUPTS */
#endif
}


/*!
******************************************************************************

 @Function      SysClearInterrupts

 @Description 	Clears specified system interrupts

 @Input         psSysData
 @Input         ui32InterruptBits

 @Return        none

******************************************************************************/
IMG_VOID SysClearInterrupts(SYS_DATA* psSysData, IMG_UINT32 ui32ClearBits)
{
#if defined(SIM_EMULATOR_INTS)
	PVR_UNREFERENCED_PARAMETER(psSysData);
	PVR_UNREFERENCED_PARAMETER(ui32ClearBits);
#else
	IMG_UINT32 ui32IntClear = 0;

	/* Map the generic interrupt flags to device specific flags */
	if (ui32ClearBits & DEVICE_SGX_INTERRUPT)
	{
		ui32IntClear |= EMULATOR_INT_CLEAR_THALIA_MASK;
	}

	OSWriteHWReg(psSysData->pvSOCRegsBase, EMULATOR_CR_INT_CLEAR, ui32IntClear);
#endif
}


/*!
******************************************************************************

 @Function      SysGetDeviceMemoryMap

 @Description	returns a device address map for the specified device

 @Input			eDeviceType - device type
 @Input			ppvDeviceMap - void ptr to receive device specific info.

 @Return   		PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysGetDeviceMemoryMap(PVRSRV_DEVICE_TYPE eDeviceType,
									IMG_VOID **ppvDeviceMap)
{

	switch(eDeviceType)
	{
		case PVRSRV_DEVICE_TYPE_SGX:
		{
			/* just return a pointer to the structure */
			*ppvDeviceMap = (IMG_VOID*)&gsSGXDeviceMap;
			break;
		}
		default:
		{
			PVR_DPF((PVR_DBG_ERROR,"SysGetDeviceMemoryMap: unsupported device type"));
		}
	}
	return PVRSRV_OK;
}

#if defined(PVR_LMA)
/*!
******************************************************************************

 @Function      SysVerifyCpuPAddrToDevPAddr

 @Description	Verify that it is valid to compute a device physical
                address from a given cpu physical address.

 @Input         cpu_paddr - cpu physical address.
 @Input         eDeviceType - device type required if DevPAddr
                    address spaces vary across devices in the same
                    system.

 @Return        IMG_TRUE or IMG_FALSE

******************************************************************************/
IMG_BOOL SysVerifyCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType,
										IMG_CPU_PHYADDR CpuPAddr)
{
	/*
		INTEGRATION_POINT:
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in
		the adapter)
	*/
	return ((CpuPAddr.uiAddr >= gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr) &&
		(CpuPAddr.uiAddr < gsSGXDeviceMap.sLocalMemCpuPBase.uiAddr + EMULATOR_SGX_MEM_REGION_SIZE))
		 ? IMG_TRUE : IMG_FALSE;
}
#endif

/*!
******************************************************************************

 @Function      SysCpuPAddrToDevPAddr

 @Description	Compute a device physical address from a cpu physical
                address. Relevant when

 @Input         cpu_paddr - cpu physical address.
 @Input         eDeviceType - device type required if DevPAddr
                    address spaces vary across devices
                    in the same system

 @Return        device physical address.

******************************************************************************/
IMG_DEV_PHYADDR SysCpuPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_CPU_PHYADDR CpuPAddr)
{
	/*
		INTEGRATION_POINT:
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the adapter)
	*/
	DevPAddr.uiAddr = CpuPAddr.uiAddr - gsSGXDeviceMap.sLocalMemSysPBase.uiAddr;

	return DevPAddr;
}

/*!
******************************************************************************

 @Function      SysSysPAddrToCpuPAddr

 @Description	Compute a cpu physical address from a system physical
                address.

 @Input         sys_paddr - system physical address.

 @Return        cpu physical address.

******************************************************************************/
IMG_CPU_PHYADDR SysSysPAddrToCpuPAddr (IMG_SYS_PHYADDR sys_paddr)
{
	IMG_CPU_PHYADDR cpu_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	cpu_paddr.uiAddr = sys_paddr.uiAddr;

	return cpu_paddr;
}

/*!
******************************************************************************

 @Function      SysCpuPAddrToSysPAddr

 @Description	Compute a system physical address from a cpu physical
                address.

 @Input         cpu_paddr - cpu physical address.

 @Return        device physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysCpuPAddrToSysPAddr (IMG_CPU_PHYADDR cpu_paddr)
{
	IMG_SYS_PHYADDR sys_paddr;

	/* This would only be an inequality if the CPU's MMU did not point to sys address 0,
	   ie. multi CPU system */
	sys_paddr.uiAddr = cpu_paddr.uiAddr;

	return sys_paddr;
}

#if defined(PVR_LMA)
/*!
******************************************************************************

 @Function      SysVerifySysPAddrToDevPAddr

 @Description	Verify that a device physical address can be obtained
                from a given system physical address.

 @Input         SysPAddr - system physical address.
 @Input         eDeviceType - device type required if DevPAddr
                    address spaces vary across devices in the same system.

 @Return        IMG_TRUE or IMG_FALSE

******************************************************************************/
IMG_BOOL SysVerifySysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	/*
		FIXME:
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the
		adapter)
	*/

	return ((SysPAddr.uiAddr >= gsSGXDeviceMap.sLocalMemSysPBase.uiAddr) &&
		(SysPAddr.uiAddr < gsSGXDeviceMap.sLocalMemSysPBase.uiAddr + EMULATOR_SGX_MEM_REGION_SIZE))
		  ? IMG_TRUE : IMG_FALSE;
}
#endif

/*!
******************************************************************************

 @Function      SysSysPAddrToDevPAddr

 @Description	Compute a device physical address from a system physical
                address.

 @Input         SysPAddr - system physical address.
 @Input         eDeviceType - device type required if DevPAddr
                    address spaces vary across devices
                    in the same system

 @Return        Device physical address.

******************************************************************************/
IMG_DEV_PHYADDR SysSysPAddrToDevPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_SYS_PHYADDR SysPAddr)
{
	/*
		INTEGRATION_POINT:
		It is assumed the translation from cpu/system to device
		physical addresses is constant for every 'services managed'
		device in a given system
	*/
    IMG_DEV_PHYADDR DevPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the adapter)
	*/
    DevPAddr.uiAddr = SysPAddr.uiAddr - gsSGXDeviceMap.sLocalMemSysPBase.uiAddr;

    return DevPAddr;
}


/*!
******************************************************************************

 @Function      SysDevPAddrToSysPAddr

 @Description	Compute a device physical address from a system physical
                address.

 @Input         DevPAddr - device physical address.
 @Input         eDeviceType - device type required if DevPAddr
                    address spaces vary across devices
                    in the same system

 @Return        System physical address.

******************************************************************************/
IMG_SYS_PHYADDR SysDevPAddrToSysPAddr (PVRSRV_DEVICE_TYPE eDeviceType, IMG_DEV_PHYADDR DevPAddr)
{
    IMG_SYS_PHYADDR SysPAddr;

	PVR_UNREFERENCED_PARAMETER(eDeviceType);

	/*
		just dip into the SGX info here
		(it's really adapter memory shared across all devices in the adapter)
	*/
    SysPAddr.uiAddr = DevPAddr.uiAddr + gsSGXDeviceMap.sLocalMemSysPBase.uiAddr;

    return SysPAddr;
}


/*****************************************************************************

 @Function      SysRegisterExternalDevice

 @Description   Called when a 3rd party device registers with services

 @Input	        psDeviceNode - the new device node.

 @Return        none

*****************************************************************************/
IMG_VOID SysRegisterExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*****************************************************************************

 @Function      SysRemoveExternalDevice

 @Description	Called when a 3rd party device unregisters from services

 @Input	        psDeviceNode - the device node being removed.

 @Return    	none

*****************************************************************************/
IMG_VOID SysRemoveExternalDevice(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	PVR_UNREFERENCED_PARAMETER(psDeviceNode);
}


/*!
******************************************************************************

 @Function	SysPrePowerState

 @Description	Perform system-level processing required before a power transition

 @Input	   eNewPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysSystemPrePowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysPostPowerState

 @Description	Perform system-level processing required after a power transition

 @Input	   eNewPowerState :

 @Return   PVRSRV_ERROR :

******************************************************************************/
PVRSRV_ERROR SysSystemPostPowerState(PVRSRV_SYS_POWER_STATE eNewPowerState)
{
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePrePowerState

 @Description	Perform system-level processing required before a device power
 				transition

 @Input		ui32DeviceIndex :
 @Input		eNewPowerState :
 @Input		eCurrentPowerState :

 @Return	PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDevicePrePowerState(IMG_UINT32				ui32DeviceIndex,
									PVRSRV_DEV_POWER_STATE	eNewPowerState,
									PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(ui32DeviceIndex);
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	return PVRSRV_OK;
}


/*!
******************************************************************************

 @Function	SysDevicePostPowerState

 @Description	Perform system-level processing required after a device power
 				transition

 @Input		ui32DeviceIndex :
 @Input		eNewPowerState :
 @Input		eCurrentPowerState :

 @Return    PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR SysDevicePostPowerState(IMG_UINT32				ui32DeviceIndex,
									 PVRSRV_DEV_POWER_STATE	eNewPowerState,
									 PVRSRV_DEV_POWER_STATE	eCurrentPowerState)
{
	PVR_UNREFERENCED_PARAMETER(ui32DeviceIndex);
	PVR_UNREFERENCED_PARAMETER(eNewPowerState);
	PVR_UNREFERENCED_PARAMETER(eCurrentPowerState);

	return PVRSRV_OK;
}


/*****************************************************************************
 @Function      SysOEMFunction

 @Description	marshalling function for custom OEM functions

 @Input	        ui32ID  - function ID
 @Input	        pvIn - in data
                pvOut - out data

 @Return        PVRSRV_ERROR
*****************************************************************************/
PVRSRV_ERROR SysOEMFunction (	IMG_UINT32	ui32ID,
								IMG_VOID	*pvIn,
								IMG_UINT32  ulInSize,
								IMG_VOID	*pvOut,
								IMG_UINT32	ulOutSize)
{
	PVR_UNREFERENCED_PARAMETER(ulInSize);
	PVR_UNREFERENCED_PARAMETER(pvIn);

	if ((ui32ID == OEM_GET_EXT_FUNCS) &&
		(ulOutSize == sizeof(PVRSRV_DC_OEM_JTABLE)))
	{
		PVRSRV_DC_OEM_JTABLE *psOEMJTable = (PVRSRV_DC_OEM_JTABLE*)pvOut;
		psOEMJTable->pfnOEMBridgeDispatch = &PVRSRV_BridgeDispatchKM;
		return PVRSRV_OK;
	}

	return PVRSRV_ERROR_INVALID_PARAMS;
}

