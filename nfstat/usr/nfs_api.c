 /*
 * the api(userspace) implementions for calling nfstat module
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

#include <string.h>
#include "nfs_api.h"

int nfs_open(void)
{
	int fd = open(NFS_DEV_FILE, O_RDONLY);
	return fd;
}


int nfs_set_type_num(int fd, __u8 maxtypenum)
{
	int err = ioctl(fd,  NFS_CMD_INIT,  &maxtypenum);
	if (err == 0) return maxtypenum;
	return nfs_get_type_num(fd);
}

__u8 nfs_get_type_num(int fd)
{
	__u8 maxtypenum = 0;
	int err = 0;
	err = ioctl(fd, NFS_CMD_GETTYPEMAX, &maxtypenum);
	if (err == 0) return maxtypenum;
	printf("nfs_get_type_num err[%d]\n", err);
	return 0;
}
int nfs_addip(int fd, const struct nfs_ipaddr *ip)
{
	return -ioctl(fd, NFS_CMD_ADDIP, ip);
}
int nfs_delip(int fd, const struct nfs_ipaddr *ip)
{
	return -ioctl(fd, NFS_CMD_RMVIP, ip);
}
int nfs_addrule(int fd, const struct nfs_rule *rule)
{
	return -ioctl(fd, NFS_CMD_ADDRULE, rule);
}

int nfs_delrule(int fd, const struct nfs_rule *rule)
{
	return -ioctl(fd, NFS_CMD_RMVRULE, rule);
}

int nfs_getcounter(int fd, const struct nfs_ipaddr *ip, char *data, size_t len)
{
	struct nfs_data d = {.len =0,};
	if(ip == NULL || data == NULL) return -1;
	d.len = len,
	memcpy(&d.ip, ip , sizeof(struct nfs_ipaddr));
	memcpy(data, &d, sizeof(struct nfs_data));
	return ioctl(fd, NFS_CMD_GETCOUNTER, data);
	return 0;
}
