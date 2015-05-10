/*
 * common definition for kernel and userspace.
 *
 * (C) 2015  Zhi Fu Zhang <zzfooo@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 */

#ifndef __NFS__H__
#define __NFS__H__

#include <linux/types.h>

struct nfs_ipaddr {
	__u8 len;
	__u8 padding[3];
	__u8 addr[16]; /*the max len of ip is 16*/
} __attribute__((aligned(sizeof(int))));


static inline const char *nfs_port2str(__u8 port, char *buf)
{
	if (port == 0){
		sprintf(buf, "ANY");
	} else {
		sprintf(buf, "%d", port);
	}
	return buf;	
}
/*ipstr buffer size */
#define NFS_IPSTR 40
static inline const char *
nfs_ip2str(const struct nfs_ipaddr *addr, char *buf)
{
        int l = 0;
        int i = 0;
        int first = -1;
        __u16 *p = (__u16 *)addr->addr;
        char *save = buf;
        if (addr->len == 4) {
                sprintf(buf, "%d.%d.%d.%d", addr->addr[0],
                                addr->addr[1], addr->addr[2], addr->addr[3]);
                return save;
        } else if (addr->len == 6) {
                for (i = 0; i < sizeof(addr->addr)/sizeof(__u16); i++, p++) {
                        if (first == -1 && *p == 0){
                                first = i;
                                l += sprintf(buf + l, "::");
                        } else if (first != -1 && first != -2 && *p == 0) {
                                continue;
                        } else {
                                if (first != -1 && *p != 0) {
                                        first = -2;
                                }
                                l += sprintf(buf + l, "%04x", *p);
                                if (i + 1!= sizeof(addr->addr)/sizeof(__u16)){
                                        l += sprintf(buf + l,  ":");
                                }
                        }
                }
                return save;
        } else {

		sprintf(buf, "ANY");
                return buf;
        }
}

/*
 *  for lip and rip,  len 0 means it does NOT care ip addr.
 *  for lport and rport, 0 means it does NOT car ip port.
 */
#define NFS_ALL	0
#define NFS_IN	1
#define NFS_OUT	2
#define NFS_FORWARD	3
static inline const char *nfs_dir2str(__u8 dir)
{
	switch (dir) {
	case NFS_ALL: return "ALL";
	case NFS_IN:  return "IN";
	case NFS_OUT: return "OUT";
	case NFS_FORWARD: return "FORWARD";
	default:
		return "unknown";
	}

}
struct nfs_rule {
	struct nfs_ipaddr lip;
	struct nfs_ipaddr rip;
	__u16 lport;
	__u16 rport;
	/* ipv6 special protocol type shall be coverted to ip4 relevant protocol
	 * types, such as we do NOT use icmpv6, but consider it as icmp. if 
	 * protocol is IPPROTO_HOPOPTS, we shall check the next_hdr field.
	 */
	__u8 protocol;
	__u8 dir; /* 0 all, 1 in, 2 out, 3 forward */
	__u8 typeidx;  
	__u8 padding;
} __attribute__((aligned(sizeof(long))));

#define  NFS_CMD_MAGIC 'n'

enum {
NFS_CMD_INIT_NUM,
#define  NFS_CMD_INIT  _IOW(NFS_CMD_MAGIC, NFS_CMD_INIT_NUM, __u8)
NFS_CMD_ADDIP_NUM,
#define  NFS_CMD_ADDIP  _IOW(NFS_CMD_MAGIC, NFS_CMD_ADDIP_NUM, struct nfs_ipaddr) 
NFS_CMD_RMVIP_NUM,
#define  NFS_CMD_RMVIP  _IOW(NFS_CMD_MAGIC, NFS_CMD_RMVIP_NUM, struct nfs_ipaddr)
NFS_CMD_ADDRULE_NUM,
#define  NFS_CMD_ADDRULE  _IOW(NFS_CMD_MAGIC, NFS_CMD_ADDRULE_NUM, struct nfs_rule)
NFS_CMD_RMVRULE_NUM,
#define  NFS_CMD_RMVRULE  _IOW(NFS_CMD_MAGIC, 5, struct nfs_rule)
NFS_CMD_GETCOUNTER_NUM,
#define  NFS_CMD_GETCOUNTER  _IOR(NFS_CMD_MAGIC, 6, __u8)
NFS_CMD_MAX
};
#define NFS_DEV_NAME "nf-stat"
#define NFS_DEV_FILE "/dev/"NFS_DEV_NAME
#endif
