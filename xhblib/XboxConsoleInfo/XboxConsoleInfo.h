#ifndef XBOXCONSOLEINFO_H
#define XBOXCONSOLEINFO_H

// Console types enumeration
typedef enum {
    REV_XENON = 0,
    REV_ZEPHYR,
    REV_FALCON,
    REV_JASPER,
    REV_TRINITY,
    REV_CORONA,
    REV_CORONA_PHISON,
    REV_WINCHESTER,
    REV_UNKNOWN
} ConsoleType;

// CPU key buffer (16 bytes)
extern unsigned char keybuf[0x10];

// Fuselines
extern unsigned long long fuseline0;
extern unsigned long long fuseline1;
extern unsigned long long fuseline2;
extern unsigned long long fuseline3;
extern unsigned long long fuseline4;
extern unsigned long long fuseline5;
extern unsigned long long fuseline6;
extern unsigned long long fuseline7;
extern unsigned long long fuseline8;
extern unsigned long long fuseline9;
extern unsigned long long fuseline10;
extern unsigned long long fuseline11;


// Function declarations
int XCI_GetConsoleType();
bool XCI_GetCPUKey();
bool XCI_GetFuses();

#endif // XBOXCONSOLEINFO_H