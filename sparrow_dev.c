/*
 * sparrow_dev.c
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
#include <linux/platform_device.h>

#define DEVICE_NAME			"sparrow"

void release_nothing(struct device *dev)
{
	return;
}

static struct platform_device spa_pdev = {
	.name 	= DEVICE_NAME,
	.id 	= -1,
	/* init .dev.release to fix Device 'xxx' does not have a release() function,
	  it is broken and must be fixed, warning */
	.dev    = {
		.release = release_nothing
	}
};

static int __init sparrow_dev_init(void) { 
	int 		ret;
	printk("Hello world, this is Sparrow Dev!\n"); 

	/*
	 * add spa_pdev to system
	 */
	ret = platform_device_register(&spa_pdev);
	if (ret < 0) {
		printk("Can't register spa_pdev!\n"); 
		return ret;
	}

	return 0; 
}

static void __exit sparrow_dev_exit(void) { 
	/*
	 * there will be a WARNING(drivers/base/core.c, device_release):
	 * Device 'xxx' does not have a release() function,
	 * it is broken and must be fixed,
	 * think it, WHY??
	 */

	// Answer: because the release function of the dev in the platform device is not initialized.
	//         platform_device_register() will initialize the pdev->dev by device_initialize(). However,
	//         NOT all field is inited, dev.release is an example
	// Except the "release_nothing()" approach, we can use platform_device_alloc() & platform_device_add() 
	// instead to avoid this warning.
	// see: https://stackoverflow.com/questions/15532170/device-has-no-release-function-what-does-this-mean
	platform_device_unregister(&spa_pdev);

	printk("Bye world, this is Sparrow Dev!\n"); 
	return; 
} 

module_init(sparrow_dev_init);
module_exit(sparrow_dev_exit);

MODULE_LICENSE("GPL");
