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
struct ips_cpu_queue_size{
	short cpuidx;
	unsigned short size;
}
struct ips_config {
	size_t dma_size;
	struct ips_cpu_queue_size queue_size[4];
	unsigned short default_queue_size;
	unsigned short unused;
	int white_black;
	int devnum;
	struct devname devlist[0];
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
enum {
	MEM_FREE =0,
	MEM_INUSE,
	MEM_USED
};

struct ip_key_info_wrap{
	struct ip_key_info info;
	unsigned int times:24,
		     state:8;
};

struct ips_dma_array{
	union {
		unsigned long long v;
#define	ips_dma_array_size (sizeof(unsigned long long)/sizeof(unsigned short))
		unsigned short element[ips_dma_array_size];
	};
};



#define IPS_DEV_NAME "ipshark2"
#define IPS_DEV_FILE "/dev/"IPS_DEV_NAME
#define IPS_DEV_PROC "driver/ipshark2"

#define IPS_CMD_MAGIC 's'

enum IPS_CMD {
	IPS_CONFIG = 0,
#define  IPS_CONFIG _IOW(IPS_CMD_MAGIC, IPS_SET_NAMELIST, struct ips_config)
	IPS_FETCH_INFO,
#define IPS_FETCH_INFO _IOR(IPS_CMD_MAGIC, IPS_FETCH_INFO, unsigned long long)
	IPS_FREE_MEM,
#define IPS_FETCH_INFO _IOR(IPS_CMD_MAGIC, IPS_FETCH_INFO, unsigned long long)

};

#endif
