#ifndef  __S6D7AA0X01_PANEL_H__
#define  __S6D7AA0X01_PANEL_H__

#include "panels.h"

extern __lcd_panel_t S6D7AA0X01_panel;

extern __s32 dsi_dcs_wr(__u32 sel,__u8 cmd,__u8* para_p,__u32 para_num);
extern __s32 dsi_dcs_wr_0para(__u32 sel,__u8 cmd);
extern __s32 dsi_dcs_wr_1para(__u32 sel,__u8 cmd,__u8 para);
extern __s32 dsi_dcs_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
extern __s32 dsi_dcs_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
extern __s32 dsi_dcs_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
extern __s32 dsi_dcs_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);
extern __s32 dsi_gen_wr_0para(__u32 sel,__u8 cmd);
extern __s32 dsi_gen_wr_1para(__u32 sel,__u8 cmd,__u8 para);
extern __s32 dsi_gen_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2);
extern __s32 dsi_gen_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3);
extern __s32 dsi_gen_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4);
extern __s32 dsi_gen_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5);

#endif
