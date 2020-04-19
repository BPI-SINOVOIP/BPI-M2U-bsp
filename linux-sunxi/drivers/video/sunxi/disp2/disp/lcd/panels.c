#include "panels.h"

struct sunxi_lcd_drv g_lcd_drv;

__lcd_panel_t* panel_array[] = {
#if defined(CONFIG_ARCH_SUN50IW3)
	&ls029b3sx02_panel,
	&vr_ls055t1sx01_panel,
	&lq101r1sx03_panel,
	&he0801a068_panel,
	&sl008pn21d_panel,
#else
	&default_eink,
	&default_panel,
	&lt070me05000_panel,
	&wtq05027d01_panel,
	&t27p06_panel,
	&dx0960be40a1_panel,
	&tft720x1280_panel,
	&S6D7AA0X01_panel,
	&gg1p4062utsw_panel,
	&vr_sharp_panel,
	&ls029b3sx02_panel,
	&S070WV20_MIPI_RGB_panel,
	&he0801a068_panel,
#endif /*endif CONFIG_ARCH_SUN50IW3 */
	/* add new panel below */
	NULL,
};

static void lcd_set_panel_funs(void)
{
	int i;

	for (i=0; panel_array[i] != NULL; i++) {
		sunxi_lcd_set_panel_funs(panel_array[i]->name, &panel_array[i]->func);
	}

	return ;
}

int lcd_init(void)
{
	sunxi_disp_get_source_ops(&g_lcd_drv.src_ops);
	lcd_set_panel_funs();

	return 0;
}
