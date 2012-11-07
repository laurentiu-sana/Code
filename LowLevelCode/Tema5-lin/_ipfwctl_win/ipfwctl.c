/*
 * SO2 - Homework #5 - Stateful firewall
 *
 * ipfwctl.c: IP firewall driver user-space control tool (Windows)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "ipfirewall.h"

#define IPFIREWALL_MOD		"ipfirewall"
#define IPFIREWALL_MOD_EXT	".sys"
#define IPFIREWALL_MOD_PATH	"objchk_wnet_x86\\i386\\" IPFIREWALL_MOD IPFIREWALL_MOD_EXT

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
	fprintf(stderr, "SO2 - IP Firewall Control [Windows]\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s mknod   - start IpFilterDriver service\n", exec);
	fprintf(stderr, "\t%s unlink  - stop IpFilterDriver service\n", exec);
	fprintf(stderr, "\t%s insmod  - insert module %s.sys\n", exec, IPFIREWALL_MOD);
	fprintf(stderr, "\t%s rmmod   - remove module %s\n", exec, IPFIREWALL_MOD);
	fprintf(stderr, "\t%s test    - test module presence\n", exec);
	fprintf(stderr, "\t%s invalid - execute ioctl 42\n", exec);
	fprintf(stderr, "\t%s add  ip_src/mask ip_dst/mask port_src_fst:port_src_lst port_dst_fst:port_dst_lst\n", exec);
	fprintf(stderr, "\t%s find ip_src/mask ip_dst/mask port_src_fst:port_src_lst port_dst_fst:port_dst_lst\n", exec);
	fprintf(stderr, "\t%s enable  - execute ioctl FW_ENABLE\n", exec);
	fprintf(stderr, "\t%s disable - execute ioctl FW_DISABLE\n", exec);
	fprintf(stderr, "\t%s list    - execute ioctl FW_LIST and print rules; exit status = number of rules\n", exec);
}

static void print_error(char *msg)
{
	char *err;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (char *) &err, 0, NULL);
	fprintf(stderr, "%s: %08x %s", msg, GetLastError(), err);
	LocalFree(err);
}

static void parse_ip(char *s, unsigned int *ip, unsigned int *mask)
{
	char *tmp, *check;
	int i;
	unsigned int tmp2 = 0;

	if (!(tmp = strchr(s, '/'))) {
		print_usage();
		ExitProcess(-1);
	}
	*tmp = 0; tmp++;

	if ((*ip = inet_addr(s)) == INADDR_NONE) {
		print_usage();
		ExitProcess(-1);
	}

	*mask = strtoul(tmp, &check, 10);
	if (*check != 0 || *mask > 32) {
		print_usage();
		ExitProcess(-1);
	}

	for (i = 0; i < *mask; i++)
		tmp2 |= 1 << i;
	*mask = tmp2;
}

void parse_range(char *s, unsigned short *start, unsigned short *stop)
{
	char *tmp, *check;
	int val;

	tmp = strchr(s, ':');
	if (!tmp) {
		print_usage();
		ExitProcess(-1);
	}
	*tmp = 0; tmp++;

	val = (unsigned short) strtoul(s, &check, 10);
	if (*check != 0 || val > 0xffff) {
		print_usage();
		ExitProcess(-1);
	}
	*start = htons(val);

	val = (unsigned short) strtoul(tmp, &check, 10);
	if (*check != 0 || val > 0xffff) {
		print_usage();
		ExitProcess(-1);
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

int main(int argc, char **argv)
{
	struct fwr rule;
	HANDLE handle;
	int ret, f = 0;
	DWORD temp;

	exec = argv[0];
	if (argc < 2) {
		print_usage();
		ExitProcess(-1);
	}

	if (!strcmp(argv[1], "mknod") && argc == 2) {
		// system("route delete 0.0.0.0 mask 0.0.0.0");
		system("net start IpFilterDriver");

		return 0;
	}

	if (!strcmp(argv[1], "unlink") && argc == 2) {
		system("net stop IpFilterDriver");

		return 0;
	}

	if (!strcmp(argv[1], "insmod") && argc == 2) {
		if (system("driver load " IPFIREWALL_MOD_PATH) < 0)
			return -1;

		return 0;
	}

	if (!strcmp(argv[1], "rmmod") && argc == 2) {
		if (system("driver unload " IPFIREWALL_MOD) < 0)
			return -1;

		return 0;
	}

	handle = CreateFile(FIREWALL_DEVICE, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL);
	if (handle == INVALID_HANDLE_VALUE) {
		print_error("open " FIREWALL_DEVICE);
		return -1;
	}

	if (!strcmp(argv[1], "test") && argc == 2) {
		CloseHandle(handle);
		return 0;
	}

	if (!strcmp(argv[1], "invalid") && argc == 2) {
		if (DeviceIoControl(handle, 42, NULL, 0, NULL, 0, &temp, NULL) == 0) {
			if (GetLastError() != ERROR_INVALID_FUNCTION) {
				print_error("ioctl 42");
				CloseHandle(handle);
				return -1;
			}

			CloseHandle(handle);
			return 0;
		}

		print_error("ioctl 42");
		CloseHandle(handle);
		return -1;
	}

	if (!strcmp(argv[1], "enable") && argc == 2) {
		if (DeviceIoControl(handle, FW_ENABLE, NULL, 0, NULL, 0, &temp, NULL) == 0) {
			print_error("ioctl FW_ENABLE");
			CloseHandle(handle);
			return -1;
		}

		CloseHandle(handle);
		return 0;
	}

	if (!strcmp(argv[1], "disable") && argc == 2) {
		if (DeviceIoControl(handle, FW_DISABLE, NULL, 0, NULL, 0, &temp, NULL) == 0) {
			print_error("ioctl FW_DISABLE");
			CloseHandle(handle);
			return -1;
		}

		CloseHandle(handle);
		return 0;
	}

	if ((!strcmp(argv[1], "add") || !strcmp(argv[1], "find")) && argc == 6) {
		parse_ip(argv[2], &rule.ip_src, &rule.ip_src_mask);
		parse_ip(argv[3], &rule.ip_dst, &rule.ip_dst_mask);
		parse_range(argv[4], &rule.port_src[0], &rule.port_src[1]);
		parse_range(argv[5], &rule.port_dst[0], &rule.port_dst[1]);

		if (argv[1][0] == 'a') {
			/* add rule */
			if (DeviceIoControl(handle, FW_ADD_RULE, &rule, sizeof(rule), NULL, 0, &temp, NULL) == 0) {
				perror("ioctl FW_ADD_RULE");
				CloseHandle(handle);
				return -1;
			}

			CloseHandle(handle);
			return 0;
		}

		f = 1;
	}

	if (f || (!strcmp(argv[1], "list") && argc == 2)) {
		char *buf = NULL;
		struct fwr *fwr = NULL;
		int size, size2, i;

		buf = (char *) malloc(sizeof(int));
		if (!buf) {
			fprintf(stderr, "out of memory\n");
			CloseHandle(handle);
			return -1;
		}

		do {
			size = 0;

			if (DeviceIoControl(handle, FW_LIST, &size, sizeof(int), buf, sizeof(int), &temp, NULL) == 0) {
				perror("ioctl FW_LIST");
				CloseHandle(handle);
				return -1;
			}

			size2 = *(int*) buf;
			size = sizeof(int) + size2 * sizeof(struct fwr);

			buf = realloc(buf, size);
			if (!buf) {
				fprintf(stderr, "out of memory\n");
				CloseHandle(handle);
				return -1;
			}

			if (DeviceIoControl(handle, FW_LIST, &size2, sizeof(int), buf, size, &temp, NULL) == 0) {
				perror("ioctl FW_LIST");
				CloseHandle(handle);
				return -1;
			}

			size = *(int*) buf;

		} while (size > size2);

		CloseHandle(handle);
		if (buf)
			fwr = (struct fwr *) (buf + sizeof(int));

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

		if (buf)
			free(buf);

		return ret;
	}

	print_usage();

	CloseHandle(handle);
	return -1;
}
