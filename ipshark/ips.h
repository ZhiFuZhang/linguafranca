#ifndef __IPS_H__HEAD
#define __IPS_H__HEAD
#include <linux/inetdevice.h>
enum {
	IPS_WHITE = 0,
	IPS_BLACK,
	IPS_NONE
};

struct devname{
	char name[IFNAMSIZ];
};
struct devname_list{
	int white_black;
	int devnum;
	struct devname *namelist;
};

#endif
