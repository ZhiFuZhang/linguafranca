#ifndef __IP_QUEUE_H__
#define __IP_QUEUE_H__
#include "intern.h"
void *ip_queue_dma_addr(void);
int ip_queue_create(const struct ips_cpu_queue_size *s,
		int num, unsigned short default_size, unsigned short ms);
void ip_queue_recycle(unsigned short *idlist, int size);
void ip_queue_move2dma(void);
void ip_queue_put(const struct ip_key_info *src);
void ip_queue_exit(void);
void ip_queue_wakeup(void);
long ip_queue_wait(void);
bool ip_queue_has_data(void);
void ip_queue_show(struct seq_file *m);
#endif
