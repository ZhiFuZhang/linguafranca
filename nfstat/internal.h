#ifndef __NFSTAT__INTERNAL__H__
#define __NFSTAT__INTERNAL__H__
#include <linux/types.h>
#include "nfs.h"

void nfsinit(u8 maxtype);
u8 nfstypesize();
bool addipentry(const nfs_ipaddr *ip);
bool rmvipentry(const nfs_ipaddr *ip);
void inccounter(struct nfs_ipaddr *ip, u8 typeidx, u64 bytes);
int readcounter(char __user *buf, size_t len)

/* return typeidx,  -1 not found*/
s16  get_typeidx(const struct nfs_rule *rule) 

#endif
