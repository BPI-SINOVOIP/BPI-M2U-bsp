########################################################################### ###
#@Title         Root makefile for SUNXI Linux.  Builds everything else.
#@Copyright     Copyright (c) Imagination Technologies Ltd. All Rights Reserved
#@License       Dual MIT/GPLv2
# 
# The contents of this file are subject to the MIT license as set out below.
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# Alternatively, the contents of this file may be used under the terms of
# the GNU General Public License Version 2 ("GPL") in which case the provisions
# of GPL are applicable instead of those above.
# 
# If you wish to allow use of your version of this file only under the terms of
# GPL, and not to allow others to use your version of this file under the terms
# of the MIT license, indicate your decision by deleting the provisions above
# and replace them with the notice and other provisions required by GPL as set
# out in the file called "GPL-COPYING" included in this distribution. If you do
# not delete the provisions above, a recipient may use your version of this file
# under the terms of either the MIT license or GPL.
# 
# This License is also included in this distribution in the file called
# "MIT-COPYING".
# 
# EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
# PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
# BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
### ###########################################################################

BUILD ?= release
export ARCH = arm
export CROSS_COMPILE ?= arm-linux-gnueabi-

NO_HARDWARE := 0
SGXCORE := 544
SGX_CORE_REV := 115
SUPPORT_SLC := 1
SGX_FEATURE_MP := 1
SGX_FEATURE_MP_CORE_COUNT := 1
SGX_FEATURE_SYSTEM_CACHE := 1
SUPPORT_ACTIVE_POWER_MANAGEMENT := 1
PVR_SYSTEM := sunxi
KERNEL_COMPONENTS := srvkm

LDM_PLATFORM ?= 1
SUPPORT_LINUX_USING_WORKQUEUES := 1

DISPLAY_CONTROLLER_COMPONENT += dc_sunxi

DISPLAY_CONTROLLER := dc_sunxi

SUPPORT_ION = 0

LIBGCC := $(shell $(CROSS_COMPILE)gcc -print-libgcc-file-name)

SGX_DYNAMIC_TIMING_INFO := 1
SYS_CUSTOM_POWERLOCK_WRAP := 1

OPK_DEFAULT := libpvrPVR2D_FLIPWSEGL.so

ifeq ($(SUPPORT_BUILD_XORG),1)
SUPPORT_DRI_DRM := 1
endif

PVR_DRI_DRM_DEV_BUS_ID := "\"platform:pvrsrvkm"\"

XORG_PVR_VIDEO ?= sunxi

ifeq ($(SUPPORT_DRI_DRM),1)
OPK_FALLBACK := libpvrPVR2D_DRIWSEGL.so
else
OPK_FALLBACK := libpvrPVR2D_BLITWSEGL.so
endif

KERNEL_COMPONENTS += $(DISPLAY_CONTROLLER_COMPONENT)

include ../config/core.mk
include ../common/xorg.mk
include ../common/dridrm.mk
include ../common/opencl.mk

$(eval $(call UserConfigMake,DISPLAY_CONTROLLER_DIR,3rdparty/$(DISPLAY_CONTROLLER_COMPONENT)))
