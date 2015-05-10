/*
 * the entrance of the module nfstat.
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




#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <linux/igmp.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/proc_fs.h>
#include <linux/sctp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include "internal.h"

struct proc_dir_entry    *ipcounter_dir = NULL;
struct proc_dir_entry    *rule_dir = NULL;



unsigned int hookfn(const struct nf_hook_ops *ops,
                               struct sk_buff *skb,
                               const struct net_device *in,
                               const struct net_device *out,
                               int (*okfn)(struct sk_buff *))
{
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		struct sctphdr *sctp;
		struct icmphdr *icmp;
		struct igmphdr *igmp;
	} transdata;

	struct nfs_rule rule = {
		.lip = {0},
		.rip = {0},
		.lport = 0,
		.rport = 0,
	};
	s16 idx = 0;
	struct iphdr *hdr = ip_hdr(skb);
	if (hdr == NULL) return NF_ACCEPT;
	if (hdr->version != 4) return NF_ACCEPT;
	if (in != NULL)
		rule.dir += 1;
	if (out != NULL)
		rule.dir += 2;
	rule.protocol = hdr->protocol;
	rule.lip.len = 4;
	rule.rip.len = 4;
	if (in != NULL) {
		memcpy(&rule.lip.addr, &hdr->daddr, rule.lip.len);
		memcpy(&rule.rip.addr, &hdr->saddr, rule.rip.len);
	} else {
		memcpy(&rule.lip.addr, &hdr->saddr, rule.lip.len);
		memcpy(&rule.rip.addr, &hdr->daddr, rule.rip.len);
	}
	switch (hdr->protocol){
	case IPPROTO_TCP:
		transdata.tcp = tcp_hdr(skb);
		if (transdata.tcp == NULL) return NF_ACCEPT;
		rule.lport =  (rule.dir == NFS_OUT ) ? transdata.tcp->source
			: transdata.tcp->dest;

		rule.rport =  (rule.dir == NFS_OUT ) ? transdata.tcp->dest
			: transdata.tcp->source;
		break;
	case IPPROTO_UDP:
		transdata.udp = udp_hdr(skb);
		if (transdata.udp == NULL) return NF_ACCEPT;
		rule.lport =  (rule.dir == NFS_OUT ) ? transdata.udp->source 
			: transdata.udp->dest;

		rule.rport =  (rule.dir == NFS_OUT ) ? transdata.udp->dest 
			: transdata.udp->source;
		break;
	case IPPROTO_SCTP:
		transdata.sctp = sctp_hdr(skb);
		if (transdata.sctp == NULL) return NF_ACCEPT;
		rule.lport =  (rule.dir == NFS_OUT ) ? transdata.sctp->source
			: transdata.sctp->dest;

		rule.rport =  (rule.dir == NFS_OUT ) ? transdata.tcp->dest
			: transdata.sctp->source;

		break;
	case IPPROTO_ICMP:
		transdata.icmp = icmp_hdr(skb);
		if (transdata.icmp == NULL) return NF_ACCEPT;
	case IPPROTO_IGMP:
		transdata.igmp = igmp_hdr(skb);
		if (transdata.igmp == NULL) return NF_ACCEPT;
		break;

	}

	rule.lport = ntohs(rule.lport);
	rule.rport = ntohs(rule.rport);
	idx =  get_typeidx(&rule);
	if (idx < 0 || idx > 255) return NF_ACCEPT;

	inccounter(&rule.lip, idx,  skb->len);
	return NF_ACCEPT;

}


/* ipv6 need some improment, will do it later */
unsigned int hookfn6(const struct nf_hook_ops *ops,
                               struct sk_buff *skb,
                               const struct net_device *in,
                               const struct net_device *out,
                               int (*okfn)(struct sk_buff *))
{
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		struct sctphdr *sctp;
		struct icmp6hdr *icmp;
	} transdata;

	struct nfs_rule rule = {
		.lip = {0},
	};
	s16 idx = 0;
	struct ipv6hdr *hdr = ipv6_hdr(skb);
	if (hdr == NULL) return NF_ACCEPT;
	if (hdr->version != 6) return NF_ACCEPT;
	if (in != NULL)
		rule.dir += 1;
	if (out != NULL)
		rule.dir += 2;
	rule.protocol = hdr->nexthdr;
	rule.lip.len = 6;
	rule.rip.len = 6;
	if (in != NULL) {
		memcpy(&rule.lip.addr, &hdr->daddr, sizeof(hdr->daddr));
		memcpy(&rule.rip.addr, &hdr->saddr, sizeof(hdr->saddr));
	} else {
		memcpy(&rule.lip.addr, &hdr->saddr, sizeof(hdr->saddr));
		memcpy(&rule.rip.addr, &hdr->daddr, sizeof(hdr->daddr));
	}
	switch (rule.protocol){
	case IPPROTO_TCP:
		transdata.tcp = tcp_hdr(skb);
		if (transdata.tcp == NULL) return NF_ACCEPT;
		rule.lport =  (rule.dir == NFS_OUT ) ? transdata.tcp->source
			: transdata.tcp->dest;

		rule.rport =  (rule.dir == NFS_OUT ) ? transdata.tcp->dest
			: transdata.tcp->source;
		break;
	case IPPROTO_UDP:
		transdata.udp = udp_hdr(skb);
		if (transdata.udp == NULL) return NF_ACCEPT;
		rule.lport =  (rule.dir == NFS_OUT ) ? transdata.udp->source 
			: transdata.udp->dest;

		rule.rport =  (rule.dir == NFS_OUT ) ? transdata.udp->dest 
			: transdata.udp->source;
		break;
	case IPPROTO_SCTP:
		transdata.sctp = sctp_hdr(skb);
		if (transdata.sctp == NULL) return NF_ACCEPT;
		rule.lport =  (rule.dir == NFS_OUT ) ? transdata.sctp->source
			: transdata.sctp->dest;

		rule.rport =  (rule.dir == NFS_OUT ) ? transdata.tcp->dest
			: transdata.sctp->source;

		break;
	case IPPROTO_ICMPV6:
		transdata.icmp = icmp6_hdr(skb);
		if (transdata.icmp == NULL) return NF_ACCEPT;
	case IPPROTO_HOPOPTS:
		break;

	}
	rule.lport = ntohs(rule.lport);
	rule.rport = ntohs(rule.rport);
	idx =  get_typeidx(&rule);
	if (idx < 0 || idx > 255) return NF_ACCEPT;

	inccounter(&rule.lip, idx, skb->len);
	return NF_ACCEPT;


}

long nfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	union {
		u8 maxtype;
		struct nfs_ipaddr ip;
		struct nfs_rule rule;
	} data;
	bool ret = true;
	u8 s = 0;
	int err = 0;
	int cmdnum = 0;
	if (_IOC_TYPE(cmd) != NFS_CMD_MAGIC) return -ENOTTY;
	cmdnum = _IOC_NR(cmd);
	pr_debug("NFS_CMD_NUM:[%d]\n", cmdnum);
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;

	if (cmd != NFS_CMD_INIT) {
		if (nfstypesize() == 0) {
			return -EIO;
		}
	}
	switch (cmd) {
	case NFS_CMD_INIT:
		if (nfstypesize() != 0) return  -EINVAL;

		if (copy_from_user(&data.maxtype, (u8 *)arg, sizeof(u8)))
			return -EFAULT;
		nfsinit(data.maxtype);
		break;
	case NFS_CMD_ADDIP:
		if (copy_from_user(&data.ip, (u8 *)arg,
					sizeof(struct nfs_ipaddr))) 
			return -EFAULT;
		ret = addipentry(&data.ip);

		break;
	case NFS_CMD_RMVIP: 
		pr_debug("NFS_CMD_RMVIP\n");
		if (copy_from_user(&data.ip, (u8 *)arg,
					sizeof(struct nfs_ipaddr))) 
			return -EFAULT;

		ret = rmvipentry(&data.ip);


		break;
	case NFS_CMD_ADDRULE:
		if (copy_from_user(&data.rule, (u8 *)arg,
					sizeof(struct nfs_rule))) 
			return -EFAULT;
		ret = addnfsrule(&data.rule);


		break;
	case NFS_CMD_RMVRULE:
		if (copy_from_user(&data.rule, (u8 *)arg,
					sizeof(struct nfs_rule))) 
			return -EFAULT;
		ret = rmvnfsrule(&data.rule);

		break;
	case NFS_CMD_GETCOUNTER:
		s = nfstypesize();
		if (copy_to_user((u8 *)arg,  &s, sizeof(s))) return -EFAULT;
		break;
	default:
		return -EFAULT;
	}
	if (ret) return 0;
	return -EIO;
}
ssize_t nfs_read(struct file *filep, char __user *data, size_t len, loff_t *f_ops)
{
	char *buf = vmalloc(len);
	int ret = -1;
	int s =  readcounter(buf, len);
	if(s > 0){
		ret = copy_to_user(data, buf, len);
	}
	if (ret != 0)  s = -1;
	vfree(buf);
	return s;

}

static struct nf_hook_ops  hooks[6] = {

	[0] =  {
		.hook = hookfn,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP_PRI_FIRST,
	},
	[1] =  {
		.hook = hookfn,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP_PRI_FIRST,
	},
	[2] =  {
		.hook = hookfn,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP_PRI_FIRST,
	},
	[3] =  {
		.hook = hookfn6,
		.owner = THIS_MODULE,
		.pf = PF_INET6,
		.hooknum = NF_INET_LOCAL_IN,
		.priority = NF_IP6_PRI_FIRST,
	},
	[4] =  {
		.hook = hookfn6,
		.owner = THIS_MODULE,
		.pf = PF_INET6,

		.hooknum = NF_INET_LOCAL_OUT,
		.priority = NF_IP6_PRI_FIRST,
	},
	[5] =  {
		.hook = hookfn6,
		.owner = THIS_MODULE,
		.pf = PF_INET6,
		.hooknum = NF_INET_FORWARD,
		.priority = NF_IP6_PRI_FIRST,
	},

};
static struct file_operations nfsops = {
	.owner = THIS_MODULE,
	.read = nfs_read,
	.unlocked_ioctl = nfs_ioctl,
};

static struct nfsdevice {
	struct cdev dev;
	dev_t devno;
} nfsdev;

static void nfs_hook_clean(void)
{
	pr_info("nfs_hook_exit\n");
	cdev_del(&nfsdev.dev);
	nf_unregister_hooks(hooks, sizeof(hooks)/sizeof(struct nf_hook_ops));
	unregister_chrdev_region(nfsdev.devno, 1);
	clear_iptree();
	clear_nfsrule();

	/* parent dir must be delted as the last step */
	remove_proc_subtree(BASEDIR, NULL);
}

static  void __exit  nfs_hook_exit(void)
{
	nfs_hook_clean();
}
static int __init nfs_hook_init(void)
{
	int err = 0;
	struct proc_dir_entry    *base_dir = NULL;


#ifdef DEBUG
	pr_info("nfs_hook_init, debug mode\n");
#else
	pr_info("nfs_hook_init\n");
#endif

	err = alloc_chrdev_region(&nfsdev.devno, 0, 1, NFS_DEV_NAME);
	if (err < 0) {
		pr_err("nf-stat failed to allocate device region\n");
		return 1;
	}
	cdev_init(&nfsdev.dev, &nfsops);
	nfsdev.dev.owner = THIS_MODULE;
	nfsdev.dev.ops = &nfsops;

	err = cdev_add(&nfsdev.dev, nfsdev.devno, 1);
	if (err) {
		pr_err("cdev_add failed for nsfdev\n");
		return 2;
	}
	nf_register_hooks(hooks, sizeof(hooks)/sizeof(struct nf_hook_ops));

	base_dir = proc_mkdir(BASEDIR, NULL);
	if (!base_dir) {
		pr_err("Could NOT create base dir /proc/%s\n", BASEDIR);
		goto fail;
	}
	ipcounter_dir = proc_mkdir(COUNTERDIR, base_dir);
	if (!ipcounter_dir) {
		pr_err("Could NOT create base dir /proc/%s/%s\n",
				BASEDIR, COUNTERDIR);
		goto fail;
	}
	rule_dir = proc_mkdir(RULEDIR, base_dir);
	if (!rule_dir) {
		pr_err("Could NOT create base dir /proc/%s/%s\n",
				BASEDIR,RULEDIR);
		goto fail;
	}	
	return 0;
  fail:
	nfs_hook_clean();
	return err;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhi Fu Zhang <zzfooo@hotmail.com>");
MODULE_DESCRIPTION("nfstat");
MODULE_VERSION("0.1");


module_init(nfs_hook_init);
module_exit(nfs_hook_exit);

