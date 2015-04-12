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

/*
 *  for lip and rip,  len 0 means it does NOT care ip addr.
 *  for lport and rport, 0 means it does NOT car ip port.
 */
static const __u8 NFS_ALL = 0;
static const __u8 NFS_IN = 1;
static const __u8 NFS_OUT = 2;
static const __u8 NFS_FORWARD = 3;
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

#define  NFS_CMD_INIT  0
#define  NFS_CMD_ADDIP  1
#define  NFS_CMD_RMVIP  2
#define  NFS_CMD_ADDRULE  3
#define  NFS_CMD_RMVRULE  4
#define  NFS_CMD_GETCOUNTER  5

#endif
