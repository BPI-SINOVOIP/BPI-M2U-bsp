/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <linux/slab.h>
#include <asm/cacheflush.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/vmalloc.h>

#include "hdcp.h"
#include "hdcp22_tx.h"
#include "hdcp22/include/ESMHost.h"

#define HDCP22_WAIT_TIME 3000
#define PAIRDATA_SIZE 300

#define INT -2

/*static u8 set_cap;*/
static struct mutex authen_mutex;
static u8 esm_on;
static u8 esm_enable;
static u8 pairdata[PAIRDATA_SIZE];
static wait_queue_head_t               esm_wait;
static struct workqueue_struct 		   *hdcp22_workqueue;
static struct work_struct			   hdcp22_work;
static esm_instance_t *esm;

static void hdcp22_authenticate_and_data_enable_work(struct work_struct *work);

static int hpi_read(void *instance, uint32_t offset, uint32_t *data)
{
	unsigned long addr =  esm->driver->hpi_base + offset;

	*data = *((volatile u32 *)addr);
	/*pr_info("hpi_read: offset:%x  data:%x\n", offset, *data);*/
	udelay(1000);
	return 0;
}

static int hpi_write(void *instance, uint32_t offset, uint32_t data)
{
	unsigned long addr =  esm->driver->hpi_base + offset;

	*((volatile u32 *)addr) = data;
	/*pr_info("hpi_write: offset:%x  data:%x\n", offset, data);*/
	udelay(1000);
	return 0;
}

static int data_read(void *instance, uint32_t offset, uint8_t *dest_buf, uint32_t nbytes)
{
	memcpy(dest_buf, (u8 *)(esm->driver->vir_data_base + offset), nbytes);
	return 0;
}

static int data_write(void *instance, uint32_t offset, uint8_t *src_buf, uint32_t nbytes)
{
	memcpy((u8 *)(esm->driver->vir_data_base + offset), src_buf, nbytes);
	return 0;
}

/*make a length of memory set a data*/
static int data_set(void *instance, uint32_t offset, uint8_t data, uint32_t nbytes)
{
	memset((u8 *)(esm->driver->vir_data_base + offset), data, nbytes);
	return 0;
}

static int esm_tx_reset(void)
{
	return ESM_Reset(esm);
}

/*static int esm_tx_kill(void)
{
	return ESM_Kill(esm);
}*/

static void esm_driver_init(unsigned long esm_hpi_base,
					 u32 code_base,
				unsigned long vir_code_base,
					 u32 code_size,
					 u32 data_base,
				unsigned long vir_data_base,
					u32 data_size)
{
	esm->driver = kmalloc(sizeof(esm_host_driver_t),
					GFP_KERNEL | __GFP_ZERO);
	if (esm->driver == NULL) {
		pr_info("ERROR: esm_host_driver_t alloc failed\n ");
		return;
	}

	esm->driver->hpi_base = esm_hpi_base;
	esm->driver->code_base = code_base;
	esm->driver->vir_code_base = vir_code_base;
	esm->driver->code_size = code_size;

	esm->driver->data_base = data_base;
	esm->driver->vir_data_base = vir_data_base;
	esm->driver->data_size = data_size;

	esm->driver->hpi_read = hpi_read;
	esm->driver->hpi_write = hpi_write;
	esm->driver->data_read = data_read;
	esm->driver->data_write = data_write;
	esm->driver->data_set = data_set;

	esm->driver->instance = NULL;
	esm->driver->idle = NULL;
}

static void esm_driver_exit(void)
{
	if (esm->driver != NULL)
		kfree(esm->driver);
}

void esm_check_print(u32 code_base,
		     unsigned long vir_code_base,
		     u32 code_size,
		     u32 data_base,
		     unsigned long vir_data_base,
		     u32 data_size)
{
	/*u32 i;*/

	/*pr_info("phy_code_base:0x%x vir_code_base:0x%lx code_size:%d\n",
			code_base, vir_code_base, code_size);
	pr_info("phy_data_base:0x%x vir_data_base:0x%lx data_size:%d\n",
			data_base, vir_data_base, data_size);*/
	/*for (i = 0; i < 10; i++)
		pr_info("esm firmware %d:0x%x\n", i,
				*((u8 *)(vir_code_base + i)));*/
}

int esm_tx_initial(unsigned long esm_hpi_base,
			      u32 code_base,
			      unsigned long vir_code_base,
			      u32 code_size,
			      u32 data_base,
			      unsigned long vir_data_base,
			      u32 data_size)
{
	esm_on = 0;
	esm_enable = 0;
	memset(pairdata, 0, PAIRDATA_SIZE);

	esm = kmalloc(sizeof(esm_instance_t), GFP_KERNEL | __GFP_ZERO);
	if (esm == NULL) {
		pr_info("ERROR: esm_instance_t alloc failed\n ");
		return -1;
	}
	esm_check_print(code_base, vir_code_base, code_size,
			data_base, vir_data_base, data_size);
	esm_driver_init(esm_hpi_base, code_base, vir_code_base, code_size,
					data_base, vir_data_base, data_size);
	mutex_init(&authen_mutex);
	init_waitqueue_head(&esm_wait);
	hdcp22_workqueue = create_workqueue("hdcp22_workqueue");
	INIT_WORK(&hdcp22_work, hdcp22_authenticate_and_data_enable_work);
	return 0;
}

void esm_tx_exit(void)
{
	if (hdcp22_workqueue != NULL)
		destroy_workqueue(hdcp22_workqueue);
	esm_driver_exit();
	if (esm != NULL)
		kfree(esm);

}

int esm_tx_open(void)
{
	ESM_STATUS err;
	esm_config_t esm_config;

	if (esm_on != 0) {
		pr_info("esm has been booted\n");
		return 0;
	}
	esm_on = 1;

	if ((esm->driver != NULL) && esm->driver->vir_data_base)
		memset((void *)esm->driver->vir_data_base, 0, esm->driver->data_size);

	memset(&esm_config, 0, sizeof(esm_config_t));
	err = ESM_Initialize(esm,
			esm->driver->code_base,
			esm->driver->code_size,
			0, esm->driver, &esm_config);
	if (err != ESM_SUCCESS) {
		pr_info("[esm-error]:esm boots fail!\n");
		return -1;
	} else {
		HDMI_INFO_MSG("esm boots successfully\n");
	}
	/*if (ESM_EnablePairing(esm, 1) != 0)
		pr_info("esm enable pairing failed\n");*/
	if (ESM_LoadPairing(esm, pairdata, esm->esm_pair_size) < 0)
		pr_info("Error: ESM Load Pairing failed\n");

	return 0;
}

/*for:
	hdmi_plugin<--->hdmi_plugout
	hdmi_suspend<--->hdmi_resume
*/
void esm_tx_close(void)
{
	/*esm_tx_kill();
	esm_tx_disable();
	esm_enable = 0;*/
	esm_enable = 0;
	esm_on = 0;
}

static int hdcp22_set_authenticate(void)
{
	ESM_STATUS err;

	mutex_lock(&authen_mutex);
	err = ESM_Authenticate(esm, 1, 1, 0);
	if (err != 0) {
		pr_info("ESM_Authenticate failed\n");
		mutex_unlock(&authen_mutex);
		return -1;
	} else {
		pr_info("ESM_Authenticate success\n");
		mutex_unlock(&authen_mutex);
		return 0;
	}
}

void esm_tx_disable(void)
{
	if (!esm_enable)
		return;
	esm_enable = 0;
	wake_up(&esm_wait);
	ESM_Authenticate(esm, 0, 0, 0);
	msleep(20);
}

static int esm_tx_enable(void)
{
	int wait_time = 0;
	LOG_TRACE();

	if (esm_enable)
		return 0;
	esm_enable = 1;

	pr_info("Sleep to wait for hdcp2.2 authentication\n");
	wait_time = wait_event_interruptible_timeout(esm_wait, !esm_enable,
						       msecs_to_jiffies(3000));
	if (wait_time > 0) {
		pr_info("Force to wake up, waiting time is less than 3s\n");
		return -1;
	}

	esm_tx_reset();
	/* Enable logging*/
	ESM_LogControl(esm, 1, 0);
	ESM_EnableLowValueContent(esm);
	if (ESM_SetCapability(esm) != ESM_HL_SUCCESS) {
		pr_info("[hdcp2.2-error]: esm set capability fail, maybe remote Rx is not 2.2 capable!\n");
		return -1;
	}

	if (hdcp22_set_authenticate()) {
		pr_info("hdcp2.2 set authenticate failed\n");
		return -1;
	}

	return 0;
}

static void hdcp22_authenticate_and_data_enable_work(struct work_struct *work)
{
	if (esm_tx_enable())
		return;
	hdcp22_data_enable(1);
}

void set_hdcp22_authenticate_and_data_enable(void)
{
	queue_work(hdcp22_workqueue, &hdcp22_work);
}

static int esm_encrypt_status_check(void)
{
	esm_status_t Status = {0, 0, 0, 0};

	/*Check to see if sync gets lost
		when running (i.e. lose authentication)*/
	if (ESM_GetStatusRegister(esm, &Status, 1) == ESM_HL_SUCCESS) {
		if (Status.esm_sync_lost) {
			pr_info("[hdcp2.2-error]esm sync lost!\n");
			return -1;
		}

		if (Status.esm_exception) {
			/*Got an exception. can check*/
			/*bits for more detail*/
			if (Status.esm_exception & 0x80000000)
				pr_info("hardware exception\n");
			else
				pr_info("solfware exception\n");

			pr_info("exception line number:%d\n",
				       (Status.esm_exception >> 10) & 0xfffff);
			pr_info("exception flag:%d\n",
					(Status.esm_exception >> 1) & 0x1ff);
			pr_info("exception Type:%s\n",
					(Status.esm_exception & 0x1) ? "notify" : "abort");
			if (((Status.esm_exception >> 1) & 0x1ff) != 109)
				return -1;
			memset(pairdata, 0, PAIRDATA_SIZE);
			if (ESM_SavePairing(esm, pairdata, &esm->esm_pair_size) != 0)
				pr_info("ESM_SavePairing failed\n");

			return 0;
		}

		if (Status.esm_auth_fail) {
			pr_info("esm status check result,failed:%d\n", Status.esm_auth_fail);
			return -1;
		}

		if (Status.esm_auth_pass) {
			return 0;
		}

		return 0;
	}
	return -1;
}

/* Check esm encrypt status and handle the status
 * return value: 1-indicate that esm is being in authenticate;
 *               0-indicate that esm authenticate is sucessful
 *              -1-indicate that esm authenticate is failed
 * */
int esm_encrypt_status_check_and_handle(void)
{
	int i = 0;

	if (!esm_enable)
		return 0;
	for (i = 0; i < 15; i++) {
		if (esm_encrypt_status_check() < 0) {
			pr_info("esm is running failed\n");
			hdcp22_set_authenticate();
		} else {
			HDMI_INFO_MSG("ESM is running normally\n");
			break;
		}
	}

	if (i >= 15)
		return -1;
	else
		return 0;
}

