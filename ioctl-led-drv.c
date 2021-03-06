/*
 * @Author: northward
 * @Github: https://github.com/northwardtop
 * @LastEditors: northward
 * @Description: 本文件为LED驱动程序
 * 使用ioctl实现应用层指令控制驱动程序
 * 实现: 按下K点亮LED
 * 驱动层:
 * 		注册一个杂项设备, 解析ioctl的指令, 
 * 		并读K值(copy_to_user)或写LED值(copy_from_user)
 * 应用层:
 * 		打开设备LED和K, 使用ioctl执行指令去读K设备值或写LED设备值
 *		
 * @Date: 2019-05-01 15:47:55
 * @LastEditTime: 2019-05-06 00:14:49
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <cfg_type.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include "key-led.h"

#define DEVICE_NAME			"LEDs"
#define LED_MINOR			131

struct led_gpio_info{
	unsigned int num;
	char name[16];
};

static const struct led_gpio_info led_info_tab[4]={
	{.num=PAD_GPIO_E+13, .name="gpioe13"},
	{.num=PAD_GPIO_C+7, .name="gpioc7"},
	{.num=PAD_GPIO_C+8, .name="gpioc8"},
	{.num=PAD_GPIO_C+17, .name="gpioc17"},
};


long gec6818_led_ioctl(struct file *fl, unsigned int cmd, unsigned long arg)
{
	//获取当前LED灯下标
	unsigned long index=arg-7;
	switch(cmd)
	{
		case GEC6818_LED_ON:
			gpio_set_value(led_info_tab[index].num, 0);
			break;
		case GEC6818_LED_OFF:
			gpio_set_value(led_info_tab[index].num, 1);
			break;
		default:
			return -ENOIOCTLCMD;
	}
	return 0;
}



/**
 * @description: 当应用调用write函数时候, 将buf数据写入内核空间
 * 		copy_from_user(kbuf,buf,len);
 * @param {type} 
 * @return: 
 */
static ssize_t gec6818_led_write(struct file *fl, const char __user *buf, size_t len, loff_t *off)
{
	int ret=-1;
	char kbuf[64]={};
	printk("LED device writing...\n");
	if(len>sizeof kbuf) //如果内核空间小装不下用户空间参数则报错
		return -EINVAL;//参数
	ret=copy_from_user(kbuf, buf, len);//返回cp字节数
	if(ret<0)
	{
		printk("copy_from_user error\n");
		return -1;
	}
	printk("user say: %s\n", kbuf);

	return len-ret;
}

/**
 * @description: read函数, 数据从内核到应用层, copy_to_user();
 * @param {type} 
 * @return: 
 */
static ssize_t gec6818_led_read(struct file *fl, char __user *buf, size_t len, loff_t *offs)
{
	int ret=-1;
	char kbuf[64]="I am from kernel data\n";
	printk("LED device reading...\n");
	if(len>sizeof kbuf) //如果内核空间小装不下用户空间参数则报错
		return -EINVAL;//参数
	ret=copy_to_user(buf, kbuf, len);
	if(ret<0)
	{
		printk("copy_to_user error\n");
		return -1;
	}
	
	return len-ret;
} 

//打开
static int gec6818_led_open(struct inode *ino, struct file *f)
{
	int i=0;
	//配置对应gpio为输出模式
	for(;i<4;++i)
		gpio_direction_output(led_info_tab[i].num, 1);
	printk("LED device opened!\n");
	return 0;
}

//关闭
static int gec6818_led_release(struct inode *i, struct file *f)
{
	printk("LED closed\n");
	return 0;
}

static const struct file_operations gec6818_led_fops={
	.owner		= THIS_MODULE,
	.write		= gec6818_led_write,
	.read 		= gec6818_led_read,
	.open		= gec6818_led_open,
	.release 	= gec6818_led_release,
	.unlocked_ioctl = gec6818_led_ioctl
};

static struct miscdevice gec6818_led_miscdev={
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVICE_NAME,
	.fops		= &gec6818_led_fops
};

static int __init gec6818_led_init(void)
{
	int i=0,ret=-1;
	//释放将要使用的gpio
	for(i=0;i<4;++i)
		gpio_free(led_info_tab[i].num);
	//申请gpio
	for(i=0;i<4;++i)
		ret=gpio_request(led_info_tab[i].num, led_info_tab[i].name);
	
	if(ret<0)
		goto gpio_request_failed;

	ret=misc_register(&gec6818_led_miscdev);
	if(ret<0)
		goto register_failed;
	printk("installed led_drv\n");

register_failed:
	printk("misc_register failed\n");
	misc_deregister(&gec6818_led_miscdev);

gpio_request_failed:
	printk("gpio_request failed\n");
	for(i=0;i<4;++i)
		gpio_free(led_info_tab[i].num);

	return ret;
}

static void __exit gec6818_led_exit(void)
{
	int i=0;
	misc_deregister(&gec6818_led_miscdev);
	for(i=0;i<4;++i)
		gpio_free(led_info_tab[i].num);
	printk("remove led_drv\n");
}

module_init(gec6818_led_init);
module_exit(gec6818_led_exit);

MODULE_AUTHOR("yonghui.lee <yonghuilee.cn@gmail.com>");
MODULE_DESCRIPTION("GEC6818 LED Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("v2.0");
