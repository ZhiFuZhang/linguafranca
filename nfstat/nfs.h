#ifndef __NFS__H__
#define __NFS__H__
struct nfs_ipaddr {
	u8 len;
	u8 addr[16]; /*the max len of ip is 16*/
};

#endif
