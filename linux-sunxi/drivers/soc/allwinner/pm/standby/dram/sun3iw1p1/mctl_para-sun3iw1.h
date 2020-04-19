#ifndef _MCTL_PARA_H
#define _MCTL_PARA_H

/*#include <linux/kernel.h>*/

typedef enum __DRAM_TYPE {
	DRAM_TYPE_SDR = 0,
	DRAM_TYPE_DDR = 1,
	DRAM_TYPE_MDDR = 2,
} __dram_type_t;

typedef struct __DRAM_PARA {
	unsigned int	base;	/* dram base address */
	unsigned int	size;	/* dram size, based on     (unit: MByte) */
	unsigned int	clk;	/* dram work clock         (unit: MHz) */
	unsigned int	access_mode;/* 0: interleave mode 1: sequence mode */
	unsigned int	cs_num;	/* dram chip count  1: one chip  2: two chip */
	unsigned int	ddr8_remap;/* 8bits data width DDR 0: normal 1: 8bits */
	__dram_type_t	sdr_ddr;	/* ddr/ddr2/sdr/mddr/lpddr/... */
	unsigned int	bwidth;		/* dram bus width */
	unsigned int	col_width;	/* column address width */
	unsigned int	row_width;	/* row address width */
	unsigned int	bank_size;	/* dram bank count */
	unsigned int	cas;		/* dram cas */
} __dram_para_t;

#endif	/*_MCTL_HAL_H*/
