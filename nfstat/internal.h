#ifndef __NFSTAT__INTERNAL__H__
#define __NFSTAT__INTERNAL__H__
#include <linux/types.h>
#include "nfs.h"
struct nfs_counter_vector {
	u64 *number;
	u64 *bytes;
};
struct ip_counter_entry {
	struct rb_node node;
	nfs_ipaddr ip;/*0:addlen, 1~17 addr */
	struct nfs_counter_vector *counter
};

void nfsinit(u8 maxtype);
struct ip_counter_entry *findipentry(const nfs_ipaddr *ip);
bool addipentry(const nfs_ipaddr *ip);
bool rmvipentry(const nfs_ipaddr *ip);
void incpkgnumber(struct ip_counter_entry *entry, u8 typeidx);
void incpkgbytes(struct ip_counter_entry *entry, u8 typeidx, u64 bytes);

void getcounter_ip(const struct ip_counter_entry *entry,
		struct nfs_counter_vector * vector);



#endif
