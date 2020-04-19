/*
 * Some macro and struct of sunxi key ladder driver.
 *
 * Copyright (C) 2017 Allwinner.
 *
 * zhouhuacai <zhouhuacai@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _SUNXI_SS_KL_H_
#define _SUNXI_SS_KL_H_

/* for debug */
/*#define SUNXI_KL_DEBUG*/

#define KL_DBG(fmt, arg...) pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define KL_ERR(fmt, arg...) pr_err("%s()%d - "fmt, __func__, __LINE__, ##arg)

#define DATA_LEN_16BYTE (16)
#define DATA_LEN_8BYTE (8)

enum {
	KL_PSMF_TASK = 0,
	KL_VSF_TASK,
	KL_FRDF_TASK,
	KL_MKDF_TASK,
	KL_DK5F_TASK,
	KL_DK4F_TASK,
	KL_DK3F_TASK,
	KL_DK2F_TASK,
	KL_DK1F_TASK,
	KL_DA_DK2_TASK,
	KL_DA_NONCE_TASK,
	KL_TASK_NUM,
};

#define TEESMC_KL_DEV_OPEN		0
#define TEESMC_KL_DEV_RELEASE	1
#define TEESMC_KL_SET_INFO      2
#define TEESMC_KL_GET_INFO      3
#define TEESMC_KL_SET_EKEY      4
#define TEESMC_KL_GEN_ALL		5

extern sunxi_ss_t *ss_dev;
extern int ss_flow_request(ss_comm_ctx_t *comm);
extern void ss_flow_release(ss_comm_ctx_t *comm);
extern void ss_task_desc_init(ce_task_desc_t *task, u32 flow);
extern int ss_kl_smc_call(u32 cmd, u32 args);

int __init sunxi_ss_kl_init(void);
void __exit sunxi_ss_kl_exit(void);

#endif /* _SUNXI_SS_KL_H_ */

