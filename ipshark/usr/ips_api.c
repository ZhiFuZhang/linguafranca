#include "../ips.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/errno.h>

static int fd = -1;
int ips_init(void)
{
	int nfd = 0;
	if (fd >= 0) {
		return -EEXIST;
	}
	nfd = open(IPS_DEV_FILE, O_RDONLY);
	if (nfd < 0) {
		return -ENOENT;
	}
	fd = nfd;
	return 0;
}

void ips_exit(void)
{
	if (fd >= 0)
		close(fd);
	fd = -1;
}

int ips_fetch(struct ip_key_info_set *s)
{
	if (fd < 0) {
		return -ENXIO;
	}
	return ioctl(fd, IPS_FETCH_INFO, (long)s);
}

int ips_config(struct devname_list *l)
{
	if (fd < 0) {
		return -ENXIO;
	}
	if (l->devnum <= 0) return -EINVAL;

	return ioctl(fd, IPS_SET_NAMELIST,(long)l);
}

