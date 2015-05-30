#include <linux/dcache.h>
#include <linux/hash.h>
#include <linux/jhash.h>
#include <linux/kernel.h>
#include "intern.h"
static const unsigned int DEVNAME_HASH_BITS = 16;






static u32 devname_jhash(const char * devname)
{
	u32 a = jhash(devname, strlen(devname), 379);
	pr_debug(IPS"str[%s], hash value[%02x]\n", devname, a);
	return a;
}
static u32 devname_hash(const char * devname)
{
	u32 a = full_name_hash(devname, strlen(devname));
	pr_debug(IPS"str[%s], full name hash value[%02x]\n", devname, a);
	a = hash_32(a, DEVNAME_HASH_BITS);
	return a;
}

#ifdef  UNITTEST
int devset_ut()
{
	(void)devname_hash("eth1.10");
	(void)devname_jhash("eth1.10");
	(void)devname_hash("eth1.11");
	(void)devname_jhash("eth1.11");
	(void)devname_hash("eth1.12");
	(void)devname_jhash("eth1.12");
	(void)devname_hash("eth1.13");
	(void)devname_jhash("eth1.13");
	(void)devname_hash("eth0.12");
	(void)devname_jhash("eth0.12");
	(void)devname_hash("eth1");
	(void)devname_jhash("eth1");
	(void)devname_hash("enp0s8");
	(void)devname_jhash("enp0s8");
	(void)devname_hash("enp0s3");
	(void)devname_jhash("enp0s3");

	return 0;
}

#endif
