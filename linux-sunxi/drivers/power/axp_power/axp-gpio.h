#ifndef AXP_GPIO_H_
#define AXP_GPIO_H_

#include <linux/kernel.h>
#include <linux/pinctrl/pinconf-sunxi.h>

#define AXP_PINCTRL_GPIO(_num)  PINCTRL_PIN(_num, "GPIO"#_num)

/*
 * GPIO Registers.
 */

#define AXP_PIN_NAME_MAX_LEN    (8)

struct axp_desc_function {
	const char *name;
	u8         muxval;
	u8         irq_valid;
};

struct axp_desc_pin {
	struct pinctrl_pin_desc  pin;
	struct axp_desc_function *functions;
};

struct axp_pinctrl_desc {
	const struct axp_desc_pin *pins;
	int           npins;
};

struct axp_pinctrl_function {
	const char  *name;
	const char  **groups;
	unsigned    ngroups;
};

struct axp_pinctrl_group {
	const char  *name;
	unsigned long   config;
	unsigned    pin;
};

struct axp_gpio_ops {
	int (*gpio_get_data)(struct axp_dev *axp_dev, int gpio);
	int (*gpio_set_data)(struct axp_dev *axp_dev, int gpio, int value);
	int (*pmx_set)(struct axp_dev *axp_dev, int gpio, int mux);
	int (*pmx_get)(struct axp_dev *axp_dev, int gpio);
};

struct axp_pinctrl {
	struct device                *dev;
	struct pinctrl_dev           *pctl_dev;
	struct gpio_chip             gpio_chip;
	struct axp_dev               *axp_dev;
	struct axp_gpio_ops          *ops;
	struct axp_pinctrl_desc      *desc;
	struct axp_pinctrl_function  *functions;
	unsigned                     nfunctions;
	struct axp_pinctrl_group     *groups;
	unsigned                     ngroups;
};

#define AXP_PIN_DESC(_pin, ...)         \
	{                                   \
		.pin = _pin,                    \
		.functions = (struct axp_desc_function[]){  \
			__VA_ARGS__, { } },         \
	}

#define AXP_FUNCTION(_val, _name)       \
	{                           \
		.name = _name,          \
		.muxval = _val,         \
	}

#define AXP_FUNCTION_IRQ(_valid)  \
	{                           \
		.name = "irq",          \
		.irq_valid = _valid,    \
	}

struct axp_gpio_irq_ops {
	int (*irq_request)(int, u32 (*handler)(int, void *), void *);
	int (*irq_free)(int);
	int (*irq_ack)(int);
	int (*irq_enable)(int);
	int (*irq_disable)(int);
	int (*irq_set_type)(int, unsigned long);
};

struct axp_gpio_irqchip {
	int gpio_no;
	u32 (*handler)(int, void *);
	void *data;
};

struct axp_pinctrl *axp_pinctrl_register(struct device *dev,
			struct axp_dev *axp_dev,
			struct axp_pinctrl_desc *desc,
			struct axp_gpio_ops *ops);
int axp_pinctrl_unregister(struct axp_pinctrl *pctl);
void axp_gpio_irq_ops_set(int pmu_num, struct axp_gpio_irq_ops *ops);
int axp_gpio_irq_valid(struct axp_pinctrl_desc *desc, int gpio_no);

extern s32 axp_debug;

#endif
