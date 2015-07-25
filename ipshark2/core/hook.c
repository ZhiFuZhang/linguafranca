#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/icmp.h>
#include <linux/igmp.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/sctp.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include "intern.h"
#include "ip_queue.h"
#include "devset.h"

static bool ignore(const struct net_device *in, const struct net_device *out)
{
	if (in != NULL){
		if (!devset_ignore(in->name)){
			return false;
		}
	}
	if (out != NULL){
		if (!devset_ignore(out->name)){
			return false;
		}
	}
	return true;
}
static unsigned int hookfn(const struct nf_hook_ops *ops,
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
	}data;
	struct ip_key_info info = { .totallen = 0};
	struct iphdr *hdr = ip_hdr(skb);
	if (hdr == NULL) return NF_ACCEPT;
	if (hdr->version != 4) return NF_ACCEPT;
	info.direct = 0;
	if (ignore(in, out)){
		return NF_ACCEPT;
	}
	if (in != NULL){
		info.direct += 1;
		memcpy(&info.in.name, in->name, sizeof(in->name));
	}
	if (out != NULL) {
		info.direct += 2;
		memcpy(&info.out.name, out->name, sizeof(out->name));
	}
	info.protocol = hdr->protocol;
	memcpy(&info.s, &hdr->saddr, sizeof(hdr->saddr));
	memcpy(&info.d, &hdr->daddr, sizeof(hdr->daddr));
	info.version = 4;	
	switch (hdr->protocol){
	case IPPROTO_TCP:
		data.tcp = tcp_hdr(skb);
		if (unlikely(data.tcp == NULL)) return NF_ACCEPT;
		info.sport = data.tcp->source;
		info.dport = data.tcp->dest;
		break;
	case IPPROTO_UDP:
		data.udp = udp_hdr(skb);
		if (unlikely(data.udp == NULL)) return NF_ACCEPT;
		info.sport = data.udp->source;
		info.dport = data.udp->dest;
		break;
	case IPPROTO_SCTP:
		data.sctp = sctp_hdr(skb);
		if (unlikely(data.sctp == NULL)) return NF_ACCEPT;
		info.sport = data.sctp->source;
		info.dport = data.sctp->dest;

		break;
	default:
		break;
	}
	info.totallen = skb->len;
	ip_queue_put(&info);
	ip_queue_wakeup();
	return NF_ACCEPT;

}
static unsigned int hookfn6(const struct nf_hook_ops *ops,
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
	}data;
	struct ip_key_info info = { .totallen = 0};
	struct ipv6hdr *hdr = ipv6_hdr(skb);
	if (hdr == NULL) return NF_ACCEPT;
	if (hdr->version != 6) return NF_ACCEPT;
	info.direct = 0;
	if (ignore(in, out)){
		return NF_ACCEPT;
	}
	if (in != NULL){
		info.direct += 1;
		memcpy(&info.in.name, in->name, sizeof(in->name));
	}
	if (out != NULL) {
		info.direct += 2;
		memcpy(&info.out.name, out->name, sizeof(out->name));
	}
	info.protocol = hdr->nexthdr;
	memcpy(&info.s, &hdr->saddr, sizeof(hdr->saddr));
	memcpy(&info.d, &hdr->daddr, sizeof(hdr->daddr));
	info.version = 6;	
	switch (info.protocol){
	case IPPROTO_TCP:
		data.tcp = tcp_hdr(skb);
		if (unlikely(data.tcp == NULL)) return NF_ACCEPT;
		info.sport = data.tcp->source;
		info.dport = data.tcp->dest;
		break;
	case IPPROTO_UDP:
		data.udp = udp_hdr(skb);
		if (unlikely(data.udp == NULL)) return NF_ACCEPT;
		info.sport = data.udp->source;
		info.dport = data.udp->dest;
		break;
	case IPPROTO_SCTP:
		data.sctp = sctp_hdr(skb);
		if (unlikely(data.sctp == NULL)) return NF_ACCEPT;
		info.sport = data.sctp->source;
		info.dport = data.sctp->dest;
		break;
	default:
		break;
	}
	info.totallen = skb->len;
	ip_queue_put(&info);
	ip_queue_wakeup();
	return NF_ACCEPT;

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

void hook_register(void)
{
	nf_register_hooks(hooks, sizeof(hooks)/sizeof(struct nf_hook_ops));
}

void hook_unregister(void)
{

	nf_unregister_hooks(hooks, sizeof(hooks)/sizeof(struct nf_hook_ops));
}

