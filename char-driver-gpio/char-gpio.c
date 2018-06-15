
// Simple Character Device Driver Module for Raspberry Pi.

#include <linux/module.h>   
#include <linux/string.h>    
#include <linux/fs.h>      
#include <linux/uaccess.h>
#include <linux/cdev.h>

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/gpio.h>

#define MY_MAJOR	89
#define MY_MINOR	0
#define GPIO_LED        18
/* Define GPIOs for LEDs */
static struct gpio leds[] = {
                {  18, GPIOF_OUT_INIT_LOW, "LED 1" },
};

MODULE_LICENSE("GPL");
MODULE_AUTHOR("RBiswasx and Wang");
MODULE_DESCRIPTION("A Simple Character Device Driver module");

static int my_open(struct inode *, struct file *);
static ssize_t my_read(struct file *, char *, size_t, loff_t *);
static ssize_t my_write(struct file *, const char *, size_t, loff_t *);
static int my_close(struct inode *, struct file *);

static char   message[100] = {0};

struct file_operations my_fops = {
        read    :       my_read,
        write   :       my_write,
        open    :       my_open,
        release :       my_close,
	owner   :       THIS_MODULE
        };

struct cdev my_cdev;

int init_module(void)
{
	dev_t devno;
	unsigned int count;
	int err, ret;

        printk("<1>Hello World\n");

	devno = MKDEV(MY_MAJOR, MY_MINOR);
	register_chrdev_region(devno, 1, "mydevice");

        cdev_init(&my_cdev, &my_fops);

	my_cdev.owner = THIS_MODULE;
        count = 1;
	err = cdev_add(&my_cdev, devno, count);
	if (err < 0)
	{
		printk("Device Add Error\n");
		return -1;
	}

	//gpio.map     = ioremap(GPIO_BASE, 4096);//p->map;
	//gpio.addr    = (volatile unsigned int *)gpio.map;
	// OUT_GPIO(GPIO_LED);   // LED

        // register LED gpios
        ret = gpio_request_array(leds, ARRAY_SIZE(leds));
        if (ret) {
                printk(KERN_ERR "Unable to request GPIOs for LEDs: %d\n", ret);
                return ret;
        }
        return 0;
}

void cleanup_module(void)
{
	dev_t devno;
	int i;

        printk("<1> Goodbye\n");

	devno = MKDEV(MY_MAJOR, MY_MINOR);
	//if (gpio.addr){
	//  /* release the mapping */
        //iounmap(gpio.addr);
	//}

        // turn all LEDs off
        for(i = 0; i < ARRAY_SIZE(leds); i++) {
                gpio_set_value(leds[i].gpio, 0);
        }
        // unregister
        gpio_free_array(leds, ARRAY_SIZE(leds));

	unregister_chrdev_region(devno, 1);
	cdev_del(&my_cdev);
}

static int my_open(struct inode *inod, struct file *filp)
{
	int major;
        int minor;
        major = imajor(inod);
        minor = iminor(inod);
        printk("*****Some body is opening me at major %d  minor %d*****\n", major, minor);
        return 0;
}

static int my_close(struct inode *inod, struct file *filp)
{
        int major;
        major = MAJOR(filp->f_inode->i_rdev);
        printk("*****Some body is closing me at major %d*****\n", major);
	return 0;
}

static ssize_t my_read(struct file *filp, char *buff, size_t len, loff_t *off)
{
        int major;
	int ret, count;
        major = MAJOR(filp->f_inode->i_rdev);
	count = strlen(message);
	ret = copy_to_user(buff, message, count);
	if (ret == 0) {
            printk("*****Some body is reading me at major %d*****\n", major);
	    printk("*****Number of bytes read :: %d *************\n", count);		
	    memset(message, 0, 100);
	    //GPIO_CLR = 1 << GPIO_LED;
	    gpio_set_value(leds[0].gpio, 0);
	} else {
    	    printk(KERN_WARNING "error on copy_to_user()\n");
	    count = -EFAULT;
	}
        return count;
}

static ssize_t my_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	int major;
	int ret, count;
	major = MAJOR(filp->f_inode->i_rdev);
	memset(message, 0, 100);
	count = strlen(buff);
	ret =copy_from_user(message, buff, count);
	if (len == 0 && ret != 0 ) {
	    printk(KERN_WARNING "error on copy_from_useri()\n");
	    count = -EFAULT;
	} else {
	    printk("*****Some body is writing me at major %d*****\n", major);
	    printk("*****Number of bytes written :: %d **********\n", count);
	    //GPIO_SET = 1 << GPIO_LED;
	    gpio_set_value(leds[0].gpio, 1);
	}
	return count;
}
