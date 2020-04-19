/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * (C) Copyright 2002-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
/* TODO: can we just include all these headers whether needed or not? */
#if defined(CONFIG_CMD_BEDBUG)
#include <bedbug/type.h>
#endif
#ifdef CONFIG_HAS_DATAFLASH
#include <dataflash.h>
#endif
#include <dm.h>
#include <environment.h>
#include <fdtdec.h>
#if defined(CONFIG_CMD_IDE)
#include <ide.h>
#endif
#include <initcall.h>
#ifdef CONFIG_PS2KBD
#include <keyboard.h>
#endif
#if defined(CONFIG_CMD_KGDB)
#include <kgdb.h>
#endif
#include <logbuff.h>
#include <malloc.h>
#ifdef CONFIG_BITBANGMII
#include <miiphy.h>
#endif
#include <mmc.h>
#include <nand.h>
#include <onenand_uboot.h>
#include <scsi.h>
#include <serial.h>
#include <spi.h>
#include <stdio_dev.h>
#include <trace.h>
#include <watchdog.h>
#ifdef CONFIG_ADDR_MAP
#include <asm/mmu.h>
#endif
#include <asm/sections.h>
#ifdef CONFIG_X86
#include <asm/init_helpers.h>
#endif
#include <dm/root.h>
#include <linux/compiler.h>
#include <linux/err.h>
#include <fdt_support.h>

#if defined(CONFIG_ALLWINNER)
#include <private_uboot.h>
#include <sunxi_board.h>
#include <sys_partition.h>
#include <sys_config.h>
#include <sys_config_old.h>
#include <smc.h>
#include <securestorage.h>
#include <arisc.h>
#ifdef CONFIG_SUNXI_MULITCORE_BOOT
#include <asm/arch/platsmp.h>
#include <cputask.h>
#endif
#endif

DECLARE_GLOBAL_DATA_PTR;

ulong monitor_flash_len;
__attribute__((section(".data")))
volatile int flash_init_flag;

void sunxi_multi_core_boot(void);

int __board_flash_wp_on(void)
{
	/*
	 * Most flashes can't be detected when write protection is enabled,
	 * so provide a way to let U-Boot gracefully ignore write protected
	 * devices.
	 */
	return 0;
}

int board_flash_wp_on(void)
	__attribute__ ((weak, alias("__board_flash_wp_on")));


__weak int sunxi_hdcp_keybox_install(void)
{
	return 0;
}

__weak int sunxi_widevine_keybox_install(void)
{
	return 0;
}

__weak int sunxi_get_hdcp_cfg(void)
{
	return 0;
}

static inline void set_sunxi_flash_init_flag(void)
{
	flash_init_flag = 1;
}
static inline int get_sunxi_flash_init_flag(void)
{
	return flash_init_flag;
}


static int initr_trace(void)
{
#ifdef CONFIG_TRACE
	trace_init(gd->trace_buff, CONFIG_TRACE_BUFFER_SIZE);
#endif

	return 0;
}

static int initr_reloc(void)
{
	set_working_fdt_addr((void*)gd->fdt_blob);

	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */
	bootstage_mark_name(BOOTSTAGE_ID_START_UBOOT_R, "board_init_r");
	return 0;
}

#ifdef CONFIG_ARM
/*
 * Some of these functions are needed purely because the functions they
 * call return void. If we change them to return 0, these stubs can go away.
 */
static int initr_caches(void)
{
	/* Enable caches */
	enable_caches();
	return 0;
}
#endif

__weak int fixup_cpu(void)
{
	return 0;
}

static int initr_reloc_global_data(void)
{
#ifdef __ARM__

	//ulong head_align_size = get_spare_head_size();
	monitor_flash_len = _end - __image_copy_start;// + head_align_size;

#elif !defined(CONFIG_SANDBOX)
	monitor_flash_len = (ulong)&__init_end - gd->relocaddr;
#endif
#if defined(CONFIG_MPC85xx) || defined(CONFIG_MPC86xx)
	/*
	 * The gd->cpu pointer is set to an address in flash before relocation.
	 * We need to update it to point to the same CPU entry in RAM.
	 * TODO: why not just add gd->reloc_ofs?
	 */
	gd->arch.cpu += gd->relocaddr - CONFIG_SYS_MONITOR_BASE;

	/*
	 * If we didn't know the cpu mask & # cores, we can save them of
	 * now rather than 'computing' them constantly
	 */
	fixup_cpu();
#endif
#ifdef CONFIG_SYS_EXTRA_ENV_RELOC
	/*
	 * Some systems need to relocate the env_addr pointer early because the
	 * location it points to will get invalidated before env_relocate is
	 * called.  One example is on systems that might use a L2 or L3 cache
	 * in SRAM mode and initialize that cache from SRAM mode back to being
	 * a cache in cpu_init_r.
	 */
	gd->env_addr += gd->relocaddr - CONFIG_SYS_MONITOR_BASE;
#endif
	return 0;
}

static int initr_serial(void)
{
	serial_initialize();
	return 0;
}

#ifdef CONFIG_AUTO_UPDATE
extern int auto_update_check(void);
#endif

extern void mem_noncache_malloc_init(uint noncache_start, uint noncache_size);
static int initr_malloc(void)
{
	ulong malloc_start;

	malloc_start = gd->relocaddr - TOTAL_MALLOC_LEN;
	mem_malloc_init((ulong)map_sysmem(malloc_start, TOTAL_MALLOC_LEN),
			TOTAL_MALLOC_LEN);
	debug("malloc start addr is %lx, size %dk \n",malloc_start,TOTAL_MALLOC_LEN>>10 );

#ifdef  CONFIG_NONCACHE_MEMORY
{
	mem_noncache_malloc_init(gd->malloc_noncache_start, CONFIG_NONCACHE_MEMORY_SIZE);
	debug("no cache malloc start addr is %lx, size %dk \n",gd->malloc_noncache_start,TOTAL_MALLOC_LEN>>10 );
}
#endif

	return 0;
}

__weak int power_init_board(void)
{
	extern int axp_reinit(void);
	axp_reinit();
	return 0;
}

static int initr_announce(void)
{
	debug("Now running in RAM - U-Boot at: %08lx\n", gd->relocaddr);
	return 0;
}



#ifdef CONFIG_GENERIC_MMC
int initr_mmc(void)
{
	puts("MMC:   ");
	mmc_initialize(gd->bd);
	return 0;
}
#endif


/*
 * Tell if it's OK to load the environment early in boot.
 *
 * If CONFIG_OF_CONFIG is defined, we'll check with the FDT to see
 * if this is OK (defaulting to saying it's OK).
 *
 * NOTE: Loading the environment early can be a bad idea if security is
 *       important, since no verification is done on the environment.
 *
 * @return 0 if environment should not be loaded, !=0 if it is ok to load
 */
static int should_load_env(void)
{
#ifdef CONFIG_OF_CONTROL
	return fdtdec_get_config_int(gd->fdt_blob, "load-environment", 1);
#elif defined CONFIG_DELAY_ENVIRONMENT
	return 0;
#else
	return 1;
#endif
}

static int initr_env(void)
{
	/* initialize environment */
	if (should_load_env())
		env_relocate();
	else
		set_default_env(NULL);

	/* Initialize from environment */
	load_addr = getenv_ulong("loadaddr", 16, load_addr);
	return 0;
}

static int initr_jumptable(void)
{
	jumptable_init();
	return 0;
}


/* enable exceptions */
#ifdef CONFIG_ARM
static int initr_enable_interrupts(void)
{
	enable_interrupts();
	return 0;
}
#endif


static int run_main_loop(void)
{
	pr_msg("inter uboot shell\n");
	/* main_loop() can return to retry autoboot, if so just run it again */
	for (;;)
		main_loop();
	return 0;
}

extern int script_init(void);
extern int PowerCheck(void);
extern int sunxi_keydata_burn_by_usb(void);

static int initr_sunxi_base(void)
{
	board_late_init();
	return 0;
}

static int sunxi_burn_key(void)
{
	pr_msg("try to burn key\n");
#ifdef CONFIG_SUNXI_KEY_BURN
	sunxi_keydata_burn_by_usb();
#endif
	return 0;
}



int initr_sunxi_display(void)
{
#ifdef CONFIG_SUNXI_DISPLAY

	int workmode = get_boot_work_mode();
	if ((workmode == WORK_MODE_USB_PRODUCT) ||
		(workmode == WORK_MODE_USB_UPDATE) ||
		(workmode == WORK_MODE_USB_DEBUG)) {
		return 0;
	}
	pr_msg("display init start\n");
	drv_disp_init();

	/* wait flash init for prepare hdcp key */
	if (sunxi_get_hdcp_cfg())
		while (!get_sunxi_flash_init_flag())
			;


#ifndef CONFIG_BOOT_GUI
	board_display_device_open();
	board_display_layer_request();
#else
	disp_devices_open();
	fb_init();
#endif
	pr_msg("display init end\n");
#endif
	return 0;
}



static int initr_sunxi_flash(void)
{
	int ret = 0;

	pr_msg("flash init start\n");
	ret = sunxi_flash_handle_init();
	if (!ret) {
#ifdef CONFIG_SUNXI_UBIFS
		sunxi_nand_mtd_init();
#endif
		sunxi_partition_init();
	}
	/* install key box for DRM */
	sunxi_hdcp_keybox_install();
	sunxi_widevine_keybox_install();
	/* flash ready, set the flag */
	set_sunxi_flash_init_flag();
	pr_msg("flash init end\n");

	return ret;
}

#ifdef CONFIG_SUNXI_MULITCORE_BOOT

static int initr_multi_core(void)
{

	if (get_boot_work_mode() == WORK_MODE_BOOT) {

		/* we use multi-core only at boot mode */
		sunxi_multi_core_boot();
		if (initr_sunxi_flash())
			return -1;
		return 0;

	}

	/* we should init disp for other boot mode: card burn mode */
	if (initr_sunxi_flash())
		return -1;
	initr_sunxi_display();


	return 0;
}
#else
static int initr_single_core(void)
{

	/* the init for flash shoul be eralier than
	    disp because of hdcp key */
	if (sunxi_get_hdcp_cfg()) {
		if (initr_sunxi_flash())
			return -1;
		initr_sunxi_display();
	} else {
		initr_sunxi_display();
		if (initr_sunxi_flash())
			return -1;
	}

	return 0;
}
#endif

static int platform_dma_init(void)
{
#ifdef CONFIG_SUNXI_DMA
	extern void sunxi_dma_init(void);
	sunxi_dma_init();
#endif
	return 0;
}
static int usb_net_init(void)
{
#if defined(CONFIG_CMD_NET)
	puts("Net:   ");
	eth_initialize(gd->bd);
#if defined(CONFIG_RESET_PHY_R)
	debug("Reset Ethernet PHY\n");
	reset_phy();
#endif
#endif
	return 0;
}
__weak int show_platform_info(void)
{
	return 0;
}

#ifndef CONFIG_SUNXI_MODULE_AXP
static int sunxi_show_logo(void)
{
#if defined(CONFIG_SUNXI_DISPLAY)
	printf("sunxi_bmp_logo_display\n");
#ifdef CONFIG_SUNXI_LOAD_JPEG
	sunxi_load_jpeg("bootlogo.jpg");
#else
	sunxi_bmp_display("bootlogo.bmp");
#endif
#endif /*endif CONFIG_SUNXI_DISPLAY */
	return 0;
}
#endif
/*
 * Over time we hope to remove these functions with code fragments and
 * stub funtcions, and instead call the relevant function directly.
 *
 * We also hope to remove most of the driver-related init and do it if/when
 * the driver is later used.
 *
 * TODO: perhaps reset the watchdog in the initcall function after each call?
 */
init_fnc_t init_sequence_r[] = {
	initr_trace,
	initr_reloc,
	initr_serial,
	/* TODO: could x86/PPC have this also perhaps? */
#ifdef CONFIG_ARM
	initr_caches,
	board_init,	/* Setup chipselects */
#endif
	/*
	 * TODO: printing of the clock inforamtion of the board is now
	 * implemented as part of bdinfo command. Currently only support for
	 * davinci SOC's is added. Remove this check once all the board
	 * implement this.
	 */
	initr_reloc_global_data,
	initr_announce,
	show_platform_info,
	initr_malloc,
	script_init,
	bootstage_relocate,
	power_init_board,
	stdio_init,
	initr_jumptable,
	console_init_r,		/* fully init console as a device */
	interrupt_init,

#if defined(CONFIG_ARM) || defined(CONFIG_x86)
	initr_enable_interrupts,
#endif

#ifdef CONFIG_SUNXI
	platform_dma_init,

#ifdef CONFIG_AUTO_UPDATE
	auto_update_check,
#endif

#ifdef CONFIG_SUNXI_MULITCORE_BOOT
	initr_multi_core,
#else
	initr_single_core,
#endif
	sunxi_burn_key,
	initr_env,
	initr_sunxi_base,

#ifndef CONFIG_SUNXI_MULITCORE_BOOT
#ifndef CONFIG_SUNXI_MODULE_AXP
	sunxi_show_logo,
#else
	PowerCheck,
#endif
#endif /*END CONFIG_SUNXI_MULITCORE_BOOT*/

#endif
	usb_net_init,
	run_main_loop,
};

extern ulong _reset_vec;
extern ulong _undefined_instruction;
extern ulong _software_interrupt;
extern ulong _prefetch_abort;
extern ulong _data_abort;
extern ulong _not_used;
extern ulong _irq;
extern ulong _fiq;

#ifndef CONFIG_ARM64
#ifdef CONFIG_USE_IRQ
void intr_vector_fix(void)
{
	int i = 0;
	ulong * vec_array[8] = {0};
	vec_array[i++] = &_reset_vec;
	vec_array[i++] = &_undefined_instruction;
	vec_array[i++] = &_software_interrupt;
	vec_array[i++] = &_prefetch_abort;
	vec_array[i++] = &_data_abort;
	vec_array[i++] = &_not_used;
	vec_array[i++] = &_irq;
	vec_array[i++] = &_fiq;

	for(i =0; i < 8; i++)
	{
		debug("before fix: vector %d, addr %lx ,value 0x%lx\n", i ,(ulong)vec_array[i], *(vec_array[i]));
		*(vec_array[i]) +=  (gd->reloc_off);
		debug("after  fix: vector %d, addr %lx ,value 0x%lx\n", i ,(ulong)vec_array[i], *(vec_array[i]));
	}
#ifdef  CONFIG_ARCH_SUN3IW1P1
    memcpy((void*) 0, (const void *) _start, 1024);
#endif
}
#endif
#endif

void board_init_r(gd_t *new_gd, ulong dest_addr)
{

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	int i;
#endif

#ifndef CONFIG_X86
	gd = new_gd;
#endif

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	for (i = 0; i < ARRAY_SIZE(init_sequence_r); i++)
		init_sequence_r[i] += gd->reloc_off;
#endif

#ifndef CONFIG_ARM64
#ifdef CONFIG_USE_IRQ
	intr_vector_fix();
#endif
#endif
	if (initcall_run_list(init_sequence_r))
		hang();

	/* NOTREACHED - run_main_loop() does not return */
	hang();
}
