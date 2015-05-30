#include <linux/kernel.h>
#include <linux/module.h>
#include "intern.h"

static  void __exit  ips_exit(void)
{

	pr_info(IPS"ips_exit\n");
	return;
}
static int __init ips_init(void)
{

	pr_info(IPS"ips_init\n");
	return 0;
}


/* following code is for unit test */
#ifdef  UNITTEST
static  void __exit  ips_ut_exit(void)
{
	pr_info(IPS"ips_ut_exit\n");
}
static int __init ips_ut_init(void)
{
	
	pr_info(IPS"ips_ut_init\n");
	devset_ut();
	return 0;
}

module_init(ips_ut_init);
module_exit(ips_ut_exit);

#else

module_init(ips_init);
module_exit(ips_exit);

#endif

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhi Fu Zhang <zzfooo@hotmail.com>");
MODULE_DESCRIPTION("ipshark");
MODULE_VERSION("0.1");


