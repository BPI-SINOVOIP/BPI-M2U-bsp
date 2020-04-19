/*
 *
 * (C) COPYRIGHT 2014 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */



#ifndef _BASE_DEVFREQ_H_
#define _BASE_DEVFREQ_H_

int kbase_devfreq_init(struct kbase_device *kbdev);
void kbase_devfreq_term(struct kbase_device *kbdev);

unsigned long sunxi_get_initfreq(void);
extern int sunxi_update_vf(struct kbase_device *kbdev,
		unsigned long *voltage, unsigned long *freq);
extern int sunxi_set_utilisition(unsigned long total_out,
				unsigned long busy_out);

#endif /* _BASE_DEVFREQ_H_ */
