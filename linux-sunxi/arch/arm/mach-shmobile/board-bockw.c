/*
 * Bock-W board support
 *
 * Copyright (C) 2013  Renesas Solutions Corp.
 * Copyright (C) 2013  Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/platform_device.h>
#include <linux/smsc911x.h>
#include <mach/common.h>
#include <mach/irqs.h>
#include <mach/r8a7778.h>
#include <asm/mach/arch.h>

static struct smsc911x_platform_config smsc911x_data = {
	.irq_polarity	= SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
	.irq_type	= SMSC911X_IRQ_TYPE_PUSH_PULL,
	.flags		= SMSC911X_USE_32BIT,
	.phy_interface	= PHY_INTERFACE_MODE_MII,
};

static struct resource smsc911x_resources[] = {
	DEFINE_RES_MEM(0x18300000, 0x1000),
	DEFINE_RES_IRQ(irq_pin(0)), /* IRQ 0 */
};

#define IRQ0MR	0x30
static void __init bockw_init(void)
{
	void __iomem *fpga;

	r8a7778_clock_init();
	r8a7778_init_irq_extpin(1);
	r8a7778_add_standard_devices();

	fpga = ioremap_nocache(0x18200000, SZ_1M);
	if (fpga) {
		/*
		 * CAUTION
		 *
		 * IRQ0/1 is cascaded interrupt from FPGA.
		 * it should be cared in the future
		 * Now, it is assuming IRQ0 was used only from SMSC.
		 */
		u16 val = ioread16(fpga + IRQ0MR);
		val &= ~(1 << 4); /* enable SMSC911x */
		iowrite16(val, fpga + IRQ0MR);
		iounmap(fpga);

		platform_device_register_resndata(
			&platform_bus, "smsc911x", -1,
			smsc911x_resources, ARRAY_SIZE(smsc911x_resources),
			&smsc911x_data, sizeof(smsc911x_data));
	}
}

static const char *bockw_boards_compat_dt[] __initdata = {
	"renesas,bockw",
	NULL,
};

DT_MACHINE_START(BOCKW_DT, "bockw")
	.init_early	= r8a7778_init_delay,
	.init_irq	= r8a7778_init_irq_dt,
	.init_machine	= bockw_init,
	.init_time	= shmobile_timer_init,
	.dt_compat	= bockw_boards_compat_dt,
MACHINE_END
