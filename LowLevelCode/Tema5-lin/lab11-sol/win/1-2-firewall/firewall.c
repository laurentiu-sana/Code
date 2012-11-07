/*
 * SO2 - Networking Lab (#11)
 *
 * Exercise #1, #2 (Firewall-Hook): simple Firewall-Hook driver
 */

#include <ntddk.h>
#include <ndis.h>
#include <ipfirewall.h>

#include "../include/filter.h"
#include "../include/NetHeaders.h"

#define ON			1
#define OFF			0
#define DEBUG			ON

#if DEBUG == ON
#define LOG(s)					\
	do {					\
		DbgPrint(s "\n");		\
	} while (0)
#else
#define LOG(s)					\
	do {					\
	} while (0)
#endif

/*
 * display an IP address in readable format
 */
#define NIPQUAD(addr) 	\
	((unsigned char *)&(addr))[0], \
	((unsigned char *)&(addr))[1], \
	((unsigned char *)&(addr))[2], \
	((unsigned char *)&(addr))[3]
#define NIPQUAD_FMT	"%u.%u.%u.%u"


static INT ioctlSet = FALSE;	/* ioctl set filter address flag */
static UINT ioctlSetAddress;	/* ioctl set filter address */		 

/*
 * test ioctlSetAddress if it has been set
 */
static int testDestinationAddress(UINT destinationAddress)
{
	if (ioctlSet == TRUE)
		return (ioctlSetAddress == destinationAddress);

	return TRUE;
}


/*
 * effective filtering function - called by cbFilterFunction after
 * buffer assembly; function signature follows Filter-Hook driver
 * callback function signature (we are lazy and like to reuse stuff :-D)
 */
static FORWARD_ACTION FilterPacket(
		unsigned char	*PacketHeader,
		unsigned char	*Packet,
		unsigned int	PacketLength,
		DIRECTION_E	direction,
		unsigned int	RecvInterfaceIndex,
		unsigned int	SendInterfaceIndex)
{
	IPHeader *iph = (IPHeader *) PacketHeader;	//IP header

	//TCP protocol & destination address is the one specified by ioctl
	if (iph->protocol == IPPROTO_TCP &&
			testDestinationAddress(iph->source)) {
		TCPHeader *tcph = (TCPHeader *) Packet;	//TCP header

		if (tcph->syn && !tcph->ack)		//init connection
			DbgPrint("TCP connection initiated from "
				NIPQUAD_FMT ":%d\n", NIPQUAD(iph->source),
				RtlUshortByteSwap(tcph->sourcePort));
	}

	return FORWARD;
}

/*
 * Firewall-Hook driver callback function
 */

static FORWARD_ACTION cbFilterFunction(
		VOID	**pData,
		UINT	RecvInterfaceIndex,
		UINT	*pSendInterfaceIndex,
		UCHAR	*pDestinationType,
		VOID	*pContext,
		UINT	ContextLength,
		struct IPRcvBuf	**pRcvBuf)
{
	FORWARD_ACTION result = FORWARD;
	char *packet = NULL;
	int bufferSize;
	struct IPRcvBuf *buffer = (struct IPRcvBuf *) *pData; 

	PFIREWALL_CONTEXT_T fwContext = (PFIREWALL_CONTEXT_T) pContext;

	unsigned int offset = 0;
	IPHeader *iph;

	if (buffer == NULL) {
		DbgPrint("buffer has no data\n");
		return result;
	}

	bufferSize = buffer->ipr_size;
	while (buffer->ipr_next != NULL) {
		buffer = buffer->ipr_next;
		bufferSize += buffer->ipr_size;
	}

	packet = (char *) ExAllocatePoolWithTag(NonPagedPool,
			bufferSize, 'p');
	if (packet == NULL) {
		DbgPrint("ipdriver.sys: out of memory.\n");
		goto err_out;
	}

	iph = (IPHeader *) packet;

	buffer = (struct IPRcvBuf *) *pData; 
	RtlCopyMemory(packet, buffer->ipr_buffer, buffer->ipr_size);
	while (buffer->ipr_next != NULL) {
		offset += buffer->ipr_size;

		buffer = buffer->ipr_next;
		RtlCopyMemory(packet + offset, buffer->ipr_buffer,
				buffer->ipr_size);
	}

	/* call true filtering function */
	result =  FilterPacket(
			packet,
			packet + (iph->headerLength * 4),
			bufferSize - (iph->headerLength * 4),
			(fwContext != NULL) ? fwContext->Direction: 0,
			RecvInterfaceIndex,
			(pSendInterfaceIndex != NULL) ? *pSendInterfaceIndex : 0);

	if (packet != NULL)
		ExFreePoolWithTag(packet, 'p');

	return result;

err_out:
	return result;
}

static NTSTATUS SetCallback(IPPacketFirewallPtr filterFunction,
		BOOLEAN load)
{
	NTSTATUS status = STATUS_SUCCESS;
	NTSTATUS waitStatus = STATUS_SUCCESS;
	UNICODE_STRING filterName;
	PDEVICE_OBJECT ipDeviceObject = NULL;
	PFILE_OBJECT ipFileObject = NULL;

	IP_SET_FIREWALL_HOOK_INFO filterData;

	KEVENT event;
	IO_STATUS_BLOCK ioStatus;
	PIRP irp;

	RtlInitUnicodeString(&filterName, DD_IP_DEVICE_NAME);
	status = IoGetDeviceObjectPointer(&filterName, STANDARD_RIGHTS_ALL, &ipFileObject, &ipDeviceObject);

	if (NT_SUCCESS(status)) {
		filterData.FirewallPtr = filterFunction;
		filterData.Priority = 1;
		filterData.Add = load;

		KeInitializeEvent(&event, NotificationEvent, FALSE);

		irp = IoBuildDeviceIoControlRequest(
				IOCTL_IP_SET_FIREWALL_HOOK,
				ipDeviceObject,
				(PVOID) &filterData,
				sizeof(IP_SET_FIREWALL_HOOK_INFO),
				NULL,
				0,
				FALSE,
				&event,
				&ioStatus);
		if (irp != NULL) {
			status = IoCallDriver(ipDeviceObject, irp);
			if (status == STATUS_PENDING) {
				waitStatus = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
				if (waitStatus != STATUS_SUCCESS)
					DbgPrint("error waiting for IP device.\n");
			}

			status = ioStatus.Status;
			if (!NT_SUCCESS(status))
				DbgPrint("IP device status error.\n");
		}
		else {
			status = STATUS_INSUFFICIENT_RESOURCES;
			DbgPrint("filter IRP creation failed.\n");
		}

		if (ipFileObject != NULL)
			ObDereferenceObject(ipFileObject);

		ipFileObject = NULL;
		ipDeviceObject = NULL;
	}
	else
		DbgPrint("error getting object pointer to IP device.\n");

	return status;
}

static NTSTATUS RegisterCallback(IPPacketFirewallPtr filterFunction)
{
	return SetCallback(filterFunction, 1);
}

static NTSTATUS UnregisterCallback(IPPacketFirewallPtr filterFunction)
{
	return SetCallback(filterFunction, 0);
}

static NTSTATUS Open(PDEVICE_OBJECT device, IRP *irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS DeviceIoControl(PDEVICE_OBJECT device, IRP *irp)
{
	ULONG controlCode, inSize, outSize;
	PIO_STACK_LOCATION pIrpStack;
	int delay;
	char* buffer;
	int information = 0;
	NTSTATUS status = STATUS_SUCCESS;

	pIrpStack = IoGetCurrentIrpStackLocation(irp);
	controlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
	buffer = irp->AssociatedIrp.SystemBuffer;
	inSize = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
	outSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	switch (controlCode) {
		case MY_IOCTL_FILTER_ADDRESS:
			ioctlSet = TRUE;
			RtlCopyMemory(&ioctlSetAddress, buffer, sizeof(UINT));
			break;

		default:
			status = STATUS_INVALID_DEVICE_REQUEST;
	}

	irp->IoStatus.Status = status;
	irp->IoStatus.Information = information; 
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return status;
}

static NTSTATUS Close(PDEVICE_OBJECT device, IRP *irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static NTSTATUS Cleanup(PDEVICE_OBJECT device, IRP *irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

static UNICODE_STRING *TO_UNICODE(const char *str, UNICODE_STRING *unicodeStr)
{
	ANSI_STRING ansiStr;

	RtlInitAnsiString(&ansiStr, str);
	if (RtlAnsiStringToUnicodeString(unicodeStr, &ansiStr, TRUE) != STATUS_SUCCESS) 
		return NULL;

	return unicodeStr;
}

void DriverUnload(PDRIVER_OBJECT driver)
{
	DEVICE_OBJECT *device;
	UNICODE_STRING linkUnicodeName;

	/* unregister callback function */
	UnregisterCallback(cbFilterFunction);

	if (TO_UNICODE(DEVICE_PATH_LINK, &linkUnicodeName)) 
		IoDeleteSymbolicLink(&linkUnicodeName);
	if (linkUnicodeName.Buffer)
		RtlFreeUnicodeString(&linkUnicodeName);

	while ((device = driver->DeviceObject))
		IoDeleteDevice(device);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registry)
{
	DEVICE_OBJECT *device;
	NTSTATUS ret;
	UNICODE_STRING devUnicodeName, linkUnicodeName;

	RtlZeroMemory(&devUnicodeName, sizeof(devUnicodeName)); 
	RtlZeroMemory(&linkUnicodeName, sizeof(linkUnicodeName));

	ret = IoCreateDevice(
			driver,
			0,
			TO_UNICODE(DEVICE_PATH_KERNEL, &devUnicodeName),
			FILE_DEVICE_UNKNOWN,
			0,
			FALSE,
			&device);
	if (ret != STATUS_SUCCESS)
		goto error;

	ret = IoCreateSymbolicLink(
			TO_UNICODE(DEVICE_PATH_LINK, &linkUnicodeName),
			&devUnicodeName);
	if (ret != STATUS_SUCCESS)
		goto error;

	device->Flags |= DO_BUFFERED_IO;

	if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer) 
		RtlFreeUnicodeString(&linkUnicodeName);

	driver->DriverUnload = DriverUnload;
	driver->MajorFunction[ IRP_MJ_CREATE ] = Open;
	driver->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = DeviceIoControl;
	driver->MajorFunction[ IRP_MJ_CLEANUP ] = Cleanup;
	driver->MajorFunction[ IRP_MJ_CLOSE ] = Close;

	/* register callback function */
	RegisterCallback(cbFilterFunction);

	return STATUS_SUCCESS;

error:
	if (devUnicodeName.Buffer)
		RtlFreeUnicodeString(&devUnicodeName);
	if (linkUnicodeName.Buffer) 
		RtlFreeUnicodeString(&linkUnicodeName);
	DriverUnload(driver);

	return ret;
}
