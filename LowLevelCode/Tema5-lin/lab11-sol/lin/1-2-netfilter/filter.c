/*
 * SO2 - Networking Lab (#11)
 *
 * Exercise #1, #2: simple netfilter module
 *
 * Solution for lab exercises.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/atomic.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include "filter.h"

MODULE_DESCRIPTION("Simple netfilter module");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL		KERN_ALERT
#define MY_DEVICE		"filter"

static struct cdev my_cdev;
static atomic_t ioctl_set;
static unsigned int ioctl_set_addr;


/*
 * test ioctl_set_addr if it has been set
 */
static int test_daddr(unsigned int dst_addr)
{
	if (atomic_read(&ioctl_set) == 1)
		return ioctl_set_addr == dst_addr;
	return 1;
}

static unsigned int my_nf_hookfn(unsigned int hooknum,
					struct sk_buff *skb,
					const struct net_device *in,
					const struct net_device *out,
					int (*okfn)(struct sk_buff *))
{
	/* get IP header */
	struct iphdr *iph = ip_hdr(skb);

	if (iph->protocol == IPPROTO_TCP && test_daddr(iph->daddr)) {
		/* get TCP header */
		struct tcphdr *tcph = tcp_hdr(skb);
		/* test for connection initiating packet */
		if (tcph->syn && !tcph->ack)
			printk(LOG_LEVEL "TCP connection initiated from "
				NIPQUAD_FMT ":%u\n",
				NIPQUAD(iph->saddr), ntohs(tcph->source));
	}

	/* let the package pass */
	return NF_ACCEPT;
}

static int my_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_close(struct inode *inode, struct file *file)
{
	return 0;
}

static int my_ioctl(struct inode *inode, struct file *file,
				unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case MY_IOCTL_FILTER_ADDRESS:
		/* set filter address from arg */
		if(copy_from_user(&ioctl_set_addr, (void *) arg, sizeof(ioctl_set_addr)))
			return -EFAULT;
		atomic_set(&ioctl_set, 1);
		break;

	default:
		return -ENOTTY;
	}

	return 0;
}

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.ioctl = my_ioctl
};

static struct nf_hook_ops my_nfho = {
	.owner       = THIS_MODULE,
	.hook        = my_nf_hookfn,
	.hooknum     = NF_INET_LOCAL_OUT,
	.pf          = PF_INET,
	.priority    = NF_IP_PRI_FIRST
};

int __init my_hook_init(void)
{
	int err;

	/* register filter device */
	err = register_chrdev_region(MKDEV(MY_MAJOR, 0), 1, MY_DEVICE);
	if (err != 0)
		return err;

	atomic_set(&ioctl_set,0);
	ioctl_set_addr = 0;

	/* init & add device */
	cdev_init(&my_cdev, &my_fops);
	cdev_add(&my_cdev, MKDEV(MY_MAJOR, 0), 1);

	/* register netfilter hook */
	err = nf_register_hook(&my_nfho);
	if (err)
		goto out;

	return 0;

out:
	/* cleanup */
	cdev_del(&my_cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1);

	return err;
}

void __exit my_hook_exit(void)
{
	/* unregister hook */
	nf_unregister_hook(&my_nfho);

	/* cleanup device */
	cdev_del(&my_cdev);
	unregister_chrdev_region(MKDEV(MY_MAJOR, 0), 1);
}

module_init(my_hook_init);
module_exit(my_hook_exit);
