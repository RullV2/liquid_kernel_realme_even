/*
 * Rusty Russell (C)2000 -- This code is GPL.
 * Patrick McHardy (c) 2006-2012
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/skbuff.h>
#include <linux/netfilter.h>
#include <linux/netfilter_bridge.h>
#include <linux/seq_file.h>
#include <linux/rcupdate.h>
#include <net/protocol.h>
#include <net/netfilter/nf_queue.h>
#include <net/dst.h>

#include "nf_internals.h"

/*
 * Hook for nfnetlink_queue to register its queue handler.
 * We do this so that most of the NFQUEUE code can be modular.
 *
 * Once the queue is registered it must reinject all packets it
 * receives, no matter what.
 */

#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
static const struct nf_queue_handler __rcu *queue_imq_handler __read_mostly;

void nf_register_queue_imq_handler(const struct nf_queue_handler *qh)
{
	rcu_assign_pointer(queue_imq_handler, qh);
}
EXPORT_SYMBOL_GPL(nf_register_queue_imq_handler);

void nf_unregister_queue_imq_handler(void)
{
	RCU_INIT_POINTER(queue_imq_handler, NULL);
	synchronize_rcu();
}
EXPORT_SYMBOL_GPL(nf_unregister_queue_imq_handler);
#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */

/* return EBUSY when somebody else is registered, return EEXIST if the
 * same handler is registered, return 0 in case of success. */
void nf_register_queue_handler(struct net *net, const struct nf_queue_handler *qh)
{
	/* should never happen, we only have one queueing backend in kernel */
	WARN_ON(rcu_access_pointer(net->nf.queue_handler));
	rcu_assign_pointer(net->nf.queue_handler, qh);
}
EXPORT_SYMBOL(nf_register_queue_handler);

/* The caller must flush their queue before this */
void nf_unregister_queue_handler(struct net *net)
{
	RCU_INIT_POINTER(net->nf.queue_handler, NULL);
}
EXPORT_SYMBOL(nf_unregister_queue_handler);

static void nf_queue_sock_put(struct sock *sk)
{
#ifdef CONFIG_INET
	sock_gen_put(sk);
#else
	sock_put(sk);
#endif
}

void nf_queue_entry_release_refs(struct nf_queue_entry *entry)
{
	struct nf_hook_state *state = &entry->state;

	/* Release those devices we held, or Alexey will kill me. */
	if (state->in)
		dev_put(state->in);
	if (state->out)
		dev_put(state->out);
	if (state->sk)
		nf_queue_sock_put(state->sk);
#if IS_ENABLED(CONFIG_BRIDGE_NETFILTER)
	if (entry->skb->nf_bridge) {
		struct net_device *physdev;

		physdev = nf_bridge_get_physindev(entry->skb);
		if (physdev)
			dev_put(physdev);
		physdev = nf_bridge_get_physoutdev(entry->skb);
		if (physdev)
			dev_put(physdev);
	}
#endif
}
EXPORT_SYMBOL_GPL(nf_queue_entry_release_refs);

/* Bump dev refs so they don't vanish while packet is out */
void nf_queue_entry_get_refs(struct nf_queue_entry *entry)
{
	struct nf_hook_state *state = &entry->state;

	if (state->in)
		dev_hold(state->in);
	if (state->out)
		dev_hold(state->out);
#if IS_ENABLED(CONFIG_BRIDGE_NETFILTER)
	if (entry->skb->nf_bridge) {
		struct net_device *physdev;

		physdev = nf_bridge_get_physindev(entry->skb);
		if (physdev)
			dev_hold(physdev);
		physdev = nf_bridge_get_physoutdev(entry->skb);
		if (physdev)
			dev_hold(physdev);
	}
#endif
}
EXPORT_SYMBOL_GPL(nf_queue_entry_get_refs);

unsigned int nf_queue_nf_hook_drop(struct net *net)
{
	const struct nf_queue_handler *qh;
	unsigned int count = 0;

	rcu_read_lock();
	qh = rcu_dereference(net->nf.queue_handler);
	if (qh)
		count = qh->nf_hook_drop(net);
	rcu_read_unlock();

	return count;
}
EXPORT_SYMBOL_GPL(nf_queue_nf_hook_drop);

#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
static int __nf_queue(struct sk_buff *skb, const struct nf_hook_state *state,
				  const struct nf_hook_entries *entries,
				  unsigned int index, unsigned int verdict)
#else /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
static int __nf_queue(struct sk_buff *skb, const struct nf_hook_state *state,
		      const struct nf_hook_entries *entries,
		      unsigned int index, unsigned int queuenum)
#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
{
	int status = -ENOENT;
	struct nf_queue_entry *entry = NULL;
	const struct nf_afinfo *afinfo;
	const struct nf_queue_handler *qh;
	struct net *net = state->net;

	#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
	//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
	unsigned int queuetype = verdict & NF_VERDICT_MASK;
	unsigned int queuenum  = verdict >> NF_VERDICT_QBITS;
	#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */

	/* QUEUE == DROP if no one is waiting, to be safe. */
	#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
	//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
	if (queuetype == NF_IMQ_QUEUE) {
		qh = rcu_dereference(queue_imq_handler);
	} else {
		qh = rcu_dereference(net->nf.queue_handler);
	}
	#else /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
	qh = rcu_dereference(net->nf.queue_handler);
	#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
	if (!qh) {
		status = -ESRCH;
		goto err;
	}

	afinfo = nf_get_afinfo(state->pf);
	if (!afinfo)
		goto err;

	entry = kmalloc(sizeof(*entry) + afinfo->route_key_size, GFP_ATOMIC);
	if (!entry) {
		status = -ENOMEM;
		goto err;
	}

	if (skb_dst(skb) && !skb_dst_force(skb)) {
		status = -ENETDOWN;
		goto err;
	}

	*entry = (struct nf_queue_entry) {
		.skb	= skb,
		.state	= *state,
		.hook_index = index,
		.size	= sizeof(*entry) + afinfo->route_key_size,
	};

	nf_queue_entry_get_refs(entry);
	afinfo->saveroute(skb, entry);
	status = qh->outfn(entry, queuenum);

	if (status < 0) {
		nf_queue_entry_release_refs(entry);
		goto err;
	}

	return 0;

err:
	kfree(entry);
	return status;
}

/* Packets leaving via this function must come back through nf_reinject(). */
int nf_queue(struct sk_buff *skb, struct nf_hook_state *state,
	     const struct nf_hook_entries *entries, unsigned int index,
	     unsigned int verdict)
{
	int ret;

	#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
	//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
	ret = __nf_queue(skb, state, entries, index, verdict);
	#else /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
	ret = __nf_queue(skb, state, entries, index, verdict >> NF_VERDICT_QBITS);
	#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */

	if (ret < 0) {
		#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
		//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
		/* IMQ Bypass */
		if (ret == -ECANCELED && skb->imq_flags == 0) {
			return 1;
		}
		#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
		if (ret == -ESRCH &&
		    (verdict & NF_VERDICT_FLAG_QUEUE_BYPASS))
			return 1;
		kfree_skb(skb);
	}

	return 0;
}

static unsigned int nf_iterate(struct sk_buff *skb,
			       struct nf_hook_state *state,
			       const struct nf_hook_entries *hooks,
			       unsigned int *index)
{
	const struct nf_hook_entry *hook;
	unsigned int verdict, i = *index;

	while (i < hooks->num_hook_entries) {
		hook = &hooks->hooks[i];
repeat:
		verdict = nf_hook_entry_hookfn(hook, skb, state);
		if (verdict != NF_ACCEPT) {
			*index = i;
			if (verdict != NF_REPEAT)
				return verdict;
			goto repeat;
		}
		i++;
	}

	*index = i;
	return NF_ACCEPT;
}

/* Caller must hold rcu read-side lock */
void nf_reinject(struct nf_queue_entry *entry, unsigned int verdict)
{
	const struct nf_hook_entry *hook_entry;
	const struct nf_hook_entries *hooks;
	struct sk_buff *skb = entry->skb;
	const struct nf_afinfo *afinfo;
	const struct net *net;
	unsigned int i;
	int err;
	u8 pf;

	rcu_read_lock();

	net = entry->state.net;
	pf = entry->state.pf;

	hooks = rcu_dereference(net->nf.hooks[pf][entry->state.hook]);

	nf_queue_entry_release_refs(entry);

	i = entry->hook_index;
	if (WARN_ON_ONCE(i >= hooks->num_hook_entries)) {
		rcu_read_unlock();
		kfree_skb(skb);
		kfree(entry);
		return;
	}

	hook_entry = &hooks->hooks[i];

	/* Continue traversal iff userspace said ok... */
	if (verdict == NF_REPEAT)
		verdict = nf_hook_entry_hookfn(hook_entry, skb, &entry->state);

	if (verdict == NF_ACCEPT) {
		afinfo = nf_get_afinfo(entry->state.pf);
		if (!afinfo || afinfo->reroute(entry->state.net, skb, entry) < 0)
			verdict = NF_DROP;
	}

	if (verdict == NF_ACCEPT) {
next_hook:
		++i;
		verdict = nf_iterate(skb, &entry->state, hooks, &i);
	}

	switch (verdict & NF_VERDICT_MASK) {
	case NF_ACCEPT:
	case NF_STOP:
		local_bh_disable();
		entry->state.okfn(entry->state.net, entry->state.sk, skb);
		local_bh_enable();
		break;
	#ifdef OPLUS_FEATURE_WIFI_LIMMITBGSPEED
	//HuangJunyuan@CONNECTIVITY.WIFI.INTERNET, 2018/06/26, Add for limit speed function
	case NF_IMQ_QUEUE:
	#endif /* OPLUS_FEATURE_WIFI_LIMMITBGSPEED */
	case NF_QUEUE:
		err = nf_queue(skb, &entry->state, hooks, i, verdict);
		if (err == 1)
			goto next_hook;
		break;
	case NF_STOLEN:
		break;
	default:
		kfree_skb(skb);
	}

	rcu_read_unlock();
	kfree(entry);
}
EXPORT_SYMBOL(nf_reinject);
