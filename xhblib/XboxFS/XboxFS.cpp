#include "stdafx.h"
#include "XboxFS.h"
#include <stdio.h>
#include <vector>
#include <string>

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

// Kernel STRING type
typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR  Buffer;
} STRING;

// Mount a device to a drive letter
HRESULT XFS_MountAllDevices(int periphPhys, char* lettreLecteur)
{
    char lecteurCible[16];
    sprintf_s(lecteurCible, "\\??\\%s", lettreLecteur);

    char * periphOriginal;
    switch (periphPhys)
    {
    case DEVICE_NAND_FLASH:
        periphOriginal = "\\Device\\Flash";
        break;
    case DEVICE_MEMORY_UNIT0:
        periphOriginal = "\\Device\\Mu0";
        break;
    case DEVICE_MEMORY_UNIT1:
        periphOriginal = "\\Device\\Mu1";
        break;
    case DEVICE_CDROM0:
        periphOriginal = "\\Device\\Cdrom0";
        break;
    case DEVICE_HARDISK0_PART0:
        periphOriginal = "\\Device\\Harddisk0\\Partition0";
        break;
    case DEVICE_HARDISK0_PART1:
        periphOriginal = "\\Device\\Harddisk0\\Partition1";
        break;
    case DEVICE_HARDISK0_PART2:
        periphOriginal = "\\Device\\Harddisk0\\Partition2";
        break;
    case DEVICE_HARDISK0_PART3:
        periphOriginal = "\\Device\\Harddisk0\\Partition3";
        break;
    case DEVICE_USB0:
        periphOriginal = "\\Device\\Mass0";
        break;
    case DEVICE_USB1:
        periphOriginal = "\\Device\\Mass1";
        break;
    case DEVICE_USB2:
        periphOriginal = "\\Device\\Mass2";
        break;
    default:
        return E_INVALIDARG;
    }

    STRING PeriphOriginal = {
        static_cast<USHORT>(strlen(periphOriginal)),
        static_cast<USHORT>(strlen(periphOriginal) + 1),
        periphOriginal
    };

    STRING LienSymbolique = {
        static_cast<USHORT>(strlen(lecteurCible)),
        static_cast<USHORT>(strlen(lecteurCible) + 1),
        lecteurCible
    };

    return static_cast<HRESULT>(ObCreateSymbolicLink(&LienSymbolique, &PeriphOriginal));
}

// Unmount a device
HRESULT XFS_UnMountAllDevices(char* lettreLecteur)
{
    char lecteurCible[16];
    sprintf_s(lecteurCible, "\\??\\%s", lettreLecteur);

    STRING LienSymbolique = {
        static_cast<USHORT>(strlen(lecteurCible)),
        static_cast<USHORT>(strlen(lecteurCible) + 1),
        lecteurCible
    };

    return static_cast<HRESULT>(ObDeleteSymbolicLink(&LienSymbolique));
}

// Wrapper for mounting with const string
HRESULT XFS_MountDevice(int deviceId, const char* driveLetter)
{
    return XFS_MountAllDevices(deviceId, const_cast<char*>(driveLetter));
}

// Wrapper for unmounting with const string
HRESULT XFS_UnmountDevice(const char* driveLetter)
{
    return XFS_UnMountAllDevices(const_cast<char*>(driveLetter));
}

// Create a directory
BOOL XFS_MakeDir(const char* path)
{
    return CreateDirectory(path, NULL);
}

// Recursive directory deletion helper
static BOOL DeleteRecursive(const char* path)
{
    WIN32_FIND_DATA findData;
    char searchPath[MAX_PATH];
    char filePath[MAX_PATH];
    
    sprintf_s(searchPath, "%s\\*", path);
    
    HANDLE hFind = FindFirstFile(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return RemoveDirectory(path);
    }
    
    do
    {
        if (strcmp(findData.cFileName, ".") == 0 || 
            strcmp(findData.cFileName, "..") == 0)
        {
            continue;
        }
        
        sprintf_s(filePath, "%s\\%s", path, findData.cFileName);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!DeleteRecursive(filePath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
        else
        {
            if (!DeleteFile(filePath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
    } while (FindNextFile(hFind, &findData));
    
    FindClose(hFind);
    return RemoveDirectory(path);
}

// Delete a file or directory (recursively)
BOOL XFS_Delete(const char* path)
{
    DWORD attributes = GetFileAttributes(path);
    
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return FALSE;
    }
    
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return DeleteRecursive(path);
    }
    else
    {
        return DeleteFile(path);
    }
}

// Move/rename a file or directory
BOOL XFS_Move(const char* src, const char* dest)
{
    return MoveFile(src, dest);
}

// Copy a file
static BOOL CopyFileInternal(const char* src, const char* dest)
{
    return CopyFile(src, dest, FALSE);
}

// Recursive directory copy helper
static BOOL CopyRecursive(const char* src, const char* dest)
{
    WIN32_FIND_DATA findData;
    char srcSearchPath[MAX_PATH];
    char srcPath[MAX_PATH];
    char destPath[MAX_PATH];
    
    if (!XFS_MakeDir(dest))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            return FALSE;
        }
    }
    
    sprintf_s(srcSearchPath, "%s\\*", src);
    
    HANDLE hFind = FindFirstFile(srcSearchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return TRUE;
    }
    
    do
    {
        if (strcmp(findData.cFileName, ".") == 0 || 
            strcmp(findData.cFileName, "..") == 0)
        {
            continue;
        }
        
        sprintf_s(srcPath, "%s\\%s", src, findData.cFileName);
        sprintf_s(destPath, "%s\\%s", dest, findData.cFileName);
        
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!CopyRecursive(srcPath, destPath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
        else
        {
            if (!CopyFileInternal(srcPath, destPath))
            {
                FindClose(hFind);
                return FALSE;
            }
        }
    } while (FindNextFile(hFind, &findData));
    
    FindClose(hFind);
    return TRUE;
}

// Copy a file or directory (recursively)
BOOL XFS_Copy(const char* src, const char* dest)
{
    DWORD attributes = GetFileAttributes(src);
    
    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        return FALSE;
    }
    
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        return CopyRecursive(src, dest);
    }
    else
    {
        return CopyFileInternal(src, dest);
    }
}

// Create an empty file
HANDLE XFS_MakeFile(const char* path)
{
    return CreateFile(
        path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
}

// Write data to a file
BOOL XFS_WriteToFile(const char* path, const void* data, DWORD dataSize, BOOL append)
{
    HANDLE hFile = CreateFile(
        path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        append ? OPEN_ALWAYS : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    
    if (append)
    {
        SetFilePointer(hFile, 0, NULL, FILE_END);
    }
    
    DWORD bytesWritten;
    BOOL result = WriteFile(hFile, data, dataSize, &bytesWritten, NULL);
    
    CloseHandle(hFile);
    return result && (bytesWritten == dataSize);
}

// Read data from a file
BOOL XFS_ReadFile(const char* path, void* buffer, DWORD bufferSize, DWORD* bytesRead)
{
    HANDLE hFile = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }
    
    BOOL result = ::ReadFile(hFile, buffer, bufferSize, bytesRead, NULL);
    CloseHandle(hFile);
    
    return result;
}

// Get file size
DWORD XFS_GetFileSize(const char* path)
{
    HANDLE hFile = CreateFile(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hFile == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    
    DWORD size = ::GetFileSize(hFile, NULL);
    CloseHandle(hFile);
    
    return size;
}

// Check if a file or directory exists
BOOL XFS_Exists(const char* path)
{
    DWORD attributes = GetFileAttributes(path);
    return (attributes != INVALID_FILE_ATTRIBUTES);
}

// Check if path is a directory
BOOL XFS_IsDirectory(const char* path)
{
    DWORD attributes = GetFileAttributes(path);
    return (attributes != INVALID_FILE_ATTRIBUTES) && 
           (attributes & FILE_ATTRIBUTE_DIRECTORY);
}