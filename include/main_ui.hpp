#pragma once
#include "abstract_singleton.hpp"
#include "sys_check.hpp"

namespace Win11SysCheck::gui
{
	class CMainUI : public CSingleton <CMainUI>
	{
	public:
		CMainUI();
		virtual ~CMainUI();

		std::string GetMenuTypeIconText(EMenuType nType);

		void OnDraw();

		auto IsOpenPtr() { return &m_bIsOpen; };
		auto IsOpen() const { return m_bIsOpen; };

		void SetDarkModeEnabled(const bool bEnabled) { m_bDarkModeEnabled = bEnabled; };

	private:
		EMenuType m_nMenuType;
		bool m_bIsOpen;
		bool m_bDarkModeEnabled;
	};
};
