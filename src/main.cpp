#include <Windows.h>
#include <powerbase.h>
#include <WinInet.h>
#include <tbs.h>
#include <intrin.h>
#include <d3d11.h>
#include <string>
#include <iostream>
#include <fstream>

#ifdef _M_IX86
#error "architecture unsupported"
#endif

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define SystemBootEnvironmentInformation 90
#define SystemSecureBootInformation 145

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


std::string CreateTempFileName()
{
	char szTempPath[MAX_PATH]{ 0 };
	GetTempPathA(MAX_PATH, szTempPath);

	char szTempName[MAX_PATH]{ 0 };
	GetTempFileNameA(szTempPath, "wsc", 1, szTempName);

	return szTempName;
}
std::string ReadFileContent(const std::string& stFileName)
{
	std::ifstream in(stFileName.c_str(), std::ios_base::binary);
	if (in)
	{
		in.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
		return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
	}
	return {};
}


int main(int, char* argv[])
{
	std::cout << "Windows 11 minimum requirement checker tool: '" << argv[0] << "' started!" << std::endl;

	const auto hNtdll = LoadLibraryA("ntdll.dll");
	if (!hNtdll)
	{
		std::cerr << "LoadLibraryA(ntdll) failed with error: " << GetLastError() << std::endl;
		std::system("PAUSE");
		return EXIT_FAILURE;
	}

	typedef NTSTATUS(NTAPI* TNtQuerySystemInformation)(UINT SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
	const auto NtQuerySystemInformation = reinterpret_cast<TNtQuerySystemInformation>(GetProcAddress(hNtdll, "NtQuerySystemInformation"));
	if (!NtQuerySystemInformation)
	{
		std::cerr << "GetProcAddress(NtQuerySystemInformation) failed with error: " << GetLastError() << std::endl;
		std::system("PAUSE");
		return EXIT_FAILURE;
	}

	typedef NTSTATUS(NTAPI* TRtlGetVersion)(LPOSVERSIONINFOEX OsVersionInfoEx);
	const auto RtlGetVersion = reinterpret_cast<TRtlGetVersion>(GetProcAddress(hNtdll, "RtlGetVersion"));
	if (!RtlGetVersion)
	{
		std::cerr << "GetProcAddress(RtlGetVersion) failed with error: " << GetLastError() << std::endl;
		std::system("PAUSE");
		return EXIT_FAILURE;
	}

	// S Mode
	{
		std::cout << "S Mode checking..." << std::endl;

		OSVERSIONINFOEX osVerEx{ 0 };
		const auto ntStatus = RtlGetVersion(&osVerEx);
		if (ntStatus != STATUS_SUCCESS)
		{
			std::cerr << "RtlGetVersion failed with status: " << std::hex << ntStatus << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tVersion: " << osVerEx.dwMajorVersion << "." << osVerEx.dwMinorVersion << std::endl;

		DWORD dwProductType = 0;
		if (!GetProductInfo(osVerEx.dwMajorVersion, osVerEx.dwMinorVersion, 0, 0, &dwProductType))
		{
			std::cerr << "GetProductInfo failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tProduct type: " << dwProductType << std::endl;

		if (dwProductType == PRODUCT_CLOUD || dwProductType == PRODUCT_CLOUDN)
		{
			std::cerr << "SMode does not supported!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "S Mode check passed!" << std::endl;
	}

	// 1 Ghz, 64-bit, dual core, known CPU
	{
		std::cout << "CPU checking..." << std::endl;

		const auto dwActiveProcessorCount = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
		if (!dwActiveProcessorCount)
		{
			std::cerr << "Active CPU does not exist in your system, GetActiveProcessorCount failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tActive processor count: " << dwActiveProcessorCount << std::endl;

		const auto hProcHeap = GetProcessHeap();
		if (!hProcHeap)
		{
			std::cerr << "GetProcessHeap failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		SYSTEM_INFO sysInfo{ 0 };
		GetNativeSystemInfo(&sysInfo);

		std::cout << "\tProcessor arch: " << sysInfo.wProcessorArchitecture << std::endl;

		if (sysInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64 &&
			sysInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_IA64 &&
			sysInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_ARM64)
		{
			std::cerr << "System processor arch must be x64!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tProcessor count: " << sysInfo.dwNumberOfProcessors << std::endl;

		if (sysInfo.dwNumberOfProcessors < 2)
		{
			std::cerr << "System processor count: " << sysInfo.dwNumberOfProcessors << " is less than minimum requirement!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		PROCESSOR_POWER_INFORMATION processorPowerInfo{ 0 };
		const ULONG ulBufferSize = sizeof(processorPowerInfo) * sysInfo.dwNumberOfProcessors;
		const auto lpBuffer = reinterpret_cast<PROCESSOR_POWER_INFORMATION*>(HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, ulBufferSize));
		if (!lpBuffer)
		{
			std::cerr << "HeapAlloc(" << ulBufferSize << " bytes) failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto lStatus = CallNtPowerInformation(ProcessorInformation, nullptr, 0, lpBuffer, ulBufferSize);
		if (lStatus != STATUS_SUCCESS)
		{
			HeapFree(hProcHeap, 0, lpBuffer);
			std::cerr << "CallNtPowerInformation failed with status: " << lStatus << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		auto nSpeedCheckCounter = 0;
		for (size_t i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
		{
			const auto lpCurrProcessorInfo = *(lpBuffer + i);
			std::cout << "\tProcessor: " << lpCurrProcessorInfo.Number + 1 << " Speed: " << lpCurrProcessorInfo.CurrentMhz << std::endl;
			if (lpCurrProcessorInfo.CurrentMhz >= 1000)
			{
				nSpeedCheckCounter++;
			}
		}
		HeapFree(hProcHeap, 0, lpBuffer);

		if (nSpeedCheckCounter < 2)
		{
			std::cerr << "System processor speed is less than minimum requirement!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto dwRevision = sysInfo.wProcessorRevision >> 8;
		const auto byRevision = LOBYTE(sysInfo.wProcessorRevision);

		int CPUIDINF[4]{ 0 };
		__cpuid(CPUIDINF, 0);

		std::string stVendor;
		stVendor += std::string(reinterpret_cast<const char*>(&CPUIDINF[1]), sizeof(CPUIDINF[1]));
		stVendor += std::string(reinterpret_cast<const char*>(&CPUIDINF[3]), sizeof(CPUIDINF[3]));
		stVendor += std::string(reinterpret_cast<const char*>(&CPUIDINF[2]), sizeof(CPUIDINF[2]));

		std::cout << "\tProcessor level: " << sysInfo.wProcessorLevel << " revision: " << dwRevision << " " << std::to_string(byRevision) << " vendor: " << stVendor << std::endl;

		// Reversed checks from Windows's tool
		if (stVendor == "AuthenticAMD")
		{
			if (sysInfo.wProcessorLevel < 0x17 || sysInfo.wProcessorLevel == 23 && !((dwRevision - 1) & 0xFFFFFFEF))
			{
				std::cerr << "Unsupported AMD CPU detected!" << std::endl;
				std::system("PAUSE");
				return EXIT_FAILURE;
			}
		}
		else if (stVendor == "GenuineIntel")
		{
			if (sysInfo.wProcessorLevel == 6)
			{
				if ((dwRevision < 0x5F && dwRevision != 85) ||
					(dwRevision == 142 && byRevision == 9) ||
					(dwRevision == 158 && byRevision == 9))
				{
					std::cerr << "Unsupported Intel CPU detected!" << std::endl;
					std::system("PAUSE");
					return EXIT_FAILURE;
				}
			}
		}
		else if (stVendor.find("Qualcomm") == std::string::npos)
		{
			std::cerr << "Unknown CPU vendor detected!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}
		else if (!IsProcessorFeaturePresent(PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE))
		{
			std::cerr << "Unsupported Qualcomm CPU detected!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "CPU check passed!" << std::endl;
	}

	// 4 GB Ram
	{
		std::cout << "RAM checking..." << std::endl;

		ULONGLONG ullTotalyMemory = 0;
		if (!GetPhysicallyInstalledSystemMemory(&ullTotalyMemory))
		{
			std::cerr << "GetPhysicallyInstalledSystemMemory failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tRAM capacity: " << ullTotalyMemory << std::endl;

		if (ullTotalyMemory <= 4096000)
		{
			std::cerr << "RAM capacity is less than minimum requirement!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "RAM check passed!" << std::endl;
	}

	// 64 GB Free disk space
	{
		std::cout << "Disk checking..." << std::endl;

		char szWinDir[MAX_PATH]{ 0 };
		const auto nWinDirLength = GetSystemWindowsDirectoryA(szWinDir, sizeof(szWinDir));
		if (!nWinDirLength)
		{
			std::cerr << "GetSystemWindowsDirectoryA failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto c_stWinDir = std::string(szWinDir, nWinDirLength);
		const auto c_stTargetDev = c_stWinDir.substr(0, 3);
		std::cout << "\tWindows directory: " << c_stWinDir << " Target device: " << c_stTargetDev << std::endl;

		ULARGE_INTEGER uiTotalNumberOfBytes{ 0 };
		if (!GetDiskFreeSpaceExA(c_stTargetDev.c_str(), nullptr, &uiTotalNumberOfBytes, nullptr))
		{
			std::cerr << "GetDiskFreeSpaceExA failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto c_dwFreeSpaceAsGB = uiTotalNumberOfBytes.QuadPart / 1024 / 1024 / 1024;

		if (c_dwFreeSpaceAsGB < 64)
		{
			std::cerr << "Disk capacity is less than minimum requirement!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "Disk check passed!" << std::endl;
	}

	// 1366x768 resolution
	{
		std::cout << "Resolution checking..." << std::endl;

		// Enum monitors
		const auto hDC = GetDC(nullptr);
		if (!hDC)
		{
			std::cerr << "GetDC failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		BOOL bHasAvailableMonitor = FALSE;
		auto OnMonitorEnum = [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
			static auto s_nIdx = 0;
			const auto pbHasAvailableMonitor = reinterpret_cast<BOOL*>(dwData);

			s_nIdx++;

			MONITORINFO monInfo{ 0 };
			monInfo.cbSize = sizeof(monInfo);

			if (!GetMonitorInfoA(hMonitor, &monInfo))
			{
				std::cerr << "GetMonitorInfoA(" << s_nIdx << ") failed with error : " << GetLastError() << std::endl;
				return TRUE;
			}

			const auto nColorDepth = GetDeviceCaps(hdcMonitor, BITSPIXEL) * GetDeviceCaps(hdcMonitor, PLANES);

			std::cout <<
				"\tMonitor: " << s_nIdx <<
				" Primary: " << bool(monInfo.dwFlags & MONITORINFOF_PRIMARY) <<
				" Color depth: " << nColorDepth <<
				" Resolution: " << monInfo.rcMonitor.right << "x" << monInfo.rcMonitor.bottom <<
				std::endl;

			if (monInfo.rcMonitor.right > 1366 && monInfo.rcMonitor.bottom > 768 && nColorDepth >= 8)
			{
				*pbHasAvailableMonitor = TRUE;
				return TRUE;
			}

			return TRUE;
		};

		if (!EnumDisplayMonitors(hDC, nullptr, OnMonitorEnum, reinterpret_cast<LPARAM>(&bHasAvailableMonitor)))
		{
			std::cerr << "EnumDisplayMonitors failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		if (!bHasAvailableMonitor)
		{
			std::cerr << "Any available display device could not detected!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		ReleaseDC(nullptr, hDC);
		std::cout << "Resolution check passed!" << std::endl;
	}

	// UEFI
	{
		std::cout << "Firmware checking..." << std::endl;

		ULONG cbSize = 0;
		SYSTEM_BOOT_ENVIRONMENT_INFORMATION sbei{ 0 };
		const auto ntStatus = NtQuerySystemInformation(SystemBootEnvironmentInformation, &sbei, sizeof(sbei), &cbSize);
		if (ntStatus != STATUS_SUCCESS)
		{
			std::cerr << "NtQuerySystemInformation(SystemBootEnvironmentInformation) failed with status: " << std::hex << ntStatus << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tFirmware type: " << sbei.FirmwareType << std::endl;

		if (sbei.FirmwareType != FirmwareTypeUefi)
		{
			std::cerr << "Boot firmware: " << sbei.FirmwareType << " is not allowed!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "Firmware check passed!" << std::endl;
	}

	// Secure boot
	{
		std::cout << "Secure boot checking..." << std::endl;

		ULONG cbSize = 0;
		SYSTEM_SECUREBOOT_INFORMATION ssbi{ 0 };
		const auto ntStatus = NtQuerySystemInformation(SystemSecureBootInformation, &ssbi, sizeof(ssbi), &cbSize);
		if (ntStatus != STATUS_SUCCESS)
		{
			std::cerr << "NtQuerySystemInformation(SystemSecureBootInformation) failed with status: " << std::hex << ntStatus << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "\tSecure boot capable: " << std::to_string(ssbi.SecureBootCapable) << " Enabled: " << std::to_string(ssbi.SecureBootEnabled) << std::endl;

		if (!ssbi.SecureBootCapable)
		{
			std::cerr << "Secure boot must be capable!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "Secure boot check passed!" << std::endl;
	}

	// TPM 2.0 
	{
		std::cout << "TPM checking..." << std::endl;

		TPM_DEVICE_INFO tpmDevInfo{ 0 };
		const auto nTpmRet = Tbsi_GetDeviceInfo(sizeof(tpmDevInfo), &tpmDevInfo);
		if (nTpmRet != TBS_SUCCESS)
		{
			std::cerr << "Tbsi_GetDeviceInfo failed with status: " << nTpmRet << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		if (tpmDevInfo.tpmVersion != 2)
		{
			std::cerr << "TPM version: " << tpmDevInfo.tpmVersion << " is less than minimum requirement!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "TPM check passed!" << std::endl;
	}

	// DirectX 12 
	{
		std::cout << "DirectX checking..." << std::endl;

		typedef HRESULT(WINAPI* TD3D11CreateDevice)(
			IDXGIAdapter* adapter, D3D_DRIVER_TYPE driver_type, HMODULE software, UINT flags, const D3D_FEATURE_LEVEL* feature_levels,
			UINT num_feature_levels, UINT sdk_version, ID3D11Device** device, D3D_FEATURE_LEVEL* feature_level,
			ID3D11DeviceContext** immediate_context
		);

		const auto hD3D11 = LoadLibraryA("d3d11.dll");
		if (!hD3D11)
		{
			std::cerr << "LoadLibraryA(d3d11) failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto D3D11CreateDevice = reinterpret_cast<TD3D11CreateDevice>(GetProcAddress(hD3D11, "D3D11CreateDevice"));
		if (!D3D11CreateDevice)
		{
			std::cerr << "GetProcAddress(D3D11CreateDevice) failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		static const D3D_FEATURE_LEVEL d3d_feature_levels[] = {
			D3D_FEATURE_LEVEL_12_1,
			D3D_FEATURE_LEVEL_12_0,
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0
		};

		D3D_FEATURE_LEVEL d3dFeatureLevel{};
		ID3D11Device* pD3d11Device = nullptr;
		ID3D11DeviceContext* pD3d11DeviceCtx = nullptr;
		const auto hResult = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, d3d_feature_levels, _countof(d3d_feature_levels),
			D3D11_SDK_VERSION, &pD3d11Device, &d3dFeatureLevel, &pD3d11DeviceCtx
		);
		if (FAILED(hResult))
		{
			std::cerr << "D3D11CreateDevice failed with status: " << std::hex << hResult << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		if (d3dFeatureLevel < D3D_FEATURE_LEVEL_12_0)
		{
			std::cerr << "DirectX 12 compability check failed! Feature level: " << std::to_string(d3dFeatureLevel) << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "DirectX check passed!" << std::endl;
	}

	// WDDM 2.X 
	{
		std::cout << "WDDM checking..." << std::endl;

		const auto c_stTempFileName = CreateTempFileName();
		const auto c_stCommand = "dxdiag /t " + c_stTempFileName;

		std::cout << "\tDxdiag output filename: " << c_stTempFileName << std::endl;

		const auto nDxdiagExitCode = std::system(c_stCommand.c_str());

		if (nDxdiagExitCode)
		{
			std::cerr << "dxdiag process execute failed with exit code: " << nDxdiagExitCode << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto c_stFileBuffer = ReadFileContent(c_stTempFileName);
		if (c_stFileBuffer.empty())
		{
			std::cerr << "dxdiag output file read failed with " << errno << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		if (c_stFileBuffer.find("Driver Model: WDDM 2") == std::string::npos)
		{
			std::cerr << "WDDM 2 is not found in dxdiag config!" << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "WDDM check passed!" << std::endl;
	}

	// Internet connection
	{
		std::cout << "Internet connection checking..." << std::endl;

		DWORD dwFlags = 0;
		if (!InternetGetConnectedState(&dwFlags, 0))
		{
			std::cerr << "InternetGetConnectedState failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		const auto dwTestConnectionRet = InternetAttemptConnect(0);
		if (dwTestConnectionRet != ERROR_SUCCESS)
		{
			std::cerr << "InternetAttemptConnect failed with error: " << GetLastError() << std::endl;
			std::system("PAUSE");
			return EXIT_FAILURE;
		}

		std::cout << "Internet connection check passed!" << std::endl;
	}

	std::cout << "All checks passed! Your system can be upgradable to Windows 11" << std::endl;
	std::system("PAUSE");
	return EXIT_SUCCESS;
}
