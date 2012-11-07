/*
 * SO2 - Networking Lab (#9)
 *
 * Exercise #3, #4: simple kernel TCP socket
 *
 * Solution for lab exercises.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/fs.h>

MODULE_DESCRIPTION("Simple kernel TCP socket");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL		KERN_ALERT
#define MY_TCP_PORT		60000
#define LISTEN_BACKLOG		5

#define ON			1
#define OFF			0
#define DEBUG			ON

#if DEBUG == ON
#define LOG(s)					\
	do {					\
		printk(KERN_DEBUG s "\n");	\
	} while (0)
#else
#define LOG(s)					\
	do {					\
	} while (0)
#endif

#define print_sock_address(addr)		\
	do {					\
		printk(LOG_LEVEL "connection established to "	\
				NIPQUAD_FMT ":%d\n", 		\
				NIPQUAD(addr.sin_addr.s_addr),	\
				ntohs(addr.sin_port));		\
	} while (0)

static struct socket *sock;	/* listening (server) socket */
static struct socket *new_sock;	/* communication socket */

int __init my_tcp_sock_init(void)
{
	int err;
	/* address to bind on */
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= htons(MY_TCP_PORT),
		.sin_addr	= { htonl(INADDR_LOOPBACK) }
	};
	int addrlen = sizeof(addr);
	/* address of peer */
	struct sockaddr_in raddr;
	int raddrlen = sizeof(addr);

	/* create listening socket */
	err = sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
	if(err < 0) {
		printk(LOG_LEVEL "can't create socket\n");
		goto out;
	}

	/* bind socket to loopback on port MY_TCP_PORT */
	err = sock->ops->bind(sock, (struct sockaddr *) &addr, addrlen);
	if (err < 0) {
		printk(LOG_LEVEL "can't bind socket\n");
		goto out_release;
	}

	/* start listening */
	err = sock->ops->listen(sock, LISTEN_BACKLOG);
	if (err < 0) {
		printk(LOG_LEVEL "can't listen on socket\n");
		goto out_release;
	}

	/* create new socket for the accepted connection */
	err = sock_create_kern(PF_INET, SOCK_STREAM, IPPROTO_TCP, &new_sock);
	if(err < 0) {
		printk(LOG_LEVEL "can't create new socket\n");
		goto out;
	}

	/* accept a connection */
	err = sock->ops->accept(sock, new_sock, 0);
	if (err < 0) {
		printk(LOG_LEVEL "can't accept new connection\n");
		goto out_release_new_sock;
	}

	/* get the address of the peer */
	err = sock->ops->getname(new_sock, (struct sockaddr *) &raddr, &raddrlen, 1);
	if (err < 0) {
		printk(LOG_LEVEL "can't find peer name\n");
		goto out_release_new_sock;
	}

	/* and print it */
	print_sock_address(raddr);

	return 0;

out_release_new_sock:
	sock_release(new_sock);
out_release:
	sock_release(sock);
out:
	return err;
}

void __exit my_tcp_sock_exit(void)
{
	sock_release(new_sock);
	sock_release(sock);
}

module_init(my_tcp_sock_init);
module_exit(my_tcp_sock_exit);
