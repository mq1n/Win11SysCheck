#pragma once
#include <Windows.h>
#include <powerbase.h>
#include <WinInet.h>
#include <tbs.h>
#include <intrin.h>
#include <dxdiag.h>
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define SystemBootEnvironmentInformation 90
#define SystemSecureBootInformation 145
#define SAFE_RELEASE(p)  { if(p) { (p)->Release(); (p)=NULL; } }

typedef struct _SYSTEM_BOOT_ENVIRONMENT_INFORMATION
{
	GUID BootIdentifier;
	FIRMWARE_TYPE FirmwareType;
} SYSTEM_BOOT_ENVIRONMENT_INFORMATION, * PSYSTEM_BOOT_ENVIRONMENT_INFORMATION;
typedef struct _SYSTEM_SECUREBOOT_INFORMATION
{
	BOOLEAN SecureBootEnabled;
	BOOLEAN SecureBootCapable;
} SYSTEM_SECUREBOOT_INFORMATION, * PSYSTEM_SECUREBOOT_INFORMATION;
typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, * PPROCESSOR_POWER_INFORMATION;

struct DISPLAY_DEVICE_INFORMATION
{
	char szDescription[255]{ '\0' };
	char szDriverModel[255]{ '\0' };
};
struct DIRECTX_VERSION_INFORMATION
{
	int nMajorVersion{ 0 };
	int nMinorVersion{ 0 };
};
