/*
 * the api(userspace) definitions for calling nfstat module
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

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "../nfs.h"
#ifndef  __NFS_API_H__
#define  __NFS_API_H__
int nfs_open(void);

/* return the maxtypenum */

int nfs_set_type_num(int fd, __u8 maxtypenum);
int nfs_addip(int fd, const struct nfs_ipaddr *ip);
int nfs_delip(int fd, const struct nfs_ipaddr *ip);
int nfs_addrule(int fd, const struct nfs_rule *rule);
int nfs_delrule(int fd, const struct nfs_rule *rule);

#endif
