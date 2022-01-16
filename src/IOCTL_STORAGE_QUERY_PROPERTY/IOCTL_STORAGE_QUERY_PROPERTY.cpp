// IOCTL_STORAGE_QUERY_PROPERTY.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <windows.h>
#include <winioctl.h>


void PrintUsage()
{
    printf("================================\n");
    printf("Usage : IOCTL_STORAGE_QUERY_PROPERTY.exe <Target Physical Disk>\n");
    printf("Example : IOCTL_STORAGE_QUERY_PROPERTY.exe PhysicalDrive1\n");
    printf("================================\n");
}


//Note: In Storport Driver, this query is implemented by SCSI INQUIRY command.
//      So you have to implement this SCSI Command.
//  VPD pages of SCSI Inquiry needed: VPD_SERIAL_NUMBER, VPD_DEVICE_IDENTIFIERS (maybe there are more others needed...)

int main(int argc, char* argv[])
{
    if(argc <2)
    {
        PrintUsage();
        return -1;
    }
    char devname[128] = {0};
    _snprintf_s(devname, 128, 127, "\\\\.\\%s", argv[1]);

    HANDLE hDevice = CreateFileA(devname, 0,    //to query storage property, you don't need R/W permission
                                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
                                OPEN_EXISTING, 0, NULL);
    if(hDevice != INVALID_HANDLE_VALUE)
    {
        STORAGE_PROPERTY_QUERY query;
        DWORD ret_size = 0;

        ZeroMemory(&query, sizeof(STORAGE_PROPERTY_QUERY));
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;
        STORAGE_DESCRIPTOR_HEADER header = {0};

        if(FALSE == DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(STORAGE_PROPERTY_QUERY),
                            &header, sizeof(STORAGE_DESCRIPTOR_HEADER), &ret_size, NULL))
        {
            printf("query device property header failed, last_error = %d\n", GetLastError());
            goto end;
        }

        printf("header return size = %d, required size = %d\n", ret_size, header.Size);
        //int size = max(header.Size, 1024);
        char buffer[2048] = {0};
        if (FALSE == DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(STORAGE_PROPERTY_QUERY),
            buffer, 2048, &ret_size, NULL))
        {
            printf("query device property failed, last_error = %d\n", GetLastError());
            goto end;
        }

        printf("property return size = %d\n", ret_size);

        STORAGE_DEVICE_DESCRIPTOR *descriptor = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
        
        printf("Parse replied STORAGE_DEVICE_DESCRIPTOR =>\n");
        printf("\tDeviceType=%d, DeviceTypeModifier=%d\n", descriptor->DeviceType, descriptor->DeviceTypeModifier);
        printf("\tRemovable Media=%d, CmdQueueing=%d\n", descriptor->RemovableMedia, descriptor->CommandQueueing);
        printf("\tVenID Offset(%d), ProdID Offset(%d), RevID Offset(%d), SN Offset(%d)\n", 
                descriptor->VendorIdOffset, descriptor->ProductIdOffset, descriptor->ProductRevisionOffset, descriptor->SerialNumberOffset);

        printf("\rraw data:\n\t");
        BYTE *rawdata = (BYTE*)buffer;
        for(int i=0; i< descriptor->Size; i++)
        {
            if(i > 0 && i%16 == 0)
                printf("\n\t");

            printf("%02X,", rawdata[i]);
        }
        printf("\n");
    }
    else
        printf("open device failed, last_error = %d\n", GetLastError());

end:
    if (hDevice != INVALID_HANDLE_VALUE)
        CloseHandle(hDevice);
}
