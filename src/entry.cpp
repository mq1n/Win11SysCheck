#include "../include/pch.hpp"
#include "../include/application.hpp"
#include "../include/log_helper.hpp"
#include "../include/d3d_impl.hpp"

volatile bool g_vbInterrupted{ false };

static void OnInterruptHandle(int signal)
{
	if (g_vbInterrupted)
		return;
	g_vbInterrupted = true;

	if (Win11SysCheck::CApplication::InstancePtr())
		Win11SysCheck::CApplication::Instance().Destroy();
}

int main(int, char**)
{
#ifndef _DEBUG
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	// Check ImGui version
	IMGUI_CHECKVERSION();

	// Setup signal handlers
	std::signal(SIGINT, &OnInterruptHandle);
	std::signal(SIGABRT, &OnInterruptHandle);
	std::signal(SIGTERM, &OnInterruptHandle);

	// Initialize instances
	static Win11SysCheck::CLogHelper s_kLogManager("Win11SysCheck", Win11SysCheck::CUSTOM_LOG_FILENAME);
	static Win11SysCheck::CD3DManager s_kD3DManager;
	static Win11SysCheck::CApplication s_kApplication(__argc, __argv);

	// Start to main routine
	return s_kApplication.MainRoutine();
}
