#include "../include/pch.hpp"
#include "../include/i18n.hpp"
#include "../include/log_helper.hpp"
#include "../include/application.hpp"

namespace Win11SysCheck
{
	CI18N::CI18N()
	{
	}
	CI18N::~CI18N()
	{
	}

	bool CI18N::Initialize()
	{
		const auto GetSystemLocale = [] {
			char szLang[5]{ '\0' };
			GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, szLang, sizeof(szLang) / sizeof(char));

			char szCountry[5]{ '\0' };
			GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, szCountry, sizeof(szCountry) / sizeof(char));

			return fmt::format("{0}_{1}", szLang, szCountry);
		};
		const auto GetDirectoryFileCount = [](std::filesystem::path path) {
			using std::filesystem::directory_iterator;
			return std::distance(directory_iterator(path), directory_iterator{});
		};
		const auto ToUTF8 = [](const std::string& input) {
			wchar_t buf[8192] = { 0 };
			MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)input.length(), buf, ARRAYSIZE(buf));
			return buf;
		};
		const auto CurrentPath = []() -> std::string {
			char buffer[MAX_PATH] = { 0 };
			if (GetCurrentDirectoryA(MAX_PATH, buffer))
				return buffer;
			return {};
		};
		const auto ReadFileContent = [](const std::string& stFileName) -> std::string {
			std::ifstream in(stFileName.c_str(), std::ios_base::binary);
			if (in)
			{
				in.exceptions(std::ios_base::badbit | std::ios_base::failbit | std::ios_base::eofbit);
				return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
			}
			return {};
		};
		const auto IsNumber = [](const std::string& s)
		{
			return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
		};

		const auto c_szDefaultLanguageFile = "en_us.json";

		const auto c_szLangConfigPath = fmt::format("{0}\\I18n", CurrentPath());
		if (!std::filesystem::exists(c_szLangConfigPath))
		{
			CLogHelper::Instance().Log(LL_ERR, "I18n path does not exist");
			return false;
		}
		if (!GetDirectoryFileCount(c_szLangConfigPath))
		{
			CLogHelper::Instance().Log(LL_ERR, "I18n path does not contain any file");
			return false;
		}

		auto locale = GetSystemLocale();
		CLogHelper::Instance().Log(LL_SYS, fmt::format("User locale: {0}", locale.c_str()));

		std::string locale_file_path;
		auto& ini = CApplication::Instance().GetConfigContext();
		const auto stPredefinedLocale = ini["locale"]["file"];
		if (!stPredefinedLocale.empty())
		{
			locale_file_path = fmt::format("{0}\\{1}.json", c_szLangConfigPath, stPredefinedLocale);
			if (std::filesystem::exists(locale_file_path))
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("Pre-defined locale config: {0} does not exist", locale_file_path.c_str()));
				return false;
			}
		}
		else
		{
			locale_file_path = fmt::format("{0}\\{1}.json", c_szLangConfigPath, locale);
			if (!std::filesystem::exists(locale_file_path))
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("Target locale config({0}) does not exist", locale_file_path.c_str()));

				locale_file_path = fmt::format("{0}\\{1}", c_szLangConfigPath, c_szDefaultLanguageFile);
				if (!std::filesystem::exists(locale_file_path))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("Default locale config: {0} does not exist", locale_file_path.c_str()));
					return false;
				}
			}
		}

		CLogHelper::Instance().Log(LL_SYS, fmt::format("Target locale file: {0}", locale_file_path.c_str()));

		auto document = Document{};
		try
		{
			const auto stBuffer = ReadFileContent(locale_file_path);
			if (stBuffer.empty())
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("File: {0} read failed!", locale_file_path.c_str()));
				return false;
			}

			document.Parse<kParseCommentsFlag>(reinterpret_cast<const char*>(stBuffer.data()));
			if (document.HasParseError())
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("Language file could NOT parsed! Error: {0} offset: {1}",
					GetParseError_En(document.GetParseError()), document.GetErrorOffset()
				));
				return false;
			}

			if (!document.IsObject())
			{
				CLogHelper::Instance().Log(LL_ERR, fmt::format("Language file base is not an object! Type: {0}", document.GetType()));
				return false;
			}

			uint32_t phase_idx = 0;
			uint32_t idx = 0;
			for (auto phase_node = document.MemberBegin(); phase_node != document.MemberEnd(); ++phase_node)
			{
				phase_idx++;

				if (!phase_node->name.IsString())
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("Language phase node: {0} key is not a string. Type: {1}",
						phase_idx, phase_node->name.GetType()
					));
					return false;
				}
				if (!phase_node->value.IsObject())
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("Language phase node: {0} value is not a object. Type: {1}",
						phase_idx, phase_node->value.GetType())
					);
					return false;
				}

				const auto current_phase_str = phase_node->name.GetString();
				if (!IsNumber(current_phase_str))
				{
					CLogHelper::Instance().Log(LL_ERR, fmt::format("Language phase context is not a number. Data: {0}", current_phase_str));
					return false;
				}
				const auto current_phase_num = std::atoi(current_phase_str);

				for (auto node = phase_node->value.MemberBegin(); node != phase_node->value.MemberEnd(); ++node)
				{
					idx++;

					if (!node->name.IsString())
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("Language file node: {0} key is not a string. Phase: {1} Type: {2}",
							idx, current_phase_str, node->name.GetType())
						);
						return false;
					}
					if (!node->value.IsString())
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("Language file node: {0} value is not a string. Phase: {1} Type: {2}",
							idx, current_phase_str, node->value.GetType())
						);
						return false;
					}

					const auto key = node->name.GetString();
					if (!IsNumber(key))
					{
						CLogHelper::Instance().Log(LL_ERR, fmt::format("Language file node: {0} key context not a number. Phase: {1} Data: {2}",
							idx, current_phase_str, key)
						);
						return false;
					}

					auto data = std::make_shared<SI18nContext>();
					data->phase = static_cast<uint8_t>(current_phase_num);
					data->index = std::atol(key);
					data->context = ToUTF8(node->value.GetString());

					m_vI18nData.emplace_back(data);
					CLogHelper::Instance().Log(LL_TRACE, fmt::format("[{0}] I18N parsing... [{1}] {2} -> {3}", idx, data->phase, data->index, node->value.GetString()));
				}
			}
		}
		catch (const std::exception& e)
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("Exception handled on parse locale config file, Error: {0}", e.what()));
			return false;
		}

		return true;
	}
	void CI18N::Destroy()
	{
	}

	std::string CI18N::__ToAnsi(const std::wstring& in)
	{
#ifdef _WIN32
#pragma warning(push) 
#pragma warning(disable: 4242 4244)
#endif // _WIN32
		auto out = std::string(in.begin(), in.end());
#ifdef _WIN32
#pragma warning(push) 
#endif // _WIN32

		return out;
	}

	std::wstring CI18N::GetLocalizedText(EPhase phase, uint8_t index)
	{
		for (const auto& i18n : m_vI18nData)
		{
			if (i18n)
			{
				if (i18n->phase == (uint8_t)phase && i18n->index == index)
				{
					return i18n->context;
				}
			}
		}
		return L"<undefined> " + std::to_wstring(index);
	}

	std::string CI18N::GetMenuTypeLocalizedText(EMenuType nType)
	{
		switch (nType)
		{
		case EMenuType::MENU_TYPE_SUMMARY:
		case EMenuType::MENU_TYPE_OS:
		case EMenuType::MENU_TYPE_BOOT:
		case EMenuType::MENU_TYPE_CPU:
		case EMenuType::MENU_TYPE_RAM:
		case EMenuType::MENU_TYPE_DISK:
		case EMenuType::MENU_TYPE_DISPLAY:
		case EMenuType::MENU_TYPE_INTERNET:
		{
			const auto c_wstText = GetLocalizedText(EPhase::PHASE_MENU_TYPE, static_cast<uint8_t>(nType));
			return __ToAnsi(c_wstText);
		} break;
		default:
			return "Undefined: " + std::to_string(static_cast<uint8_t>(nType));
		}
	}

	std::string CI18N::GetCommonLocalizedText(ECommonTextID nID)
	{
		switch (nID)
		{
		case ECommonTextID::TEXT_ID_UNKNOWN:
		case ECommonTextID::TEXT_ID_INITIALIZING:
		case ECommonTextID::TEXT_ID_COMPATIBLE_OK:
		case ECommonTextID::TEXT_ID_COMPATIBLE_FAIL:
		case ECommonTextID::TEXT_ID_STATUS:
		case ECommonTextID::TEXT_ID_DETAILS:
		case ECommonTextID::TEXT_ID_SAVE_RESULT:
		case ECommonTextID::TEXT_ID_DARK_MODE:
		case ECommonTextID::TEXT_ID_CAPABLE:
		case ECommonTextID::TEXT_ID_ENABLED:
		case ECommonTextID::TEXT_ID_DISABLED:
		case ECommonTextID::TEXT_ID_VERSION:
		case ECommonTextID::TEXT_ID_BUILD:
		case ECommonTextID::TEXT_ID_RESULT:
		case ECommonTextID::TEXT_ID_CAN_UPGRADABLE:
		case ECommonTextID::TEXT_ID_CAN_NOT_UPGRADABLE:
		case ECommonTextID::TEXT_ID_EXPORT_SUCCESS:
		case ECommonTextID::TEXT_ID_EXPORT_FAIL:
		case ECommonTextID::TEXT_ID_ABOUT_WIN_11:
		case ECommonTextID::TEXT_ID_SYSTEM_SPEC_LINK:
		case ECommonTextID::TEXT_ID_SUPPORTED_CPU_TITLE:
		{
			const auto c_wstText = GetLocalizedText(EPhase::PHASE_COMMON, static_cast<uint8_t>(nID));
			return __ToAnsi(c_wstText);
		} break;
		default:
			return "Undefined: " + std::to_string(static_cast<uint8_t>(nID));
		}
	}

	std::string CI18N::GetSystemDetailsLocalizedText(ESystemDetailID nID)
	{
		switch (nID)
		{
		case ESystemDetailID::SYSTEM_DETAIL_OS_VERSION:
		case ESystemDetailID::SYSTEM_DETAIL_OS_SP_VERSION:
		case ESystemDetailID::SYSTEM_DETAIL_OS_BUILD_VERSION:
		case ESystemDetailID::SYSTEM_DETAIL_OS_PLATFORM_ID:
		case ESystemDetailID::SYSTEM_DETAIL_OS_PRODUCT_TYPE:
		case ESystemDetailID::SYSTEM_DETAIL_BOOT_ENVIRONMENT:
		case ESystemDetailID::SYSTEM_DETAIL_BOOT_SECURE_BOOT:
		case ESystemDetailID::SYSTEM_DETAIL_BOOT_TPM:
		case ESystemDetailID::SYSTEM_DETAIL_CPU_NAME:
		case ESystemDetailID::SYSTEM_DETAIL_CPU_DETAILS:
		case ESystemDetailID::SYSTEM_DETAIL_CPU_ARCH:
		case ESystemDetailID::SYSTEM_DETAIL_CPU_ACTIVE_PROCESSOR_COUNT:
		case ESystemDetailID::SYSTEM_DETAIL_CPU_PROCESSOR_COUNT:
		case ESystemDetailID::SYSTEM_DETAIL_RAM_TOTAL:
		{
			const auto c_wstText = GetLocalizedText(EPhase::PHASE_SYSTEM_DETAIL, static_cast<uint8_t>(nID));
			return __ToAnsi(c_wstText);
		} break;
		default:
			return "Undefined: " + std::to_string(static_cast<uint8_t>(nID));
		}
	}
};
