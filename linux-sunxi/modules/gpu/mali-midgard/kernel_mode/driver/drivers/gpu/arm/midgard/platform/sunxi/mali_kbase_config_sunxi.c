/*
 * Copyright (C) 2016-2017 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/clk-private.h>
#include <linux/clk/sunxi.h>
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <linux/ioport.h>
#include <linux/sunxi-smc.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <linux/byteorder/generic.h>
#include <linux/opp.h>
#include <linux/devfreq.h>
#include <linux/delay.h>
#include <linux/delay.h>
#include <mali_kbase_device_internal.h>
#include <mali_kbase_pm_internal.h>

#include "platform.h"

#define SMC_REG_BASE 0x3007000
#define SMC_GPU_DRM_REG (SMC_REG_BASE) + 0x54
#define GPU_POWEROFF_GATING_REG (0x07010000 + 0x0254)
#define DVFS_DOWN_DELAY (2)
#define POWER_DOWN_DELAY (3)
#define TEMP_DOWN_DELAY_MAX (2000)
#define TEMP_DOWN_DELAY_MIN (100)

#define SUNXI_DFSO_UPTHRESHOLD	(85)
#define SUNXI_DFSO_DOWNDIF	(14)

#define sunxi_gpu_printf(level, format, args...) \
{ \
	if ((sunxi_mali->debug) >= (level)) \
		printk("Mali(%d) "format, __LINE__, ## args); \
}

static struct sunxi_private *sunxi_mali;

static int sunxi_man_ctl(struct sunxi_private *sunxi_mali, struct aw_vf_table *target);
static int sunxi_platform_init(struct kbase_device *kbdev);
static int sunxi_update_power(struct sunxi_private *sunxi_mali, struct aw_vf_table *target);
static void sunxi_platform_term(struct kbase_device *kbdev);

struct kbase_platform_funcs_conf sunxi_platform_conf = {
	.platform_init_func = sunxi_platform_init,
	.platform_term_func = sunxi_platform_term,
};

static inline void set_reg_bit(unsigned char bit,
			bool val, void __iomem *ioaddr)
{
	int reg_data;
	reg_data = readl(ioaddr);
	if (!val)
		reg_data &= ~(1 << bit);
	else
		reg_data |= (1 << bit);

	writel(reg_data, ioaddr);
}

void sunxi_power_gate(int on)
{
	set_reg_bit(0, on, sunxi_mali->power_gate_addr);
}

static inline int sunxi_check_level(unsigned long rate)
{
	int i = 0;
	while (i < sunxi_mali->nr_of_vf) {
		if (sunxi_mali->vf_table[i].freq > rate)
			break;
		i++;
	}

	return i == 0 ? 0 : i-1;
}

static int sunxi_set_clock_nosafe(struct aw_vf_table *target)
{
	int err;
	unsigned long pll_freq;
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;
	pll_freq = target->freq;
	while (pll_freq < 288000000)
		pll_freq *= 2;

	err = clk_set_rate(sunxi_mali->gpu_pll_clk, pll_freq);
	if (err) {
		sunxi_gpu_printf(0, "set pll clock err(%d) freq(%ld).\n",
			err, target->freq);
	}

	err = clk_set_rate(kbdev->clock, target->freq);
	if (err) {
		sunxi_gpu_printf(0, "set core clock err(%d) freq(%ld).\n",
			err, target->freq);
	}

	kbdev->current_voltage = target->vol;
	kbdev->current_freq = target->freq;
	sunxi_mali->power_ctl.current_level = sunxi_check_level(target->freq);

	return err;
}

static int sunxi_set_clock_safe(struct aw_vf_table *target, bool now)
{
	int err;
	bool do_resume = now;
	int retry_cnt = 3;
	unsigned long flags;
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;

	if (kbdev->current_freq > target->freq) {
		sunxi_mali->down_vfs++;
		if (sunxi_mali->down_vfs < DVFS_DOWN_DELAY && !now)
			return 0;
	} else {
		sunxi_mali->down_vfs = 0;
	}

	spin_lock(&sunxi_mali->target_lock);
	sunxi_mali->target_vf.freq = target->freq;
	sunxi_mali->target_vf.vol = target->vol;
	sunxi_mali->do_dvfs = 1;
	spin_unlock(&sunxi_mali->target_lock);

	if (!now) {
		err = wait_event_timeout(sunxi_mali->dvfs_done,
			sunxi_mali->do_dvfs == 0, msecs_to_jiffies(50));
		if (err <= 0) {
			sunxi_gpu_printf(1, "waite for gpu idle:%d %d"
				" form(%ld)to(%ld) status:%d 0x%02x\n",
				err, sunxi_mali->do_dvfs,
				kbdev->current_freq,
				target->freq,
				sunxi_mali->power_ctl.power_on,
				sunxi_mali->power_ctl.power_status);
		}
		do_resume = sunxi_mali->do_dvfs;
	}

	while (do_resume) {
		if (kbdev->pm.suspending == true)
			break;
		sunxi_mali->dvfs_ing = true;
		kbase_pm_suspend(kbdev);
		kbase_pm_resume(kbdev);
		sunxi_mali->dvfs_ing = false;
retry:
		/* JetCui: make a sure designed */
		if (!kbdev->pm.backend.gpu_in_desired_state &&
			retry_cnt-- > 0) {
			spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
			kbase_pm_check_transitions_nolock(kbdev);
			spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
			msleep(10);
			goto retry;
		}
		do_resume = 0;
	}

	return 0;
}

static int sunxi_power_on(struct kbase_device *kbdev,
		enum power_status status)
{
	int ret = 0;
	struct power_control *power_ctl;
	power_ctl = &sunxi_mali->power_ctl;

	mutex_lock(&sunxi_mali->power_lock);

	if (sunxi_mali->power_ctl.power_status)
		goto done;

#if defined(CONFIG_REGULATOR)
	if (regulator_is_enabled(sunxi_mali->regulator)) {
		ret = 0;
		printk(KERN_INFO "Mali: already power on\n");
		goto done;
	}
	ret = regulator_enable(sunxi_mali->regulator);
	if (ret) {
		printk(KERN_ERR "Mali: failed to enable GPU power!\n");
		goto done;
	}
#endif
	sunxi_power_gate(0);

	ret = clk_prepare_enable(kbdev->clock);
	if (ret) {
		printk(KERN_ERR "Mali: Failed to enable GPU core clock!\n");
		regulator_disable(sunxi_mali->regulator);
	}

done:
	sunxi_mali->power_ctl.power_status |= status;
	mutex_unlock(&sunxi_mali->power_lock);

	sunxi_gpu_printf(2, "sunxi_power_on: status=%d\n",
		 sunxi_mali->power_ctl.power_status);

	return ret;

}

static int sunxi_power_off(struct kbase_device *kbdev,
		enum power_status status)
{
	struct power_control *power_ctl;
	power_ctl = &sunxi_mali->power_ctl;

	if ((power_ctl->scenectrl_target > sunxi_mali->power_ctl.max_normal_level)
		  && power_ctl->en_scenectrl) {
		return 0;
	}

	mutex_lock(&sunxi_mali->power_lock);
	if ((sunxi_mali->power_ctl.power_status & ~status) ||
		!sunxi_mali->power_ctl.power_status) {
		goto done;
	}

	clk_disable_unprepare(kbdev->clock);
	sunxi_power_gate(1);

#if defined(CONFIG_REGULATOR)
	if (!regulator_is_enabled(sunxi_mali->regulator)) {
		printk(KERN_INFO "Mali: already power off\n");
		goto done;
	}

	if (regulator_disable(sunxi_mali->regulator))
		printk(KERN_ERR "Mali: failed to disable GPU power!\n");
#endif

done:
	sunxi_mali->power_ctl.power_status &= ~status;
	mutex_unlock(&sunxi_mali->power_lock);

	sunxi_gpu_printf(3, "sunxi_power_off: status=%d\n",
		sunxi_mali->power_ctl.power_status);

	return 0;

}

static void sunxi_poweroff_delay(struct work_struct *work)
{
	struct kbase_device *kbdev;
	struct sunxi_private *sunxi_mali;
	enum power_status stats = POWER_GPU_STATUS;

	sunxi_mali = container_of((struct delayed_work *)work,
				struct sunxi_private, power_off_delay);
	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;
	if (kbdev->pm.suspending == true)
		stats |= POWER_MAN_STATUS;
	sunxi_power_off(kbdev, stats);
}

int sunxi_check_benchmark(struct sunxi_private *sunxi_mali, unsigned long freq)
{
	struct power_control *power_ctl;
	unsigned int  target_level;
	struct kbase_device *kbdev;

	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;
	power_ctl = &sunxi_mali->power_ctl;

	target_level = sunxi_check_level(freq);

	if (sunxi_mali->nr_of_vf < 2)
		return 0;

	if (target_level >= power_ctl->bk_threshold) {
		power_ctl->benckmark_mode++;
		if (power_ctl->benckmark_mode > 4)
			power_ctl->normal_mode = 0;
	} else {
		power_ctl->normal_mode++;
		if (power_ctl->normal_mode > 2)
			power_ctl->benckmark_mode = 0;
	}

	return power_ctl->benckmark_mode > 4;
}

#ifdef CONFIG_PM_DEVFREQ
int sunxi_update_vf(struct kbase_device *kbdev,
		unsigned long *voltage, unsigned long *freq)
{
	struct aw_vf_table target;
	struct devfreq *devfreq = NULL;
	devfreq = kbdev->devfreq;

#if IS_ENABLED(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND)
	if (devfreq->data == NULL)
		devfreq->data = (void *)&sunxi_mali->sunxi_date;
#endif
	if (devfreq->max_freq == 0)
		devfreq->max_freq = sunxi_mali->vf_table[sunxi_mali->power_ctl.max_normal_level].freq;
	if (devfreq->min_freq == 0)
		devfreq->min_freq = sunxi_mali->vf_table[0].freq;
	if (sunxi_mali->power_ctl.en_dvfs == 0
		|| sunxi_mali->power_ctl.man_ctrl) {
		sunxi_gpu_printf(2, "Mali: dvfs is closed(man,scen,dvfs)(%d,%d,%d).\n",
			sunxi_mali->power_ctl.man_ctrl,
			sunxi_mali->power_ctl.en_scenectrl,
			sunxi_mali->power_ctl.en_dvfs);
		*voltage = kbdev->current_voltage;
		*freq = kbdev->current_freq;
		return 0;
	}

	if (!((kbdev->inited_subsys >> 12) & 1)) {
		*freq = sunxi_mali->init_freq;
		goto setdone;
	}

	/*
	 * Jet Cui: check for the benchmark,
	 * and  for the devfreq_get_freq_level(==function) the bug,
	 * we must make sure that the frq must in the opp table.
	 */
	if (sunxi_check_benchmark(sunxi_mali, *freq)) {
		int levl;
		levl = sunxi_check_level(devfreq->max_freq);
		*freq = devfreq->max_freq;
		*voltage = sunxi_mali->vf_table[levl].vol;
	}
	if (kbdev->current_freq == *freq)
		goto setdone;

	target.freq = *freq;
	target.vol = *voltage;

	sunxi_set_clock_safe(&target, 0);
	*freq = target.freq;
	*voltage = target.vol;

setdone:
	return 0;
}
#endif

static void sunxi_update_ctl_dvf(struct sunxi_private *sunxi_mali, bool to_dvfs)
{
	struct power_control *power_ctl;
	struct kbase_device *kbdev;
#ifdef CONFIG_PM_DEVFREQ
	struct devfreq *devfreq = NULL;
	struct aw_vf_table *pcur_vf;
	struct opp *temp_opp;
	unsigned long rate;
#endif

	power_ctl = &sunxi_mali->power_ctl;
	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;

#ifdef CONFIG_PM_DEVFREQ
	devfreq = kbdev->devfreq;
	pcur_vf = &sunxi_mali->cur_vf;

	if (to_dvfs) {
		rate = pcur_vf->freq;

		rcu_read_lock();
		temp_opp = opp_find_freq_floor(kbdev->dev, &rate);
		if (!IS_ERR_OR_NULL(temp_opp) &&
			!IS_ERR_OR_NULL(devfreq)) {
			devfreq->previous_freq = rate;
			kbdev->current_freq = rate;
			kbdev->current_voltage = opp_get_voltage(temp_opp);
		} else {
			kbdev->current_freq = pcur_vf->freq;
			kbdev->current_voltage = pcur_vf->vol;
		}
		rcu_read_unlock();
	} else {
		if (IS_ERR_OR_NULL(devfreq))
			goto default_d;

		rate = kbdev->current_freq;

		rcu_read_lock();
		temp_opp = opp_find_freq_ceil(kbdev->dev, &rate);
		if (IS_ERR_OR_NULL(temp_opp)) {
			rcu_read_unlock();
			goto default_d;
		}
		pcur_vf->freq = rate;
		pcur_vf->vol = opp_get_voltage(temp_opp);
		rcu_read_unlock();

		power_ctl->current_level = sunxi_check_level(pcur_vf->freq);
	}
#endif
	return;

#ifdef CONFIG_PM_DEVFREQ
default_d:
	pcur_vf->freq = kbdev->current_freq;
#endif
	power_ctl->current_level = sunxi_check_level(kbdev->current_freq);
}

static int sunxi_update_power(struct sunxi_private *sunxi_mali,
	struct aw_vf_table *target)
{
	struct kbase_device *kbdev;
	struct aw_vf_table *pcur_vf;
	struct power_control *power_ctl;
	int err = 0;

	power_ctl = &sunxi_mali->power_ctl;
	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;
	pcur_vf = &sunxi_mali->cur_vf;

	if (target->vol > 1500000 || target->vol < 600000) {
		printk(KERN_ERR "Mali: set a error volt(%ld)um!\n", target->vol);
		return -1;
	}

	if (pcur_vf->freq < target->freq)
		err = regulator_set_voltage(sunxi_mali->regulator, target->vol, target->vol);
	if (err) {
		printk(KERN_ERR "Mali: set regulate up err(%d)!\n", err);
		goto done_err;
	}

	if (target->freq) {
		err = sunxi_set_clock_nosafe(target);
		if (err) {
			printk(KERN_ERR "Mali: set clock err(%d)!\n", err);
			goto done_err;
		}
	}
	if (pcur_vf->freq > target->freq)
		err = regulator_set_voltage(sunxi_mali->regulator, target->vol, target->vol);
	if (err) {
		printk(KERN_ERR "Mali: set regulate down err(%d).\n", err);
		goto done_err;
	}

	pcur_vf->vol = regulator_get_voltage(sunxi_mali->regulator);
	pcur_vf->freq = clk_get_rate(kbdev->clock);
	power_ctl->current_level = sunxi_check_level(target->freq);

	sunxi_gpu_printf(1, "set:%ldmV-%ldMHz get:%ldmV-%ldMHz pll:%ldMHz\n",
		target->vol / 1000, target->freq / 1000 / 1000,
		pcur_vf->vol / 1000,
		pcur_vf->freq / 1000 / 1000,
		clk_get_rate(sunxi_mali->gpu_pll_clk) / 1000 / 1000);

	return 0;
done_err:
	return err;

}

static int sunxi_man_ctl(struct sunxi_private *sunxi_mali,
		struct aw_vf_table *target)
{
	int err = 0;

	if (target->freq == 0)
		target->freq = sunxi_mali->cur_vf.freq;

	if (target->vol == 0)
		target->vol = sunxi_mali->cur_vf.vol;

	err = sunxi_set_clock_safe(target, 1);
	return err;
}

static void sunxi_ctl_change(struct sunxi_private *sunxi_mali,
	enum power_ctl_level level, bool en, struct aw_vf_table *target)
{
	struct power_control *power_ctl;
	struct kbase_device *kbdev;
	struct devfreq *devfreq;
	int target_level;
	bool dvfs_switch = 0;
	bool now = 0;

	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;
	power_ctl = &sunxi_mali->power_ctl;
#ifdef CONFIG_PM_DEVFREQ
	devfreq = kbdev->devfreq;
#endif
	sunxi_gpu_printf(2, "cmd:0x%08x, %s\n", level, en ? "on" : "off");
	if (target)
		sunxi_gpu_printf(2, "target:%ld, %ld\n",
				target->vol, target->freq);

	mutex_lock(&sunxi_mali->ctrl_lock);

	switch (level) {
	case POWER_ON_OFF_CTL:
		sunxi_mali->power_ctl.en_idle = !en;
		if (en)
			sunxi_power_on(kbdev, POWER_MAN_STATUS);
		else
			sunxi_power_off(kbdev, POWER_MAN_STATUS);
	case POWER_MAN_CTL:
		if (level&POWER_MAN_CTL) {
			if (en) {
				if (!power_ctl->man_ctrl) {
					sunxi_update_ctl_dvf(sunxi_mali, 0);
					devfreq_suspend_device(devfreq);
				}
				power_ctl->man_ctrl = 1;
				if (target)
					goto set_target;
				goto change_ok;
			}
			if (power_ctl->man_ctrl != en)
				dvfs_switch = 1;
			power_ctl->man_ctrl = en;
		}
	case POWER_TEMP_CTL:
		if (level&POWER_TEMP_CTL) {
			if (target) {
				if (target->freq <= sunxi_mali->power_ctl.max_normal_level)
					power_ctl->tempctrl_target = target->freq;
				else
					power_ctl->tempctrl_target = sunxi_mali->power_ctl.max_normal_level;

			}
			power_ctl->en_tempctrl = en;
		}
	case POWER_SCENE_CTL:
		if (level&POWER_SCENE_CTL) {
			power_ctl->en_scenectrl = en;
			if (target) {
				if (target->freq < sunxi_mali->nr_of_vf)
					power_ctl->scenectrl_target = target->freq;
				else
					power_ctl->scenectrl_target = sunxi_mali->power_ctl.max_normal_level;
			}
		}
	case POWER_DVFS_CTL:
		if (sunxi_mali->temp_status == TEMP_STATUS_RUNNING
			&& power_ctl->en_tempctrl) {
			target_level = power_ctl->tempctrl_target;
			if (target_level < power_ctl->current_level
				|| devfreq == NULL
				|| devfreq->stop_polling)
				now = 1;
		} else if (power_ctl->en_scenectrl
			&& power_ctl->scenectrl_cmd) {
			target_level = power_ctl->scenectrl_target;
		} else {
			target_level = sunxi_mali->power_ctl.max_normal_level;
		}

		target = &sunxi_mali->vf_table[target_level];

		if (level&POWER_DVFS_CTL) {
			if (power_ctl->en_dvfs != en)
				dvfs_switch = 1;
			power_ctl->en_dvfs = en;
		}
#ifdef CONFIG_PM_DEVFREQ
		if (devfreq) {
			devfreq->max_freq = sunxi_mali->vf_table[target_level].freq;
			devfreq->min_freq = sunxi_mali->vf_table[0].freq;
			if (power_ctl->en_scenectrl
				&& power_ctl->scenectrl_cmd)
				devfreq->min_freq = sunxi_mali->vf_table[target_level].freq;
		}
#endif
		if (power_ctl->man_ctrl) {
			printk(KERN_INFO "Mali: you must close the man_ctrl:%d!\n",
				power_ctl->man_ctrl);
			goto change_ok;
		}

		if (dvfs_switch && devfreq != NULL) {
			if (power_ctl->en_dvfs) {
				sunxi_update_ctl_dvf(sunxi_mali, 1);
				devfreq_resume_device(devfreq);
			} else {
				devfreq_suspend_device(devfreq);
			}
		}

		if (!power_ctl->en_dvfs || devfreq == NULL || now)
			goto set_target;
		break;

	default:
		printk(KERN_ERR "Mali: you must give us a good set.\n");
	}

change_ok:
	mutex_unlock(&sunxi_mali->ctrl_lock);
	return;

set_target:
	sunxi_man_ctl(sunxi_mali, target);
	mutex_unlock(&sunxi_mali->ctrl_lock);
	return;
}

#ifdef CONFIG_SUNXI_GPU_COOLING

static void sunxi_temp_down_delay(struct work_struct *work)
{
	struct sunxi_private *sunxi_mali;
	struct aw_vf_table target;
	int target_level;
	int diff_target = 0;
	static unsigned int time_msec = TEMP_DOWN_DELAY_MAX;
	static int stable_cnt;
	static int conti_down;
	static int conti_up;
	enum temp_status ttemp_status;

	sunxi_mali = container_of((struct delayed_work *)work,
				struct sunxi_private, temp_donwn_delay);

	target_level = sunxi_mali->power_ctl.tempctrl_target;
	ttemp_status = sunxi_mali->temp_status;

	mutex_lock(&sunxi_mali->temp_lock);

	if (sunxi_mali->temp_current_level <= 0) {
		target_level = sunxi_mali->power_ctl.max_normal_level;
		ttemp_status = TEMP_STATUS_NORNAL;
		conti_up = 0;
		conti_down = 0;
		stable_cnt = 0;
		goto set_target;
	}
restart:
	if (sunxi_mali->temp_current_level == TEMP_LEVEL_URGENT) {
		stable_cnt = 0;
		conti_up = 0;
		conti_down++;
		ttemp_status = TEMP_STATUS_RUNNING;
		if (sunxi_mali->power_ctl.tempctrl_target == 0)
			printk(KERN_WARNING "Mali has set the least vf,no idea:%d.\n", conti_down);

		diff_target = sunxi_mali->last_set_target - sunxi_mali->power_ctl.tempctrl_target;
		diff_target /= 2;
		if (diff_target > 0)
			diff_target = -diff_target;
		if (diff_target == 0)
			diff_target = -sunxi_mali->power_ctl.tempctrl_target/2;

		time_msec /= conti_down;
		if (time_msec < TEMP_DOWN_DELAY_MIN)
			time_msec = TEMP_DOWN_DELAY_MIN;
		diff_target -= conti_down;
	}

	if (sunxi_mali->temp_current_level == TEMP_LEVEL_STABELED
		|| sunxi_mali->power_ctl.tempctrl_target >= sunxi_mali->power_ctl.max_normal_level) {
		stable_cnt++;
		conti_down = 0;
		time_msec = TEMP_DOWN_DELAY_MAX;
		if (stable_cnt >= 3) {
			ttemp_status = TEMP_STATUS_LOCKED;
			sunxi_mali->stable_cnt++;
			sunxi_mali->stable_statistics += sunxi_mali->power_ctl.tempctrl_target;
			if (sunxi_mali->power_ctl.tempctrl_target >= sunxi_mali->power_ctl.max_normal_level
				&& sunxi_mali->temp_current_level < TEMP_LEVEL_URGENT) {
				ttemp_status = TEMP_STATUS_NORNAL;
				conti_up = 0;
				conti_down = 0;
				stable_cnt = 0;

				goto cal_statistics;
			}
		}
	}

	if (sunxi_mali->temp_current_level == TEMP_LEVEL_LOW) {
		if (sunxi_mali->power_ctl.tempctrl_target != sunxi_mali->power_ctl.max_normal_level)
			stable_cnt = 0;
		conti_down = 0;
		conti_up++;
		ttemp_status = TEMP_STATUS_RUNNING;
		time_msec = TEMP_DOWN_DELAY_MAX;
		diff_target =  sunxi_mali->last_set_target - sunxi_mali->power_ctl.tempctrl_target;
		if (diff_target <= 0)
			diff_target =  sunxi_mali->power_ctl.max_normal_level - sunxi_mali->power_ctl.tempctrl_target;
		diff_target /= 2;
		if (diff_target <= 0)
			diff_target = 0;
		diff_target += conti_up;
		time_msec /= conti_up;
	}

	target_level += diff_target;
	if (target_level > sunxi_mali->power_ctl.max_normal_level)
		target_level = sunxi_mali->power_ctl.max_normal_level;
	if (target_level < 0)
		target_level = 0;

	sunxi_mali->last_set_target = sunxi_mali->power_ctl.tempctrl_target;

set_target:
	target.vol = 0;
	target.freq = target_level;
	sunxi_gpu_printf(1, "The delay_work set to level:%d diff:%d en:%d\n",
		target_level, diff_target, sunxi_mali->power_ctl.en_tempctrl);
	if (target_level != sunxi_mali->power_ctl.tempctrl_target)
		sunxi_ctl_change(sunxi_mali, POWER_TEMP_CTL, sunxi_mali->power_ctl.en_tempctrl, &target);

cal_statistics:
	if (ttemp_status != TEMP_STATUS_NORNAL) {
		schedule_delayed_work(&sunxi_mali->temp_donwn_delay,
			msecs_to_jiffies(time_msec));
		sunxi_mali->temp_status = ttemp_status;
	} else {
		if (sunxi_mali->stable_cnt > 0)
			sunxi_mali->last_stable_target =
				sunxi_mali->stable_statistics
				/ sunxi_mali->stable_cnt;
		sunxi_mali->temp_status = ttemp_status;

		if (sunxi_mali->temp_current_level == TEMP_LEVEL_URGENT)
			goto restart;
	}
	mutex_unlock(&sunxi_mali->temp_lock);

}

int thermal_cool_callback(int level)
{
	struct power_control *power_ctl;
	struct kbase_device *kbdev;
	struct aw_vf_table target;
	int target_level;
	target_level = sunxi_mali->power_ctl.max_normal_level;

	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;
	power_ctl = &sunxi_mali->power_ctl;

	if (level < 0)
		goto failed;

	if (level > TEMP_LEVEL_URGENT)
		goto failed;

	sunxi_mali->temp_current_level = level;

	if (level == TEMP_LEVEL_URGENT
		&& sunxi_mali->temp_status == TEMP_STATUS_NORNAL) {
		sunxi_mali->temp_status = TEMP_STATUS_RUNNING;
		sunxi_mali->stable_statistics = 0;
		sunxi_mali->stable_cnt = 0;

		schedule_delayed_work(&sunxi_mali->temp_donwn_delay,
			msecs_to_jiffies(TEMP_DOWN_DELAY_MAX));

		if (sunxi_mali->last_stable_target < sunxi_mali->power_ctl.current_level)
			target_level = sunxi_mali->last_stable_target;
		else
			target_level = sunxi_mali->power_ctl.current_level / 2;
		sunxi_mali->last_stable_target = sunxi_mali->nr_of_vf / 2;

		goto set_target;
	}

	if (sunxi_mali->temp_status != TEMP_STATUS_NORNAL)
		return 0;

set_target:
	target.vol = 0;
	target.freq = target_level;
	sunxi_gpu_printf(1, "The thermal control set to level:%d enable:%d\n",
		level, power_ctl->en_tempctrl);

	sunxi_ctl_change(sunxi_mali, POWER_TEMP_CTL, power_ctl->en_tempctrl, &target);
	return 0;

failed:
	printk(KERN_ERR "Mali: thermal give err level:%d\n", level);
	return -1;
}
#endif /* CONFIG_SUNXI_GPU_COOLING */

static ssize_t scene_ctrl_cmd_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", sunxi_mali->power_ctl.scenectrl_cmd);
}

static ssize_t scene_ctrl_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val;
	struct aw_vf_table target;
	struct power_control *power_ctl;
	enum scene_ctrl_cmd cmd;

	power_ctl = &sunxi_mali->power_ctl;

	err = kstrtoul(buf, 10, &val);

	if (err) {
		printk(KERN_ERR "Mali: invalid parameter!");
		return count;
	}

	cmd = (enum scene_ctrl_cmd)val;
	switch (cmd) {
	case SCENE_CTRL_NORMAL_MODE:
		target.freq = sunxi_mali->power_ctl.max_normal_level;
		break;

	case SCENE_CTRL_PERFORMANCE_MODE:
		target.freq = sunxi_mali->nr_of_vf - 1;
		break;

	default:
		printk(KERN_ERR "Mali: invalid scene ctrl command %d!\n", cmd);
		return count;
	}

	power_ctl->scenectrl_cmd = cmd;
	target.vol = 0;
	sunxi_ctl_change(sunxi_mali, POWER_SCENE_CTL,
					power_ctl->en_scenectrl,
					&target);

	sunxi_gpu_printf(2, "The scene control command is %d.\n", cmd);

	return count;
}

static ssize_t scene_ctrl_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "enable:%d\n",
				sunxi_mali->power_ctl.en_scenectrl);
}

static ssize_t scene_ctrl_status_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int err;
	unsigned long val;

	err = strict_strtoul(buf, 10, &val);

	if (err) {
		printk(KERN_ERR "Mali: invalid parameter!");
		return count;
	}

	sunxi_ctl_change(sunxi_mali, POWER_SCENE_CTL, !!val, NULL);
	sunxi_gpu_printf(2, "the scene control enable: %d\n", !!val);

	return count;
}

static DEVICE_ATTR(command, 0660, scene_ctrl_cmd_show, scene_ctrl_cmd_store);
static DEVICE_ATTR(status, 0660, scene_ctrl_status_show, scene_ctrl_status_store);

static struct attribute *scene_ctrl_attributes[] = {
	&dev_attr_command.attr,
	&dev_attr_status.attr,
	NULL
};

struct attribute_group scene_ctrl_attribute_group = {
	.name = "scenectrl",
	.attrs = scene_ctrl_attributes
};

#ifdef CONFIG_DEBUG_FS
static bool get_value(char *cmd, char *buf,
		int cmd_size, int total_size, unsigned long *value)
{
	char data[10];
	bool found = 0;
	int i, j;

	if (buf == NULL)
		return 0;

	for (; !((*buf >= 'A' && *buf <= 'Z') || (*buf >= 'a' && *buf <= 'z')); buf++)
		total_size--;

	if (!strncmp(buf, cmd, cmd_size)) {
		for (i = 0; i < total_size; i++) {
			if (*(buf+i) == ':' || *(buf+i) == '=') {
				for (j = 0; j < total_size - i - 1; j++)
					data[j] = *(buf + i + 1 + j);
				data[j] = '\0';
				found = 1;
				break;
			}
		}
		if (found) {
			if (strict_strtoul(data, 10, value)) {
				printk(KERN_ERR "Mali: invalid parameter!\n");
				return 0;
			}
		}
	}

	return found;
}

static ssize_t write_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *offp)
{
	int i, temp_level, token = 0;
	bool found = 0, man_ctrl = 0, tempctrl = 0, temp_found = 0, man_found = 0;
	char buffer[100];
	unsigned long value = 0;
	struct power_control *power_ctl;
	struct aw_vf_table target;
	power_ctl = &sunxi_mali->power_ctl;
	target = sunxi_mali->cur_vf;
	man_ctrl = power_ctl->man_ctrl;
	tempctrl = power_ctl->en_tempctrl;
	temp_level = power_ctl->tempctrl_target;

	if (count >= sizeof(buffer))
		return -ENOMEM;

	if (copy_from_user(buffer, buf, count))
			return -EFAULT;

	buffer[count] = '\0';

	for (i = 0; i < count; i++) {
		if (*(buffer+i) == ';') {
			found = get_value("debug", buffer + token,
						5, i - token, &value);
			if (found)
				sunxi_mali->debug = value;

			found = get_value("power", buffer + token,
						5, i - token, &value);
			if (found)
				sunxi_ctl_change(sunxi_mali,
					POWER_ON_OFF_CTL, !!value, NULL);

			found = get_value("man", buffer + token,
						3, i - token, &value);
			if (found) {
				man_found = 1;
				man_ctrl = !!value;
			}

			found = get_value("fq", buffer + token,
						2, i - token, &value);
			if (found && value >= 0) {
				man_found = 1;
				target.freq = value*1000*1000;
			}

			found = get_value("vl", buffer + token,
						2, i - token, &value);
			if (found && value >= 0) {
				target.vol = value*1000;
				man_found = 1;
			}

			found = get_value("tm", buffer + token,
						2, i - token, &value);
			if (found) {
				tempctrl = !!value;
				temp_found = 1;
			}

			found = get_value("tl", buffer + token,
						2, i - token, &value);
			if (found) {
				temp_level = value;
				temp_found = 1;
			}

			found = get_value("dvfs", buffer + token,
						4, i - token, &value);
			if (found)
				sunxi_ctl_change(sunxi_mali,
					POWER_DVFS_CTL, !!value, NULL);
			token = i + 1;
		}
	}

	if (man_found)
		sunxi_ctl_change(sunxi_mali, POWER_MAN_CTL, man_ctrl, &target);

	if (temp_found) {
		target.freq = temp_level;
		sunxi_ctl_change(sunxi_mali, POWER_TEMP_CTL, tempctrl, &target);
	}

	return count;
}

const struct file_operations write_fops = {
	.owner = THIS_MODULE,
	.write = write_write,
};

int sunxi_set_utilisition(unsigned long total_out, unsigned long busy_out)
{
	sunxi_mali->utilisition = (busy_out*100)/total_out;
	return 0;
}

static int dump_debugfs_show(struct seq_file *s, void *private_data)
{
	int i;
	struct power_control *power_ctl;
	struct aw_vf_table *cur_vf;
	struct aw_vf_table *vf_table;
	struct kbase_device *kbdev;
#if defined(CONFIG_PM_DEVFREQ) || defined(CONFIG_MALI_MIDGARD_DVFS)
	unsigned long total_out, busy_out;
#endif
	kbdev = (struct kbase_device *)sunxi_mali->kbase_mali;

	power_ctl = &sunxi_mali->power_ctl;
	cur_vf = &sunxi_mali->cur_vf;
	vf_table = sunxi_mali->vf_table;
	if (!power_ctl->man_ctrl)
		sunxi_update_ctl_dvf(sunxi_mali, 0);
#if defined(CONFIG_PM_DEVFREQ) || defined(CONFIG_MALI_MIDGARD_DVFS)
	kbase_pm_get_dvfs_utilisation(kbdev,
				&total_out, &busy_out);
	sunxi_mali->utilisition = (busy_out*100)/total_out;
#endif
	seq_printf(s, "power %02x power_on:%d;\n",
		power_ctl->power_status, power_ctl->power_on);
	seq_printf(s, "mutex status:%d;\n",
		atomic_read(&sunxi_mali->power_lock.count));
	seq_printf(s, "dvfs: %s;\n",
		power_ctl->en_dvfs ? "on" : "off");
	seq_printf(s, "man_ctrl: %s;\n",
		power_ctl->man_ctrl ? "on" : "off");
	seq_printf(s, "Curent:  voltage: %4ldmV;",
		cur_vf->vol/1000);
	seq_printf(s, "       frequency: %3ldMHz;\n",
		cur_vf->freq/1000/1000);
	seq_printf(s, "tempctrl: %s at level:%d templ:%d status:%d last_stable:%d\n",
		power_ctl->en_tempctrl ? "on" : "off",
		power_ctl->tempctrl_target,
		sunxi_mali->temp_current_level,
		sunxi_mali->temp_status,
		sunxi_mali->last_stable_target);
	seq_printf(s, "scenctrl: %s at level:%d\n",
		power_ctl->en_scenectrl ? "on" : "off",
		power_ctl->scenectrl_target);

	seq_printf(s, "number of level: %d\n current level:%d\n max_normal:%d\n",
		sunxi_mali->nr_of_vf,
		power_ctl->current_level,
		power_ctl->max_normal_level);
	seq_printf(s, "   level  voltage  frequency\n");
	for (i = 0; i < sunxi_mali->nr_of_vf; i++)
		seq_printf(s, "%s%3d   %4ldmV      %3ldMHz\n",
				vf_table[i].freq == cur_vf->freq ? "-> " : "   ",
				i, vf_table[i].vol / 1000,
				vf_table[i].freq / 1000000);

	seq_printf(s, "\n");
	seq_printf(s, "Power status for core:\n");
	seq_printf(s, "active: %d %llx  %llx  %llx  %llx  %llx  off:%d;\n",
		kbdev->pm.active_count,
		kbdev->shader_available_bitmap,
		kbdev->tiler_available_bitmap,
		kbdev->shader_needed_bitmap,
		kbdev->shader_inuse_bitmap,
		kbdev->pm.backend.desired_shader_state,
		kbdev->pm.backend.poweroff_wait_in_progress
		);
	seq_printf(s, "Desired state :\n");
	seq_printf(s, "\tShader=%016llx\n",
			kbdev->pm.backend.desired_shader_state);
	seq_printf(s, "\tTiler =%016llx\n",
			kbdev->pm.backend.desired_tiler_state);
	seq_printf(s, "Current state :\n");
	seq_printf(s, "\tShader=%08x%08x active:%08x%08x\n",
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(SHADER_READY_HI), NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(SHADER_READY_LO),
				NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(SHADER_PWRACTIVE_HI), NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(SHADER_PWRACTIVE_LO),
				NULL));
	seq_printf(s, "\tTiler =%08x%08x active:%08x%08x\n",
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(TILER_READY_HI), NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(TILER_READY_LO), NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(TILER_PWRACTIVE_HI), NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(TILER_PWRACTIVE_LO), NULL));
	seq_printf(s, "\tL2    =%08x%08x\n",
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(L2_READY_HI), NULL),
			kbase_reg_read(kbdev,
				GPU_CONTROL_REG(L2_READY_LO), NULL));
	seq_printf(s, "Cores transitioning :\n");
	seq_printf(s, "\tShader=%08x%08x\n",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(
					SHADER_PWRTRANS_HI), NULL),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(
					SHADER_PWRTRANS_LO), NULL));
	seq_printf(s, "\tTiler =%08x%08x\n",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(
					TILER_PWRTRANS_HI), NULL),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(
					TILER_PWRTRANS_LO), NULL));
	seq_printf(s,  "\tL2    =%08x%08x\n",
			kbase_reg_read(kbdev, GPU_CONTROL_REG(
					L2_PWRTRANS_HI), NULL),
			kbase_reg_read(kbdev, GPU_CONTROL_REG(
					L2_PWRTRANS_LO), NULL));
	seq_printf(s,  "\nutilisition = %ld%%\n", sunxi_mali->utilisition);
	seq_printf(s, "=========================================\n");

	return 0;
}

static int dump_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_debugfs_show, inode->i_private);
}

const struct file_operations dump_fops = {
	.owner = THIS_MODULE,
	.open = dump_debugfs_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sunxi_mali_creat_debug_fs(void)
{

	sunxi_mali->my_debug = debugfs_create_dir("sunxi_gpu", NULL);
	if (!debugfs_create_file("dump", 0644,
		sunxi_mali->my_debug, NULL, &dump_fops))
		goto Fail;
	if (!debugfs_create_file("write", 0644,
		sunxi_mali->my_debug, NULL, &write_fops))
		goto Fail;
	return 0;

Fail:
	debugfs_remove_recursive(sunxi_mali->my_debug);
	return -ENOENT;
}

#endif /* CONFIG_DEBUG_FS */

#ifdef CONFIG_OF
static void sunxi_parse_fex(struct device_node *np, struct sunxi_private *sunxi)
{
	int val;
	if (!of_property_read_u32(np, "gpu_idle", &val))
		sunxi->power_ctl.en_idle = !!val;
	if (!of_property_read_u32(np, "dvfs_status", &val))
		sunxi->power_ctl.en_dvfs = !!val;
	if (!of_property_read_u32(np, "temp_ctrl_status", &val))
		sunxi->power_ctl.en_tempctrl = !!val;
	if (!of_property_read_u32(np, "scene_ctrl_status", &val))
		sunxi->power_ctl.en_scenectrl = !!val;
	if (!of_property_read_u32(np, "max_normal_level", &val))
		if (val < sunxi->nr_of_vf && val >= 0)
			sunxi->power_ctl.max_normal_level = val;
}
#endif /* CONFIG_OF */

static int sunxi_protected_mode_enter(struct kbase_device *kbdev)
{

	set_reg_bit(0, 1, sunxi_mali->smc_drm_addr);

	return 0;
}

static int sunxi_protected_mode_reset(struct kbase_device *kbdev)
{

	set_reg_bit(0, 0, sunxi_mali->smc_drm_addr);

	return 0;
}

static bool sunxi_protected_mode_supported(struct kbase_device *kbdev)
{
	return true;
}

struct kbase_protected_ops sunxi_protected_ops = {
	.protected_mode_enter = sunxi_protected_mode_enter,
	.protected_mode_reset = sunxi_protected_mode_reset,
	.protected_mode_supported = sunxi_protected_mode_supported,
};

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int ret = 1;
	enum power_status status = POWER_GPU_STATUS;

	cancel_delayed_work_sync(&sunxi_mali->power_off_delay);

	if (sunxi_mali->do_dvfs) {
		struct aw_vf_table target_vf;

		sunxi_mali->do_dvfs = 0;
		wake_up(&sunxi_mali->dvfs_done);

		spin_lock(&sunxi_mali->target_lock);
		target_vf = sunxi_mali->target_vf;
		spin_unlock(&sunxi_mali->target_lock);

		sunxi_update_power(sunxi_mali, &target_vf);
	}
	sunxi_mali->power_ctl.power_on = 1;
	if (!sunxi_mali->power_ctl.en_idle)
		status |= POWER_MAN_STATUS;
	ret = sunxi_power_on(kbdev, status);

	return ret;
}

void pm_callback_power_resume(struct kbase_device *kbdev)
{
	return;
}
void pm_callback_power_suspend(struct kbase_device *kbdev)
{
	struct aw_vf_table target_vf;

	if (sunxi_mali->do_dvfs) {

		sunxi_mali->do_dvfs = 0;
		wake_up(&sunxi_mali->dvfs_done);

		spin_lock(&sunxi_mali->target_lock);
		target_vf = sunxi_mali->target_vf;
		spin_unlock(&sunxi_mali->target_lock);

		sunxi_update_power(sunxi_mali, &target_vf);
	}
	if (!sunxi_mali->dvfs_ing)
		schedule_delayed_work(&sunxi_mali->power_off_delay, sunxi_mali->delay_hz);

}

static void pm_callback_power_off(struct kbase_device *kbdev)
{

	if (sunxi_mali->do_dvfs) {
		struct aw_vf_table target_vf;

		sunxi_mali->do_dvfs = 0;
		wake_up(&sunxi_mali->dvfs_done);

		spin_lock(&sunxi_mali->target_lock);
		target_vf = sunxi_mali->target_vf;
		spin_unlock(&sunxi_mali->target_lock);

		sunxi_update_power(sunxi_mali, &target_vf);
	}
	/* JetCui:if you want down the power, you can set it to the most little vf */
	sunxi_mali->power_ctl.power_on = 0;
	schedule_delayed_work(&sunxi_mali->power_off_delay, sunxi_mali->delay_hz);
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_power_suspend,
	/* JetCui:why there is suspend, no resume?
	  * fix gpu driver resume do not open irq and other hardware,
	  * or you can modify kbase_pm_clock_on();
	  */
	.power_resume_callback = NULL,
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_off_callback = NULL,

};

static int sunxi_init_clock(struct sunxi_private *sunxi, struct device_node *np,
	unsigned long *freq, unsigned long *volt)
{
	const struct property *prop;
	const __be32 *val;
	int nr, j, i;
	struct aw_vf_table tmp;
	struct aw_vf_table *pvf_t, *pvf_l;

	sunxi = sunxi;
	prop = of_find_property(np, "operating-points", NULL);
	if (!prop)
		goto init_default;
	if (!prop->value)
		goto init_default;

	nr = prop->length / sizeof(u32);
	if (nr % 2)
		goto init_default;

	sunxi->nr_of_vf = nr / 2;
	val = prop->value;

	sunxi->vf_table = kzalloc(sizeof(struct aw_vf_table) * sunxi->nr_of_vf, GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi->vf_table)) {
		printk(KERN_ERR "Mali: failed to get GPU vf_table!\n");
		return -1;
	}

	val = prop->value;
	pvf_t = sunxi->vf_table;
	while (nr) {
		pvf_t->freq = be32_to_cpup(val++) * 1000;
		pvf_t->vol = be32_to_cpup(val++);
		pvf_t++;
		nr -= 2;
	}
	/* sort it  :little-->biggest*/
	nr = prop->length / sizeof(u32) / 2;
	j = 0;
	i = 0;
	while (--nr) {
		j = i;
		pvf_l = &sunxi->vf_table[i];
		while (j < nr) {
			pvf_t = &sunxi->vf_table[++j];
			if (pvf_t->freq < pvf_l->freq)
				pvf_l = pvf_t;
		}
		tmp = sunxi->vf_table[i];
		sunxi->vf_table[i] = *pvf_l;
		*pvf_l = tmp;
		sunxi_gpu_printf(1, "%d:(%ld,%ld)\n", i, sunxi->vf_table[i].vol / 1000,
			sunxi->vf_table[i].freq / 1000 / 1000);
		i++;
	}
	sunxi_gpu_printf(1, "%d:(%ld,%ld)\n", i, sunxi->vf_table[i].vol / 1000,
				sunxi->vf_table[i].freq / 1000 / 1000);
	*freq = sunxi->vf_table[i].freq;
	*volt = sunxi->vf_table[i].vol;
	sunxi->target_vf.freq = *freq;
	sunxi->target_vf.vol = *volt;

	return 0;

init_default:
	sunxi->vf_table = kzalloc(sizeof(struct aw_vf_table), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi->vf_table)) {
		printk(KERN_ERR "Mali: failed to get GPU vf_table!\n");
		return -1;
	}
	sunxi->vf_table[0].freq = *freq;
	sunxi->vf_table[0].vol = *volt;
	sunxi->nr_of_vf = 1;

	return 0;
}

unsigned long sunxi_get_initfreq(void)
{
	/* Jet Cui: we must make sure that the frq must in the opp table.*/
	return sunxi_mali->init_freq;
}

int kbase_platform_early_init(void)
{
	struct device_node *np;
	struct clk *gpu_core_clk;
	unsigned long freq = 624000000;
	unsigned long volt = 900000;
	int err = -EINVAL;

	sunxi_mali = kzalloc(sizeof(struct sunxi_private), GFP_KERNEL);
	if (IS_ERR_OR_NULL(sunxi_mali)) {
		printk(KERN_ERR "Mali: failed to get GPU device node!\n");
		goto failed;
	}

	np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (IS_ERR_OR_NULL(np)) {
		printk(KERN_ERR "Mali: failed to get GPU device node!\n");
		err = -EINVAL;
		goto failed;
	}
	if (sunxi_init_clock(sunxi_mali, np, &freq, &volt)) {
		err = -EINVAL;
		goto failed;
	}
#if defined(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND) && defined(CONFIG_MALI_DEVFREQ)
	sunxi_mali->power_ctl.en_dvfs = 1;
#endif
#if defined(CONFIG_SUNXI_GPU_COOLING)
	sunxi_mali->power_ctl.en_tempctrl = 1;
	sunxi_mali->power_ctl.tempctrl_target = sunxi_mali->nr_of_vf - 1;
#endif
	sunxi_mali->power_ctl.en_scenectrl = 0;
	sunxi_mali->debug = 0;
	sunxi_mali->power_ctl.max_normal_level = sunxi_mali->nr_of_vf - 1;
	sunxi_mali->power_gate_addr = ioremap(GPU_POWEROFF_GATING_REG, 1);
	sunxi_mali->smc_drm_addr = ioremap(SMC_GPU_DRM_REG, 1);

#ifdef CONFIG_OF
	sunxi_parse_fex(np, sunxi_mali);
#endif

#if defined(CONFIG_REGULATOR)
	sunxi_mali->regulator = regulator_get(NULL, "vdd-gpu");
	if (IS_ERR_OR_NULL(sunxi_mali->regulator)) {
		printk(KERN_ERR "Mali: Failed to get regulator!\n");
		err = -EINVAL;
		goto failed;
	}

	err = regulator_set_voltage(sunxi_mali->regulator, volt, volt);
	if (err) {
		printk(KERN_ERR "Mali: failed to increase voltage, err(%d)!\n", err);
		goto failed;
	}
#endif

	sunxi_mali->gpu_pll_clk = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(sunxi_mali->gpu_pll_clk)) {
		printk(KERN_ERR "Mali: failed to get GPU pll clock!\n");
		err = -EINVAL;
		goto failed;
	}

	gpu_core_clk = of_clk_get(np, 1);
	if (IS_ERR_OR_NULL(gpu_core_clk)) {
		printk(KERN_ERR "Mali: failed to get GPU core clock!\n");
		err = -EINVAL;
		goto failed;
	}

	clk_set_parent(gpu_core_clk, sunxi_mali->gpu_pll_clk);
	/* The initialized frequency should be in vf table */
	sunxi_mali->init_freq = freq;

	gpu_core_clk->flags |= CLK_SET_RATE_PARENT;
	err = clk_set_rate(sunxi_mali->gpu_pll_clk, freq);
	if (err) {
		printk(KERN_ERR "Mali: failed to set pll clock!\n");
		goto failed;
	}

	err = clk_set_rate(gpu_core_clk, freq);
	if (err) {
		printk(KERN_ERR "Mali: failed to set core clock!\n");
		goto failed;
	}

	sunxi_mali->cur_vf.freq = clk_get_rate(gpu_core_clk);
	sunxi_mali->cur_vf.vol = regulator_get_voltage(sunxi_mali->regulator);
	err = regulator_enable(sunxi_mali->regulator);
	if (err)
		printk(KERN_ERR "Mali: failed to enable GPU regulate!\n");

	sunxi_power_gate(0);

	if (!sunxi_mali->power_ctl.en_idle)
		sunxi_mali->power_ctl.power_status |= POWER_MAN_STATUS;

#ifdef CONFIG_SUNXI_GPU_COOLING
	INIT_DELAYED_WORK(&sunxi_mali->temp_donwn_delay, sunxi_temp_down_delay);
	sunxi_mali->last_stable_target = -1;
	mutex_init(&sunxi_mali->temp_lock);

#endif /* CONFIG_SUNXI_GPU_COOLING */
#ifdef CONFIG_DEBUG_FS
	sunxi_mali_creat_debug_fs();
#endif
	mutex_init(&sunxi_mali->power_lock);
	mutex_init(&sunxi_mali->ctrl_lock);
	spin_lock_init(&sunxi_mali->target_lock);

	init_waitqueue_head(&sunxi_mali->dvfs_done);
	INIT_DELAYED_WORK(&sunxi_mali->power_off_delay, sunxi_poweroff_delay);

#if IS_ENABLED(CONFIG_DEVFREQ_GOV_SIMPLE_ONDEMAND)
	sunxi_mali->sunxi_date.downdifferential = SUNXI_DFSO_DOWNDIF;
	sunxi_mali->sunxi_date.upthreshold = SUNXI_DFSO_UPTHRESHOLD;
#endif
	sunxi_mali->power_ctl.bk_threshold = sunxi_mali->nr_of_vf -
				(sunxi_mali->nr_of_vf / 10);
	sunxi_mali->delay_hz = POWER_DOWN_DELAY;

	sunxi_gpu_printf(1, "power:(%ld,%ld)==>(%d,%ld)"
			"en:%d  bk:%d\n",
			volt/1000, freq/1000/1000,
			regulator_get_voltage(sunxi_mali->regulator) / 1000,
			clk_get_rate(gpu_core_clk) / 1000 / 1000,
			regulator_is_enabled(sunxi_mali->regulator),
			sunxi_mali->power_ctl.bk_threshold);

	return 0;
failed:
	kfree(sunxi_mali);
	return err;
}

int sunxi_platform_init(struct kbase_device *kbdev)
{
	sunxi_mali->kbase_mali = (void *)kbdev;
	kbdev->regulator = sunxi_mali->regulator;
	return 0;
}

void sunxi_platform_term(struct kbase_device *kbdev)
{
#ifdef CONFIG_SUNXI_GPU_COOLING
	gpu_thermal_cool_unregister();
#endif
#ifdef CONFIG_DEBUG_FS
	debugfs_remove_recursive(sunxi_mali->my_debug);
#endif
	cancel_delayed_work_sync(&sunxi_mali->power_off_delay);
	iounmap(sunxi_mali->power_gate_addr);
	iounmap(sunxi_mali->smc_drm_addr);
	kfree(sunxi_mali->vf_table);
	kfree(sunxi_mali);
	sunxi_mali = NULL;
	printk(KERN_INFO "sunxi_mali term.\n");
}

