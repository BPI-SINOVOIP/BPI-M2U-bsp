#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/timer.h> 
#include <linux/clk.h>
#include <linux/sys_config.h>
#include <linux/gpio.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>

static volatile u32 key_val;
static struct mutex		gk_mutex;
static struct input_dev *sunxigk_dev;
static int suspend_flg = 0;
static unsigned int power_gpio;

#ifdef CONFIG_PM
static struct dev_pm_domain keyboard_pm_domain;
#endif

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	/*if (unlikely(debug_mask & level_mask)) */\
	printk(KERN_INFO fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);

static irqreturn_t sunxi_isr_gk(int irq, void *dummy)
{
	dprintk(DEBUG_INT, "sunxi gpio key int \n");
	mutex_lock(&gk_mutex);
	if(__gpio_get_value(power_gpio)){
		if (suspend_flg) {
			input_report_key(sunxigk_dev, KEY_POWER, 1);
			input_sync(sunxigk_dev);
			dprintk(DEBUG_INT, "report data: power key down \n");
			suspend_flg = 0;
		}
		input_report_key(sunxigk_dev, KEY_POWER, 0);
		input_sync(sunxigk_dev);
		dprintk(DEBUG_INT, "report data: power key up \n");
	}else{
		input_report_key(sunxigk_dev, KEY_POWER, 1);
		input_sync(sunxigk_dev);
		dprintk(DEBUG_INT, "report data: power key down \n");
		suspend_flg = 0;
	}
	mutex_unlock(&gk_mutex);
	return IRQ_HANDLED;
}

static int sunxi_keyboard_suspend(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "sunxi gpio key suspend \n");
	mutex_lock(&gk_mutex);
	suspend_flg = 1;
	mutex_unlock(&gk_mutex);
	return 0;
}
static int sunxi_keyboard_resume(struct device *dev)
{
	dprintk(DEBUG_SUSPEND, "sunxi gpio key resume \n");
	return 0;
}

static int sunxigk_script_init(void)
{
	struct device_node *node;
	unsigned int gpio;

	node = of_find_node_by_type(NULL, "gpio_recovery_key");
	if (!node) {
		printk("GPIO_POWER: can't get device node.\n");
		return -1;
	}

	gpio = of_get_named_gpio(node, "recovery_gpio", 0);
	printk("GPIO_POWER: Power key io number = %d.\n", gpio);

	power_gpio = gpio;

	return 0;
}

static int sunxigk_gpio_init(struct device *dev)
{
	int ret;
	
	if(!gpio_is_valid(power_gpio)){
		dev_err(dev, "get gpio recovery_gpio failed\n");
	}
	else {
		ret = devm_gpio_request(dev, power_gpio, "gk_recovery");
		if(ret < 0){
			dev_err(dev,"can't request recovery gpio %d\n", power_gpio);
			return ret;
		}

		ret = gpio_direction_input(power_gpio);
		if (ret < 0) {
			dev_err(dev,"can't request input direction recovery gpio %d\n",
				power_gpio);
			return ret;
		}
	}

	return 0;
}

static int __init sunxigk_init(void)
{
	int err =0;
	int irq_number = 0;
	
	dprintk(DEBUG_INIT, "sunxigk_init \n");
	if(sunxigk_script_init()){
		err = -EFAULT;
		goto fail1;
	}
	
	sunxigk_dev = input_allocate_device();
	if (!sunxigk_dev) {
		pr_err( "sunxigk: not enough memory for input device\n");
		err = -ENOMEM;
		goto fail1;
	}

	sunxigk_dev->name = "sunxi-gpiokey";  
	sunxigk_dev->phys = "sunxikbd/input0"; 
	sunxigk_dev->id.bustype = BUS_HOST;      
	sunxigk_dev->id.vendor = 0x0001;
	sunxigk_dev->id.product = 0x0001;
	sunxigk_dev->id.version = 0x0100;

#ifdef REPORT_REPEAT_KEY_BY_INPUT_CORE
	sunxigk_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP);
	printk(KERN_DEBUG "REPORT_REPEAT_KEY_BY_INPUT_CORE is defined, support report repeat key value. \n");
#else
	sunxigk_dev->evbit[0] = BIT_MASK(EV_KEY);
#endif
	set_bit(KEY_POWER, sunxigk_dev->keybit);
	mutex_init(&gk_mutex);

	err = input_register_device(sunxigk_dev);
	if (err)
		goto fail2;
	
	err = sunxigk_gpio_init(&sunxigk_dev->dev);
	if(err < 0){
		pr_err("init recovery gpio %d failed\n", power_gpio);
		goto fail2;
	}
	
	irq_number = gpio_to_irq(power_gpio);
	if (request_irq(irq_number, sunxi_isr_gk, 
			       IRQF_TRIGGER_FALLING |IRQF_TRIGGER_RISING |IRQF_NO_SUSPEND, "sunxi_gk", NULL)) {
		err = -EBUSY;
		pr_err(KERN_DEBUG "request irq failure. \n");
		goto fail3;
	}

#ifdef CONFIG_PM
	keyboard_pm_domain.ops.suspend = sunxi_keyboard_suspend;
	keyboard_pm_domain.ops.resume = sunxi_keyboard_resume;
	sunxigk_dev->dev.pm_domain = &keyboard_pm_domain;	
#endif

	dprintk(DEBUG_INIT, "sunxikbd_init end\n");

	return 0;

fail3:	
	input_unregister_device(sunxigk_dev);
fail2:	
	input_free_device(sunxigk_dev);
fail1:
	printk(KERN_DEBUG "sunxikbd_init failed. \n");

	return err;
}

static void __exit sunxigk_exit(void)
{	

	devm_free_irq(&sunxigk_dev->dev, gpio_to_irq(power_gpio), NULL);
	input_unregister_device(sunxigk_dev);
}

module_init(sunxigk_init);
module_exit(sunxigk_exit);

MODULE_AUTHOR(" <@>");
MODULE_DESCRIPTION("sunxi gpio power key driver");
MODULE_LICENSE("GPL");


