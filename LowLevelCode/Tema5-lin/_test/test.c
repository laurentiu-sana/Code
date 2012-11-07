/*
 * SO2 - Homework #5 - Stateful firewall
 *
 * test.c: IP firewall driver tester
 */

#include <stdio.h> 
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>

#define IPFWCTL			"./ipfwctl"

#define SHOULD_FILTER		1
#define SHOULD_PASS		2

#define SHOULD_NOT_ADD_RULE	3
#define SHOULD_ADD_RULE		4

#define DIR_OUTSIDE		5
#define DIR_INSIDE		6

#define CONNECT_TIMEOUT		5
#define ACCEPT_TIMEOUT		5
#define RECV_TIMEOUT		2
#define RULE_EXPIRE_TIMEOUT	1

#define SSH_PORT		22
#define OUTPUT_PAD		65

static int control_socket = -1; /* socket for control connection */
static int seq = 1;		/* sequence number of ping-pongs */
static int dir;			/* where is the test being run */
static int rules;		/* number of filter rules in driver */
static int passes;		/* number of passed tests */
static int failures;		/* number of failed tests */

/*
 * display an IP address in readable format
 */
#define NIPQUAD(addr)			\
	((unsigned char *)&(addr))[0],	\
	((unsigned char *)&(addr))[1],	\
	((unsigned char *)&(addr))[2],	\
	((unsigned char *)&(addr))[3]
#define NIPQUAD_FMT		"%u.%u.%u.%u"

/*
 * test macros
 */
#define test_header(s)							\
do {									\
	int i;								\
									\
        printf("######## %s ", s); 					\
	for (i = OUTPUT_PAD - strlen(s); i > 0; i--)			\
		putchar('#');						\
	putchar('\n');							\
	fflush(stdout);							\
} while (0)

#define test_error(msg, t, fatal)					\
do {									\
	int i;								\
									\
        printf(" %s ", msg); 						\
	for (i = OUTPUT_PAD - strlen(msg); i > 0; i--)			\
		putchar('.');						\
	putchar(' ');							\
	fflush(stdout);							\
	errno = 0;							\
	if (!(t)) {							\
		failures++;						\
		if (errno)						\
			printf("FAILED! errno=%d [%s]\n",		\
				errno, strerror(errno));		\
		else							\
			printf("FAILED!\n");				\
		if (fatal) {						\
			cleanup();					\
			exit(EXIT_FAILURE);				\
		}							\
	} else {							\
		passes++;						\
		printf("PASSED\n");					\
	}								\
	fflush(stdout);							\
} while (0)

#define test_value(msg, value, expected)				\
do {									\
	int i;								\
									\
        printf(" %s ", msg);	 					\
	for (i = OUTPUT_PAD - strlen(msg); i > 0; i--)			\
		putchar('.');						\
	putchar(' ');							\
	fflush(stdout);							\
	if ((value) != (expected)) {					\
		failures++;						\
		printf("FAILED! [got %d, expected %d]\n",		\
			value, expected);				\
	} else {							\
		passes++;						\
		printf("PASSED [%d]\n", value);				\
	}								\
	fflush(stdout);							\
} while (0)

#define test_error_fmt(fmt, t, fatal, ...)				\
do {									\
	char buf[256];							\
	sprintf(buf, fmt, __VA_ARGS__);					\
	test_error(buf, t, fatal);					\
} while (0)

#define test(msg, t)			test_error(msg, t, 0)
#define fatal_test(msg, t)		test_error(msg, t, 1)
#define test_fmt(fmt, t, ...)		test_error_fmt(fmt, t, 0, __VA_ARGS__)
#define fatal_test_fmt(fmt, t, ...)	test_error_fmt(fmt, t, 1, __VA_ARGS__)

#define test_socket(sock, pass)						\
	((pass) == SHOULD_PASS ? (sock) : -(sock))

/*
 * unload module and delete device file
 */
static void cleanup()
{
	system(IPFWCTL " rmmod 2>/dev/null");
	system(IPFWCTL " unlink 2>/dev/null");

	if (control_socket != -1) {
		shutdown(control_socket, SHUT_RDWR);
		close(control_socket);
		control_socket = -1;
	}
}

/*
 * SIGINT/SIGTERM handler - cleanup and exit
 */
static void int_handler(int sig)
{
	cleanup();
	exit(EXIT_FAILURE);
}

/*
 * SIGALRM handler - do nothing
 */
static void alarm_handler(int sig)
{
}

static void cancel_alarm(void)
{
	alarm(0);
}

static void set_alarm(unsigned int timeout)
{
	alarm(timeout); 
}

static int system_check(char *cmd)
{
	int ret;

	ret = system(cmd);
	if (WIFEXITED(ret))
		return WEXITSTATUS(ret);

	if (WIFSIGNALED(ret))
		fprintf(stderr, "%s: terminated by signal %d\n", cmd, WTERMSIG(ret));
	else
		perror(cmd);

	cleanup();
	exit(EXIT_FAILURE);
}

static int firewall_mknod(void)
{
	return system_check(IPFWCTL " mknod");
}

static int firewall_unlink(void)
{
	return system_check(IPFWCTL " unlink");
}

static int firewall_insmod(void)
{
	return system_check(IPFWCTL " insmod");
}

static int firewall_rmmod(void)
{
	return system_check(IPFWCTL " rmmod");
}

static int firewall_test(void)
{
	return system_check(IPFWCTL " test");
}

static int firewall_invalid(void)
{
	return system_check(IPFWCTL " invalid");
}

static int firewall_add_find(int add, const char *srcip, int srcmask, const char *dstip, int dstmask,
		unsigned short srcport1, unsigned short srcport2,
		unsigned short dstport1, unsigned short dstport2)
{
	char buf[256];
	sprintf(buf, IPFWCTL " %s %s/%d %s/%d %hu:%hu %hu:%hu", add ? "add" : "find",
						srcip, srcmask, dstip, dstmask,
						srcport1, srcport2, dstport1, dstport2);
	return system_check(buf);
}

static int firewall_add(const char *srcip, int srcmask, const char *dstip, int dstmask,
		unsigned short srcport1, unsigned short srcport2,
		unsigned short dstport1, unsigned short dstport2)
{
	if (firewall_add_find(1, srcip, srcmask, dstip, dstmask, srcport1, srcport2, dstport1, dstport2))
		return -1;
	if (firewall_add_find(0, srcip, srcmask, dstip, dstmask, srcport1, srcport2, dstport1, dstport2))
		return -1;
	return 0;
}

static int firewall_enable(void)
{
	return system_check(IPFWCTL " enable");
}

static int firewall_disable(void)
{
	return system_check(IPFWCTL " disable");
}

static int firewall_list(void)
{
	return system_check(IPFWCTL " list >/dev/null");
}

static int firewall_list_verbose(void)
{
	return system_check(IPFWCTL " list");
}

/*
 * test number of rules in driver
 */
static void test_rules(int new_rules, char *fmt, ...)
{
	int n;
	char buf[256], buf2[256];
	va_list ap;

	rules += new_rules;
	n = firewall_list();
	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	sprintf(buf2, "number of rules [%s]", buf);
	test_value(buf2, n, rules);
	if (n != rules)
		/* show all rules if there is a mismatch */
		firewall_list_verbose();
}

static unsigned int resolve_hostname(const char *hostname)
{
	struct hostent *hostinfo;
	struct in_addr *in_addr;

	hostinfo = gethostbyname(hostname);
	if (hostinfo == NULL) {
		fprintf(stderr, "%s: can't resolve hostname [%d]\n", hostname, h_errno);
		exit(EXIT_FAILURE);
	}

	in_addr = (struct in_addr *) hostinfo->h_addr;
	return in_addr->s_addr;
}

static unsigned int get_peer(int sock)
{
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);

	if (getpeername(sock, (struct sockaddr *) &addr, &addr_len) < 0) {
		perror("getpeername");
		exit(EXIT_FAILURE);
	}

	return addr.sin_addr.s_addr;
}

static int do_listen(int type, unsigned short port, int reuse)
{
	int sock, on = 1;
	struct sockaddr_in name;

	/* create the socket */
	sock = socket(PF_INET, type, 0);
	if (sock < 0) 
		return -1;

	if (reuse) {
		/* this is to allow listening on ports that are in TIME_WAIT state */
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
			perror("setsockopt(SO_REUSEADDR)");
		}
	}

	/* bind to local port */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	name.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		close(sock);
		return -1;
	}

	if (type == SOCK_STREAM && (listen(sock, 1) < 0)) {
		close(sock);
		return -1;
	}

	return sock;
}

/*
 * ip is in network-byte order, port is in host-byte order
 */
static int do_connect(int type, unsigned int ip, int port)
{
	struct sockaddr_in name;
	int sock;

	/* create the socket */
	sock = socket(PF_INET, type, 0);
	if (sock < 0) 
		return -1;

	/* fill the address */
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = ip;
	name.sin_port = htons(port);

	set_alarm(CONNECT_TIMEOUT);
	if (connect(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
		cancel_alarm();
		close(sock);
		return -1;
	}
	cancel_alarm();

	return sock;
}

static int do_accept(int sock, int timeout)
{
	int csock;
	struct sockaddr_in caddr;
	socklen_t csize = sizeof(caddr);
    
	set_alarm(timeout);
	csock = accept(sock, (struct sockaddr *) &caddr, &csize);
	cancel_alarm();

	return csock;
}

static int do_read(int fd, char *buffer, int size)
{
	int n, partial = 0;
	
	while (1) {
		n = read(fd, buffer + partial, size - partial);
		if (n <= 0)
		        return -1;
		partial += n;
	        if (partial == size)
	    	        return 0;
	}
}

static int do_write(int fd, char *buffer, int size)
{
	int n, partial = 0;

	while (1) {
		n = write(fd, buffer + partial, size - partial);
		if (n <= 0)
		        return -1;
		partial += n;
	        if (partial == size)
	    	        return 0;
	}
}

static int do_send(int sock, int seq) 
{
	errno = 0;
	return do_write(sock, (char*) &seq, sizeof(seq));
}

static int do_recv(int sock)
{
	int seq, ret;

	set_alarm(RECV_TIMEOUT);
	errno = 0;
	ret = do_read(sock, (char*) &seq, sizeof(seq));
	cancel_alarm();

	return ret ? -1 : seq;
}

static int do_sendto(int sock, int seq, struct sockaddr_in *addr, socklen_t addr_len)
{
	errno = 0;
	return sendto(sock, &seq, 4, 0, (struct sockaddr*) addr, addr_len) == 4 ? 0 : -1;
}

static int do_recvfrom(int sock, struct sockaddr_in *addr, socklen_t *addr_len)
{
	int seq, ret;

	set_alarm(RECV_TIMEOUT);
	errno = 0;
	ret = recvfrom(sock, &seq, 4, 0, (struct sockaddr*) addr, addr_len);
	cancel_alarm();

	return ret != 4 ? -1 : seq;
}

#define ping_pong(msg, sock, pass)						\
	do {									\
		int expected;							\
		test_fmt("%s ping #%d", do_send(sock, seq) == 0, msg, seq);	\
		seq++;								\
		expected = (pass) == SHOULD_PASS ? seq : -1;			\
		test_fmt("%s pong #%d%s", do_recv(sock) == expected, msg, seq,	\
			(pass) == SHOULD_PASS ? "" : " [should timeout]");	\
		seq++;								\
	} while (0)

#define pong_ping(msg, sock, pass)						\
	do {									\
		int expected;							\
		expected = (pass) == SHOULD_PASS ? seq : -1;			\
		test_fmt("%s pong #%d%s", do_recv(sock) == expected, msg, seq,	\
			(pass) == SHOULD_PASS ? "" : " [should timeout]");	\
		seq++;								\
		test_fmt("%s ping #%d", do_send(sock, seq) == 0, msg, seq);	\
		seq++;								\
	} while (0)

#define udp_pong_ping(msg, sock, pass)						\
	do {									\
		int expected;							\
		struct sockaddr_in addr;					\
		socklen_t len = sizeof(struct sockaddr_in);			\
		expected = (pass) == SHOULD_PASS ? seq : -1;			\
		test_fmt("%s pong #%d%s",						\
			do_recvfrom(sock, &addr, &len) == expected, msg, seq,	\
			(pass) == SHOULD_PASS ? "" : " [should filter]");	\
		seq++;								\
		/* only send packet if we should pass (we need an addr) */	\
		if ((pass) == SHOULD_PASS)					\
			do_sendto(sock, seq, &addr, len);			\
		test_fmt("%s ping #%d", 1, msg, seq);				\
		seq++;								\
	} while (0)

#define control_ping_pong(sock)		ping_pong("control", sock, SHOULD_PASS);
#define control_pong_ping(sock)		pong_ping("control", sock, SHOULD_PASS);

/*
 * wait for connection
 *
 * picks a random port for binding on, then sends that port number
 * through the control socket
 *
 * the connection should be successful if pass is SHOULD_PASS,
 * and it should fail if pass is SHOULD_FILTER
 *
 * if add_rule is SHOULD_ADD_RULE, it adds a rule to the firewall before the test
 */
static void wait_conn(int type, int pass, int add_rule)
{
	int sock, sock2, port;

 	do {
		port = 1024 + (rand() % 64000);
	} while ((sock = do_listen(type, port, 0)) < 0);

	/*
	 * we are waiting for connection on the inside (the local system
	 * is running the IP firewall driver)
	 */
	if (add_rule == SHOULD_ADD_RULE) {
		test("add firewall rule", firewall_add("0.0.0.0", 0, "0.0.0.0", 0, 0, 65535, port, port) == 0);
		test_rules(1, "static rule should have been added");
	}

	/* send port to control socket */
	test_fmt("send port %d", do_send(control_socket, port) == 0, port);

	if (type == SOCK_STREAM) {
		/* wait for TCP connection */
		test_fmt("accept TCP connection [should %s]",
				test_socket(sock2 = do_accept(sock, ACCEPT_TIMEOUT), pass),
				pass == SHOULD_PASS ? "accept" : "timeout");
		if (pass == SHOULD_PASS)
			pong_ping("TCP", sock2, pass);
		if (sock2 >= 0)
			close(sock2);
	} else {
		udp_pong_ping("UDP", sock, pass);
	}

	close(sock);
}

/*
 * initiate a connection
 *
 * the control socket is used to receive a port number; that port
 * number is a server socket listening for a connection on host
 * hostname
 *
 * the connection should be successful if pass is SHOULD_PASS,
 * and it should fail if pass is SHOULD_FILTER
 */
static void make_conn(int type, unsigned int ip, int pass)
{
	int port, sock;

	/* receive port number to connect to */
	test("recv port", (port = do_recv(control_socket)) > 0);

	/* connect to peer; for UDP no packets are actually sent, so no test */
	if (type == SOCK_STREAM)
		test_fmt("initiate TCP connection to " NIPQUAD_FMT ":%d [should %s]",
				test_socket(sock = do_connect(type, ip, port), pass),
				NIPQUAD(ip), port, pass == SHOULD_PASS ? "connect" : "timeout");
	else
		sock = do_connect(type, ip, port);

	/* always do a ping-pong test for UDP */
	if (type == SOCK_DGRAM || pass == SHOULD_PASS)
		ping_pong(type == SOCK_STREAM ? "TCP" : "UDP", sock, pass);

	if (sock >= 0)
		close(sock);

	/* check rules */
	if (pass == SHOULD_PASS) {
		if (dir == DIR_INSIDE)
			test_rules(1, "dynamic rule should have been added");
		sleep(RULE_EXPIRE_TIMEOUT);
		if (dir == DIR_INSIDE)
			test_rules(-1, "dynamic rule should have expired");
	} else {
		if (dir == DIR_INSIDE)
			test_rules(0, "should not change");
	}
}

/*
 * inside test: test from the system using the driver
 */
static int inside(int cport, unsigned int ip)
{
	dir = DIR_INSIDE;

	// cleanup();

	test_header("IP firewall driver tester - INSIDE");

	test_header("Phase 0: Module/device presence");

	fatal_test("ipfwctl mknod", firewall_mknod() == 0);
	fatal_test("ipfwctl insmod", firewall_insmod() == 0);
	fatal_test("ipfwctl test", firewall_test() == 0);
	fatal_test("ipfwctl rmmod", firewall_rmmod() == 0);
	fatal_test("ipfwctl insmod", firewall_insmod() == 0);

	test_header("Phase 1: IOCTL interface");

	test("ipfwctl disable", firewall_disable() == 0);
	test_rules(0, "rule list should be empty");

	/* add rule to allow ssh connections to inside host */
	test("ipfwctl add", firewall_add("0.0.0.0", 0, "0.0.0.0", 0, 0, 65535, SSH_PORT, SSH_PORT) == 0);
	test_rules(1, "static rule should have been added");

	test("ipfwctl enable", firewall_enable() == 0);
	test_rules(0, "should not change");
	test("ipfwctl disable", firewall_disable() == 0);
	test_rules(0, "should not change");
	test("ipfwctl enable", firewall_enable() == 0);
	test_rules(0, "should not change");
	test("ipfwctl invalid", firewall_invalid() == 0);

	test_header("Phase 3: Control connection (static rule)");

	fatal_test_fmt("initating control connection to " NIPQUAD_FMT ":%d",
		(control_socket = do_connect(SOCK_STREAM, ip, cport)) > 0, NIPQUAD(ip), cport);
	test_rules(1, "dynamic rule should have been added");

	control_pong_ping(control_socket);

	test_header("Phase 4: Inbound filter (no matching rule)");

	wait_conn(SOCK_STREAM, SHOULD_FILTER, SHOULD_NOT_ADD_RULE);

    	control_ping_pong(control_socket);

	wait_conn(SOCK_DGRAM, SHOULD_FILTER, SHOULD_NOT_ADD_RULE);

	control_pong_ping(control_socket);

	test_header("Phase 5: Inbound pass (static rule)");

	wait_conn(SOCK_STREAM, SHOULD_PASS, SHOULD_ADD_RULE);

	control_ping_pong(control_socket);

   	wait_conn(SOCK_DGRAM, SHOULD_PASS, SHOULD_ADD_RULE);

	control_pong_ping(control_socket);

	test_header("Phase 6: Outbound pass (dynamic rule)");

	make_conn(SOCK_STREAM, ip, SHOULD_PASS);

	control_ping_pong(control_socket);

	make_conn(SOCK_DGRAM, ip, SHOULD_PASS);

	control_pong_ping(control_socket);

	test_header("Phase 7: Inbound pass (firewall disabled)");

	test("ipfwctl disable", firewall_disable() == 0);
	test_rules(0, "should not change");

	wait_conn(SOCK_STREAM, SHOULD_PASS, SHOULD_NOT_ADD_RULE);

    	control_ping_pong(control_socket);

	wait_conn(SOCK_DGRAM, SHOULD_PASS, SHOULD_NOT_ADD_RULE);

	control_pong_ping(control_socket);

	test_header("Phase 8: Cleanup");

	shutdown(control_socket, SHUT_RDWR);
	close(control_socket);
	control_socket = -1;

	test("ipfwctl rmmod", firewall_rmmod() == 0);
	test("ipfwctl unlink", firewall_unlink() == 0);

	test_header("Results - Inside");

	printf(" PASSED TESTS: %d\n", passes);
	printf(" FAILED TESTS: %d\n", failures);
	printf("\n");

	return 0;
}

/*
 * outside test: test from the external system - not using the driver
 */
static int outside(int cport)
{
	int sock;
	unsigned int ip;

	dir = DIR_OUTSIDE;

	test_header("IP firewall driver tester - OUTSIDE");

	test_header("Phase 3: Control connection (static rule)");

	/* get listening socket, accept connection and leave it open */
	fatal_test_fmt("waiting for control connection on port %d",
		(sock = do_listen(SOCK_STREAM, cport, 1)) > 0 && (control_socket = do_accept(sock, 0)) > 0, cport);

	/* get peer ip */
	ip = get_peer(control_socket);

	control_ping_pong(control_socket);

	test_header("Phase 4: Inbound filter (no matching rule)");

	make_conn(SOCK_STREAM, ip, SHOULD_FILTER);

	control_pong_ping(control_socket);

	make_conn(SOCK_DGRAM, ip, SHOULD_FILTER);

	control_ping_pong(control_socket);

	test_header("Phase 5: Inbound pass (static rule)");

	make_conn(SOCK_STREAM, ip, SHOULD_PASS);

	control_pong_ping(control_socket);

	make_conn(SOCK_DGRAM, ip, SHOULD_PASS);

	control_ping_pong(control_socket);

	test_header("Phase 6: Outbound pass (dynamic rule)");

	wait_conn(SOCK_STREAM, SHOULD_PASS, SHOULD_NOT_ADD_RULE);

	control_pong_ping(control_socket);

	wait_conn(SOCK_DGRAM, SHOULD_PASS, SHOULD_NOT_ADD_RULE);

	control_ping_pong(control_socket);

	test_header("Phase 7: Inbound pass (firewall disabled)");

	make_conn(SOCK_STREAM, ip, SHOULD_PASS);

	control_pong_ping(control_socket);

	make_conn(SOCK_DGRAM, ip, SHOULD_PASS);

	control_ping_pong(control_socket);

	shutdown(control_socket, SHUT_RDWR);
	close(control_socket);
	control_socket = -1;

	test_header("Results - Outside");

	printf(" PASSED TESTS: %d\n", passes);
	printf(" FAILED TESTS: %d\n", failures);
	printf("\n");

	return 0; 
}

/*
 * print help message to user
 */

static void print_usage(char *exec)
{
	fprintf(stderr, "SO2 - IP Firewall Tester\n");
	fprintf(stderr, "Usage: %s inside <port> <ip_address>\n", exec);
	fprintf(stderr, "       %s outside <port>\n", exec);
}

/*
 * Usage:
 * ./test inside <port> <ip_address>
 *	ip_address - external IP address
 *
 * ./test outside <port>
 */

int main(int argc, char **argv)
{
	if (argc < 3 || argc > 4) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	srand(time(NULL));

	signal(SIGINT, int_handler);	
	signal(SIGTERM, int_handler);	
	signal(SIGALRM, alarm_handler);	
	siginterrupt(SIGALRM, 1);

	if (argc == 4 && !strcmp(argv[1], "inside")) {
		printf("SO2 - IP Firewall Tester - INSIDE\n");
		return inside(atoi(argv[2]), resolve_hostname(argv[3]));
	}

	if (argc == 3 && !strcmp(argv[1], "outside")) {
		printf("SO2 - IP Firewall Tester - OUTSIDE\n");
		return outside(atoi(argv[2]));
	}

	print_usage(argv[0]);
	exit(EXIT_FAILURE);

	return 0;
}
