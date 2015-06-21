#ifndef __IPS_H__HEAD
#define __IPS_H__HEAD
#include <linux/types.h>
enum {
	IPS_WHITE = 0,
	IPS_BLACK,
	IPS_NONE
};
#define IFNAMSIZ 16
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
		char s[16];
	};
	union {
		char d4[4];
		char d6[16];
		char d[16];
	};

#define IPS_IN	1
#define IPS_OUT	2
#define IPS_FORWARD	3
	__u8 version:4,
	     direct:4;
	__u8 protocol;
	__u16 sport;
	__u16 dport;
	__u16 totallen;
};

struct ip_key_info_set{
	__u32 n;
	union {
		struct ip_key_info *array;
		char *buf;
	};
};


#define IPS_DEV_NAME "ipshark"
#define IPS_DEV_FILE "/dev/"IPS_DEV_NAME
#define IPS_DEV_PROC "driver/ipshark"

#define IPS_CMD_MAGIC 'i'


enum IPS_CMD {
	IPS_SET_NAMELIST = 0,
#define  IPS_SET_NAMELIST _IOW(IPS_CMD_MAGIC, IPS_SET_NAMELIST, struct devname_list)
	IPS_FETCH_INFO,
#define IPS_FETCH_INFO _IOR(IPS_CMD_MAGIC, IPS_FETCH_INFO, struct devname_list)

};

#endif
