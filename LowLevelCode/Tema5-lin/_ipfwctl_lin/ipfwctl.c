/*
 * SO2 - Homework #5 - Stateful firewall
 *
 * ipfwctl.c: IP firewall driver user-space control tool (Linux)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ipfirewall.h"

#define IPFIREWALL_MOD		"ipfirewall"
#define IPFIREWALL_MOD_EXT	".ko"
#define IPFIREWALL_DEV		"/dev/" IPFIREWALL_MOD

/*
 * display an IP address in readable format
 */
#define NIPQUAD(addr)			\
	((unsigned char *)&(addr))[0],	\
	((unsigned char *)&(addr))[1],	\
	((unsigned char *)&(addr))[2],	\
	((unsigned char *)&(addr))[3]
#define NIPQUAD_FMT		"%u.%u.%u.%u"

static char *exec;

/*
 * print usage syntax to user
 */
static void print_usage(void)
{
	fprintf(stderr, "SO2 - IP Firewall Control [Linux]\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s mknod   - create device %s with major %d\n", exec, IPFIREWALL_DEV, IPFIREWALL_MAJOR);
	fprintf(stderr, "\t%s unlink  - delete device %s\n", exec, IPFIREWALL_DEV);
	fprintf(stderr, "\t%s insmod  - insert module %s%s\n", exec, IPFIREWALL_MOD, IPFIREWALL_MOD_EXT);
	fprintf(stderr, "\t%s rmmod   - remove module %s\n", exec, IPFIREWALL_MOD);
	fprintf(stderr, "\t%s test    - test module presence\n", exec);
	fprintf(stderr, "\t%s invalid - execute ioctl 42\n", exec);
	fprintf(stderr, "\t%s add  ip_src/mask ip_dst/mask port_src_fst:port_src_lst port_dst_fst:port_dst_lst\n", exec);
	fprintf(stderr, "\t%s find ip_src/mask ip_dst/mask port_src_fst:port_src_lst port_dst_fst:port_dst_lst\n", exec);
	fprintf(stderr, "\t%s enable  - execute ioctl FW_ENABLE\n", exec);
	fprintf(stderr, "\t%s disable - execute ioctl FW_DISABLE\n", exec);
	fprintf(stderr, "\t%s list    - execute ioctl FW_LIST and print rules; exit status = number of rules\n", exec);
}

static void parse_ip(char *s, unsigned int *ip, unsigned int *mask)
{
	char *tmp, *check;
	int i;
	unsigned int tmp2 = 0;

	if (!(tmp = strchr(s, '/'))) {
		print_usage();
		exit(-1);
	}
	*tmp = 0; tmp++;

	if ((*ip = inet_addr(s)) == INADDR_NONE) {
		print_usage();
		exit(-1);
	}

	*mask = strtoul(tmp, &check, 10);
	if (*check != 0 || *mask > 32) {
		print_usage();
		exit(-1);
	}

	for (i = 0; i < *mask; i++)
		tmp2 |= 1 << i;
	*mask = tmp2;
}

static void parse_range(char *s, unsigned short *start, unsigned short *stop)
{
	char *tmp, *check;
	int val;

	tmp = strchr(s, ':');
	if (!tmp) {
		print_usage();
		exit(-1);
	}
	*tmp = 0; tmp++;

	val = (unsigned short) strtoul(s, &check, 10);
	if (*check != 0 || val > 0xffff) {
		print_usage();
		exit(-1);
	}
	*start = htons(val);

	val = (unsigned short) strtoul(tmp, &check, 10);
	if (*check != 0 || val > 0xffff) {
		print_usage();
		exit(-1);
	}
	*stop = htons(val);
}

static int mask_len(int x)
{
	int i;

	for (i = 0; i < 32; i++)
		if (!(x & (1 << i)))
			return i;

	return i;
}

static int system_check(char *cmd)
{
	int ret;

	ret = system(cmd);
	if (ret == -1) {
		perror(cmd);
		return -1;
	}
	if (ret && WIFEXITED(ret)) {
		fprintf(stderr, "%s: exit status %d\n", cmd, WEXITSTATUS(ret));
		return -1;
	}
	if (ret && WIFSIGNALED(ret)) {
		fprintf(stderr, "%s: terminated by signal %d\n", cmd, WTERMSIG(ret));
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct fwr rule;
	int fd, ret, f = 0;

	exec = argv[0];
	if (argc < 2) {
		print_usage();
		return -1;
	}

	if (!strcmp(argv[1], "mknod") && argc == 2) {
		if (mknod(IPFIREWALL_DEV, S_IFCHR, makedev(IPFIREWALL_MAJOR, 0)) < 0) {
			perror("mknod " IPFIREWALL_DEV);
			return -1;
		}

		return 0;
	}

	if (!strcmp(argv[1], "unlink") && argc == 2) {
		if (unlink(IPFIREWALL_DEV) < 0) {
			perror("unlink " IPFIREWALL_DEV);
			return -1;
		}

		return 0;
	}

	if (!strcmp(argv[1], "insmod") && argc == 2) {
		if (system_check("insmod " IPFIREWALL_MOD IPFIREWALL_MOD_EXT) < 0)
			return -1;

		return 0;
	}

	if (!strcmp(argv[1], "rmmod") && argc == 2) {
		if (system_check("rmmod " IPFIREWALL_MOD) < 0)
			return -1;

		return 0;
	}

	fd = open(IPFIREWALL_DEV, O_RDWR);
	if (fd < 0) {
		perror("open " IPFIREWALL_DEV);
		return -1;
	}

	if (!strcmp(argv[1], "test") && argc == 2) {
		close(fd);
		return 0;
	}

	if (!strcmp(argv[1], "invalid") && argc == 2) {
		if (ioctl(fd, 42, NULL) < 0) {
			if (errno != ENOTTY) {
				perror("ioctl 42");
				close(fd);
				return -1;
			}

			close(fd);
			return 0;
		}

		perror("ioctl 42");
		close(fd);
		return -1;
	}

	if (!strcmp(argv[1], "enable") && argc == 2) {
		if (ioctl(fd, FW_ENABLE, NULL) < 0) {
			perror("ioctl FW_ENABLE");
			close(fd);
			return -1;
		}

		close(fd);
		return 0;
	}

	if (!strcmp(argv[1], "disable") && argc == 2) {
		if (ioctl(fd, FW_DISABLE, NULL) < 0) {
			perror("ioctl FW_DISABLE");
			close(fd);
			return -1;
		}

		close(fd);
		return 0;
	}

	if ((!strcmp(argv[1], "add") || !strcmp(argv[1], "find")) && argc == 6) {
		parse_ip(argv[2], &rule.ip_src, &rule.ip_src_mask);
		parse_ip(argv[3], &rule.ip_dst, &rule.ip_dst_mask);
		parse_range(argv[4], &rule.port_src[0], &rule.port_src[1]);
		parse_range(argv[5], &rule.port_dst[0], &rule.port_dst[1]);

		if (argv[1][0] == 'a') {
			/* add rule */
			if (ioctl(fd, FW_ADD_RULE, &rule) < 0) {
				perror("ioctl FW_ADD_RULE");
				close(fd);
				return -1;
			}

			close(fd);
			return 0;
		}

		f = 1;
	}

	if (f || (!strcmp(argv[1], "list") && argc == 2)) {
		struct fwr *fwr = NULL;
		int size, i;

		do {
			size = 0;

			if (ioctl(fd, FW_LIST, &size) < 0 && errno != ENOSPC) {
				perror("ioctl FW_LIST");
				close(fd);
				return -1;
			}

			fwr = (struct fwr *) realloc(fwr, size * sizeof(struct fwr));
			if (!fwr) {
				fprintf(stderr, "out of memory\n");
				close(fd);
				return -1;
			}
			*(int*) fwr = size;

			size = ioctl(fd, FW_LIST, fwr);
			if (size < 0 && errno != ENOSPC) {
				perror("ioctl FW_LIST");
				close(fd);
				return -1;
			}
		} while (size < 0);

		close(fd);

		if (f) {
			/* find rule */
			ret = -1;
			for (i = 0; i < size && ret; i++)
				if (!memcmp(&fwr[i], &rule, sizeof(struct fwr)))
					ret = 0;
			if (ret)
				fprintf(stderr, "rule not found\n");
		} else {
			/* list rules */
			ret = size;
			for (i = 0; i < size; i++)
				printf(NIPQUAD_FMT "/%d " NIPQUAD_FMT "/%d %d:%d %d:%d\n",
					NIPQUAD(fwr[i].ip_src), mask_len(fwr[i].ip_src_mask),
					NIPQUAD(fwr[i].ip_dst), mask_len(fwr[i].ip_dst_mask),
					ntohs(fwr[i].port_src[0]), ntohs(fwr[i].port_src[1]),
					ntohs(fwr[i].port_dst[0]), ntohs(fwr[i].port_dst[1]));
		}

		if (fwr)
			free(fwr);

		return ret;
	}

	print_usage();

	close(fd);
	return -1;
}
