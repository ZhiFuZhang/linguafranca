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

struct ip_key_info{
	struct devname in;
	struct devname out;
	/* C11 ISO, unamed field */
	union{	
		char s4[4];
		char s6[16];
	};
	union {
		char d4[4];
		char d6[16];
	};
	__u8 version:4,
	     direct:4;
	__u8 protocal;
	__u16 sport;
	__u16 dport;
	__u16 totallen;
};

#endif
