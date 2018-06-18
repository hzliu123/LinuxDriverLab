/*
 * linux/drivers/char/pi_buttons.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>

#define DEVICE_NAME "buttons"
#define NUM_BUTTONS 1

/* Define GPIOs for LEDs */
static struct gpio leds[NUM_BUTTONS] = {
                {  18, GPIOF_OUT_INIT_LOW, "LED 1" },
            //  {  23, GPIOF_OUT_INIT_LOW, "LED 2" },
};

/**
 * struct gpio - a structure describing a GPIO with configuration
 * @gpio:       the GPIO number
 * @flags:      GPIO configuration as specified by GPIOF_*
 * @label:      a literal description string of this GPIO
 */
//struct gpio {
//        unsigned        gpio;
//        unsigned long   flags;
//        const char      *label;
//};


struct button_desc {
	int index;
	struct gpio button;
	struct timer_list timer;
	int value;    //1: OFF, 0: ON
};
static struct button_desc buttons[NUM_BUTTONS] = {
	{ 0, {4, GPIOF_IN, "KEY0"} },
//	{ 1, {17, GPIOF_IN, "KEY1"} },
};
static volatile int ev_press[NUM_BUTTONS] = {0};
static volatile int press = 0;
static char to_user_string[100] = {0};  

static struct fasync_struct *pi_buttons_async_queue;
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);

static void pi_buttons_timer(unsigned long _data)
{
	struct button_desc *bdata = (struct button_desc *)_data;
	unsigned tmp;

	tmp = gpio_get_value(bdata->button.gpio);
        bdata->value = tmp;
	if (tmp == 0) {
		ev_press[bdata->index] = 1;
	        press = 1;
        	// turn corresponding LEDs on
               	gpio_set_value(leds[bdata->index].gpio, 1);
		wake_up_interruptible(&button_waitq);
	} else {
		ev_press[bdata->index] = 0;
        	// turn corresponding LEDs off
               	gpio_set_value(leds[bdata->index].gpio, 0);
	}
	printk("timer KEY %d: %d \n", bdata->button.gpio, tmp);
}

static irqreturn_t button_interrupt(int irq, void *dev_id)
{
	struct button_desc *bdata = (struct button_desc *)dev_id;
	mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(40));
	kill_fasync(&pi_buttons_async_queue, SIGIO, POLL_IN);
	return IRQ_HANDLED;
}

static int pi_buttons_open(struct inode *inode, struct file *file)
{
	int irq;
	int i;
	int err = 0;
	for (i = 0; i < ARRAY_SIZE(buttons); i++) {
		if (!buttons[i].button.gpio)
			continue;
		setup_timer(&buttons[i].timer, pi_buttons_timer,
				(unsigned long)&buttons[i]);
		irq = gpio_to_irq(buttons[i].button.gpio);
		err = request_irq(irq, button_interrupt,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				buttons[i].button.label, (void *)&buttons[i]);
		if (err)
			break;
	}
	if (err) {
		i--;
		for (; i >= 0; i--) {
			if (!buttons[i].button.gpio)
				continue;
			irq = gpio_to_irq(buttons[i].button.gpio);
			disable_irq(irq);
			free_irq(irq, (void *)&buttons[i]);
			del_timer_sync(&buttons[i].timer);
		}
		return -EBUSY;
	}
	for (i=0; i<ARRAY_SIZE(ev_press); i++) ev_press[i] = 0;
	press = 0;
	return 0;
}

static int pi_buttons_close(struct inode *inode, struct file *file)
{
	int irq, i;

	for (i = 0; i < ARRAY_SIZE(buttons); i++) {
		if (!buttons[i].button.gpio)
			continue;

		irq = gpio_to_irq(buttons[i].button.gpio);
		free_irq(irq, (void *)&buttons[i]);

		del_timer_sync(&buttons[i].timer);
	}

	return 0;
}

static int pi_buttons_read(struct file *filp, char __user *buff,
		size_t count, loff_t *offp)
{
	unsigned long err;
	int i;
	if (!press) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(button_waitq, press);
	}
	memset(to_user_string, 0, sizeof(to_user_string));
	for (i=0; i<ARRAY_SIZE(ev_press); i++) {
            if (ev_press[i] == 1) {strcat(to_user_string, " ON"); }
            if (ev_press[i] == 0) strcat(to_user_string, " OFF"); 
	}
        strcat(to_user_string, " \n");

	err = copy_to_user((void *)buff, (const void *)(to_user_string),
			min(strlen(to_user_string), count));
	//printk("read: %s %d %d %lu\n", to_user_string, strlen(to_user_string), count, err);
	for (i=0; i<ARRAY_SIZE(ev_press); i++) ev_press[i] = 0;
	press = 0;
	return err ? -EFAULT : min(strlen(to_user_string), count);
}

static unsigned int pi_buttons_poll( struct file *file,
		struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &button_waitq, wait);
	if (press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int pi_buttons_fasync(int fd, struct file *filp, int on)
{
        return fasync_helper(fd, filp, on, &pi_buttons_async_queue);
}


static struct file_operations dev_fops = {
	.owner		= THIS_MODULE,
	.open		= pi_buttons_open,
	.release	= pi_buttons_close, 
	.read		= pi_buttons_read,
	.poll		= pi_buttons_poll,
        .fasync         = pi_buttons_fasync,
};

static struct miscdevice misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVICE_NAME,
	.fops		= &dev_fops,
};

static int __init button_dev_init(void)
{
	int ret;

        // register LED gpios
        ret = gpio_request_array(leds, ARRAY_SIZE(leds));
        if (ret) {
                printk(KERN_ERR "Unable to request GPIOs for LEDs: %d\n", ret);
                return ret;
        }

	ret = misc_register(&misc);
	printk(DEVICE_NAME"\tinitialized\n");
	return ret;
}

static void __exit button_dev_exit(void)
{
	int i;
        // turn all LEDs off
        for(i = 0; i < ARRAY_SIZE(leds); i++) {
                gpio_set_value(leds[i].gpio, 0);
        }
        // unregister LED gpios
        gpio_free_array(leds, ARRAY_SIZE(leds));

	misc_deregister(&misc);
}

module_init(button_dev_init);
module_exit(button_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc. and Wang");

