#include "nfs_api.h"

int nfs_open(void)
{
	int fd = open(NFS_DEV_FILE, O_RDWR);
	return fd;
}
int nfs_init(int fd, __u8 maxtypenum)
{
	return -ioctl(fd,  NFS_CMD_INIT,  &maxtypenum);

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
