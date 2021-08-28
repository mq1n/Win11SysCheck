#include "../include/pch.hpp"
#include "../include/sys_check.hpp"
#include "../include/log_helper.hpp"
#include "../include/application.hpp"
#include "../include/main_ui.hpp"
#include "../include/simple_timer.hpp"

namespace Win11SysCheck
{
	CSysCheck::CSysCheck() :
		m_hNtdll(nullptr), m_fnNtQuerySystemInformation(nullptr), m_fnRtlGetVersion(nullptr)
	{
		for (size_t i = 0; i < static_cast<uint8_t>(EMenuType::MENU_TYPE_MAX); ++i)
		{
			m_mapStatuses[static_cast<EMenuType>(i)] = EStatus::STATUS_UNKNOWN;
		}
	}
	CSysCheck::~CSysCheck()
	{
	}

	bool CSysCheck::Initialize()
	{
		m_hNtdll = LoadLibraryA("ntdll.dll");
		if (!m_hNtdll)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("LoadLibraryA(ntdll) failed with error: {0}", GetLastError()));
			return false;
		}

		m_fnNtQuerySystemInformation = reinterpret_cast<TNtQuerySystemInformation>(GetProcAddress(m_hNtdll, "NtQuerySystemInformation"));
		if (!m_fnNtQuerySystemInformation)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("GetProcAddress(NtQuerySystemInformation) failed with error: {0}", GetLastError()));
			return false;
		}

		m_fnRtlGetVersion = reinterpret_cast<TRtlGetVersion>(GetProcAddress(m_hNtdll, "RtlGetVersion"));
		if (!m_fnRtlGetVersion)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("GetProcAddress(RtlGetVersion) failed with error: {0}", GetLastError()));
			return false;
		}

		return LoadSystemInformations();
	}
	void CSysCheck::Destroy()
	{
		FreeLibrary(m_hNtdll);
	}

	bool CSysCheck::ExportResult(std::string& stFileName)
	{
		time_t curTime = { 0 };
		std::time(&curTime);

		stFileName = fmt::format("result_{0}.json", static_cast<DWORD>(curTime));

		if (std::filesystem::exists(stFileName))
			std::filesystem::remove(stFileName);

		std::ofstream ofs(stFileName);
		if (!ofs)
		{
			CLogHelper::Instance().Log(LL_ERR, "Output file create failed!");
			return false;
		}

		StringBuffer s;
		PrettyWriter <StringBuffer> writer(s);

		writer.StartObject();

		for (const auto& [type, details] : m_mapSystemDetails)
		{
			const auto c_stLocalizedName = CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(type);

			writer.Key(c_stLocalizedName.c_str());
			writer.StartObject();

			const auto stTitle = details.stTitle.substr(5, details.stTitle.size()); // Remove icon characters from title
			writer.Key(stTitle.c_str());
			writer.StartArray();
			for (const auto& detail : details.vecTexts)
			{
				writer.String(detail.c_str());
			}
			writer.EndArray();

			writer.EndObject();
		}

		writer.EndObject();

		ofs << std::setw(4) << s.GetString() << std::endl;
		ofs.close();
		return true;
	}

	EStatus CSysCheck::GetMenuStatus(EMenuType nMenuType)
	{
		const auto it = m_mapStatuses.find(nMenuType);
		if (it == m_mapStatuses.end())
			return EStatus::STATUS_UNKNOWN;
		return it->second;
	}
	void CSysCheck::SetMenuStatus(EMenuType nMenuType, EStatus nStatus)
	{
		auto& it = m_mapStatuses.find(nMenuType);
		if (it == m_mapStatuses.end())
			return;
		it->second = nStatus;
	}

	std::string CSysCheck::__GetFirmwareName(const uint8_t type)
	{
		switch (type)
		{
		case FirmwareTypeBios:
			return "Legacy/Bios";
		case FirmwareTypeUefi:
			return "UEFI";
		default:
			return "Unknown: " + std::to_string(type);
		}
	}

	std::string CSysCheck::__GetPartitionName(const uint8_t type)
	{
		switch (type)
		{
		case PARTITION_STYLE_MBR:
			return "MBR";
		case PARTITION_STYLE_GPT:
			return "GPT";
		case PARTITION_STYLE_RAW:
			return "RAW";
		default:
			return "Unknown";
		}
	}

	std::string CSysCheck::__GetVolumePath(PCHAR szVolumeName)
	{
		std::string stName;
		BOOL Success = FALSE;
		PCHAR Names = NULL;
		DWORD CharCount = MAX_PATH + 1;

		for (;;)
		{
			Names = (PCHAR)new BYTE[CharCount * sizeof(CHAR)];
			if (!Names)
				return stName;

			Success = GetVolumePathNamesForVolumeNameA(szVolumeName, Names, CharCount, &CharCount);
			if (Success)
				break;

			if (GetLastError() != ERROR_MORE_DATA)
				break;

			delete[] Names;
			Names = NULL;
		}

		stName = std::string(Names);

		if (Names != NULL)
		{
			delete[] Names;
			Names = NULL;
		}

		return stName;
	}

	HRESULT CSysCheck::__GetStringValue(IDxDiagContainer* pContainerObject, wchar_t* lpwszName, char* lpszValue, int nValueLength)
	{
		VARIANT var{};
		VariantInit(&var);

		HRESULT hr{};
		if (FAILED(hr = pContainerObject->GetPropA(lpwszName, &var)))
			return hr;

		if (var.vt != VT_BSTR)
			return E_INVALIDARG;

		size_t cbConvSize = 0;
		wcstombs_s(&cbConvSize, lpszValue, nValueLength, var.bstrVal, SysStringLen(var.bstrVal));
		lpszValue[nValueLength - 1] = '\0';

		VariantClear(&var);
		return S_OK;
	}
	HRESULT CSysCheck::__GetIntValue(IDxDiagContainer* pContainerObject, wchar_t* lpwszName, int* pnValue)
	{
		VARIANT var{};
		VariantInit(&var);

		HRESULT hr{};
		if (FAILED(hr = pContainerObject->GetPropA(lpwszName, &var)))
			return hr;

		if (var.vt != VT_UI4)
			return E_INVALIDARG;

		*pnValue = var.ulVal;

		VariantClear(&var);
		return S_OK;
	}

	// Source: https://stackoverflow.com/a/579448
	bool CSysCheck::__GetMonitorSizeFromEDID(const HKEY hEDIDRegKey, short& WidthMm, short& HeightMm)
	{
		BYTE EDID[1024];
		DWORD edidsize = sizeof(EDID);

		const auto lStatus = RegQueryValueExA(hEDIDRegKey, "EDID", nullptr, nullptr, EDID, &edidsize);
		if (lStatus != ERROR_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("RegQueryValueExA(EDID) failed with status: {0}", lStatus));
			return false;
		}

		DWORD p = 8;
		char model2[9]{ '\0' };

		char byte1 = EDID[p];
		char byte2 = EDID[p + 1];
		model2[0] = ((byte1 & 0x7C) >> 2) + 64;
		model2[1] = ((byte1 & 3) << 3) + ((byte2 & 0xE0) >> 5) + 64;
		model2[2] = (byte2 & 0x1F) + 64;
		sprintf_s(model2 + 3, sizeof(model2) - 3, "%X%X%X%X", (EDID[p + 3] & 0xf0) >> 4, EDID[p + 3] & 0xf, (EDID[p + 2] & 0xf0) >> 4, EDID[p + 2] & 0x0f);

		WidthMm = EDID[22];
		HeightMm = EDID[21];
		return true;
	}

	void CSysCheck::AddMonitorInformation(const std::string& stInfo)
	{
		m_vMonitorInfos.emplace_back(stInfo);
	}

	bool CSysCheck::__LoadOSInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "OS Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_OS;
		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;

		OSVERSIONINFOEX osVerEx{ 0 };
		const auto ntStatus = m_fnRtlGetVersion(&osVerEx);
		if (ntStatus != STATUS_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_SYS, fmt::format("RtlGetVersion failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(ntStatus))));
			return bRet;
		}
		if (osVerEx.dwBuildNumber >= 21996)
			osVerEx.dwMajorVersion = 11;

		DWORD dwProductType = 0;
		if (!GetProductInfo(osVerEx.dwMajorVersion, osVerEx.dwMinorVersion, 0, 0, &dwProductType))
		{
			CLogHelper::Instance().Log(LL_SYS, fmt::format("GetProductInfo failed with error: {0}", GetLastError()));
			return bRet;
		}

		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}.{2}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_OS_VERSION),
			osVerEx.dwMajorVersion, osVerEx.dwMinorVersion
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}.{2}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_OS_SP_VERSION),
			osVerEx.wServicePackMajor, osVerEx.wServicePackMinor
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_OS_BUILD_VERSION),
			osVerEx.dwBuildNumber
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_OS_PLATFORM_ID),
			osVerEx.dwPlatformId
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_OS_PRODUCT_TYPE),
			dwProductType
		));

		m_mapSystemDetails[nType] = sysDetails;

		if (dwProductType != PRODUCT_CLOUD && dwProductType != PRODUCT_CLOUDN)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("OS informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}
	bool CSysCheck::__LoadBootInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "Boot Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_BOOT;

		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;

		ULONG cbSize = 0;
		SYSTEM_BOOT_ENVIRONMENT_INFORMATION sbei{ 0 };
		auto ntStatus = m_fnNtQuerySystemInformation(SystemBootEnvironmentInformation, &sbei, sizeof(sbei), &cbSize);
		if (ntStatus != STATUS_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_SYS, fmt::format("NtQuerySystemInformation(SystemBootEnvironmentInformation) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(ntStatus))));
			return bRet;
		}

		SYSTEM_SECUREBOOT_INFORMATION ssbi{ 0 };
		ntStatus = m_fnNtQuerySystemInformation(SystemSecureBootInformation, &ssbi, sizeof(ssbi), &cbSize);
		if (ntStatus != STATUS_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_SYS, fmt::format("NtQuerySystemInformation(SystemSecureBootInformation) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(ntStatus))));
			return bRet;
		}

		auto bTpmEnabled = true;
		TPM_DEVICE_INFO tpmDevInfo{ 0 };
		const auto nTpmRet = Tbsi_GetDeviceInfo(sizeof(tpmDevInfo), &tpmDevInfo);
		if (nTpmRet == TBS_E_TPM_NOT_FOUND)
		{
			bTpmEnabled = false;
		}
		else if (nTpmRet != TBS_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_SYS, fmt::format("Tbsi_GetDeviceInfo failed with status: {0}", nTpmRet));
			return bRet;
		}

		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		sysDetails.vecTexts.emplace_back(fmt::format("{0}:\n\t\tFirmware: {1}\n\t\tFlags: {2}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_BOOT_ENVIRONMENT),
			__GetFirmwareName(sbei.FirmwareType),
			sbei.BootFlags
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}:\n\t\t{1}: {2}\n\t\t{3}: {4}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_BOOT_SECURE_BOOT),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_CAPABLE),
			ssbi.SecureBootCapable ? true : false,
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_ENABLED),
			ssbi.SecureBootEnabled ? true : false
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}:\n\t\t{1}: {2}\n\t\t{3}: {4}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_BOOT_TPM),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_STATUS),
			bTpmEnabled ?
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_ENABLED) :
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DISABLED),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_VERSION),
			tpmDevInfo.tpmVersion
		));

		m_mapSystemDetails[nType] = sysDetails;

		if (sbei.FirmwareType == FirmwareTypeUefi && ssbi.SecureBootCapable && bTpmEnabled && tpmDevInfo.tpmVersion == 2)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("Boot informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}
	bool CSysCheck::__LoadCPUInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "CPU Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_CPU;

		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;

		const auto dwActiveProcessorCount = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
		if (!dwActiveProcessorCount)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("Active CPU does not exist in your system, GetActiveProcessorCount failed with error: {0}", GetLastError()));
			return bRet;
		}

		SYSTEM_INFO sysInfo{ 0 };
		GetNativeSystemInfo(&sysInfo);

		const auto nProcessorModel = sysInfo.wProcessorRevision >> 8;
		const auto byProcessorStepping = LOBYTE(sysInfo.wProcessorRevision);

		int arCPUID[4]{ 0 };
		__cpuid(arCPUID, 0);

		std::string stVendor{};
		stVendor += std::string(reinterpret_cast<const char*>(&arCPUID[1]), sizeof(arCPUID[1]));
		stVendor += std::string(reinterpret_cast<const char*>(&arCPUID[3]), sizeof(arCPUID[3]));
		stVendor += std::string(reinterpret_cast<const char*>(&arCPUID[2]), sizeof(arCPUID[2]));

		std::string stProcessorName{};
		DWORD dwPlatformSpecField = 0;
		HKEY hKey{ 0 };
		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			DWORD dwType = REG_SZ;
			char szBuffer[512]{ '\0' };
			DWORD cbSize = 512;
			if (RegQueryValueExA(hKey, "ProcessorNameString", nullptr, &dwType, (PBYTE)(&szBuffer), &cbSize) == ERROR_SUCCESS)
			{
				stProcessorName = szBuffer;
			}

			dwType = REG_DWORD;
			cbSize = sizeof(dwPlatformSpecField);
			const auto lStatus = RegQueryValueExA(hKey, "Platform Specific Field 1", nullptr, &dwType, (PBYTE)(&dwPlatformSpecField), &cbSize);
			if (lStatus != ERROR_SUCCESS)
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("RegQueryValueExA(Platform Specific Field 1) failed with status: {0}", lStatus));
				return bRet;
			}

			RegCloseKey(hKey);
		}

		const auto hProcHeap = GetProcessHeap();
		if (!hProcHeap)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("GetProcessHeap failed with error: {0}", GetLastError()));
			return bRet;
		}

		PROCESSOR_POWER_INFORMATION processorPowerInfo{ 0 };
		const auto ulBufferSize = static_cast<ULONG>(sizeof(processorPowerInfo) * sysInfo.dwNumberOfProcessors);
		const auto lpBuffer = reinterpret_cast<PROCESSOR_POWER_INFORMATION*>(HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, ulBufferSize));
		if (!lpBuffer)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("HeapAlloc({0} bytes) failed with error: {1}", ulBufferSize, GetLastError()));
			return bRet;
		}

		auto lStatus = CallNtPowerInformation(ProcessorInformation, nullptr, 0, lpBuffer, ulBufferSize);
		if (lStatus != STATUS_SUCCESS)
		{
			HeapFree(hProcHeap, 0, lpBuffer);
			CLogHelper::Instance().Log(LL_ERR, fmt::format("CallNtPowerInformation failed with error: {0}", GetLastError()));
			return bRet;
		}

		auto nSpeedCheckCounter = 0;
		for (size_t i = 0; i < sysInfo.dwNumberOfProcessors; ++i)
		{
			const auto lpCurrProcessorInfo = *(reinterpret_cast<PROCESSOR_POWER_INFORMATION*>(reinterpret_cast<LPBYTE>(lpBuffer) + (sizeof(processorPowerInfo) * i)));
			if (lpCurrProcessorInfo.MaxMhz >= 1000)
			{
				nSpeedCheckCounter++;
			}
		}
		HeapFree(hProcHeap, 0, lpBuffer);

		const auto bIsX64 =
			sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
			sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ||
			sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64;

		auto bIsSupportedCPU = true;
		auto qwAtomicSuppVal = 0ull;
		DWORD qwAtomicSuppValSize = sizeof(qwAtomicSuppVal);

		// Reversed checks from Windows's tool
		if (stVendor == "AuthenticAMD")
		{
			if (sysInfo.wProcessorLevel < 0x17 || (sysInfo.wProcessorLevel == 23 && !((nProcessorModel - 1) & 0xFFFFFFEF)))
			{
				CLogHelper::Instance().Log(LL_ERR, "Unsupported AMD CPU detected!");
				bIsSupportedCPU = false;
			}
		}
		else if (stVendor == "GenuineIntel")
		{
			if (sysInfo.wProcessorLevel == 6)
			{
				if (nProcessorModel > 0x5F)
				{
					if (nProcessorModel == 142)
					{
						if (byProcessorStepping != 9)
						{
							CLogHelper::Instance().Log(LL_ERR, "Unsupported Intel CPU detected!");
							bIsSupportedCPU = false;
						}
						if (dwPlatformSpecField != 16)
						{
							CLogHelper::Instance().Log(LL_ERR, "Unsupported Intel CPU detected!");
							bIsSupportedCPU = false;
						}
					}
					if (nProcessorModel != 158 || byProcessorStepping != 9 || dwPlatformSpecField == 8)
					{
						CLogHelper::Instance().Log(LL_ERR, "Unsupported Intel CPU detected!");
						bIsSupportedCPU = false;
					}
				}
				if (nProcessorModel != 85)
				{
					CLogHelper::Instance().Log(LL_ERR, "Unsupported Intel CPU detected!");
					bIsSupportedCPU = false;
				}
			}
		}
		else if (stVendor.find("Qualcomm") == std::string::npos)
		{
			CLogHelper::Instance().Log(LL_ERR, "Unknown CPU vendor detected!");
			bIsSupportedCPU = false;
		}
		else if (!IsProcessorFeaturePresent(PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE))
		{
			CLogHelper::Instance().Log(LL_ERR, "Unsupported Qualcomm CPU detected!");
			bIsSupportedCPU = false;
		}
		else if (lStatus = RegGetValueA(HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0", "CP 4030", RRF_RT_REG_QWORD, 0, &qwAtomicSuppVal, &qwAtomicSuppValSize) != ERROR_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("Atomic support registry read failed with status: {0}", lStatus));
			bIsSupportedCPU = false;
		}
		else if ((unsigned int)(qwAtomicSuppVal >> 20) & 0xF < 2)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("Qualcomm CPU family check failed, atomicsResult: {0}", (unsigned int)(qwAtomicSuppVal >> 20) & 0xF));
			bIsSupportedCPU = false;
		}

		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		sysDetails.vecTexts.emplace_back(fmt::format("{0}:\n\t\t{1}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_CPU_NAME),
			stProcessorName
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}:\n\t\tModel: {1}\n\t\tStepping: {2}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_CPU_DETAILS),
			nProcessorModel, byProcessorStepping
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}:\n\t\tID: {1}\n\t\tx64: {2}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_CPU_ARCH),
			sysInfo.wProcessorArchitecture, bIsX64
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_CPU_ACTIVE_PROCESSOR_COUNT),
			dwActiveProcessorCount
		));
		sysDetails.vecTexts.emplace_back(fmt::format("{0}: {1}",
			CApplication::Instance().GetI18N()->GetSystemDetailsLocalizedText(ESystemDetailID::SYSTEM_DETAIL_CPU_PROCESSOR_COUNT),
			sysInfo.dwNumberOfProcessors
		));

		m_mapSystemDetails[nType] = sysDetails;

		if (bIsX64 && sysInfo.dwNumberOfProcessors >= 2 && nSpeedCheckCounter >= 2 && bIsSupportedCPU)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("CPU informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}
	bool CSysCheck::__LoadRAMInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "RAM Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_RAM;

		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;

		MEMORYSTATUSEX memInfo{ 0 };
		memInfo.dwLength = sizeof(memInfo);
		if (!GlobalMemoryStatusEx(&memInfo))
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("GlobalMemoryStatusEx failed with error: {0}", GetLastError()));
			return bRet;
		}

		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		sysDetails.vecTexts.emplace_back(fmt::format("Physical memory:\n\t\tTotal:{0} MB\n\t\tAvailable: {1} MB",
			memInfo.ullTotalPhys / 1024,
			memInfo.ullAvailPhys / 1024
		));

		m_mapSystemDetails[nType] = sysDetails;

		if (memInfo.ullTotalPhys / 1024 >= 4096000)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("RAM informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}
	bool CSysCheck::__LoadDiskInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "Disk Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_DISK;

		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;

		std::vector <SDiskContext> vDiskList;

		char szVolumeName[MAX_PATH]{ '\0' };
		const auto FindHandle = FindFirstVolumeA(szVolumeName, ARRAYSIZE(szVolumeName));
		if (!FindHandle || FindHandle == INVALID_HANDLE_VALUE)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("FindFirstVolumeA failed with error: {0}", GetLastError()));
			return bRet;
		}

		do
		{
			auto nIndex = strlen(szVolumeName) - 1;

			if (szVolumeName[0] != '\\' || szVolumeName[1] != '\\' || szVolumeName[2] != '?' ||
				szVolumeName[3] != '\\' || szVolumeName[nIndex] != '\\')
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("FindFirstVolume/FindNextVolume returned a bad path: {0}", szVolumeName));
				break;
			}

			szVolumeName[nIndex] = '\0';

			char szDeviceName[MAX_PATH]{ '\0' };
			const auto CharCount = QueryDosDeviceA(&szVolumeName[4], szDeviceName, ARRAYSIZE(szDeviceName));
			if (CharCount == 0)
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("QueryDosDeviceA failed with error: {0}", GetLastError()));
				break;
			}

			szVolumeName[nIndex] = '\\';

			CLogHelper::Instance().Log(LL_SYS, fmt::format("Found a device: {0} Volume name: {1}", szDeviceName, szVolumeName));

			const auto stPath = __GetVolumePath(szVolumeName);
			if (!stPath.empty() && stPath.size() < 5) // Ignore 5 < volumes(unaccesible windows sandbox paths)
			{
				char szFS[MAX_PATH]{ '\0' };
				if (!GetVolumeInformationA(stPath.c_str(), NULL, 0, NULL, NULL, NULL, szFS, sizeof(szFS)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("GetVolumeInformationA failed with error: {0}", GetLastError()));
				}

				SDiskContext diskCtx{};
				diskCtx.stPath = stPath;
				diskCtx.stDeviceName = szDeviceName;
				diskCtx.stVolumeName = szVolumeName;
				diskCtx.stFileSystem = szFS;
				vDiskList.emplace_back(diskCtx);
			}
		} while (FindNextVolumeA(FindHandle, szVolumeName, ARRAYSIZE(szVolumeName)));

		FindVolumeClose(FindHandle);


		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		auto bHaveAvailableDiskSpace = false;
		uint8_t idx = 0;
		for (auto& diskCtx : vDiskList)
		{
			idx++;

			diskCtx.stVolumeName.pop_back();

			auto nPartitionType = -1;
			auto hDevice = CreateFileA(diskCtx.stVolumeName.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
			if (hDevice && hDevice != INVALID_HANDLE_VALUE)
			{
				const auto cbNewLayout = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + sizeof(PARTITION_INFORMATION_EX) * 4 * 25;
				auto partInfo = reinterpret_cast<DRIVE_LAYOUT_INFORMATION_EX*>(new BYTE[cbNewLayout]);
				if (partInfo)
				{
					DWORD dwRetLength = 0;
					if (!DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, nullptr, 0, partInfo, cbNewLayout, &dwRetLength, nullptr))
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("'{0}' DeviceIoControl failed with error: {1}", diskCtx.stPath.c_str(), GetLastError()));
					}
					else
					{
						if (partInfo->PartitionCount)
							nPartitionType = partInfo->PartitionEntry[0].PartitionStyle;
					}

					delete partInfo;
					partInfo = nullptr;
				}
				CloseHandle(hDevice);
			}

			ULARGE_INTEGER uiTotalNumberOfBytes{ 0 };
			if (!GetDiskFreeSpaceExA(diskCtx.stPath.c_str(), nullptr, &uiTotalNumberOfBytes, nullptr))
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("'{0}' GetDiskFreeSpaceExA failed with error: {1}", diskCtx.stPath.c_str(), GetLastError()));
			}
			else
			{
				const auto c_dwFreeSpaceAsMB = uiTotalNumberOfBytes.QuadPart / 1024 / 1024;
				if (c_dwFreeSpaceAsMB > 64000)
				{
					bHaveAvailableDiskSpace = true;
				}
			}

			sysDetails.vecTexts.emplace_back(fmt::format("Disk: {0}\n\t\tPath: {1}\n\t\tDevice Name: {2}\n\t\tVolume Name: {3}\n\t\tFile System: {4}\n\t\tPartition: {5}\n\t\tDisk space: {6} MB",
				idx,
				diskCtx.stPath,
				diskCtx.stDeviceName,
				diskCtx.stVolumeName,
				diskCtx.stFileSystem,
				__GetPartitionName(nPartitionType),
				uiTotalNumberOfBytes.QuadPart / 1024 / 1024
			));
		}
		m_mapSystemDetails[nType] = sysDetails;

		if (bHaveAvailableDiskSpace)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("Disk informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}
	bool CSysCheck::__LoadDisplayInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "Display Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_DISPLAY;

		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;


		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		// Enum monitors
		BOOL bHasAvailableMonitor = FALSE;
		{
			const auto hDC = GetDC(nullptr);
			if (!hDC)
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("GetDC failed with error: {0}", GetLastError()));
				return bRet;
			}

			auto OnMonitorEnum = [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
				static auto s_nIdx = 0;
				const auto c_pbHasAvailableMonitor = reinterpret_cast<BOOL*>(dwData);

				s_nIdx++;

				MONITORINFOEX monInfo{ 0 };
				monInfo.cbSize = sizeof(monInfo);

				if (!GetMonitorInfoA(hMonitor, &monInfo))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("GetMonitorInfoA failed with error: {0}", GetLastError()));
					return TRUE;
				}

				DISPLAY_DEVICE displayDevice{ 0 };
				displayDevice.cb = sizeof(displayDevice);

				if (!EnumDisplayDevicesA(monInfo.szDevice, 0, &displayDevice, 0))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("EnumDisplayDevicesA failed with error: {0}", GetLastError()));
					return TRUE;
				}

				const auto c_nBPC = GetDeviceCaps(hdcMonitor, BITSPIXEL);
				const auto c_stIsPrimary = (monInfo.dwFlags & MONITORINFOF_PRIMARY) ? "true" : "false";

				std::stringstream ss;
				ss <<
					"Monitor: " << s_nIdx <<
					"\n\t\tID: " << displayDevice.DeviceID <<
					"\n\t\tName: " << displayDevice.DeviceName << " - " << displayDevice.DeviceString <<
					"\n\t\tPrimary: " << c_stIsPrimary <<
					"\n\t\tBPC: " << c_nBPC <<
					"\n\t\tResolution: " << monInfo.rcMonitor.right << "x" << monInfo.rcMonitor.bottom;
				CApplication::Instance().GetSysCheck()->AddMonitorInformation(ss.str());

				if (monInfo.rcMonitor.bottom >= 720 && c_nBPC >= 8)
				{
					*c_pbHasAvailableMonitor = TRUE;
					return TRUE;
				}

				return TRUE;
			};

			if (!EnumDisplayMonitors(hDC, nullptr, OnMonitorEnum, reinterpret_cast<LPARAM>(&bHasAvailableMonitor)))
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("EnumDisplayMonitors failed with error: {0}", GetLastError()));
				return bRet;
			}

			ReleaseDC(nullptr, hDC);
		}

		// Enum display devices from registry
		auto bHasCompatibleDisplay = false;
		std::vector <std::string> vDisplayDevRegDir;
		{
			HKEY hKey{};
			auto lStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Enum\\DISPLAY", 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hKey);
			if (lStatus == ERROR_SUCCESS)
			{
				std::vector <std::string> vDisplayDevList;
				DWORD dwIndex = 0;
				while (true)
				{
					DWORD dwKeyLen = MAX_PATH;
					char szNewKeyName[MAX_PATH]{ '\0' };

					lStatus = RegEnumKeyExA(hKey, dwIndex, szNewKeyName, &dwKeyLen, 0, nullptr, nullptr, nullptr);
					if (ERROR_SUCCESS != lStatus)
						break;

					if (szNewKeyName[0] != '\0')
					{
						std::cout << "\t[" << std::to_string(dwIndex + 1) << "] display device detected: " << szNewKeyName << std::endl;
						vDisplayDevList.emplace_back(szNewKeyName);
					}
					dwIndex++;
				}

				for (const auto& stDisplayDevice : vDisplayDevList)
				{
					const auto stSubKey = "SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\" + stDisplayDevice;

					HKEY hSubKey{};
					lStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, stSubKey.c_str(), 0, KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &hSubKey);
					if (lStatus == ERROR_SUCCESS)
					{
						while (true)
						{
							DWORD dwKeyLen = MAX_PATH;
							char szNewKeyName[MAX_PATH]{ '\0' };

							lStatus = RegEnumKeyExA(hSubKey, dwIndex, szNewKeyName, &dwKeyLen, 0, nullptr, nullptr, nullptr);
							if (ERROR_SUCCESS != lStatus)
								break;

							char szParamPath[512]{ '\0' };
							sprintf_s(szParamPath, "%s\\%s\\Device Parameters", stSubKey.c_str(), szNewKeyName);
							vDisplayDevRegDir.emplace_back(szParamPath);
							dwIndex++;
						}
					}
				}

				dwIndex = 0;
				for (const auto& stDisplayDevRegPath : vDisplayDevRegDir)
				{
					dwIndex++;

					HKEY hSubPathKey{};
					lStatus = RegOpenKeyExA(HKEY_LOCAL_MACHINE, stDisplayDevRegPath.c_str(), 0, KEY_READ | KEY_QUERY_VALUE, &hSubPathKey);
					if (lStatus == ERROR_SUCCESS)
					{
						// Get monitor size as CM
						short sWidthCM = 0, sHeightCM = 0;
						if (__GetMonitorSizeFromEDID(hSubPathKey, sWidthCM, sHeightCM))
						{
							// Convert to diagonal
							const auto dDisplayCM = sqrt(sWidthCM * sWidthCM + sHeightCM * sHeightCM);

							// CM to inches
							const auto dDisplayInches = dDisplayCM * 0.393700787;

							const auto stDetails = fmt::format("Display device: {0}\n\t\tRegistry path: {1}\n\t\tDisplay size: {2:L} inches", dwIndex, stDisplayDevRegPath, dDisplayInches);
							sysDetails.vecTexts.emplace_back(stDetails);

							// Check size
							if (dDisplayInches >= 9)
							{
								CLogHelper::Instance().Log(LL_SYS, fmt::format("Available display device with size: {0} detected!", dDisplayInches));
								bHasCompatibleDisplay = true;
								break;
							}
						}

						RegCloseKey(hSubPathKey);
					}
				}
			}
		}

		// Enum display devices from dxdiag API
		DIRECTX_VERSION_INFORMATION dxVerInfo{};
		std::vector <std::shared_ptr <DISPLAY_DEVICE_INFORMATION>> vDisplayDevices;
		{
			auto bCompleted = false;
			auto bComInitialized = false;
			IDxDiagContainer* pObject = nullptr;
			IDxDiagContainer* pContainer = nullptr;
			IDxDiagProvider* pDxDiagProvider = nullptr;
			IDxDiagContainer* pDxDiagRoot = nullptr;
			IDxDiagContainer* pDxDiagSystemInfo = nullptr;

			do
			{
				auto hr = CoInitialize(nullptr);
				bComInitialized = SUCCEEDED(hr);

				if (FAILED(hr = CoCreateInstance(CLSID_DxDiagProvider, nullptr, CLSCTX_INPROC_SERVER, IID_IDxDiagProvider, (LPVOID*)&pDxDiagProvider)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("CoCreateInstance(CLSID_DxDiagProvider) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}
				if (!pDxDiagProvider)
				{
					CLogHelper::Instance().Log(LL_ERR, "CoCreateInstance(CLSID_DxDiagProvider) returned nullptr!");
					hr = E_POINTER;
					break;
				}

				DXDIAG_INIT_PARAMS dxDiagInitParam{ 0 };
				dxDiagInitParam.dwSize = sizeof(dxDiagInitParam);
				dxDiagInitParam.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
				dxDiagInitParam.bAllowWHQLChecks = false;
				dxDiagInitParam.pReserved = nullptr;

				if (FAILED(hr = pDxDiagProvider->Initialize(&dxDiagInitParam)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("pDxDiagProvider(Initialize) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				if (FAILED(hr = pDxDiagProvider->GetRootContainer(&pDxDiagRoot)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("pDxDiagProvider(GetRootContainer) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				if (FAILED(hr = pDxDiagRoot->GetChildContainer(L"DxDiag_DisplayDevices", &pContainer)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("pDxDiagRoot(GetChildContainer) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				DWORD dwInstanceCount = 0;
				if (FAILED(hr = pContainer->GetNumberOfChildContainers(&dwInstanceCount)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("pContainer(GetNumberOfChildContainers) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				CLogHelper::Instance().Log(LL_SYS, fmt::format("{0} instance found!", dwInstanceCount));

				// Display devices
				for (DWORD i = 0; i < dwInstanceCount; i++)
				{
					const auto devInfo = std::make_shared<DISPLAY_DEVICE_INFORMATION>();

					wchar_t wszContainer[256]{ L'\0' };
					if (FAILED(hr = pContainer->EnumChildContainerNames(i, wszContainer, 256)))
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("pContainer(EnumChildContainerNames) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
						break;
					}

					if (FAILED(hr = pContainer->GetChildContainer(wszContainer, &pObject)) || !pObject)
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("pContainer(GetChildContainer) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
						break;
					}

					if (FAILED(hr = __GetStringValue(pObject, L"szDescription", devInfo->szDescription, _countof(devInfo->szDescription))))
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("__GetStringValue(szDescription) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
						break;
					}

					if (FAILED(hr = __GetStringValue(pObject, L"szDriverModelEnglish", devInfo->szDriverModel, _countof(devInfo->szDriverModel))))
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("__GetStringValue(szDriverModelEnglish) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
						break;
					}

					sysDetails.vecTexts.emplace_back(fmt::format("Display devices:\n\t\tDevice index: {0}\n\t\tDescription: {1}\n\t\tModel: {2}", i + 1, devInfo->szDescription, devInfo->szDriverModel));

					vDisplayDevices.emplace_back(devInfo);
					SAFE_RELEASE(pObject);
				}

				// DirectX
				if (FAILED(hr = pDxDiagRoot->GetChildContainer(L"DxDiag_SystemInfo", &pDxDiagSystemInfo)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("GetChildContainer(DxDiag_SystemInfo) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				if (FAILED(hr = __GetIntValue(pDxDiagSystemInfo, L"dwDirectXVersionMajor", &dxVerInfo.nMajorVersion)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("__GetIntValue(dwDirectXVersionMajor) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				if (FAILED(hr = __GetIntValue(pDxDiagSystemInfo, L"dwDirectXVersionMinor", &dxVerInfo.nMinorVersion)))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("__GetIntValue(dwDirectXVersionMinor) failed with status: {0}", fmt::ptr(reinterpret_cast<void*>(hr))));
					break;
				}

				sysDetails.vecTexts.emplace_back(fmt::format("\n\tDirectX:\n\t\tVersion: {0}.{1}", dxVerInfo.nMajorVersion, dxVerInfo.nMinorVersion));

				bCompleted = true;
			} while (FALSE);

			SAFE_RELEASE(pObject);
			SAFE_RELEASE(pContainer);
			SAFE_RELEASE(pDxDiagSystemInfo);
			SAFE_RELEASE(pDxDiagRoot);
			SAFE_RELEASE(pDxDiagProvider);

			if (bComInitialized)
				CoUninitialize();

			if (!bCompleted)
			{
				CLogHelper::Instance().Log(LL_ERR, "DirectX/WDDM check failed!");
				return bRet;
			}
		}

		auto bHasAvailableWDDM = false;
		for (const auto& spDeviceInfo : vDisplayDevices)
		{
			if (spDeviceInfo)
			{
				const std::string c_stModelInfo = spDeviceInfo->szDriverModel;
				if (c_stModelInfo.find("WDDM 2") != std::string::npos ||
					c_stModelInfo.find("WDDM 3") != std::string::npos)
				{
					bHasAvailableWDDM = true;
					break;
				}
			}
		}

		for (const auto& stMonitorInfo : m_vMonitorInfos)
		{
			sysDetails.vecTexts.emplace_back(stMonitorInfo);
		}

		m_mapSystemDetails[nType] = sysDetails;

		if (bHasAvailableMonitor && (!vDisplayDevRegDir.empty() && bHasCompatibleDisplay) && dxVerInfo.nMajorVersion >= 12 && bHasAvailableWDDM)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("Display informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}
	bool CSysCheck::__LoadInternetInformations()
	{
		static auto timer = CSimpleTimer<std::chrono::milliseconds>();
		CLogHelper::Instance().Log(LL_SYS, "Internet Informations loading...");

		auto bRet = false;

		const auto nType = EMenuType::MENU_TYPE_INTERNET;

		m_mapStatuses[nType] = EStatus::STATUS_INITIALIZING;

		DWORD dwFlags = 0;
		const auto bNetworkState = InternetGetConnectedState(&dwFlags, 0);
		if (!bNetworkState)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("InternetGetConnectedState failed with error: {0}", GetLastError()));
		}

		const auto dwTestConnectionRet = InternetAttemptConnect(0);
		if (dwTestConnectionRet != ERROR_SUCCESS)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("InternetAttemptConnect failed with error: {0}", GetLastError()));
		}

		SSystemDetails sysDetails{};
		const auto stIconText = CApplication::Instance().GetUI()->GetMenuTypeIconText(nType);
		sysDetails.stTitle = fmt::format("{0}  {1} {2}:", stIconText, CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(nType),
			CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DETAILS)
		);

		sysDetails.vecTexts.emplace_back(fmt::format("Network state: {0}\n\tInternet state: {1}",
			bNetworkState,
			dwTestConnectionRet == ERROR_SUCCESS ? 1 : 0
		));

		m_mapSystemDetails[nType] = sysDetails;

		if (bNetworkState && dwTestConnectionRet == ERROR_SUCCESS)
			m_mapStatuses[nType] = EStatus::STATUS_OK;
		else
			m_mapStatuses[nType] = EStatus::STATUS_FAIL;

		bRet = true;
		CLogHelper::Instance().Log(LL_SYS, fmt::format("Internet informations loaded in: {0} ms", timer.diff()));
		return bRet;
	}

	DWORD CSysCheck::__ThreadRoutine(void)
	{
		const auto bStatus = __LoadOSInformations() &&
			__LoadBootInformations() &&
			__LoadCPUInformations() &&
			__LoadRAMInformations() &&
			__LoadDiskInformations() &&
			__LoadDisplayInformations() &&
			__LoadInternetInformations();

		if (!bStatus)
		{
			CLogHelper::Instance().Log(LL_CRI, "System information processing failed!");
			std::exit(EXIT_FAILURE);
		}

		return EXIT_SUCCESS;
	}

	DWORD WINAPI CSysCheck::__StartThreadRoutine(LPVOID lpParam)
	{
		const auto This = reinterpret_cast<CSysCheck*>(lpParam);
		return This->__ThreadRoutine();
	}
	bool CSysCheck::LoadSystemInformations()
	{
		const auto hThread = CreateThread(nullptr, 0, __StartThreadRoutine, (void*)this, 0,  nullptr);
		if (!hThread || hThread == INVALID_HANDLE_VALUE)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("CreateThread failed with error: {0}", GetLastError()));
			return false;
		}

		CloseHandle(hThread);
		return true;	
	}

	SSystemDetails CSysCheck::GetSystemDetails(EMenuType nType)
	{
		const auto it = m_mapSystemDetails.find(nType);
		if (it == m_mapSystemDetails.end())
			return {};
		return it->second;
	}

	bool CSysCheck::CanSystemUpgradable()
	{
		uint8_t nCounter = 0;

		const auto nMaxCount = static_cast<uint8_t>(EMenuType::MENU_TYPE_MAX);
		for (size_t i = 0; i < nMaxCount; ++i)
		{
			const auto nStatus = GetMenuStatus(static_cast<EMenuType>(i));
			if (nStatus == EStatus::STATUS_OK)
				nCounter++;
		}

		return nCounter == nMaxCount - 2; // Ignore max and summary
	}
};
