#pragma once
#include "abstract_singleton.hpp"
#include "window.hpp"
#include "main_ui.hpp"
#include "sys_check.hpp"
#include "i18n.hpp"
#include <mini/ini.h>

namespace Win11SysCheck
{
	class CApplication : public CSingleton <CApplication>
	{
	public:
		CApplication(int argc, char* argv[]);
		virtual ~CApplication();

		void Destroy();
		int MainRoutine();

		bool OpenWebPage(const std::string& stAddress);

		auto GetWindow() const	 { return m_spWND.get(); };
		auto GetUI() const		 { return m_spUI.get(); };
		auto GetI18N() const	 { return m_spI18N.get(); };
		auto GetSysCheck() const { return m_spSysCheck.get(); };
		auto GetConfigFile()	 { return m_pConfigFile; };
		auto& GetConfigContext() { return m_pConfigContext; };

	private:
		std::shared_ptr <gui::CWindow>	m_spWND;
		std::shared_ptr <gui::CMainUI>	m_spUI;
		std::shared_ptr <CI18N>			m_spI18N;
		std::shared_ptr <CSysCheck>		m_spSysCheck;
		mINI::INIFile					m_pConfigFile;
		mINI::INIStructure				m_pConfigContext;
	};
};
