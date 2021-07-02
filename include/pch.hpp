#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <d3d9.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#define DIRECTINPUT_VERSION 0x0800
#include <Windows.h>
#include <Windowsx.h>
#include <dinput.h>
#include <powerbase.h>
#include <WinInet.h>
#include <tbs.h>
#include <intrin.h>
#include <dxdiag.h>
#include <comdef.h>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <map>
#include <memory>
#include <algorithm>
#include <functional>
#include <filesystem>
#include <csignal>
#include <thread>
#include <atomic>
#include <ctime>

#include <fmt/format.h>
#include <mini/ini.h>
using namespace std::string_literals;

#include <rapidjson/document.h>
#include <rapidjson/reader.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#include <rapidjson/filereadstream.h>
using namespace rapidjson;

#define CONFIG_FILE_NAME "./config.ini"
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define SystemBootEnvironmentInformation 90
#define SystemSecureBootInformation 145
#define SAFE_RELEASE(p)  { if(p) { (p)->Release(); (p)=NULL; } }
#ifndef PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE    34   
#endif

typedef struct _SYSTEM_BOOT_ENVIRONMENT_INFORMATION
{
	GUID BootIdentifier;
	FIRMWARE_TYPE FirmwareType;
	ULONGLONG BootFlags;
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
