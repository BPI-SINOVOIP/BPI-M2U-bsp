/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Copyright (C) 2012 ARM Limited
 *
 * Author: Will Deacon <will.deacon@arm.com>
 */

#define pr_fmt(fmt) "psci: " fmt

#include <linux/init.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/reboot.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <uapi/linux/psci.h>

#include <asm/compiler.h>
#include <asm/errno.h>
#include <asm/psci.h>
#include <asm/smp_plat.h>
#include <asm/suspend.h>
#include <asm/system_misc.h>
#include <asm/cpuidle.h>

struct psci_operations psci_ops;

static int (*invoke_psci_fn)(u32, u32, u32, u32);
typedef int (*psci_initcall_t)(const struct device_node *);

asmlinkage int __invoke_psci_fn_hvc(u32, u32, u32, u32);
asmlinkage int __invoke_psci_fn_smc(u32, u32, u32, u32);

enum psci_function {
	PSCI_FN_CPU_SUSPEND,
	PSCI_FN_CPU_ON,
	PSCI_FN_CPU_OFF,
	PSCI_FN_MIGRATE,
	PSCI_FN_AFFINITY_INFO,
	PSCI_FN_MIGRATE_INFO_TYPE,
	PSCI_FN_MAX,
};

static DEFINE_PER_CPU_READ_MOSTLY(struct psci_power_state *, psci_power_state);

static u32 psci_function_id[PSCI_FN_MAX];

static int psci_to_linux_errno(int errno)
{
	switch (errno) {
	case PSCI_RET_SUCCESS:
		return 0;
	case PSCI_RET_NOT_SUPPORTED:
		return -EOPNOTSUPP;
	case PSCI_RET_INVALID_PARAMS:
		return -EINVAL;
	case PSCI_RET_DENIED:
		return -EPERM;
	};

	return -EINVAL;
}

static u32 psci_power_state_pack(struct psci_power_state state)
{
	return ((state.id << PSCI_0_2_POWER_STATE_ID_SHIFT)
			& PSCI_0_2_POWER_STATE_ID_MASK) |
		((state.type << PSCI_0_2_POWER_STATE_TYPE_SHIFT)
		 & PSCI_0_2_POWER_STATE_TYPE_MASK) |
		((state.affinity_level << PSCI_0_2_POWER_STATE_AFFL_SHIFT)
		 & PSCI_0_2_POWER_STATE_AFFL_MASK);
}

static void psci_power_state_unpack(u32 power_state,
				    struct psci_power_state *state)
{
	state->id = (power_state & PSCI_0_2_POWER_STATE_ID_MASK) >>
			PSCI_0_2_POWER_STATE_ID_SHIFT;
	state->type = (power_state & PSCI_0_2_POWER_STATE_TYPE_MASK) >>
			PSCI_0_2_POWER_STATE_TYPE_SHIFT;
	state->affinity_level =
			(power_state & PSCI_0_2_POWER_STATE_AFFL_MASK) >>
			PSCI_0_2_POWER_STATE_AFFL_SHIFT;
}

static int psci_get_version(void)
{
	int err;

	err = invoke_psci_fn(PSCI_0_2_FN_PSCI_VERSION, 0, 0, 0);
	return err;
}

static int psci_cpu_suspend(struct psci_power_state state,
			    unsigned long entry_point)
{
	int err;
	u32 fn, power_state;

	fn = psci_function_id[PSCI_FN_CPU_SUSPEND];
	power_state = psci_power_state_pack(state);
	err = invoke_psci_fn(fn, power_state, entry_point, 0);
	return psci_to_linux_errno(err);
}

static int psci_cpu_off(struct psci_power_state state)
{
	int err;
	u32 fn, power_state;

	fn = psci_function_id[PSCI_FN_CPU_OFF];
	power_state = psci_power_state_pack(state);
	err = invoke_psci_fn(fn, power_state, 0, 0);
	return psci_to_linux_errno(err);
}

static int psci_cpu_on(unsigned long cpuid, unsigned long entry_point)
{
	int err;
	u32 fn;

	fn = psci_function_id[PSCI_FN_CPU_ON];
	err = invoke_psci_fn(fn, cpuid, entry_point, 0);
	return psci_to_linux_errno(err);
}

static int psci_migrate(unsigned long cpuid)
{
	int err;
	u32 fn;

	fn = psci_function_id[PSCI_FN_MIGRATE];
	err = invoke_psci_fn(fn, cpuid, 0, 0);
	return psci_to_linux_errno(err);
}

static int psci_affinity_info(unsigned long target_affinity,
		unsigned long lowest_affinity_level)
{
	int err;
	u32 fn;

	fn = psci_function_id[PSCI_FN_AFFINITY_INFO];
	err = invoke_psci_fn(fn, target_affinity, lowest_affinity_level, 0);
	return err;
}

static int psci_migrate_info_type(void)
{
	int err;
	u32 fn;

	fn = psci_function_id[PSCI_FN_MIGRATE_INFO_TYPE];
	err = invoke_psci_fn(fn, 0, 0, 0);
	return err;
}

static int __maybe_unused cpu_psci_cpu_init_idle(struct device_node *cpu_node,
						 int cpu)
{
	int i, ret, count = 0;
	struct psci_power_state *psci_states;
	struct device_node *state_node;

	/*
	 * If the PSCI cpu_suspend function hook has not been initialized
	 * idle states must not be enabled, so bail out
	 */
	if (!psci_ops.cpu_suspend)
		return -EOPNOTSUPP;

	/* Count idle states */
	while ((state_node = of_parse_phandle(cpu_node, "cpu-idle-states",
					      count))) {
		count++;
		of_node_put(state_node);
	}

	if (!count)
		return -ENODEV;

	psci_states = kcalloc(count, sizeof(*psci_states), GFP_KERNEL);
	if (!psci_states)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		u32 psci_power_state;

		state_node = of_parse_phandle(cpu_node, "cpu-idle-states", i);

		ret = of_property_read_u32(state_node,
					   "arm,psci-suspend-param",
					   &psci_power_state);
		if (ret) {
			pr_warn(" * %s missing arm,psci-suspend-param property\n",
				state_node->full_name);
			of_node_put(state_node);
			goto free_mem;
		}

		of_node_put(state_node);
		pr_debug("psci-power-state %#x index %d\n", psci_power_state,
							    i);
		psci_power_state_unpack(psci_power_state, &psci_states[i]);
	}
	/* Idle states parsed correctly, initialize per-cpu pointer */
	per_cpu(psci_power_state, cpu) = psci_states;
	return 0;

free_mem:
	kfree(psci_states);
	return ret;
}

static int get_set_conduit_method(struct device_node *np)
{
	const char *method;

	pr_info("probing for conduit method from DT.\n");

	if (of_property_read_string(np, "method", &method)) {
		pr_warn("missing \"method\" property\n");
		return -ENXIO;
	}

	if (!strcmp("hvc", method)) {
		invoke_psci_fn = __invoke_psci_fn_hvc;
	} else if (!strcmp("smc", method)) {
		invoke_psci_fn = __invoke_psci_fn_smc;
	} else {
		pr_warn("invalid \"method\" property: %s\n", method);
		return -EINVAL;
	}
	return 0;
}

static void psci_sys_reset(enum reboot_mode reboot_mode, const char *cmd)
{
	invoke_psci_fn(PSCI_0_2_FN_SYSTEM_RESET, 0, 0, 0);
}

static void psci_sys_poweroff(void)
{
	invoke_psci_fn(PSCI_0_2_FN_SYSTEM_OFF, 0, 0, 0);
}

/*
 * PSCI Function IDs for v0.2+ are well defined so use
 * standard values.
 */
static int psci_0_2_init(struct device_node *np)
{
	int err, ver;

	err = get_set_conduit_method(np);

	if (err)
		goto out_put_node;

	ver = psci_get_version();

	if (ver == PSCI_RET_NOT_SUPPORTED) {
		/* PSCI v0.2 mandates implementation of PSCI_ID_VERSION. */
		pr_err("PSCI firmware does not comply with the v0.2 spec.\n");
		err = -EOPNOTSUPP;
		goto out_put_node;
	} else {
		pr_info("PSCIv%d.%d detected in firmware.\n",
				PSCI_VERSION_MAJOR(ver),
				PSCI_VERSION_MINOR(ver));

		if (PSCI_VERSION_MAJOR(ver) == 0 &&
				PSCI_VERSION_MINOR(ver) < 2) {
			err = -EINVAL;
			pr_err("Conflicting PSCI version detected.\n");
			goto out_put_node;
		}
	}

	pr_info("Using standard PSCI v0.2 function IDs\n");
	psci_function_id[PSCI_FN_CPU_SUSPEND] = PSCI_0_2_FN_CPU_SUSPEND;
	psci_ops.cpu_suspend = psci_cpu_suspend;

	psci_function_id[PSCI_FN_CPU_OFF] = PSCI_0_2_FN_CPU_OFF;
	psci_ops.cpu_off = psci_cpu_off;

	psci_function_id[PSCI_FN_CPU_ON] = PSCI_0_2_FN_CPU_ON;
	psci_ops.cpu_on = psci_cpu_on;

	psci_function_id[PSCI_FN_MIGRATE] = PSCI_0_2_FN_MIGRATE;
	psci_ops.migrate = psci_migrate;

	psci_function_id[PSCI_FN_AFFINITY_INFO] = PSCI_0_2_FN_AFFINITY_INFO;
	psci_ops.affinity_info = psci_affinity_info;

	psci_function_id[PSCI_FN_MIGRATE_INFO_TYPE] =
		PSCI_0_2_FN_MIGRATE_INFO_TYPE;
	psci_ops.migrate_info_type = psci_migrate_info_type;

	arm_pm_restart = psci_sys_reset;

	pm_power_off = psci_sys_poweroff;

out_put_node:
	of_node_put(np);
	return err;
}

/*
 * PSCI < v0.2 get PSCI Function IDs via DT.
 */
static int psci_0_1_init(struct device_node *np)
{
	u32 id;
	int err;

	err = get_set_conduit_method(np);

	if (err)
		goto out_put_node;

	pr_info("Using PSCI v0.1 Function IDs from DT\n");

	if (!of_property_read_u32(np, "cpu_suspend", &id)) {
		psci_function_id[PSCI_FN_CPU_SUSPEND] = id;
		psci_ops.cpu_suspend = psci_cpu_suspend;
	}

	if (!of_property_read_u32(np, "cpu_off", &id)) {
		psci_function_id[PSCI_FN_CPU_OFF] = id;
		psci_ops.cpu_off = psci_cpu_off;
	}

	if (!of_property_read_u32(np, "cpu_on", &id)) {
		psci_function_id[PSCI_FN_CPU_ON] = id;
		psci_ops.cpu_on = psci_cpu_on;
	}

	if (!of_property_read_u32(np, "migrate", &id)) {
		psci_function_id[PSCI_FN_MIGRATE] = id;
		psci_ops.migrate = psci_migrate;
	}

out_put_node:
	of_node_put(np);
	return err;
}

static const struct of_device_id psci_of_match[] __initconst = {
	{ .compatible = "arm,psci", .data = psci_0_1_init},
	{ .compatible = "arm,psci-0.2", .data = psci_0_2_init},
	{},
};

int __init psci_init(void)
{
	struct device_node *np;
	const struct of_device_id *matched_np;
	psci_initcall_t init_fn;

	np = of_find_matching_node_and_match(NULL, psci_of_match, &matched_np);
	if (!np)
		return -ENODEV;

	init_fn = (psci_initcall_t)matched_np->data;
	return init_fn(np);
}

#ifdef CONFIG_CPU_IDLE
static int psci_suspend_finisher(unsigned long index)
{
	struct psci_power_state *state = __get_cpu_var(psci_power_state);

	return psci_ops.cpu_suspend(state[index - 1],
				    virt_to_phys(cpu_resume));
}

static int __maybe_unused cpu_psci_cpu_suspend(int cpu, unsigned long index)
{
	int ret;
	struct psci_power_state *state = __get_cpu_var(psci_power_state);

	printk("%s-%u, %d, %lu\n", __func__, __LINE__, cpu, index);
	/*
	 * idle state index 0 corresponds to wfi, should never be called
	 * from the cpu_suspend operations
	 */
	if (WARN_ON_ONCE(!index))
		return -EINVAL;

	if (state[index - 1].type == PSCI_POWER_STATE_TYPE_STANDBY)
		ret = psci_ops.cpu_suspend(state[index - 1], 0);
	else
		ret = __cpu_suspend(index, psci_suspend_finisher);

	printk("%s-%u, %d, %lu\n", __func__, __LINE__, cpu, index);
	return ret;
}

static struct cpuidle_ops arm_cpuidle_ops __initdata = {
	.suspend = cpu_psci_cpu_suspend,
	.init = cpu_psci_cpu_init_idle,
};

CPUIDLE_METHOD_OF_DECLARE(arm, "psci", &arm_cpuidle_ops);
#endif /* CONFIG_CPU_IDLE */
