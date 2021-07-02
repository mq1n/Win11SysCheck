#include "../include/pch.hpp"
#include "../include/application.hpp"
#include "../include/window.hpp"
#include "../include/log_helper.hpp"
#include "../include/d3d_impl.hpp"

namespace Win11SysCheck
{
	// cotr & dotr
	CApplication::CApplication(int, char**) :
		m_pConfigFile(mINI::INIFile(CONFIG_FILE_NAME))
	{
		CLogHelper::Instance().Log(LL_SYS, fmt::format("Win11SysCheck started! Build: {} {}", __DATE__, __TIME__));
	}
	CApplication::~CApplication()
	{
	}

	// main routines
	void CApplication::Destroy()
	{
		if (m_spI18N)
			m_spI18N->Destroy();

		if (m_spSysCheck)
			m_spSysCheck->Destroy();

		if (m_spWND)
			m_spWND->Destroy();
	}

	int CApplication::MainRoutine()
	{
		auto nRetCode = EXIT_FAILURE;

		// Declare
		m_spWND = std::make_shared<gui::CWindow>();
		assert(m_spWND || m_spWND.get());

		m_spUI = std::make_shared<gui::CMainUI>();
		assert(m_spUI || m_spUI.get());

		m_spI18N = std::make_shared<CI18N>();
		assert(m_spI18N || m_spI18N.get());

		m_spSysCheck = std::make_shared<CSysCheck>();
		assert(m_spSysCheck || m_spSysCheck.get());

		// Initialize
		do
		{
			/*
			BOOL Wow64Process = FALSE;
			if (IsWow64Process(GetCurrentProcess(), &Wow64Process) && Wow64Process)
			{
				CLogHelper::Instance().Log(LL_ERR, "This process cannot be run under Wow64.");
				break;
			}
			*/

			if (!std::filesystem::exists(CONFIG_FILE_NAME))
			{
				CLogHelper::Instance().Log(LL_WARN, "Config file does not exist!");

				std::ofstream ofs(CONFIG_FILE_NAME);
				if (!ofs)
				{
					CLogHelper::Instance().Log(LL_ERR, "Config file create failed!");
					break;
				}
				ofs << "";
				ofs.close();
			}

			if (!m_pConfigFile.read(m_pConfigContext))
			{
				CLogHelper::Instance().Log(LL_ERR, "Config file read failed!");
				break;
			}

			// Setup I18N
			if (!m_spI18N->Initialize())
			{
				CLogHelper::Instance().Log(LL_ERR, "I18N initialization failed!");
				break;
			}

			// Setup UI
			m_spWND->SetInstance(GetModuleHandle(0));
			m_spWND->Assemble("Win11SysCheck", "Win11SysCheck", gui::rectangle(0, 0, 680, 540), nullptr);

			CD3DManager::Instance().SetWindow(m_spWND->GetHandle());
			CD3DManager::Instance().CreateDevice();
			CD3DManager::Instance().SetupImGui();

			// Show main windows container window
			::ShowWindow(m_spWND->GetHandle(), SW_SHOW);
			::UpdateWindow(m_spWND->GetHandle());

			// Setup updater
			if (!m_spSysCheck->Initialize())
			{
				CLogHelper::Instance().Log(LL_ERR, "SysCheck initialization failed!");
				break;
			}

			// Main loop	
			m_spWND->PeekMessageLoop([]() {
				CD3DManager::Instance().OnRenderFrame();
			});

			nRetCode = EXIT_SUCCESS;
		} while (FALSE);

		// End-of-loop cleanup
		Destroy();

		return nRetCode;
	}

	bool CApplication::OpenWebPage(const std::string& stAddress)
	{
		// Initialize COM before calling ShellExecute().
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		// Execute process
		const auto iExecuteRet = (INT_PTR)ShellExecuteA(nullptr, "open", stAddress.c_str(), nullptr, nullptr, SW_HIDE);
		if (iExecuteRet <= 32) // If the function succeeds, it returns a value greater than 32.
		{
			CLogHelper::Instance().Log(LL_ERR, fmt::format("ShellExecuteA failed with status: {0} error: {1}", iExecuteRet, GetLastError()));
			return false;
		}

		return true;
	}
};
