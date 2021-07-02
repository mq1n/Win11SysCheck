#include "../include/pch.hpp"
#include "../include/main_ui.hpp"
#include "../include/application.hpp"
#include <notify/notify.h>

#define ICON_FA_CLIPBOARD "\xef\x8c\xa8"	// U+f328
#define ICON_FA_WINDOWS "\xef\x85\xba"	// U+f17a
#define ICON_FA_HOURGLASS_HALF "\xef\x89\x92"	// U+f252
#define ICON_FA_MICROCHIP "\xef\x8b\x9b"	// U+f2db
#define ICON_FA_MEMORY "\xef\x94\xb8"	// U+f538
#define ICON_FA_HDD "\xef\x82\xa0"	// U+f0a0
#define ICON_FA_DESKTOP "\xef\x84\x88"	// U+f108
#define ICON_FA_WIFI "\xef\x87\xab"	// U+f1eb
#define ICON_FA_POLL_H "\xef\x9a\x82"	// U+f682
#define ICON_FA_SAVE "\xef\x83\x87"	// U+f0c7
#define ICON_FA_MOON "\xef\x86\x86"	// U+f186

#define IMGUI_GREEN_COLOR ImVec4(0.05f, 0.7f, 0.3f, 1.0f)
#define IMGUI_RED_COLOR ImVec4(1.0f, 0.0f, 0.0f, 1.0f)
#define IMGUI_BLUE_COLOR ImVec4(0.10f, 0.80f, 1.00f, 1.0f)
#define IMGUI_GREY_COLOR ImVec4(0.5f, 0.5f, 0.5f, 1.f)

namespace Win11SysCheck::gui
{
	std::string GetStatusText(EStatus nStatus)
	{
		switch (nStatus)
		{
		case EStatus::STATUS_UNKNOWN:
			return CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_UNKNOWN);
		case EStatus::STATUS_INITIALIZING:
			return CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_INITIALIZING);
		case EStatus::STATUS_OK:
			return CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_COMPATIBLE_OK);
		case EStatus::STATUS_FAIL:
			return CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_COMPATIBLE_FAIL);
		default:
			return "Undefined: " + std::to_string(static_cast<uint8_t>(nStatus));
		}
	}
	ImVec4 GetStatusColor(EStatus nStatus)
	{
		switch (nStatus)
		{
		case EStatus::STATUS_INITIALIZING:
			return IMGUI_BLUE_COLOR;
		case EStatus::STATUS_OK:
			return IMGUI_GREEN_COLOR;
		case EStatus::STATUS_FAIL:
			return IMGUI_RED_COLOR;
		default:
			return IMGUI_GREY_COLOR;
		}
	}

	CMainUI::CMainUI() :
		m_nMenuType(EMenuType::MENU_TYPE_UNKNOWN),
		m_bIsOpen(true),
		m_bDarkModeEnabled(false)
	{
	}
	CMainUI::~CMainUI()
	{
	}

	std::string CMainUI::GetMenuTypeIconText(EMenuType nType)
	{
		switch (nType)
		{
		case EMenuType::MENU_TYPE_SUMMARY:
			return ICON_FA_CLIPBOARD;
		case EMenuType::MENU_TYPE_OS:
			return ICON_FA_WINDOWS;
		case EMenuType::MENU_TYPE_BOOT:
			return ICON_FA_HOURGLASS_HALF;
		case EMenuType::MENU_TYPE_CPU:
			return ICON_FA_MICROCHIP;
		case EMenuType::MENU_TYPE_RAM:
			return ICON_FA_MEMORY;
		case EMenuType::MENU_TYPE_DISK:
			return ICON_FA_HDD;
		case EMenuType::MENU_TYPE_DISPLAY:
			return ICON_FA_DESKTOP;
		case EMenuType::MENU_TYPE_INTERNET:
			return ICON_FA_WIFI;
		default:
			return std::to_string(static_cast<uint8_t>(nType));
		}
	}

	void CMainUI::OnDraw()
	{
		static const auto sc_rcMainWndRect = CApplication::Instance().GetWindow()->GetRect();
		ImGui::SetNextWindowPos(ImVec2(0, 2));
		ImGui::SetNextWindowSize(ImVec2((float)sc_rcMainWndRect.width, (float)(sc_rcMainWndRect.height - 10)));

		static const auto sc_nWndFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::Begin("MainWnd", IsOpenPtr(), sc_nWndFlags))
		{
			if (ImGui::BeginChild("MenuChildWnd", ImVec2(180, 300), true))
			{
				for (size_t i = 1; i < static_cast<uint8_t>(EMenuType::MENU_TYPE_MAX); ++i)
				{
					const auto c_nCurrentMenu = static_cast<EMenuType>(i);
					const auto c_stCurrentMenuName = CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(c_nCurrentMenu);
					const auto c_stCurrentMenuIcon = GetMenuTypeIconText(c_nCurrentMenu);

					if (i != 1)
						ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Text(c_stCurrentMenuIcon.c_str());
					ImGui::SameLine();
					ImGui::SetCursorPosX(48);
					if (ImGui::Selectable(c_stCurrentMenuName.c_str()))
					{
						m_nMenuType = c_nCurrentMenu;
					}
				}

				ImGui::EndChild();
			}

			ImGui::SameLine();

			if (ImGui::BeginChild("ContextChildWnd", ImVec2(450, 450), true, ImGuiWindowFlags_HorizontalScrollbar))
			{
				if (m_nMenuType == EMenuType::MENU_TYPE_SUMMARY)
				{
					// Common text
					const auto c_stLocalizedStatus = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_STATUS);
					const auto c_stLocalizedResultTitle = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_RESULT);

					// Menu results
					static auto s_bOnce = false;
					for (size_t i = 2; i < static_cast<uint8_t>(EMenuType::MENU_TYPE_MAX); ++i)
					{
						const auto c_nCurrentMenu = static_cast<EMenuType>(i);
						if (!s_bOnce)
							CApplication::Instance().GetSysCheck()->GetSystemDetails(c_nCurrentMenu);

						const auto c_stCurrentMenuName = CApplication::Instance().GetI18N()->GetMenuTypeLocalizedText(c_nCurrentMenu);
						const auto c_stCurrentMenuIcon = GetMenuTypeIconText(c_nCurrentMenu);

						ImGui::Spacing();

						ImGui::Text(c_stCurrentMenuIcon.c_str());
						ImGui::SameLine();
						ImGui::SetCursorPosX(48);
						ImGui::Text("%s %s: ", c_stCurrentMenuName.c_str(), c_stLocalizedStatus.c_str());

						const auto nStatus = CApplication::Instance().GetSysCheck()->GetMenuStatus(c_nCurrentMenu);
						const auto stStatus = GetStatusText(nStatus);
						const auto v4StatusColor = GetStatusColor(nStatus);
						ImGui::TextColored(v4StatusColor, "\t\t%s", stStatus.c_str());
					}
					s_bOnce = true;

					ImGui::Spacing();
					ImGui::Spacing();
					ImGui::Spacing();

					// Can upgradable result
					ImGui::Text("%s", ICON_FA_POLL_H);
					ImGui::SameLine();
					ImGui::SetCursorPosX(48);
					ImGui::Text("%s:", c_stLocalizedResultTitle.c_str());

					const auto bResult = CApplication::Instance().GetSysCheck()->CanSystemUpgradable();
					const auto v4StatusColor = GetStatusColor(bResult ? EStatus::STATUS_OK : EStatus::STATUS_FAIL);
					const auto c_stLocalizedResult = CApplication::Instance().GetI18N()->GetCommonLocalizedText(bResult ? ECommonTextID::TEXT_ID_CAN_UPGRADABLE : ECommonTextID::TEXT_ID_CAN_NOT_UPGRADABLE);
					ImGui::TextColored(v4StatusColor, "\t\t%s", c_stLocalizedResult.c_str());
				}
				else
				{
					const auto sc_pDetails = CApplication::Instance().GetSysCheck()->GetSystemDetails(m_nMenuType);
					
					ImGui::Spacing();
					ImGui::Text(sc_pDetails.stTitle.c_str());

					for (const auto& sysCtxDetail : sc_pDetails.vecTexts)
					{
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Text("\t%s", sysCtxDetail.c_str());
					}

					if (m_nMenuType == EMenuType::MENU_TYPE_CPU)
					{
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Spacing();
						ImGui::Spacing();

						static const auto sc_stSupportedCPUText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_SUPPORTED_CPU_TITLE);
						ImGui::Text(sc_stSupportedCPUText.c_str());

						ImGui::Spacing();
						if (ImGui::Selectable("\t\t# AMD"))
						{
							CApplication::Instance().OpenWebPage("https://docs.microsoft.com/en-us/windows-hardware/design/minimum/supported/windows-11-supported-amd-processors");
						}
						ImGui::Spacing();
						if (ImGui::Selectable("\t\t# Intel"))
						{
							CApplication::Instance().OpenWebPage("https://docs.microsoft.com/en-us/windows-hardware/design/minimum/supported/windows-11-supported-intel-processors");
						}
						ImGui::Spacing();
						if (ImGui::Selectable("\t\t# Qualcomm"))
						{
							CApplication::Instance().OpenWebPage("https://docs.microsoft.com/en-us/windows-hardware/design/minimum/supported/windows-11-supported-qualcomm-processors");
						}
					}
				}

				ImGui::EndChild();
			}

			ImGui::SetCursorPosX(18);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 160);

			if (ImGui::BeginChild("LinksChildWnd", ImVec2(180, 160), false))
			{
				ImGui::Spacing();

				if (ImGui::Selectable("# GitHub"))
				{
					CApplication::Instance().OpenWebPage("https://github.com/mq1n/Win11SysCheck");
				}

				ImGui::Spacing();

				static const auto sc_stAboutWin11Text = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_ABOUT_WIN_11);
				if (ImGui::Selectable(sc_stAboutWin11Text.c_str()))
				{
					CApplication::Instance().OpenWebPage("https://www.microsoft.com/en-us/windows/windows-11");
				}

				ImGui::Spacing();

				static const auto sc_stSysSpecText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_SYSTEM_SPEC_LINK);
				if (ImGui::Selectable(sc_stSysSpecText.c_str()))
				{
					CApplication::Instance().OpenWebPage("https://www.microsoft.com/en-us/windows/windows-11-specifications");
				}

				ImGui::Spacing();

				static const auto sc_stVersionText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_VERSION);
				ImGui::Text("%s:\n\t1.%s", sc_stVersionText.c_str(), _GIT_VERSION_);

				static const auto sc_stBuildText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_BUILD);
				ImGui::Text("%s:\n\t%s %s", sc_stBuildText.c_str(), __DATE__, __TIME__);

				ImGui::EndChild();
			}

			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 70);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10);

			if (ImGui::Checkbox(ICON_FA_MOON, &m_bDarkModeEnabled))
			{
				if (m_bDarkModeEnabled)
				{
					auto& ini = CApplication::Instance().GetConfigContext();
					ini["ui"]["dark_mode"] = "1";
					CApplication::Instance().GetConfigFile().write(ini);

					ImGui::StyleColorsDark();
				}
				else
				{
					auto& ini = CApplication::Instance().GetConfigContext();
					ini["ui"]["dark_mode"] = "0";
					CApplication::Instance().GetConfigFile().write(ini);

					ImGui::StyleColorsLight();
				}
			}

			ImGui::SameLine();

			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 180);			

			static const auto sc_stTooltipText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_DARK_MODE);
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("%s", sc_stTooltipText.c_str());

			static const auto sc_stExportButtonText = fmt::format("{0}  {1}", ICON_FA_SAVE, CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_SAVE_RESULT));
			if (ImGui::Button(sc_stExportButtonText.c_str()))
			{
				static const auto sc_stExportSuccessText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_EXPORT_SUCCESS);
				static const auto sc_stExportFailText = CApplication::Instance().GetI18N()->GetCommonLocalizedText(ECommonTextID::TEXT_ID_EXPORT_FAIL);

				std::string stFileName;
				if (CApplication::Instance().GetSysCheck()->ExportResult(stFileName))
					notify::insert({ toast_type::toast_type_success, 3000, sc_stExportSuccessText.c_str(), stFileName.c_str() });
				else
					notify::insert({ toast_type::toast_type_error, 3000, sc_stExportFailText.c_str() });
			}

			notify::render();
			ImGui::End();
		}
	}
};
