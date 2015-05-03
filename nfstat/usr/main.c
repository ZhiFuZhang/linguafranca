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
	struct nfs_ipaddr ip = { 
		.len = 4,
		.addr = {
			[0] = 223,
			[1] = 234,
			[2] = 97,
			[3] = 66,
		}	
	
	};
	struct nfs_ipaddr ip2 = { 
		.len = 4,
		.addr = {
			[0] = 223,
			[1] = 234,
			[2] = 97,
			[3] = 166,
		}	
	
	};

	if (fd < 0) {
		printf("open nfs failed, (%d)\n", fd);
	}
	ret = nfs_set_type_num(fd, 4);
	if (ret == 0) {
		printf("nfs_init failed, %d", ret);	
		//return -1;
	}
	nfs_addip(fd, &ip);
	
	nfs_addip(fd, &ip2);
	return 0;
}
