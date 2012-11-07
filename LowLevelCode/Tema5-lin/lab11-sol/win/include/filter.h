#ifndef MY_FILTER_H__
#define MY_FILTER_H__	1

/* ioctl command to pass address to filter driver */
#define MY_IOCTL_FILTER_ADDRESS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x01, \
             METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA )

#define DEVICE_NAME		"FilterDevice"
#define DEVICE_PATH_USER	"\\\\.\\" DEVICE_NAME
#define DEVICE_PATH_KERNEL	"\\Device\\" DEVICE_NAME
#define DEVICE_PATH_LINK	"\\??\\" DEVICE_NAME

#endif
