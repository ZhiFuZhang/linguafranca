#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <linux/igmp.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/sctp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include "internal.h"

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
	};
	s16 idx = 0;
	struct iphdr *hdr = ip_hdr(skb);
	if (hdr == NULL) return NF_ACCEPT;
	if (in != NULL)
		rule.dir += 1;
	if (out != NULL)
		rule.dir += 2;
	rule.protocol = hdr->protocol;
	rule.lip.len = 4;
	rule.rip.len = 4;
	if (in != NULL) {
		memcpy(&rule.lip.addr, &hdr->daddr, sizeof(hdr->daddr));
		memcpy(&rule.rip.addr, &hdr->saddr, sizeof(hdr->saddr));
	} else {
		memcpy(&rule.lip.addr, &hdr->saddr, sizeof(hdr->saddr));
		memcpy(&rule.rip.addr, &hdr->daddr, sizeof(hdr->daddr));
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
	idx =  get_typeidx(&rule);
	if (idx < 0 || idx > 255) return NF_ACCEPT;

	inccounter(&rule.lip, idx,  hdr->tot_len);
	return NF_ACCEPT;

}
unsigned int hookfn6(const struct nf_hook_ops *ops,
                               struct sk_buff *skb,
                               const struct net_device *in,
                               const struct net_device *out,
                               int (*okfn)(struct sk_buff *))
{
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
	if (cmd != NFS_CMD_INIT) {
		if (nfstypesize() == 0) {
			return -1;
		}
	}
	switch (cmd) {
	case NFS_CMD_INIT:
		if (copy_from_user(&data.maxtype, (u8 *)arg, sizeof(u8)))
			return -EFAULT;
		nfsinit(data.maxtype);
		break;
	case NFS_CMD_ADDIP:
		if (copy_from_user(&data.ip, (u8 *)arg,
					sizeof(struct nfs_ipaddr))) 
			return -EFAULT;
		addipentry(&data.ip);

		break;
	case NFS_CMD_RMVIP: 
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

#if 0
static struct nfs_hook_ops  hooks[6] = {
	[0] =  {
		.hook = hookfn,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_IP_LOCAL_IN,
		.priority = NF_IP_PRI_FIRST,
	},
	[1] =  {
		.hook = hookfn,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_IP_LOCAL_OUT,
		.priority = NF_IP_PRI_FIRST,
	},
	[2] =  {
		.hook = hookfn,
		.owner = THIS_MODULE,
		.pf = PF_INET,
		.hooknum = NF_IP_FORWARD,
		.priority = NF_IP_PRI_FIRST,
	},
	[3] =  {
		.hook = hookfn6,
		.owner = THIS_MODULE,
		.pf = PF_INET6,
		.hooknum = NF_IP6_LOCAL_IN,
		.priority = NF_IP6_PRI_FIRST,
	},
	[4] =  {
		.hook = hookfn6,
		.owner = THIS_MODULE,
		.pf = PF_INET6,
		.hooknum = NF_IP6_LOCAL_OUT,
		.priority = NF_IP6_PRI_FIRST,
	},
	[5] =  {
		.hook = hookfn6,
		.owner = THIS_MODULE,
		.pf = PF_INET6,
		.hooknum = NF_IP6_FORWARD,
		.priority = NF_IP6_PRI_FIRST,
	},

};
#endif

static struct file_operations nfsops = {
	.owner = THIS_MODULE,
	.read = nfs_read,
	.unlocked_ioctl = nfs_ioctl,
};

static struct nfsdevice {
	struct cdev dev;
	dev_t devno;
} nfsdev;

static int __init nfs_hook_init(void)
{
	int err = 0;
	pr_info("nfs_hook_init");
	err = alloc_chrdev_region(&nfsdev.devno, 0, 1, "nf-stat");
	if (err < 0) {
		pr_err("nf-stat failed to allocate device region");
		return 1;
	}
	cdev_init(&nfsdev.dev, &nfsops);
	nfsdev.dev.owner = THIS_MODULE;
	nfsdev.dev.ops = &nfsops;

	err = cdev_add(&nfsdev.dev, nfsdev.devno, 1);
	if (err) {
		pr_err("cdev_add failed for nsfdev");
		return 2;
	}
	//nf_register_hook(hooks);
	return 0;
}

static  void __exit  nfs_hook_exit(void)
{
	pr_info("nfs_hook_exit");
	//nf_unregister_hooks(hooks);
	cdev_del(&nfsdev.dev);
	unregister_chrdev_region(nfsdev.devno, 1);

}
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zhi Fu Zhang <zzfooo@hotmail.com>");
MODULE_DESCRIPTION("nfstat");
MODULE_VERSION("0.1");


module_init(nfs_hook_init);
module_exit(nfs_hook_exit);

