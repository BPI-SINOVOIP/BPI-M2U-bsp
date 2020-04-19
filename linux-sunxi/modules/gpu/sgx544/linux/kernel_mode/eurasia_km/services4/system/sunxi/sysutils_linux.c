/*************************************************************************/ /*!
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
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
#include <aw/platform.h>

int GpuThermalCool(int level);

#ifdef CONFIG_SUNXI_GPU_COOLING
extern int gpu_thermal_cool_register(int (*cool) (int));
extern int gpu_thermal_cool_unregister(void);
#endif /* CONFIG_SUNXI_GPU_COOLING */

#ifdef CONFIG_CPU_BUDGET_THERMAL
extern int ths_read_data(int value);
#endif /* CONFIG_CPU_BUDGET_THERMAL */

IMG_VOID AwDeInit(IMG_VOID);

extern struct platform_device *gpsPVRLDMDev;

static const char pvr_dev_name[] = "pvr";

extern unsigned int sunxi_get_soc_bin(void);

static int Min(int a, int b)
{
	return a < b ? a : b;
}

#ifdef CONFIG_CPU_BUDGET_THERMAL
static long GetTemperature(IMG_VOID)
{
    return ths_read_data(aw_private.sensor_num);
}
#endif /* CONFIG_CPU_BUDGET_THERMAL */

static IMG_VOID AssertResetSignal(struct aw_clk_data clk_data)
{
	if (!clk_data.need_reset)
		return;

	if (sunxi_periph_reset_assert(clk_data.clk_handle))
		PVR_DPF((PVR_DBG_ERROR, "Failed to pull down gpu %s clock reset!",
				clk_data.clk_name));
	else
		PVR_LOG(("Pull down gpu %s clock reset successfully.",
				clk_data.clk_name));
}

static IMG_VOID DeAssertResetSignal(struct aw_clk_data clk_data)
{
	if (!clk_data.need_reset)
		return;

	if (sunxi_periph_reset_deassert(clk_data.clk_handle))
		PVR_DPF((PVR_DBG_ERROR, "Failed to release gpu %s clock reset!",
					clk_data.clk_name));
	else
		PVR_LOG(("Release gpu %s clock reset successfully.",
				clk_data.clk_name));
}

static IMG_VOID AssertGpuResetSignal(IMG_VOID)
{
	int i;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 0; i < 4; i++)
		AssertResetSignal(clk_data[i]);
}

static IMG_VOID DeAssertGpuResetSignal(IMG_VOID)
{
	int i;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 3; i >= 0; i--)
		DeAssertResetSignal(clk_data[i]);
}

static IMG_VOID GetGpuRegulator(IMG_VOID)
{
	struct regulator *regulator;
	char *regulator_id = aw_private.pm.regulator_id;

	if (NULL == regulator_id)
		return;

	regulator = regulator_get(NULL, regulator_id);
	if (IS_ERR_OR_NULL(regulator)) {
		PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu regulator!"));
		return;
	}

	PVR_LOG(("Get gpu regulator successfully."));
	if (regulator_enable(regulator))
		PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu external power!"));
	else
		PVR_LOG(("Enable gpu external power successfully."));

	aw_private.pm.regulator = regulator;
}

static IMG_VOID EnableGpuPower(IMG_VOID)
{
	struct regulator *regulator = aw_private.pm.regulator;

	if (IS_ERR_OR_NULL(regulator))
		return;

	if (regulator_is_enabled(regulator))
		return;

	if (regulator_enable(regulator)) {
		PVR_DPF((PVR_DBG_ERROR, "Failed to enable gpu external power!"));
	} else {
		PVR_LOG(("Enable gpu external power successfully."));

		/* Delay for gpu power stability */
		mdelay(2);
	}
}

static IMG_VOID DisableGpuPower(IMG_VOID)
{
	struct regulator *regulator = aw_private.pm.regulator;
	if (IS_ERR_OR_NULL(regulator))
		return;

	if (!regulator_is_enabled(regulator))
		return;

	if (regulator_disable(regulator))
		PVR_DPF((PVR_DBG_ERROR, "Failed to disable gpu external power!"));
	else
		PVR_LOG(("Disable gpu external power successfully."));
}

static int GetCurrentVol(IMG_VOID)
{
	struct regulator *regulator = aw_private.pm.regulator;

    return regulator_get_voltage(regulator)/1000; /* mV */
}

static IMG_VOID SetGpuVol(int vol /* mV */)
{
	struct regulator *regulator = aw_private.pm.regulator;
	int current_vol = aw_private.pm.current_vol;

	if (IS_ERR_OR_NULL(regulator))
		return;

	if (vol > 1300 || vol <= 0)
		return;

	if (vol == current_vol)
		return;

	if (regulator_set_voltage(regulator, vol*1000, vol*1000) != 0) {
		PVR_DPF((PVR_DBG_ERROR, "Failed to set gpu power voltage to %d mV!", vol));
	} else {
		PVR_LOG(("The voltage has been set to %d mV", vol));

		/* Delay for gpu voltage stability */
		mdelay(2);

		current_vol = GetCurrentVol();
		aw_private.pm.current_vol = current_vol;
	}
}

static IMG_VOID GetGpuClk(IMG_VOID)
{
	int i;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 0; i < 4; i++) {
#ifdef CONFIG_OF
		clk_data[i].clk_handle = of_clk_get(aw_private.np_gpu, i);
#else
		clk_data[i].clk_handle = clk_get(NULL, clk_data[i].clk_id);
#endif /* CONFIG_OF */

		if (IS_ERR_OR_NULL(clk_data[i].clk_handle))
			PVR_DPF((PVR_DBG_ERROR, "Failed to get gpu %s clock id!",
					clk_data[i].clk_name));
		else
			PVR_LOG(("Get gpu %s clock id successfully.",
					clk_data[i].clk_name));
	}
}

static IMG_VOID EnableGpuClk(IMG_VOID)
{
	int i;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 0; i < 4; i++)
		if (clk_prepare_enable(clk_data[i].clk_handle)) {
			PVR_DPF((PVR_DBG_ERROR, "Failed to "
					"enable gpu %s clock!\n",
					clk_data[i].clk_name));
		}
}

static IMG_VOID DisableGpuClk(IMG_VOID)
{
	int i;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 3; i >= 0; i--)
		clk_disable_unprepare(clk_data[i].clk_handle);
}

static IMG_VOID SetGpuClkParent(IMG_VOID)
{
	int i, j;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 0; i < 4; i++) {
		if (NULL == clk_data[i].clk_parent_name)
			continue;

		for (j = 0; j < 4; j++) {
			if (clk_data[i].clk_parent_name != clk_data[j].clk_name)
				continue;

			if (IS_ERR_OR_NULL(clk_data[i].clk_handle))
				continue;

			if (IS_ERR_OR_NULL(clk_data[j].clk_handle))
				continue;

			if (clk_set_parent(clk_data[i].clk_handle,
							clk_data[j].clk_handle))
				PVR_DPF((PVR_DBG_ERROR, "Failed to set the parent of gpu %s clock!",
							clk_data[i].clk_name));
			else
				PVR_LOG(("Set the parent of gpu %s clock to %s.",
						clk_data[i].clk_name,
						clk_data[i].clk_parent_name));
		}
	}
}

static long int GetCurrentFreq(struct clk *clk_handle)
{
    return clk_get_rate(clk_handle)/(1000 * 1000); /* MHz */
}

static IMG_VOID SetClkVal(int freq /* MHz */)
{
	int i;
	struct aw_clk_data *clk_data = aw_private.pm.clk;

	for (i = 0; i < 4; i++) {
		if (IS_ERR_OR_NULL(clk_data[i].clk_handle))
			continue;

		if (clk_data[i].freq_type == 0)
			continue;
		else if (clk_data[i].freq_type > 1)
			freq = clk_data[i].freq_type;

		if (clk_set_rate(clk_data[i].clk_handle, freq*1000*1000)) {
			PVR_DPF((PVR_DBG_ERROR, "Failed to set the frequency of gpu %s clock to %d MHz!",
						clk_data[i].clk_name, freq));
		} else {
			clk_data[i].freq = GetCurrentFreq(clk_data[i].clk_handle);
			PVR_LOG(("The frequency of gpu %s clock has been set to %d MHz",
					clk_data[i].clk_name, freq));
		}
	}
}

static IMG_VOID SetGpuClkVal(int freq /* MHz */)
{
	PVRSRV_ERROR err;
	int current_freq = aw_private.pm.clk[0].freq;

	if (freq == current_freq)
		return;

	mutex_lock(&aw_private.pm.dvfs_lock);
	err = PVRSRVDevicePreClockSpeedChange(0, IMG_TRUE, NULL);

	if (err != PVRSRV_OK)
		return;

	SetClkVal(freq);

	PVRSRVDevicePostClockSpeedChange(0, IMG_TRUE, NULL);
	mutex_unlock(&aw_private.pm.dvfs_lock);
}

static IMG_VOID DvfsChange(int level)
{
	int freq = aw_private.pm.vf_table[level].freq;
	int vol  = aw_private.pm.vf_table[level].vol;
	int current_level = aw_private.pm.current_level;

	if (level == current_level)
		return;

	if (freq < aw_private.pm.clk[0].freq) {
		SetGpuClkVal(freq);
		SetGpuVol(vol);
	} else {
		SetGpuVol(vol);
		SetGpuClkVal(freq);
	}

	aw_private.pm.current_level = level;

	PVR_LOG(("The current level has been set to %d.", level));
}

int GetIrqNum(int irq_num)
{
	irq_num = aw_private.irq_num;
	return irq_num;
}

static IMG_VOID SetBit(unsigned char bit,
						unsigned int val,
						phys_addr_t phys_addr)
{
	unsigned char reg_data;
	void __iomem *ioaddr = ioremap(phys_addr, 1);

	reg_data = sunxi_smc_readl(ioaddr);
	if (val == 0)
		reg_data &= ~(1 << bit);
	else if (val == 1)
		reg_data |= (1 << bit);
	else
		return;

	sunxi_smc_writel(reg_data, ioaddr);
}

#ifdef CONFIG_CPU_BUDGET_THERMAL
static int gpu_throttle_notifier_call(struct notifier_block *nfb, unsigned long mode, void *cmd)
{
	long temperature;
	int retval = NOTIFY_DONE, i = 0;
	int begin_level = aw_private.pm.begin_level;
	int level_to_be = begin_level;
	struct aw_tl_table *tl_table = aw_private.tempctrl.tl_table;

	if (!aw_private.tempctrl.temp_ctrl_status)
		return retval;

	temperature = GetTemperature();
	if (temperature <= tl_table[0].temp)
		level_to_be = begin_level;

	for (i = aw_private.tempctrl.count - 1; i >= 0; i--) {
		if (temperature >= tl_table[i].temp) {
			level_to_be = tl_table[i].level;
			break;
		}
	}

	GpuThermalCool(level_to_be);

	return retval;
}

static struct notifier_block gpu_throttle_notifier = {
.notifier_call = gpu_throttle_notifier_call,
};
#endif /* CONFIG_CPU_BUDGET_THERMAL */

static PVRSRV_ERROR PowerLockWrap(SYS_SPECIFIC_DATA *psSysSpecData,
							IMG_BOOL bTryLock)
{
	if (!in_interrupt()) {
		if (bTryLock) {
			int locked = mutex_trylock(&psSysSpecData->sPowerLock);
			if (locked == 0)
				return PVRSRV_ERROR_RETRY;
		} else {
			mutex_lock(&psSysSpecData->sPowerLock);
		}
	}

	return PVRSRV_OK;
}

static IMG_VOID PowerLockUnwrap(SYS_SPECIFIC_DATA *psSysSpecData)
{
	if (!in_interrupt())
		mutex_unlock(&psSysSpecData->sPowerLock);
}

PVRSRV_ERROR SysPowerLockWrap(IMG_BOOL bTryLock)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	return PowerLockWrap(psSysData->pvSysSpecificData, bTryLock);
}

IMG_VOID SysPowerLockUnwrap(IMG_VOID)
{
	SYS_DATA	*psSysData;

	SysAcquireData(&psSysData);

	PowerLockUnwrap(psSysData->pvSysSpecificData);
}

/*
 * This function should be called to unwrap the Services power lock, prior
 * to calling any function that might sleep.
 * This function shouldn't be called prior to calling EnableSystemClocks
 * or DisableSystemClocks, as those functions perform their own power lock
 * unwrapping.
 * If the function returns IMG_TRUE, UnwrapSystemPowerChange must be
 * called to rewrap the power lock, prior to returning to Services.
 */
IMG_BOOL WrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	return IMG_TRUE;
}

IMG_VOID UnwrapSystemPowerChange(SYS_SPECIFIC_DATA *psSysSpecData)
{
	
}

/*
 * Return SGX timining information to caller.
 */
IMG_VOID SysGetSGXTimingInformation(SGX_TIMING_INFORMATION *psTimingInfo)
{
	int freq = aw_private.pm.clk[0].freq * 1000 * 1000;
#if !defined(NO_HARDWARE)
	PVR_ASSERT(atomic_read(&gpsSysSpecificData->sSGXClocksEnabled) != 0);
#endif
	psTimingInfo->ui32CoreClockSpeed = freq;
	psTimingInfo->ui32HWRecoveryFreq = SYS_SGX_HWRECOVERY_TIMEOUT_FREQ;
	psTimingInfo->ui32uKernelFreq = SYS_SGX_PDS_TIMER_FREQ;
#if defined(SUPPORT_ACTIVE_POWER_MANAGEMENT)
	psTimingInfo->bEnableActivePM = IMG_TRUE;
#else
	psTimingInfo->bEnableActivePM = IMG_FALSE;
#endif /* SUPPORT_ACTIVE_POWER_MANAGEMENT */
	psTimingInfo->ui32ActivePowManLatencyms = SYS_SGX_ACTIVE_POWER_LATENCY_MS;
}

/*!
******************************************************************************

 @Function  EnableSGXClocks

 @Description Enable SGX clocks

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSGXClocks(SYS_DATA *psSysData, IMG_BOOL bNoDev)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData;

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already enabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) != 0)
		return PVRSRV_OK;

	PVR_DPF((PVR_DBG_MESSAGE, "EnableSGXClocks: Enabling SGX Clocks"));

	EnableGpuClk();

	SysEnableSGXInterrupts(psSysData);

	/* Indicate that the SGX clocks are enabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 1);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function  DisableSGXClocks

 @Description Disable SGX clocks.

 @Return   none

******************************************************************************/
IMG_VOID DisableSGXClocks(SYS_DATA *psSysData)
{
#if !defined(NO_HARDWARE)
	SYS_SPECIFIC_DATA *psSysSpecData;

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	/* SGX clocks already disabled? */
	if (atomic_read(&psSysSpecData->sSGXClocksEnabled) == 0)
		return;

	PVR_DPF((PVR_DBG_MESSAGE, "DisableSGXClocks: Disabling SGX Clocks"));

	SysDisableSGXInterrupts(psSysData);

	DisableGpuClk();

	/* Indicate that the SGX clocks are disabled */
	atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

#else	/* !defined(NO_HARDWARE) */
	PVR_UNREFERENCED_PARAMETER(psSysData);
#endif	/* !defined(NO_HARDWARE) */
}

static int GetU32FromFex(char *main_key, char *sub_key)
{
	u32 value;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
	script_item_u val;
	script_item_value_type_e type;

	type = script_get_item(main_key, sub_key, &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type)
		goto err_out;

	value = val.val;
#else
	if (of_property_read_u32(aw_private.np_gpu, sub_key, &value))
		goto err_out;
#endif

	return value;

err_out:
	PVR_DPF((PVR_DBG_ERROR, "Failed to get [%s, %s] in sys_config.fex!\n",
					main_key, sub_key));
	return -1;
}

static IMG_VOID ParseFex(IMG_VOID)
{
	int val, i;
	char main_key[15] = {0};
	char sub_key[11] = {0};

	sprintf(main_key, "%s%d", "gpu_sgx544_", sunxi_get_soc_bin());

	val = GetU32FromFex(main_key, "dvfs_status");
	if (val == 0 || val == 1)
		aw_private.pm.dvfs_status = val;

	val = GetU32FromFex(main_key, "temp_ctrl_status");
	if (val == 0 || val == 1)
		aw_private.tempctrl.temp_ctrl_status = val;

	val = GetU32FromFex(main_key, "scene_ctrl_status");
	if (val == 0 || val == 1)
		aw_private.pm.scene_ctrl_status = val;

	val = GetU32FromFex(main_key, "max_level");
	if (val >= 0)
		aw_private.pm.max_level = val;

	val = GetU32FromFex(main_key, "begin_level");
	if (val >= 0 && val <= aw_private.pm.max_level)
			aw_private.pm.begin_level = val;

	for (i = 0; i <= aw_private.pm.max_level; i++) {
		sprintf(sub_key, "lv%d_freq", i);
		val = GetU32FromFex(main_key, sub_key);
		if (val > 0)
			aw_private.pm.vf_table[i].freq = val;

		sprintf(sub_key, "lv%d_volt", i);
		val = GetU32FromFex(main_key, sub_key);
		if (val > 0)
			aw_private.pm.vf_table[i].vol = val;
	}

#ifdef CONFIG_CPU_BUDGET_THERMAL
	val = GetU32FromFex(main_key, "tlt_count");
	if (val > 0) {
		if (val < aw_private.tempctrl.count)
			aw_private.tempctrl.count = val;

	for (i = 0; i < aw_private.tempctrl.count; i++) {
		sprintf(sub_key, "tl%d_temp", i);
		val = GetU32FromFex(main_key, sub_key);
		if (val > 0 )
			aw_private.tempctrl.tl_table[i].temp = val;
		else
			break;

		sprintf(sub_key, "tl%d_level", i);
		val = GetU32FromFex(main_key, sub_key);
		if (val >= 0)
			aw_private.tempctrl.tl_table[i].level = val;
		else
			break;
        }

	if (i < aw_private.tempctrl.count)
		aw_private.tempctrl.count = i;
	}
#endif /* CONFIG_CPU_BUDGET_THERMAL */
}

static int GetValue(char *cmd, char *buf, int cmd_size, int total_size)
{
	char data[5];
	unsigned long val = -1;
	int i, j;

	if (!strncmp(buf, cmd, cmd_size)) {
		for (i = 0; i < total_size; i++) {
			if (*(buf+i) == ':') {
				for (j = 0; j < total_size - i - 1; j++)
					data[j] = *(buf + i + 1 + j);
				data[j] = '\0';
				break;
			}
		}

		if (strict_strtoul(data, 10, &val)) {
			PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!\n"));
		}
	}

	return val;
}

static IMG_VOID ReviseCurrentLevel(IMG_VOID)
{
	int i, freq, vol;
	int max_level = aw_private.pm.max_level;
	int current_vol = aw_private.pm.current_vol;
	struct regulator *regulator = aw_private.pm.regulator;

	aw_private.pm.current_level = -1;
	for (i = 0; i <= max_level; i++) {
		freq = aw_private.pm.vf_table[i].freq;
		vol = aw_private.pm.vf_table[i].vol;

		if (aw_private.pm.clk[0].freq != freq)
			continue;

		if (IS_ERR_OR_NULL(regulator) || current_vol == vol) {
			aw_private.pm.current_level = i;
			break;
		}
	}
}

static ssize_t WriteWrite(struct file *filp,
						const char __user *buf,
						size_t count,
						loff_t *offp)
{
	int val, i, token = 0;
	char buffer[100];

	if (count >= sizeof(buffer))
		return -ENOMEM;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	buffer[count] = '\0';

	for (i = 0; i < count; i++) {
		if (*(buffer+i) == ';') {
			val = GetValue("enable",
							buffer + token,
							6,
							i - token);
			if (val == 0 || val == 1) {
				aw_private.debug.enable = val;
				break;
			}

			val = GetValue("frequency",
							buffer + token,
							9,
							i - token);
			if (val >= 0) {
				if (val == 0 || val == 1) {
					aw_private.debug.frequency = val;
				} else {
					SetGpuClkVal(val);

					ReviseCurrentLevel();
				}
				break;
			}

			val = GetValue("voltage",
							buffer + token,
							7,
							i - token);
			if (val >= 0) {
				if (val == 0 || val == 1)
					aw_private.debug.voltage = val;
				else
					SetGpuVol(val);
				break;
			}

			val = GetValue("tempctrl",
							buffer + token,
							8,
							i - token);
			if (val >= 0 && val <= 3) {
				if (val == 0 || val == 1)
					aw_private.debug.tempctrl = val;
				else
					aw_private.tempctrl.temp_ctrl_status = val - 2;
				break;
			}

			val = GetValue("dvfs",
							buffer + token,
							4,
							i - token);
			if (val >= 0 && val <= 3) {
				if (val == 0 || val == 1)
					aw_private.debug.dvfs = val;
				else
					aw_private.pm.dvfs_status = val - 2;
				break;
			}

			val = GetValue("level",
							buffer + token,
							5,
							i - token);
			if (val == 0 || val == 1) {
				aw_private.debug.level = val;
				break;
			}

			token = i + 1;
		}
	}

	return count;
}

static struct file_operations write_fops = {
	.owner = THIS_MODULE,
	.write = WriteWrite,
};

static int DumpDebugfsShow(struct seq_file *s, void *private_data)
{
	int i;
	bool temp_ctrl_status = aw_private.tempctrl.temp_ctrl_status;
	bool scene_ctrl_status = aw_private.pm.scene_ctrl_status;
	int max_available_level = aw_private.pm.max_available_level;
	int current_level = aw_private.pm.current_level;

	if (!aw_private.debug.enable)
		return 0;

	if (aw_private.debug.frequency)
		seq_printf(s, "frequency: %3dMHz; ",
				aw_private.pm.clk[0].freq);

	if (aw_private.debug.voltage)
		seq_printf(s, "voltage: %4dmV; ",
				aw_private.pm.current_vol);

	if (aw_private.debug.tempctrl)
		seq_printf(s, "tempctrl: %s; ",
				temp_ctrl_status ? "on" : "off");

	if (aw_private.debug.scenectrl)
		seq_printf(s, "scenectrl: %s; ",
			scene_ctrl_status ? "on" : "off");

	if (aw_private.debug.dvfs)
		seq_printf(s, "dvfs: %s; ",
			aw_private.pm.dvfs_status ? "on" : "off");

	if (aw_private.debug.level) {
		seq_printf(s, "\nmax_available_level: %d\n",
					max_available_level);
		seq_printf(s, "current_level: %d\n",
					current_level);
		seq_printf(s, "cool_level: %d\n",
					aw_private.pm.cool_level);
		seq_printf(s, "scene_ctrl_cmd: %d\n",
					aw_private.pm.scene_ctrl_cmd);
		seq_printf(s, "   level  voltage  frequency\n");
		for (i = 0; i <= aw_private.pm.max_level; i++)
			seq_printf(s, "%s%3d   %4dmV      %3dMHz\n",
					i == current_level ? "-> " : "   ",
					i, aw_private.pm.vf_table[i].vol,
					aw_private.pm.vf_table[i].freq);
		seq_printf(s, "=========================================");
	}
	seq_printf(s, "\n");

	return 0;
}

static int DumpDebugfsOpen(struct inode *inode, struct file *file)
{
	return single_open(file, DumpDebugfsShow, inode->i_private);
}

static const struct file_operations dump_fops = {
	.owner = THIS_MODULE,
	.open = DumpDebugfsOpen,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static IMG_VOID CreateDebugfsNode(IMG_VOID)
{
	struct dentry *pvr_debugfs_dir;
	pvr_debugfs_dir = debugfs_create_dir(pvr_dev_name, NULL);
	if (ERR_PTR(-ENODEV) == pvr_debugfs_dir)
		/* Debugfs not supported. */
		return;

	debugfs_create_file("dump", 0440,
			pvr_debugfs_dir, NULL, &dump_fops);
	debugfs_create_file("write", 0220,
			pvr_debugfs_dir, NULL, &write_fops);
}

static ssize_t SceneCtrlCmdShow(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	 return sprintf(buf, "%d\n", aw_private.pm.scene_ctrl_cmd);
}

static ssize_t SceneCtrlCmdStore(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err, level_to_be;
	unsigned long val;
	int begin_level = aw_private.pm.begin_level;
	int max_available_level = aw_private.pm.max_available_level;

	err = strict_strtoul(buf, 10, &val);

	if (err) {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		return count;
	}

	if (val == 0) {
		level_to_be = Min(begin_level, max_available_level);
	} else if (val == 1) {
		level_to_be = aw_private.pm.max_available_level;
	} else {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		return count;
	}

	DvfsChange(level_to_be);
	aw_private.pm.scene_ctrl_cmd = val;
	PVR_LOG(("The scene control command has been set to %ld", val));
	return count;
}

static ssize_t SceneCtrlStatusShow(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	 return sprintf(buf, "%d\n", aw_private.pm.scene_ctrl_status);
}

static ssize_t SceneCtrlStatusStore(struct device *dev,
						struct device_attribute *attr,
						const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);

	if (err) {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
		return count;
	}

	if (val == 0 || val == 1) {
		aw_private.pm.scene_ctrl_status = val;
		PVR_LOG(("The scene control status has been set to %ld", val));
	} else {
		PVR_DPF((PVR_DBG_ERROR, "Invalid parameter!"));
	}

	return count;
}

static DEVICE_ATTR(command, 0660, SceneCtrlCmdShow, SceneCtrlCmdStore);
static DEVICE_ATTR(status, 0660, SceneCtrlStatusShow, SceneCtrlStatusStore);

static struct attribute *scene_ctrl_attributes[] = {
	&dev_attr_command.attr,
	&dev_attr_status.attr,
	NULL
};

struct attribute_group scene_ctrl_attribute_group = {
	.name = "scenectrl",
	.attrs = scene_ctrl_attributes
};

static IMG_VOID CreateFsNodeForUserSpace(IMG_VOID)
{
	int res;

	res = sysfs_create_group(&gpsPVRLDMDev->dev.kobj,
						&scene_ctrl_attribute_group);
	if (res) {
		PVR_DPF((PVR_DBG_ERROR, "Failed to create sysfs!"));
	}
}

int GpuThermalCool(int level)
{
	int level_to_be, max_available_level;
	int begin_level = aw_private.pm.begin_level;

	if (!aw_private.tempctrl.temp_ctrl_status)
		return 0;

	if (level < aw_private.pm.max_available_level)
		aw_private.pm.max_available_level = level;

	max_available_level = aw_private.pm.max_available_level;
	if ((aw_private.pm.scene_ctrl_cmd))
		level_to_be = max_available_level;
	else
		level_to_be = Min(max_available_level, begin_level);

	DvfsChange(level_to_be);

	aw_private.pm.cool_level = level;

	return 0;
}
#ifdef CONFIG_SUNXI_GPU_COOLING
EXPORT_SYMBOL(GpuThermalCool);
#endif /* CONFIG_SUNXI_GPU_COOLING */

static IMG_VOID AwInit(IMG_VOID)
{
	int freq, vol;
#ifdef CONFIG_OF
	char ch[25] = {0};
	struct resource *res;
#endif /* CONFIG_OF */

#ifdef CONFIG_OF
	sprintf(ch, "%s%d", "imagination,sgx-544-", sunxi_get_soc_bin());
	aw_private.np_gpu = of_find_compatible_node(NULL, NULL, ch);
#endif /* CONFIG_OF */

	ParseFex();

	CreateFsNodeForUserSpace();

	CreateDebugfsNode();

#ifdef CONFIG_OF
	res = platform_get_resource_byname(gpsPVRLDMDev,
							IORESOURCE_IRQ,
							"IRQGPU");
	if (res)
		aw_private.irq_num = res->start;
#endif /* CONFIG_OF */

	GetGpuRegulator();

	aw_private.pm.cool_level = -1;
	aw_private.pm.current_level = aw_private.pm.begin_level;
	aw_private.pm.max_available_level = aw_private.pm.max_level;
	freq = aw_private.pm.vf_table[aw_private.pm.begin_level].freq;
	vol = aw_private.pm.vf_table[aw_private.pm.begin_level].vol;

	GetGpuClk();

	SetGpuClkParent();

	SetGpuVol(vol);

	SetClkVal(freq);

	mutex_init(&aw_private.pm.dvfs_lock);

#ifdef CONFIG_SUNXI_GPU_COOLING
	gpu_thermal_cool_register(GpuThermalCool);
#endif /* CONFIG_SUNXI_GPU_COOLING */

#ifdef CONFIG_CPU_BUDGET_THERMAL
	register_budget_cooling_notifier(&gpu_throttle_notifier);
#endif /* CONFIG_CPU_BUDGET_THERMAL */
}

IMG_VOID AwDeInit(IMG_VOID)
{
	struct regulator *regulator = aw_private.pm.regulator;

	if (!IS_ERR_OR_NULL(regulator))
		regulator_put(regulator);

#ifdef CONFIG_SUNXI_GPU_COOLING
	gpu_thermal_cool_unregister();
#endif /* CONFIG_SUNXI_GPU_COOLING */
}

static IMG_VOID AwSuspend(SYS_DATA *psSysData)
{
	AssertGpuResetSignal();

	/*
	 * Always disable the SGX clocks when the system clocks are disabled.
	 * This saves having to make an explicit call to DisableSGXClocks if
	 * active power management is enabled.
	 */
	DisableSGXClocks(psSysData);

	/* Set gpu power off gating valid */
	SetBit(aw_private.poweroff_gate.bit, 1, aw_private.poweroff_gate.addr);

	DisableGpuPower();
}

static IMG_VOID AwResume(IMG_VOID)
{
	EnableGpuPower();

	/* Set gpu power off gating invalid */
	SetBit(aw_private.poweroff_gate.bit, 0, aw_private.poweroff_gate.addr);

	DeAssertGpuResetSignal();
}

/*!
******************************************************************************

 @Function  EnableSystemClocks

 @Description Setup up the clocks for the graphics device to work.

 @Return   PVRSRV_ERROR

******************************************************************************/
PVRSRV_ERROR EnableSystemClocks(SYS_DATA *psSysData)
{
	SYS_SPECIFIC_DATA *psSysSpecData;

	psSysSpecData = (SYS_SPECIFIC_DATA *) psSysData->pvSysSpecificData;

	PVR_TRACE(("EnableSystemClocks: Enabling System Clocks"));
	if (!psSysSpecData->bSysClocksOneTimeInit) {
		AwInit();

		mutex_init(&psSysSpecData->sPowerLock);

		atomic_set(&psSysSpecData->sSGXClocksEnabled, 0);

		psSysSpecData->bSysClocksOneTimeInit = IMG_TRUE;
	}

	AwResume();

	return PVRSRV_OK;
}

/*!
******************************************************************************

 @Function  DisableSystemClocks

 @Description Disable the graphics clocks.

 @Return  none

******************************************************************************/
IMG_VOID DisableSystemClocks(SYS_DATA *psSysData)
{
	PVR_TRACE(("DisableSystemClocks: Disabling System Clocks"));

	AwSuspend(psSysData);
}

PVRSRV_ERROR SysDvfsInitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}

PVRSRV_ERROR SysDvfsDeinitialize(SYS_SPECIFIC_DATA *psSysSpecificData)
{
	PVR_UNREFERENCED_PARAMETER(psSysSpecificData);

	return PVRSRV_OK;
}
