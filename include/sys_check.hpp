#pragma once
#include "abstract_singleton.hpp"
#include "pch.hpp"

namespace Win11SysCheck
{
	enum class EStatus : uint8_t
	{
		STATUS_UNKNOWN,
		STATUS_INITIALIZING,
		STATUS_OK,
		STATUS_FAIL
	};

	enum class EMenuType : uint8_t
	{
		MENU_TYPE_UNKNOWN,
		MENU_TYPE_SUMMARY,
		MENU_TYPE_OS,
		MENU_TYPE_BOOT,
		MENU_TYPE_CPU,
		MENU_TYPE_RAM,
		MENU_TYPE_DISK,
		MENU_TYPE_DISPLAY,
		MENU_TYPE_INTERNET,
		MENU_TYPE_MAX
	};

	struct SSystemDetails
	{
		std::string stTitle;
		std::vector <std::string> vecTexts;
	};

	struct SDiskContext
	{
		std::string stPath;
		std::string stDeviceName;
		std::string stVolumeName;
		std::string stFileSystem;
	};

	class CSysCheck : public CSingleton <CSysCheck>
	{
		typedef NTSTATUS(NTAPI* TNtQuerySystemInformation)(UINT SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
		typedef NTSTATUS(NTAPI* TRtlGetVersion)(LPOSVERSIONINFOEX OsVersionInfoEx);

	public:
		CSysCheck();
		virtual ~CSysCheck();

		bool Initialize();
		void Destroy();

		bool ExportResult(std::string& stFileName);

		void AddMonitorInformation(const std::string& stInfo);
		bool LoadSystemInformations();
		SSystemDetails GetSystemDetails(EMenuType nType);
		bool CanSystemUpgradable();

		EStatus GetMenuStatus(EMenuType nMenuType);
		void SetMenuStatus(EMenuType nMenuType, EStatus nStatus);

	protected:
		HRESULT __GetStringValue(IDxDiagContainer* pContainerObject, wchar_t* lpwszName, char* lpszValue, int nValueLength);
		HRESULT __GetIntValue(IDxDiagContainer* pContainerObject, wchar_t* lpwszName, int* pnValue);
		bool __GetMonitorSizeFromEDID(const HKEY hEDIDRegKey, short& WidthMm, short& HeightMm);

		bool __LoadOSInformations();
		bool __LoadBootInformations();
		bool __LoadCPUInformations();
		bool __LoadRAMInformations();
		bool __LoadDiskInformations();
		bool __LoadDisplayInformations();
		bool __LoadInternetInformations();

		std::string __GetFirmwareName(const uint8_t type);
		std::string __GetPartitionName(const uint8_t type);
		std::string __GetVolumePath(PCHAR VolumeName);

	protected:
		DWORD					__ThreadRoutine(void);
		static DWORD WINAPI		__StartThreadRoutine(LPVOID lpParam);

	private:
		HMODULE m_hNtdll;
		TNtQuerySystemInformation m_fnNtQuerySystemInformation;
		TRtlGetVersion m_fnRtlGetVersion;
		std::map <EMenuType, EStatus> m_mapStatuses;
		std::map <EMenuType, SSystemDetails> m_mapSystemDetails;
		std::vector <std::string> m_vMonitorInfos;
	};
};
