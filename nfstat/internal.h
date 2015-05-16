/*
 *  internal.h - definitions for the nfstat module,
 *  it is used internal of the kernel only .
 *
 * (C) 2015  Zhi Fu Zhang <zzfooo@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 */

#ifndef __NFSTAT__INTERNAL__H__
#define __NFSTAT__INTERNAL__H__
#include <linux/proc_fs.h>
#include <linux/rbtree.h>
#include <linux/types.h>
#include "nfs.h"

void nfsinit(u8 maxtype);
u8 nfstypesize(void);
bool addipentry(const struct nfs_ipaddr *ip);
bool rmvipentry(const struct nfs_ipaddr *ip);
void inccounter(const struct nfs_ipaddr *ip, u8 typeidx, u64 bytes);

bool nfs_getcounter_perip(const struct nfs_ipaddr *ip, char __user *data, size_t len);
int readcounter(char *buf, size_t len);
void clear_iptree(void);

/* return typeidx,  -1 not found*/
s16  get_typeidx(const struct nfs_rule *rule);
bool addnfsrule(const struct nfs_rule *rule);
bool rmvnfsrule(const struct nfs_rule *rule);
void clear_nfsrule(void);
#define  BASEDIR "nfs_debug"
#define  COUNTERDIR "ipcounter"
#define  RULEDIR "rule"
extern struct proc_dir_entry    *ipcounter_dir;
extern struct proc_dir_entry    *rule_dir;

static inline void hexdump(unsigned char *data, size_t len) {
	int i = 0;
	for (i = 0; i < len; i ++){
		pr_info("data:[%02x]\n", data[i]);
	}
}

#ifndef rbtree_postorder_for_each_entry_safe
static inline struct rb_node *nfs_rb_left_most(const struct rb_node *node)
{
	if (node == NULL) return NULL;
	for (;;) {
                if (node->rb_left)
                        node = node->rb_left;
                else if (node->rb_right)
                        node = node->rb_right;
                else
                        return (struct rb_node *)node;
        }

}
static inline struct rb_node *nfs_rb_first(const struct rb_root *root)
{
	return nfs_rb_left_most(root->rb_node);

}
static inline struct rb_node *nfs_rb_next(const struct rb_node *node)
{
	struct rb_node *parent;
	if (node == NULL) return NULL;
	parent = rb_parent(node);
	if (parent == NULL) return parent;
	if (node != parent->rb_left) return parent; 
	if (parent->rb_right == NULL) return parent;
	return nfs_rb_left_most(parent->rb_right);
}

#define nfs_rb_entry_safe(ptr, type, member) \
        ({ typeof(ptr) ____ptr = (ptr); \
	            ____ptr ? rb_entry(____ptr, type, member) : NULL; \
	         })

#define nfs_rbtree_postorder_for_each_entry_safe(pos, n, root, field) \
	        for (pos = nfs_rb_entry_safe(nfs_rb_first(root), typeof(*pos), field); \
				             pos && ({ n = nfs_rb_entry_safe(nfs_rb_next(&pos->field), \
							                             typeof(*pos), field); 1; }); \
				             pos = n)
#else

#define nfs_rbtree_postorder_for_each_entry_safe rbtree_postorder_for_each_entry_safe
#endif

#endif
