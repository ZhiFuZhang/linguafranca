#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "ut.h"
static  void __exit  ips_exit(void)
{

	pr_info(IPS"ips_exit\n");
	return;
}
static int __init ips_init(void)
{
	int err = 0;
	pr_info(IPS"ips_init\n");
#ifdef DEBUG
	pr_info(IPS"debug mode is open\n");
#endif
	runtestsuite(devset_ut);
	runtestsuite(ip_queue_ut);
	return err;
}


module_init(ips_init);
module_exit(ips_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhi Fu Zhang <zzfooo@hotmail.com>");
MODULE_DESCRIPTION("ipshark2, ut");
MODULE_VERSION("0.1");


