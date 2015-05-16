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
#include <linux/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "nfs_api.h"
int main()
{
	int fd = nfs_open();
	int ret = 0;
	struct nfs_ipaddr ip = { 
		.len = 4,
		.addr = {
			[0] = 192,
			[1] = 168,
			[2] = 1,
			[3] = 104,
		}	
	
	};
	struct nfs_ipaddr ip2 = { 
		.len = 4,
		.addr = {
			[0] = 10,
			[1] = 0,
			[2] = 3,
			[3] = 15,
		}	
	
	}; 
	char *buf = NULL;
	if (fd < 0) {
		printf("open nfs failed, (%d)\n", fd);
		return -1;
	}
	printf("the current type num is [%d]\n", nfs_get_type_num(fd));
	ret = nfs_set_type_num(fd, 4);
	printf("nfs set num %d\n", ret);
	if (ret == 0) {
		printf("nfs_init failed, %d", ret);	
		return -1;
	}
	size_t len = ret *  sizeof(struct nfs_counter_vector);
	buf = malloc(len);
	memset(buf, 0xec, len);
	nfs_addip(fd, &ip);

	struct nfs_rule rule = {
		.lip = {
			.len = 0,
		},
		.rip = {
			.len = 0,
		},
		.lport = 0,
		.rport = 0,
		.protocol = IPPROTO_ICMP,
		.dir = NFS_ALL,
		.typeidx = 0,

	};

	struct nfs_rule rule2 = {
		.lip = {
			.len = 0,
		},
		.rip = {
			.len = 0,
		},
		.lport = 0,
		.rport = 0,
		.protocol = IPPROTO_TCP,
		.dir = NFS_OUT,
		.typeidx = 1,
	};
	struct nfs_rule rule3 = {
		.lip = {
			.len = 0,
		},
		.rip = {
			.len = 0,
		},
		.lport = 0,
		.rport = 80,
		.protocol = IPPROTO_TCP,
		.dir = NFS_IN,
		.typeidx = 2,
	};
	struct nfs_rule rule4 = {
		.lip = {
			.len = 0,
		},
		.rip = {
			.len = 0,
		},
		.lport = 0,
		.rport = 80,
		.protocol = IPPROTO_TCP,
		.dir = NFS_OUT,
		.typeidx = 3,

	};
	nfs_addrule(fd, &rule);

	nfs_addrule(fd, &rule3);
	nfs_addrule(fd, &rule4);
	nfs_addrule(fd, &rule2);
	nfs_addip(fd, &ip2);

	sleep(10);
	nfs_getcounter(fd, &ip, buf, len);
	size_t i = 0;
	struct nfs_counter_vector *vp = (struct nfs_counter_vector *)buf;
	for (i = 0; i < len/sizeof(struct nfs_counter_vector); i++) {
		printf("(%ld)pkts:%llu, bytes:%llu\n",i, vp[i].number, vp[i].bytes);
	}
	nfs_delrule(fd, &rule);

	sleep(10);
	ret = nfs_delip(fd, &ip);
	printf("delip [%d]\n", ret);
	return 0;
}
