#include "../include/pch.hpp"
#include "../include/widget.hpp"
#include "../include/log_helper.hpp"
#include <imgui_impl_win32.h>

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Win11SysCheck
{
	namespace gui
	{
		std::unordered_map <HWND, CWidget*> CWidget::m_upWidgets;

		LRESULT CALLBACK CWidget::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
				return true;

			const auto current_widget = CWidget::m_upWidgets[hWnd];
			if (current_widget)
			{
				if (current_widget->m_umCustomMessages[message])
				{
					return current_widget->m_umCustomMessages[message](hWnd, wParam, lParam);
				}
				else
				{
					if (current_widget->m_bIsWindow)
					{
						return DefWindowProc(hWnd, message, wParam, lParam);
					}
					else
					{
						return CallWindowProc(current_widget->m_pOriginalWndProc, hWnd, message, wParam, lParam);
					}
				}
			}

			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		CWidget::CWidget(HWND hParentWnd, HINSTANCE hInstance, bool bIsWindow)
			: m_hFont(nullptr), m_hWnd(nullptr), m_hParentWnd(hParentWnd), m_pOriginalWndProc(nullptr), m_bIsWindow(bIsWindow), m_bVisible(true)
		{
			m_hInstance = (hInstance ? hInstance : reinterpret_cast<HINSTANCE>(GetWindowLongPtrA(hParentWnd, GWLP_HINSTANCE)));
		}
		CWidget::~CWidget()
		{
			if (m_hWnd)
			{
				m_upWidgets.erase(m_hWnd);
				if (IsWindow(m_hWnd))
					DestroyWindow(m_hWnd);

				m_hWnd = nullptr;
			}

			if (m_hFont)
			{
				DeleteObject(m_hFont);
				m_hFont = nullptr;
			}

			m_bVisible = false;
		}

		bool CWidget::Create(const std::string& stClassName, const std::string& stWidgetName, rectangle& rcRectangle, DWORD dwStyle, DWORD dwExStyle, HMENU hMenu)
		{
			m_hWnd = CreateWindowExA(
				dwExStyle, stClassName.c_str(), stWidgetName.c_str(), dwStyle,
				rcRectangle.x, rcRectangle.y, rcRectangle.width, rcRectangle.height,
				m_hParentWnd, hMenu, m_hInstance, nullptr
			);
			if (!m_hWnd)
			{
				CLogHelper::Instance().LogError("CreateWindow fail!", true);
				return false;
			}

			m_hFont = CreateFontA(13, 0, 0, 0, FW_DONTCARE, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 5, DEFAULT_PITCH | FF_DONTCARE, "Tahoma");
			if (!m_hFont)
			{
				CLogHelper::Instance().LogError("CreateFontA fail!", true);
				return false;
			}

			SendMessage(m_hWnd, WM_SETFONT, reinterpret_cast<WPARAM>(m_hFont), TRUE);

			if (!m_bIsWindow)
			{
				m_pOriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(CWidget::WndProc)));
			}

			m_rcCurrRect = rcRectangle;
			m_upWidgets[m_hWnd] = this;
			return true;
		}

		void CWidget::AddMessageHandlers(const std::vector <TMessagePair>& vMessages)
		{
			for (const auto& msg : vMessages)
			{
				m_umCustomMessages[msg.first] = msg.second;
			}
		}
		void CWidget::RemoveMessageHandler(uint32_t message)
		{
			m_umCustomMessages.erase(message);
		}

		bool CWidget::SetText(const std::string& stText) const
		{
			return (SetWindowTextA(m_hWnd, stText.c_str()) != FALSE);
		}

		HWND CWidget::GetHandle() const
		{
			return m_hWnd;
		}

		void CWidget::SetInstance(HINSTANCE hInstance)
		{
			m_hInstance = hInstance;
		}
		HINSTANCE CWidget::GetInstance() const
		{
			return m_hInstance;
		}

		void CWidget::SetVisible(bool bIsVisible)
		{
			m_bVisible = bIsVisible;

			if (m_bVisible)
				ShowWindow(m_hWnd, SW_SHOW);
			else
				ShowWindow(m_hWnd, SW_HIDE);
		}
		bool CWidget::IsVisible() const
		{
			return m_bVisible;
		}

		void CWidget::Show()
		{
			m_bVisible = true;
			ShowWindow(m_hWnd, SW_SHOW);
		}
		void CWidget::Hide()
		{
			m_bVisible = false;
			ShowWindow(m_hWnd, SW_HIDE);
		}

		int	CWidget::GetScreenWidth() const
		{
			return GetSystemMetrics(SM_CXSCREEN);
		}
		int	CWidget::GetScreenHeight() const
		{
			return GetSystemMetrics(SM_CYSCREEN);
		}

		void CWidget::GetWindowRect(RECT* pRC)
		{
			::GetWindowRect(m_hWnd, pRC);
		}
		void CWidget::GetClientRect(RECT* pRC)
		{
			::GetClientRect(m_hWnd, pRC);
		}

		void CWidget::GetMousePosition(POINT* pPT)
		{
			GetCursorPos(pPT);
			ScreenToClient(m_hWnd, pPT);
		}

		void CWidget::SetPosition(int x, int y)
		{
			SetWindowPos(m_hWnd, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		}
		void CWidget::SetCenterPosition()
		{
			RECT rc{ 0 };
			GetClientRect(&rc);

			const auto nWindowWidth = rc.right - rc.left;
			const auto nWindowHeight = rc.bottom - rc.top;

			SetPosition((GetScreenWidth() - nWindowWidth) / 2, (GetScreenHeight() - nWindowHeight) / 2);
		}
	}
};
