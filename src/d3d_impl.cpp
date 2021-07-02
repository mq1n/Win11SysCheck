#include "../include/pch.hpp"
#include "../include/d3d_impl.hpp"
#include "../include/application.hpp"
#include "../include/log_helper.hpp"
#include "../include/fa-solid-900.h"
#include "../include/fa-brands-400.h"

#define ICON_MIN_FA 0xe005
#define ICON_MAX_FA 0xf8ff

namespace Win11SysCheck
{
	// cotr & dotr
	CD3DManager::CD3DManager() :
		m_pD3DEx(nullptr), m_pD3DDevEx(nullptr), m_bInitialized(false), m_hWnd(nullptr)
	{
		ZeroMemory(&m_pD3DParams, sizeof(m_pD3DParams));
	}
	CD3DManager::~CD3DManager()
	{
		CleanupDevice();
	}

	void CD3DManager::CreateDevice()
	{
		auto hRet = Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx);
		if (FAILED(hRet))
		{
			CLogHelper::Instance().LogError(fmt::format("Direct3DCreate9Ex fail! Error: %u Ret: %p", GetLastError(), hRet), true);
			return;
		}

		// Create the D3DDevice
		ZeroMemory(&m_pD3DParams, sizeof(m_pD3DParams));
		m_pD3DParams.Windowed = TRUE;
		m_pD3DParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
		m_pD3DParams.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
		m_pD3DParams.EnableAutoDepthStencil = TRUE;
		m_pD3DParams.AutoDepthStencilFormat = D3DFMT_D16;
		m_pD3DParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
		//m_pD3DParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate

		hRet = m_pD3DEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_pD3DParams, nullptr, &m_pD3DDevEx);
		if (FAILED(hRet))
		{
			CLogHelper::Instance().LogError(fmt::format("CreateDeviceEx fail! Error: %u Ret: %p", GetLastError(), hRet), true);
			return;
		}

		m_bInitialized = true;
	}

	void CD3DManager::CleanupDevice()
	{
		if (!m_bInitialized)
			return;
		m_bInitialized = false;

		if (m_pD3DDevEx)
		{
			m_pD3DDevEx->Release();
			m_pD3DDevEx = nullptr;
		}
		if (m_pD3DEx)
		{
			m_pD3DEx->Release();
			m_pD3DEx = nullptr;
		}
	}

	void CD3DManager::ResetDevice()
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();

		const auto hr = m_pD3DDevEx->Reset(&m_pD3DParams);
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);

		ImGui_ImplDX9_CreateDeviceObjects();
	}

	void CD3DManager::SetPosition(int width, int height)
	{
		if (m_pD3DDevEx)
		{
			m_pD3DParams.BackBufferWidth = width;
			m_pD3DParams.BackBufferHeight = height;

			ResetDevice();
		}
	}

	void CD3DManager::SetupImGui()
	{
		m_v4ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

		// ImGui_ImplWin32_EnableDpiAwareness();

		// Setup Dear ImGui context
		ImGui::CreateContext();

		auto& io = ImGui::GetIO();
		io.IniFilename = nullptr;
		io.ConfigWindowsMoveFromTitleBarOnly = true;
		io.Fonts->AddFontDefault();

		static const ImWchar icons_ranges[] = {
			ICON_MIN_FA, ICON_MAX_FA,
			0
		};
		ImFontConfig imFontConfig;
		imFontConfig.MergeMode = true;

		io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 16.f, &imFontConfig, icons_ranges);
		io.Fonts->AddFontFromMemoryTTF((void*)fa_brands_400, sizeof(fa_brands_400), 16.f, &imFontConfig, icons_ranges);

		ImGui_ImplDX9_Init(m_pD3DDevEx);
		ImGui_ImplWin32_Init(m_hWnd);

		auto& ini = CApplication::Instance().GetConfigContext();
		const auto& dark_mode = ini["ui"]["dark_mode"];
		if (dark_mode == "1")
		{
			ImGui::StyleColorsDark();
			CApplication::Instance().GetUI()->SetDarkModeEnabled(true);
		}
		else
		{
			ImGui::StyleColorsLight();
			CApplication::Instance().GetUI()->SetDarkModeEnabled(false);
		}

		auto style = &ImGui::GetStyle();
		style->WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style->WindowPadding = ImVec2(15, 8);
		style->WindowRounding = 5.0f;
		style->FramePadding = ImVec2(5, 5);
		style->FrameRounding = 4.0f;
		style->ItemSpacing = ImVec2(12, 8);
		style->ItemInnerSpacing = ImVec2(8, 6);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 15.0f;
		style->ScrollbarRounding = 9.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;
	}
	void CD3DManager::CleanupImGui()
	{
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void CD3DManager::OnRenderFrame()
	{
		// Start the Dear ImGui frame
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Get Custom UI 
		static auto s_pMainUI = CApplication::Instance().GetUI();

		// Draw Custom UI
		if (s_pMainUI)
			s_pMainUI->OnDraw();

		// Check UI window status
		if (!s_pMainUI || (s_pMainUI && !s_pMainUI->IsOpen()))
			SendMessage(CApplication::Instance().GetWindow()->GetHandle(), WM_DESTROY, 0, 0);

		// Render
		ImGui::EndFrame();

		m_pD3DDevEx->SetRenderState(D3DRS_ZENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		m_pD3DDevEx->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

		const auto clear_col_dx = D3DCOLOR_RGBA((int)(m_v4ClearColor.x * m_v4ClearColor.w * 255.0f), (int)(m_v4ClearColor.y * m_v4ClearColor.w * 255.0f), (int)(m_v4ClearColor.z * m_v4ClearColor.w * 255.0f), (int)(m_v4ClearColor.w * 255.0f));
		m_pD3DDevEx->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);

		if (m_pD3DDevEx->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			m_pD3DDevEx->EndScene();
		}

		// Handle loss of D3D9 device
		const auto hr = m_pD3DDevEx->Present(nullptr, nullptr, nullptr, nullptr);
		if (hr == D3DERR_DEVICELOST && m_pD3DDevEx->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}
};
