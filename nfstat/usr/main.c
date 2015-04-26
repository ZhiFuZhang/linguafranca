/*
 *  the demo of using nfstat
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
#include "nfs_api.h"
int main()
{
	int fd = nfs_open();
	int ret = 0;
	struct nfs_ipaddr ip = { 0 };
	if (fd < 0) {
		printf("open nfs failed, (%d)\n", fd);
	}
	ret = nfs_init(fd, 4);
	if (ret != 0) {
		printf("nfs_init failed, %d", ret);	
		return -1;
	}
	nfs_addip(fd, &ip);
	
	return 0;
}
