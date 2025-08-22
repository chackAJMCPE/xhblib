#ifndef XBOXFS_H
#define XBOXFS_H

// Device ID macros
#define DEVICE_NAND_FLASH         0
#define DEVICE_MEMORY_UNIT0       1
#define DEVICE_MEMORY_UNIT1       2
#define DEVICE_CDROM0             3
#define DEVICE_HARDISK0_PART0     4
#define DEVICE_HARDISK0_PART1     5
#define DEVICE_HARDISK0_PART2     6
#define DEVICE_HARDISK0_PART3     7
#define DEVICE_USB0               8
#define DEVICE_USB1               9
#define DEVICE_USB2               10

// External kernel functions
extern "C" int __stdcall ObCreateSymbolicLink(struct _STRING*, struct _STRING*);
extern "C" int __stdcall ObDeleteSymbolicLink(struct _STRING*);

// Mounting functions
HRESULT XFS_MountAllDevices(int deviceId, char* driveLetter);
HRESULT XFS_UnMountAllDevices(char* driveLetter);

// File operations
HRESULT XFS_MountDevice(int deviceId, const char* driveLetter);
HRESULT XFS_UnmountDevice(const char* driveLetter);
BOOL XFS_MakeDir(const char* path);
BOOL XFS_Delete(const char* path);
BOOL XFS_Move(const char* src, const char* dest);
BOOL XFS_Copy(const char* src, const char* dest);
HANDLE XFS_MakeFile(const char* path);
BOOL XFS_WriteToFile(const char* path, const void* data, DWORD dataSize, BOOL append = FALSE);
BOOL XFS_ReadFile(const char* path, void* buffer, DWORD bufferSize, DWORD* bytesRead);
DWORD XFS_GetFileSize(const char* path);
BOOL XFS_Exists(const char* path);
BOOL XFS_IsDirectory(const char* path);

#endif // XBOXFS_H