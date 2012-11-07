/*
 * SO2 - Networking Lab (#11)
 *
 * Exercise #2: test filter module
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "../include/filter.h"

static void print_usage(char *argv0)
{
	fprintf(stderr, "Usage: %s address\n"
			"\taddress must be a string containing an IP dotted address\n", argv0);
}

static void error(char* message)
{
	fprintf(stderr, "%s (error code: %d)\n", message, GetLastError());
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	HANDLE hDevice;
	BOOL status;
	DWORD bytesReturned;
	UINT addr;

	if (argc != 2) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	/* get address */
	addr = inet_addr(argv[1]);

	hDevice = CreateFile(
			DEVICE_PATH_USER,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
		error("Error opening device");

	/* pass address to filter through DeviceIoControl */
	status = DeviceIoControl(
			hDevice,
			MY_IOCTL_FILTER_ADDRESS,
			&addr, sizeof(addr),
			NULL, 0,
			&bytesReturned,
			NULL);
	if (!status)
		error("DeviceIoControl error");

	status = CloseHandle(hDevice);
	if (!status)
		error("Error closing device");

	return 0;
}
