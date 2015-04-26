#include <fcntl.h>
#include <sys/ioctl.h>
#include "../nfs.h"
#ifndef  __NFS_API_H__
#define  __NFS_API_H__
int nfs_open(void);
int nfs_init(int fd, __u8 maxtypenum);
int nfs_addip(int fd, const struct nfs_ipaddr *ip);
int nfs_delip(int fd, const struct nfs_ipaddr *ip);
int nfs_addrule(int fd, const struct nfs_rule *rule);
int nfs_delrule(int fd, const struct nfs_rule *rule);

#endif
