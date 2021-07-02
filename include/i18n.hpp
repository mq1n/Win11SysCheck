#pragma once
#include "abstract_singleton.hpp"
#include "pch.hpp"
#include "sys_check.hpp"

namespace Win11SysCheck
{
	struct SI18nContext
	{
		uint8_t phase{ 0 };
		uint32_t index{ 0 };
		std::wstring context{ L"" };
	};

	enum class EPhase : uint8_t
	{
		PHASE_UNDEFINED,
		PHASE_COMMON,
		PHASE_MENU_TYPE,
		PHASE_SYSTEM_DETAIL
	};

	enum class ECommonTextID : uint8_t
	{
		TEXT_ID_UNDEFINED,
		TEXT_ID_UNKNOWN,
		TEXT_ID_INITIALIZING,
		TEXT_ID_COMPATIBLE_OK,
		TEXT_ID_COMPATIBLE_FAIL,
		TEXT_ID_STATUS,
		TEXT_ID_DETAILS,
		TEXT_ID_SAVE_RESULT,
		TEXT_ID_DARK_MODE,
		TEXT_ID_CAPABLE,
		TEXT_ID_ENABLED,
		TEXT_ID_DISABLED,
		TEXT_ID_VERSION,
		TEXT_ID_BUILD,
		TEXT_ID_RESULT,
		TEXT_ID_CAN_UPGRADABLE,
		TEXT_ID_CAN_NOT_UPGRADABLE,
		TEXT_ID_EXPORT_SUCCESS,
		TEXT_ID_EXPORT_FAIL,
		TEXT_ID_ABOUT_WIN_11,
		TEXT_ID_SYSTEM_SPEC_LINK,
		TEXT_ID_SUPPORTED_CPU_TITLE
	};
	enum class ESystemDetailID : uint8_t
	{
		SYSTEM_DETAIL_ID_UNDEFINED,
		SYSTEM_DETAIL_OS_VERSION,
		SYSTEM_DETAIL_OS_SP_VERSION,
		SYSTEM_DETAIL_OS_BUILD_VERSION,
		SYSTEM_DETAIL_OS_PLATFORM_ID,
		SYSTEM_DETAIL_OS_PRODUCT_TYPE,
		SYSTEM_DETAIL_BOOT_ENVIRONMENT,
		SYSTEM_DETAIL_BOOT_SECURE_BOOT,
		SYSTEM_DETAIL_BOOT_TPM,
		SYSTEM_DETAIL_CPU_NAME,
		SYSTEM_DETAIL_CPU_DETAILS,
		SYSTEM_DETAIL_CPU_ARCH,
		SYSTEM_DETAIL_CPU_ACTIVE_PROCESSOR_COUNT,
		SYSTEM_DETAIL_CPU_PROCESSOR_COUNT,
		SYSTEM_DETAIL_RAM_TOTAL
	};

	class CI18N : public CSingleton <CI18N>
	{
	public:
		CI18N();
		virtual ~CI18N();

		bool Initialize();
		void Destroy();

		std::wstring GetLocalizedText(EPhase phase, uint8_t index);
		std::string GetCommonLocalizedText(ECommonTextID nID);
		std::string GetMenuTypeLocalizedText(EMenuType nType);
		std::string GetSystemDetailsLocalizedText(ESystemDetailID nID);

	protected:
		std::string __ToAnsi(const std::wstring& in);

	private:
		std::map <EMenuType, EStatus> m_mapStatuses;
		std::vector <std::shared_ptr <SI18nContext>> m_vI18nData;
	};
};
