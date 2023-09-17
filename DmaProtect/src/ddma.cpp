#include "ddma.h"

#define ATA_IO_TIMEOUT (2)
#define ATA_CMD_READ_SECTORS (0x20)
#define ATA_CMD_WRITE_SECTORS (0x30)
#define ATA_DEVICE_TRANSPORT_LBA (0x40)
#define ATA_SECTOR_SIZE (0x200)

#include <ntddscsi.h>

static NTSTATUS AtaIssueCommand(IN PDEVICE_OBJECT device, IN USHORT flag, IN UCHAR command,
    IN PVOID buffer) {

    KEVENT event;
    KeInitializeEvent(&event, SynchronizationEvent, FALSE);

    ATA_PASS_THROUGH_DIRECT request = { 0 };
    request.Length = sizeof(request);
    request.AtaFlags = flag | ATA_FLAGS_USE_DMA;
    request.DataTransferLength = PAGE_SIZE;
    request.TimeOutValue = ATA_IO_TIMEOUT;
    request.DataBuffer = buffer;

    // For the sake of brevity this uses the first sectors (unsafe!)
    request.CurrentTaskFile[1] = PAGE_SIZE / ATA_SECTOR_SIZE;
    request.CurrentTaskFile[5] = ATA_DEVICE_TRANSPORT_LBA;
    request.CurrentTaskFile[6] = command;

    IO_STATUS_BLOCK ioStatusBlock;
    PIRP irp = IoBuildDeviceIoControlRequest(IOCTL_ATA_PASS_THROUGH_DIRECT, device, &request,
        sizeof(request), &request, sizeof(request), FALSE,
        &event, &ioStatusBlock);

    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS status = IoCallDriver(device, irp);

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatusBlock.Status;
    }

    return status;
}

NTSTATUS AtaReadPage(IN PDEVICE_OBJECT device, OUT PVOID dest) {
    return AtaIssueCommand(device, ATA_FLAGS_DATA_IN, ATA_CMD_READ_SECTORS, dest);
}

NTSTATUS AtaWritePage(IN PDEVICE_OBJECT device, IN PVOID src) {
    return AtaIssueCommand(device, ATA_FLAGS_DATA_OUT, ATA_CMD_WRITE_SECTORS, src);
}

extern "C" POBJECT_TYPE * IoDriverObjectType;
extern "C" NTSTATUS ObReferenceObjectByName(IN PUNICODE_STRING objectName, IN ULONG attributes,
    IN PACCESS_STATE passedAccessState,
    IN ACCESS_MASK desiredAccess,
    IN POBJECT_TYPE objectType,
    IN KPROCESSOR_MODE accessMode,
    IN OUT PVOID parseContext, OUT PVOID * object);

extern "C" NTSTATUS IoEnumerateDeviceObjectList(
    _In_  PDRIVER_OBJECT DriverObject,
    _Out_writes_bytes_to_opt_(DeviceObjectListSize, (*ActualNumberDeviceObjects) * sizeof(PDEVICE_OBJECT)) PDEVICE_OBJECT* DeviceObjectList,
    _In_  ULONG          DeviceObjectListSize,
    _Out_ PULONG         ActualNumberDeviceObjects
);

NTSTATUS GetDeviceObjectList(IN PDRIVER_OBJECT driverObject, OUT PDEVICE_OBJECT** outDevices,
    OUT PULONG outDeviceCount) {

    ULONG count = 0;
    NTSTATUS status = IoEnumerateDeviceObjectList(driverObject, NULL, 0, &count);

    if (status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    ULONG size = count * sizeof(PDEVICE_OBJECT);
    PDEVICE_OBJECT* devices = (PDEVICE_OBJECT*)ExAllocatePool(NonPagedPoolNx, size);
    if (devices) {
        *outDeviceCount = count;

        status = IoEnumerateDeviceObjectList(driverObject, devices, size, &count);
        if (NT_SUCCESS(status)) {
            *outDevices = devices;
        }
        else {
            ExFreePool(devices);
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NTSTATUS DiskFind(OUT PDISK* outDisk) {
    PDISK disk = (PDISK)ExAllocatePool(NonPagedPoolNx, sizeof(DISK));
    if (!disk) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UNICODE_STRING diskStr = RTL_CONSTANT_STRING(L"\\Driver\\Disk");
    PDRIVER_OBJECT diskObject;

    NTSTATUS status = ObReferenceObjectByName(&diskStr, OBJ_CASE_INSENSITIVE, NULL, 0,
        *IoDriverObjectType, KernelMode, NULL, (PVOID*)&diskObject);

    if (NT_SUCCESS(status)) {
        PDEVICE_OBJECT* devices;
        ULONG deviceCount;

        status = GetDeviceObjectList(diskObject, &devices, &deviceCount);

        if (NT_SUCCESS(status)) {
            status = STATUS_NOT_FOUND;

            for (ULONG i = 0; i < deviceCount; ++i) {
                PDEVICE_OBJECT device = devices[i];

                if (status == STATUS_NOT_FOUND && NT_SUCCESS(AtaReadPage(device, disk->Buffer))) {
                    disk->Device = device;
                    status = STATUS_SUCCESS;
                    continue;
                }

                ObDereferenceObject(device);
            }

            ExFreePool(devices);
        }

        ObDereferenceObject(diskObject);
    }

    if (NT_SUCCESS(status)) {
        *outDisk = disk;
    }
    else {
        ExFreePool(disk);
    }

    return status;
}

NTSTATUS DdmaProvider::DiskCopy(PVOID dest, PVOID src) {
    if (!pDisk)
        return STATUS_NOT_IMPLEMENTED;

    // Read from src by writing to disk
    NTSTATUS status = AtaWritePage(pDisk->Device, src);
    if (NT_SUCCESS(status)) {
        // Write to dest by reading from disk
        status = AtaReadPage(pDisk->Device, dest);

        // Restore original sectors
        AtaWritePage(pDisk->Device, pDisk->Buffer);
    }

    return status;
}

DdmaProvider::DdmaProvider()
{
    DiskFind(&pDisk);
}

DdmaProvider::~DdmaProvider()
{
    ObDereferenceObject(pDisk->Device);
    ExFreePool(pDisk);
}
