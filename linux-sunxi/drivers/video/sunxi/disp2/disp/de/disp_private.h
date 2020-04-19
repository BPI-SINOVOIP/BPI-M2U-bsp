#ifndef _DISP_PRIVATE_H_
#define _DISP_PRIVATE_H_

#include "disp_features.h"
#if defined(CONFIG_ARCH_SUN50IW1P1)
#include "./lowlevel_sun50iw1/disp_al.h"
#elif defined(CONFIG_ARCH_SUN50IW2)
#include "./lowlevel_v2x/disp_al.h"
#elif defined(CONFIG_ARCH_SUN8IW10)
#include "./lowlevel_sun8iw10/disp_al.h"
#elif defined(CONFIG_ARCH_SUN8IW11)
#include "./lowlevel_v2x/disp_al.h"
#elif defined(CONFIG_ARCH_SUN8IW17)
#include "./lowlevel_v2x/disp_al.h"
#elif defined(CONFIG_ARCH_SUN50IW3)
#include "./lowlevel_v3x/disp_al.h"
#elif defined(CONFIG_ARCH_SUN50IW6)
#include "./lowlevel_v3x/disp_al.h"
#else
#error "undefined platform!!!"
#endif

struct dmabuf_item {
	struct list_head list;
	int fd;
	struct dma_buf *buf;
	struct dma_buf_attachment *attachment;
	struct sg_table *sgt;
	dma_addr_t dma_addr;
	unsigned long long id;
};

/* fb_addrss_transfer - framebuffer address transfer
 *
 * @format: pixel format
 * @size: size for each plane
 * @align: align for each plane
 * @depth: depth perception for stereo image
 * @dma_addr: the start addrss of this buffer
 *
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct fb_address_transfer {
	enum disp_pixel_format format;
	struct disp_rectsz size[3];
	unsigned int align[3];
	int depth;
	dma_addr_t dma_addr;
	unsigned long long addr[3];
	unsigned long long trd_right_addr[3];
};

/* disp_format_attr - display format attribute
 *
 * @format: pixel format
 * @bits: bits of each component
 * @hor_rsample_u: reciprocal of horizontal sample rate
 * @hor_rsample_v: reciprocal of horizontal sample rate
 * @ver_rsample_u: reciprocal of vertical sample rate
 * @hor_rsample_v: reciprocal of vertical sample rate
 * @uvc: 1: u & v component combined
 * @interleave: 0: progressive, 1: interleave
 * @factor & div: bytes of pixel = factor / div (bytes)
 *
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct disp_format_attr {
	enum disp_pixel_format format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};

extern struct disp_device* disp_get_lcd(u32 disp);

extern struct disp_device* disp_get_hdmi(u32 disp);

extern struct disp_manager* disp_get_layer_manager(u32 disp);

extern struct disp_layer* disp_get_layer(u32 disp, u32 chn, u32 layer_id);
extern struct disp_layer* disp_get_layer_1(u32 disp, u32 layer_id);
extern struct disp_smbl* disp_get_smbl(u32 disp);
extern struct disp_enhance* disp_get_enhance(u32 disp);
extern struct disp_capture* disp_get_capture(u32 disp);

extern s32 disp_delay_ms(u32 ms);
extern s32 disp_delay_us(u32 us);
extern s32 disp_init_lcd(disp_bsp_init_para * para);
extern s32 disp_init_hdmi(disp_bsp_init_para *para);
extern s32 disp_init_tv(void);//(disp_bsp_init_para * para);
extern s32 disp_tv_set_func(struct disp_device*  ptv, struct disp_tv_func * func);
extern s32 disp_init_tv_para(disp_bsp_init_para * para);
extern s32 disp_tv_set_hpd(struct disp_device*  ptv, u32 state);
extern s32 disp_init_vga(void);
extern s32 disp_init_edp(disp_bsp_init_para *para);

extern s32 disp_init_mgr(disp_bsp_init_para * para);
extern s32 disp_init_enhance(disp_bsp_init_para * para);
extern s32 disp_init_smbl(disp_bsp_init_para * para);
extern s32 disp_init_capture(disp_bsp_init_para *para);

extern s32 disp_init_eink(disp_bsp_init_para * para);
extern s32 write_edma(struct disp_eink_manager*  manager);
extern s32 disp_init_format_convert_manager(disp_bsp_init_para *para);

extern struct disp_eink_manager* disp_get_eink_manager(unsigned int disp);

#include "disp_device.h"

u32 dump_layer_config(struct disp_layer_config_data *data);
void *disp_vmap(unsigned long phys_addr, unsigned long size);
void disp_vunmap(const void *vaddr);

struct dmabuf_item *disp_dma_map(int fd);
void disp_dma_unmap(struct dmabuf_item *item);
s32 disp_set_fb_info(struct fb_address_transfer *fb, bool left_eye);
s32 disp_set_fb_base_on_depth(struct fb_address_transfer *fb);
#endif

