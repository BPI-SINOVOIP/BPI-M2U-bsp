/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "de_feat.h"

static const struct de_feat *de_cur_features;

static const int sun50iw3_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int sun50iw3_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int sun50iw3_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int sun50iw3_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw3_de_is_support_smbl[] = {
	/* CH0 */
	1,
	/* CH1 */
	0,
};

static const int sun50iw3_de_supported_output_types[] = {
	/* DISP0 */
	DE_OUTPUT_TYPE_LCD,
	/* DISP1 */
	DE_OUTPUT_TYPE_LCD,
	/* edp0 */
	DE_OUTPUT_TYPE_EDP,
};

static const int sun50iw3_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int sun50iw3_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int sun50iw3_de_scale_line_buffer_yuv[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun50iw3_de_scale_line_buffer_rgb[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun50iw3_de_scale_line_buffer_ed[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun50iw3_de_is_support_edscale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw3_de_is_support_de_noise[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw3_de_is_support_cdc[] = {
	/* DISP0 CH0 */
	0,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const struct de_feat sun50iw3_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = sun50iw3_de_num_chns,
	.num_vi_chns = sun50iw3_de_num_vi_chns,
	.num_layers = sun50iw3_de_num_layers,
	.is_support_vep = sun50iw3_de_is_support_vep,
	.is_support_smbl = sun50iw3_de_is_support_smbl,
	.is_support_wb = sun50iw3_de_is_support_wb,
	.supported_output_types = sun50iw3_de_supported_output_types,
	.is_support_scale = sun50iw3_de_is_support_scale,
	.scale_line_buffer_yuv = sun50iw3_de_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = sun50iw3_de_scale_line_buffer_rgb,
	.scale_line_buffer_ed = sun50iw3_de_scale_line_buffer_ed,
	.is_support_edscale = sun50iw3_de_is_support_edscale,
	.is_support_de_noise = sun50iw3_de_is_support_de_noise,
	.is_support_cdc = sun50iw3_de_is_support_cdc,
};

static const int sun50iw6_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int sun50iw6_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int sun50iw6_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int sun50iw6_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw6_de_is_support_smbl[] = {
	/* CH0 */
	1,
	/* CH1 */
	0,
};

static const int sun50iw6_de_supported_output_types[] = {
	/* DISP0 */
	DE_OUTPUT_TYPE_LCD,
	/* DISP1 */
	DE_OUTPUT_TYPE_HDMI,
};

static const int sun50iw6_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int sun50iw6_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int sun50iw6_de_scale_line_buffer[] = {
	/* DISP0 */
	4096,
	/* DISP1 */
	2048,
};

static const int sun50iw6_de_scale_line_buffer_yuv[] = {
	/* DISP0 CH0 */
	4096,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun50iw6_de_scale_line_buffer_rgb[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun50iw6_de_scale_line_buffer_ed[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun50iw6_de_is_support_edscale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw6_de_is_support_de_noise[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw6_de_is_support_cdc[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const struct de_feat sun50iw6_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = sun50iw6_de_num_chns,
	.num_vi_chns = sun50iw6_de_num_vi_chns,
	.num_layers = sun50iw6_de_num_layers,
	.is_support_vep = sun50iw6_de_is_support_vep,
	.is_support_smbl = sun50iw6_de_is_support_smbl,
	.is_support_wb = sun50iw6_de_is_support_wb,
	.supported_output_types = sun50iw6_de_supported_output_types,
	.is_support_scale = sun50iw6_de_is_support_scale,
	.scale_line_buffer_yuv = sun50iw6_de_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = sun50iw6_de_scale_line_buffer_rgb,
	.scale_line_buffer_ed = sun50iw6_de_scale_line_buffer_ed,
	.is_support_edscale = sun50iw6_de_is_support_edscale,
	.is_support_de_noise = sun50iw6_de_is_support_de_noise,
	.is_support_cdc = sun50iw6_de_is_support_cdc,
};

static const int sun8iw11_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int sun8iw11_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int sun8iw11_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int sun8iw11_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun8iw11_de_is_support_smbl[] = {
	/* CH0 */
	1,
	/* CH1 */
	0,
};

static const int sun8iw11_de_supported_output_types[] = {
	/* tcon0 */
	DE_OUTPUT_TYPE_LCD,
	/* tcon0 */
	DE_OUTPUT_TYPE_LCD,
	/* tcon2 */
	DE_OUTPUT_TYPE_TV | DE_OUTPUT_TYPE_HDMI,
	/* tcon3 */
	DE_OUTPUT_TYPE_TV | DE_OUTPUT_TYPE_HDMI,
};

static const int sun8iw11_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int sun8iw11_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int sun8iw11_de_scale_line_buffer_yuv[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun8iw11_de_scale_line_buffer_rgb[] = {
	/* DISP0 CH0 */
	2048,
	/* DISP0 CH1 */
	2048,
	/* DISP0 CH2 */
	2048,
	/* DISP0 CH3 */
	2048,
	/* DISP1 CH0 */
	2048,
	/* DISP1 CH1 */
	2048,
};

static const int sun8iw11_de_scale_line_buffer_ed[] = {
	/* DISP0 CH0 */
	0,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun8iw11_de_is_support_edscale[] = {
	/* DISP0 CH0 */
	0,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun8iw11_de_is_support_de_noise[] = {
	/* DISP0 CH0 */
	0,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun8iw11_de_is_support_cdc[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const struct de_feat sun8iw11_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = sun8iw11_de_num_chns,
	.num_vi_chns = sun8iw11_de_num_vi_chns,
	.num_layers = sun8iw11_de_num_layers,
	.is_support_vep = sun8iw11_de_is_support_vep,
	.is_support_smbl = sun8iw11_de_is_support_smbl,
	.is_support_wb = sun8iw11_de_is_support_wb,
	.supported_output_types = sun8iw11_de_supported_output_types,
	.is_support_scale = sun8iw11_de_is_support_scale,
	.scale_line_buffer_yuv = sun8iw11_de_scale_line_buffer_yuv,
	.scale_line_buffer_rgb = sun8iw11_de_scale_line_buffer_rgb,
	.scale_line_buffer_ed = sun8iw11_de_scale_line_buffer_ed,
	.is_support_edscale = sun8iw11_de_is_support_edscale,
	.is_support_de_noise = sun8iw11_de_is_support_de_noise,
	.is_support_cdc = sun8iw11_de_is_support_cdc,
};

int de_feat_get_num_screens(void)
{
	return de_cur_features->num_screens;
}

int de_feat_get_num_devices(void)
{
	return de_cur_features->num_devices;
}

int de_feat_get_num_chns(unsigned int disp)
{
	return de_cur_features->num_chns[disp];
}

int de_feat_get_num_vi_chns(unsigned int disp)
{
	return de_cur_features->num_vi_chns[disp];
}

int de_feat_get_num_ui_chns(unsigned int disp)
{
	return de_cur_features->num_chns[disp] -
	    de_cur_features->num_vi_chns[disp];
}

int de_feat_get_num_layers(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int num_layers = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		num_layers += de_cur_features->num_layers[index];

	return num_layers;
}

int de_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->num_layers[index];
}

int de_feat_is_support_vep(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support_vep = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support_vep += de_cur_features->is_support_vep[index];

	return is_support_vep;
}

int de_feat_is_support_vep_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_vep[index];
}

int de_feat_is_support_smbl(unsigned int disp)
{
	return de_cur_features->is_support_smbl[disp];
}

int de_feat_is_supported_output_types(unsigned int disp,
				      unsigned int output_type)
{
	return de_cur_features->supported_output_types[disp] & output_type;
}

int de_feat_is_support_wb(unsigned int disp)
{
	return de_cur_features->is_support_wb[disp];
}

int de_feat_is_support_scale(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support_scale = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support_scale += de_cur_features->is_support_scale[index];

	return is_support_scale;
}

int de_feat_is_support_scale_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_scale[index];
}

int de_feat_is_support_edscale(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support += de_cur_features->is_support_edscale[index];

	return is_support;
}

int de_feat_is_support_edscale_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_edscale[index];
}

int de_feat_is_support_de_noise(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support += de_cur_features->is_support_de_noise[index];

	return is_support;
}

int de_feat_is_support_de_noise_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_de_noise[index];
}

int de_feat_is_support_cdc_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_cdc[index];
}

int de_feat_get_scale_linebuf_for_yuv(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->scale_line_buffer_yuv[index];
}

int de_feat_get_scale_linebuf_for_rgb(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->scale_line_buffer_rgb[index];
}

int de_feat_get_scale_linebuf_for_ed(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->scale_line_buffer_ed[index];
}

int de_feat_init(void)
{
#if defined(CONFIG_ARCH_SUN50IW3P1)
	de_cur_features = &sun50iw3_de_features;
#elif defined(CONFIG_ARCH_SUN50IW6P1)
	de_cur_features = &sun50iw6_de_features;
#elif defined(CONFIG_ARCH_SUN8IW11P1)
	de_cur_features = &sun8iw11_de_features;
#else
#error "undefined platform!!!"
#endif
	return 0;
}
