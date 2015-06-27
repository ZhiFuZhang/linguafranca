#ifndef __IPS_API__H_
#define __IPS_API__H_

#include "../ips.h"
int ips_init(void);
void ips_exit(void);
int ips_fetch(struct ip_key_info_set *s);
int ips_config(struct devname_list *l);
#endif

