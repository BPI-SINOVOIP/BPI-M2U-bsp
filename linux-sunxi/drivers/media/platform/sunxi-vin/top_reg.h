
#ifndef __CSIC__TOP__REG__H__
#define __CSIC__TOP__REG__H__

#include <linux/types.h>

#define MAX_CSIC_TOP_NUM 2

/*register value*/

/*register data struct*/

struct csic_feature_list {
	unsigned int dma_num;
	unsigned int vipp_num;
	unsigned int isp_num;
	unsigned int ncsi_num;
	unsigned int mcsi_num;
	unsigned int parser_num;
};

struct csic_version {
	unsigned int ver_big;
	unsigned int ver_small;
};

int csic_top_set_base_addr(unsigned int sel, unsigned long addr);
void csic_top_enable(unsigned int sel);
void csic_top_disable(unsigned int sel);
void csic_top_wdr_mode(unsigned int sel, unsigned int en);
void csic_top_sram_pwdn(unsigned int sel, unsigned int en);
void csic_top_switch_ncsi(unsigned int sel, unsigned int en);
void csic_top_version_read_en(unsigned int sel, unsigned int en);

void csic_isp_input_select(unsigned int sel, unsigned int isp, unsigned int in,
				unsigned int psr, unsigned int ch);
void csic_vipp_input_select(unsigned int sel, unsigned int vipp,
				unsigned int isp, unsigned int ch);

void csic_feature_list_get(unsigned int sel, struct csic_feature_list *fl);
void csic_version_get(unsigned int sel, struct csic_version *v);

#endif /* __CSIC__TOP__REG__H__ */
