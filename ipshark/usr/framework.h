#ifndef __FRAMWORK_H__
#define __FRAMWORK_H__
#include "ips_api.h"
/**
 * ips_frame_register - register the function that is used to handle ip key info
 * @handle:  the function
 * @newthreadused: in the handle function how many new thread used.
 */
void ips_frame_register(void (*handle)(struct ip_key_info_set s), int newthreadused);
int ips_frame_start(struct devname_list *l);
void ips_frame_destroy(struct ip_key_info_set *s);
void ips_frame_stop();
#endif
