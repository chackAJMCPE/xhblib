#include "stdafx.h"
#include <stdio.h>
#include "XboxConsoleInfo.h"

// Kernel functions
struct _XBOX_HARDWARE_INFO; 
extern "C" _XBOX_HARDWARE_INFO* XboxHardwareInfo;
extern "C" NTSYSAPI PVOID NTAPI MmGetPhysicalAddress(IN PVOID Address);

// CPU key buffer definition
unsigned char keybuf[0x10];

// fuselines
unsigned long long fuseline0;
unsigned long long fuseline1;
unsigned long long fuseline2;
unsigned long long fuseline3;
unsigned long long fuseline4;
unsigned long long fuseline5;
unsigned long long fuseline6;
unsigned long long fuseline7;
unsigned long long fuseline8;
unsigned long long fuseline9;
unsigned long long fuseline10;
unsigned long long fuseline11;

// Hypervisor syscall function
unsigned long long __declspec(naked) HvxPeekCall(DWORD key, unsigned long long type, 
                                                unsigned long long SourceAddress, 
                                                unsigned long long DestAddress, 
                                                unsigned long long lenInBytes)
{ 
    __asm {
        li      r0, 0x0
        sc
        blr
    }
}

// Wrapper for HvxPeekCall with fixed key and type
unsigned long long HvxPeek(unsigned long long SourceAddress, 
                          unsigned long long DestAddress, 
                          unsigned long long lenInBytes)
{
    return HvxPeekCall(0x72627472, 5, SourceAddress, DestAddress, lenInBytes);
}

// Console type detection
int XCI_GetConsoleType() {
    // Retrieve console type value, defaulting to 0x70000000 if XboxHardwareInfo is NULL
    DWORD value = XboxHardwareInfo ? *(DWORD*)XboxHardwareInfo & 0xF0000000 : 0x70000000;
    BYTE PCIBridgeRevisionID = XboxHardwareInfo ? *((BYTE*)XboxHardwareInfo + 5) : 0;
    
    // Determine console type
    switch (value) {
        case 0x00000000:
            return REV_XENON;
        case 0x10000000:
            return REV_ZEPHYR;
        case 0x20000000:
            return REV_FALCON;
        case 0x30000000:
            return REV_JASPER;
        case 0x40000000:
            return REV_TRINITY;
        case 0x50000000:
            if (PCIBridgeRevisionID == 0x70) {
                return REV_CORONA_PHISON;
            } else {
                return REV_CORONA;
            }
        case 0x60000000:
            return REV_WINCHESTER;
        case 0x70000000:
            return REV_UNKNOWN;
        default:
            return REV_UNKNOWN;
    }
}

// CPU key retrieval
bool XCI_GetCPUKey()
{
    PBYTE buf = (PBYTE)XPhysicalAlloc(0x10, MAXULONG_PTR, 0, MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
    if (buf != NULL)
    {
        unsigned long long dest = 0x8000000000000000ULL | ((DWORD)MmGetPhysicalAddress(buf)&0xFFFFFFFF);
        ZeroMemory(buf, 0x10);
        unsigned long long ret = HvxPeek(0x20ULL, dest, 0x10ULL);
        if(ret == 0x72627472 || ret == dest)
        {
            memcpy(keybuf, buf, 0x10);
            XPhysicalFree(buf);
            return true;
        }
        else
            printf("SysCall Return value: 0x%llX\n", ret);
        XPhysicalFree(buf);
    }
    return false;
}

// Fuse line reading
unsigned long long getFuseline(DWORD fuse) {
    if (fuse >= 12) return 0;  // Only 12 fuse lines (0-11)

    unsigned long long value = 0;
    BYTE* buffer = (BYTE*)XPhysicalAlloc(8, MAXULONG_PTR, 0, 
                       MEM_LARGE_PAGES|PAGE_READWRITE|PAGE_NOCACHE);
    
    if (buffer) {
        // Each fuse line is 0x200 bytes apart
        unsigned long long hvAddress = 0x8000020000020000ULL + (fuse * 0x200);
        unsigned long long physAddr = (unsigned long long)(ULONG_PTR)MmGetPhysicalAddress(buffer);
        unsigned long long destAddr = 0x8000000000000000ULL | (physAddr & 0xFFFFFFFF);

        unsigned long long result = HvxPeekCall(0x72627472, 5, hvAddress, destAddr, 8);
        
        if (result == 0x72627472 || result == destAddr) {
            value = *((unsigned long long*)buffer);
        } else {
            printf("Error reading fuse %d: 0x%llX\n", fuse, result);
        }
        
        XPhysicalFree(buffer);
    }
    return value;
}

bool XCI_GetFuses() {
    unsigned long long* lines[] = {
        &fuseline0, &fuseline1, &fuseline2, &fuseline3,
        &fuseline4, &fuseline5, &fuseline6, &fuseline7,
        &fuseline8, &fuseline9, &fuseline10, &fuseline11
    };

    for (int i = 0; i < 12; i++) {
        *lines[i] = getFuseline(i);
    }
    return true;
}