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
struct nfs_ipaddr {
	u8 len;
	u8 padding[3];
	u8 addr[16]; /*the max len of ip is 16*/
} __attribute__((aligned(sizeof(long))));

/*
 *  for lip and rip,  len 0 means it does NOT care ip addr.
 *  for lport and rport, 0 means it does NOT car ip port.
 */
static const u8 NFS_ALL = 0;
static const u8 NFS_IN = 1;
static const u8 NFS_OUT = 2;
static const u8 NFS_FORWARD = 3;
struct nfs_rule {
	struct nfs_ipaddr lip;
	struct nfs_ipaddr rip;
	u16 lport;
	u16 rport;
	u8 protocol;
	u8 dir; /* 0 all, 1 in, 2 out, 3 forward */
	u8 typeidx;  
	u8 padding;
} __attribute__((aligned(sizeof(long))));

#define  NFS_CMD_INIT  0
#define  NFS_CMD_ADDIP  1
#define  NFS_CMD_RMVIP  2
#define  NFS_CMD_ADDRULE  3
#define  NFS_CMD_RMVRULE  4
#define  NFS_CMD_GETCOUNTER  5

#endif
