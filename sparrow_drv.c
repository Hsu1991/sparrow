/*
 * sparrow_drv.c
 *
 * A demonstation driver using Module in Linux kernel
 *
 * Author: wowo<www.wowotech.net>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/string.h>

#include <asm/uaccess.h>

#define DEVICE_NAME			"sparrow"

#define SPA_DEFAULT_MAJOR 	(111)	/* major device number, if invalid, dynamic alloc it */
#define SPA_NUMS 			(1)		/* device numbers */


/* the sparrow char device and its major */
static struct cdev 			spa_cdev;
static unsigned int			spa_major = SPA_DEFAULT_MAJOR;
static unsigned int 		devt = 0;

/* a short memory used for read/write demo */
#define BUF_LEN				(32)
static char 				spa_mem[BUF_LEN];

static struct cdev mycdev;
static struct class *myclass = NULL;

extern void TS_test_stop(void);
extern int TS_test_start(void);

static dev_t dev = 0;
static struct class *dev_class;

static int spa_open(struct inode *inode, struct file *filp) {
	unsigned int major, minor;

	/* 
	 * we can get the major and minor number from filp pointer,
	 * if the char device has multiple minors,
	 * we can know which one is being operated. 
	 */ 
	major = imajor(inode);
	minor = iminor(inode);

	printk("%s, major = %d, minor = %d!\n", __func__, major, minor); 
	return 0;
}

static ssize_t spa_read(struct file *filp, char *buf,
						size_t len, loff_t *offset) {
	printk("%s, len = %d\n", __func__, len); 

	if (filp == NULL || buf == NULL || len == 0) {
		printk("Params error!\n"); 
		return -1;
	}

	if (len > BUF_LEN) {
		len = BUF_LEN;
	}

	/* skip the offset variable */
	copy_to_user(buf, spa_mem, len);

	return len;
}

static ssize_t spa_write(struct file *filp, const char *buf,
						 size_t len, loff_t *offset) {
	printk("%s, len = %d\n", __func__, len); 

	if (filp == NULL || buf == NULL || len == 0) {
		printk("Params error!\n"); 
		return -1;
	}

	if (len > BUF_LEN) {
		len = BUF_LEN;
	}
	//
	/* clear mem first so that stop can work*/
	memset(spa_mem, 0, BUF_LEN);

	/* skip the offset variable */
	copy_from_user(spa_mem, buf, len);

	/* Thread Synchronization Testing */
	if (strcmp(spa_mem, "start\n") == 0) {
		TS_test_start();
	}

	if (strcmp(spa_mem, "stop\n") == 0) {
		TS_test_stop();
	}
	return len;
}

static int spa_close(struct inode *inode, struct file *filp) {
	printk("%s\n", __func__); 
	return 0;
}

static struct file_operations spa_fops = {
	.open 		= spa_open,
	.read 		= spa_read,
	.write 		= spa_write,
	.release 	= spa_close,
};

static int spa_probe(struct platform_device *pdev) {
	int 		ret;
	dev_t 		devno;

	printk("%s!\n", __func__); 

	/*
	 * register device numbers,
	 * static or dynamic according to the init spa_major
	 */
	if (spa_major > 0) {				/* static register */
		devno 	= MKDEV(spa_major, 0);	/* get the first device number */
		ret 	= register_chrdev_region(devno, SPA_NUMS, DEVICE_NAME);
		if (ret < 0) {
			printk("Can't static register chrdev region!\n");    
			return ret;
		}
	} else { 							/* dynamic alloc */
		ret = alloc_chrdev_region(&devno, 0, SPA_NUMS, DEVICE_NAME) ;
		if (ret < 0) {
			printk("Can't alloc chrdev region!\n");
			return ret;
		}

		/* saved the major number */
		spa_major = MAJOR(devno);
	}

	devt = devno;

	printk("spa_major =  %d(0x%x)\n", spa_major, spa_major);
	printk("devno= %d(0x%x)\n", devno, devno);

	myclass = class_create(DEVICE_NAME "_sys");
	struct device *tmp = NULL;
	tmp = device_create(myclass, NULL, devno, NULL, DEVICE_NAME"_dev");
	if(IS_ERR(tmp)){
		pr_err("%s:%d error code %d\n", __func__, __LINE__, PTR_ERR(tmp));
	}
	
	/*
     * add the char device to system
     */
	cdev_init(&spa_cdev, &spa_fops);

	// spa_cdev.owner 	= THIS_MODULE;
	spa_cdev.ops	= &spa_fops;

	ret = cdev_add(&spa_cdev, devno, SPA_NUMS);
	if (ret < 0) {
		printk("Can't add char device!\n");
		unregister_chrdev_region(devno, SPA_NUMS);
		return ret;
	}

	return 0;
}

static void cdev_cleanup(void) {
	printk("clean up auto gen device node");
	device_destroy(myclass, devt);
	class_destroy(myclass);
}

static int spa_remove(struct platform_device *pdev) {
	printk("%s!\n", __func__); 

	/* remove char device and unregister device number */
	cdev_del(&spa_cdev);
	cdev_cleanup();
    unregister_chrdev_region(MKDEV(spa_major, 0), SPA_NUMS);

	TS_test_stop();
	return 0; 
}

static struct platform_driver spa_pdrv = {
	.driver	= {
		.name 	= DEVICE_NAME,
	},
	.probe 		= spa_probe,
	.remove		= spa_remove,
};

static int __init sparrow_drv_init(void) { 
	int ret;

	printk("Hello world, this is Sparrow Drv!\n"); 

	if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
		pr_err("Cannot allocate major number for device\n");
		return -1;
	}
	pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));

	/*Creating struct class*/
	dev_class = class_create("etx_class");
	if(IS_ERR(dev_class)){
		pr_err("Cannot create the struct class for device\n");
	}

	/*Creating device*/
	if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
		pr_err("Cannot create the Device\n");
	}
	pr_info("Kernel Module Inserted Successfully...\n");
	

 	/*
	 * add spa_pdrv to system
	 */
	ret = platform_driver_register(&spa_pdrv);
	if (ret < 0) {
		printk("Can't register spa_pdev!\n"); 
		return ret;
	}

	return 0;
}

static void __exit sparrow_drv_exit(void) { 
	platform_driver_unregister(&spa_pdrv);
	printk("Bye world, this is Sparrow Drv!\n"); 
	        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        unregister_chrdev_region(dev, 1);
        pr_info("Kernel Module Removed Successfully...\n");
	return; 
} 

module_init(sparrow_drv_init);
module_exit(sparrow_drv_exit);

MODULE_LICENSE("GPL");
