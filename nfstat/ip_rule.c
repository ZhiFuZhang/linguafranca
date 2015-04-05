/*
 * A structure to store the rules which the user want to stat.
 *
 * (C) 2015  Zhi Fu Zhang <zzfooo@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

static struct rb_root ruletree = RB_ROOT;
DEFINE_RWLOCK(ruletreelock);

static inline struct nfs_rule_entry *new()
{
	struct nfs_rule_entry *entry = kzalloc(sizeof(struct nfs_rule_entry),
		    GFP_KERNEL);
	return entry;
}

static inline void delete(struct nfs_rule_entry *entry)
{
	kfree(entry);
}

static inline int portcmp(u16 src, u16 dst)
{
	if (src != 0 && dst != 0){
		if (src > dst) {
			return 1;
		} else if (src < dst) {
			return -1;
		}
	}
	return 0;
}
static inline int ipcmp(const struct nfs_ipaddr *src,
		const struct nfs_ipaddr *dst)
{
	if (src->len != 0 && dst->len != 0) {
		return memcmp(src, dst, sizeof(struct nfs_ipaddr));
	}
	return 0;
}
static int rulecmp(const nfs_rule *src, const nfs_rule *dst)
{
	int ret = 0;
	if (src->protocal > dst->protocal) {
		return 1;
	} else if (src->protocal < dst ->protocal) {
		return -1;
	} 
	if (src->dir > dst->dir) {
		return 1;
	} else if (src->dir < dst->dir) {
		return -1;
	}
	ret = portcmp(src->lport; dst->lport);
	if (ret !=0) return ret;
	ret = portcmp(src->rport, dst->rport);
	if (ret !=0) return ret;
	ret = ipcmp(src->lip, dst->lip);
	if (ret !=0) return ret;
	ret = ipcmp(src->rip, dst->rip);
	if (ret !=0) return ret;
	return 0;
}
