#ifndef __INTERN_H__HEAD
#define __INTERN_H__HEAD

#include "ips.h"
#define IPS "ipshark:"

struct dev_entry{
	struct devname n;
	u32 hash;
	struct hlist_node node;
};

#ifdef  UNITTEST
#undef IPS
#define IPS "ipshark_ut:"
/* return faile test case num */
int devset_ut(void);
#else
#define devset_ut 0
#endif

#endif
