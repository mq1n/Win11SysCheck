#include "../include/pch.hpp"
#include "../include/widget.hpp"
#include "../include/window.hpp"
#include "../include/d3d_impl.hpp"
#include "../include/log_helper.hpp"

namespace Win11SysCheck
{
	namespace gui
	{
		CWindow::CWindow() :
			CWidget(0, 0, true)
		{
		}	
		CWindow::~CWindow()
		{
			Destroy();
		}

		void CWindow::Assemble(const std::string& stClassName, const std::string& stWindowName, rectangle rcRectangle, char* szIconName)
		{
			m_stClassName = stClassName;

			if (!CreateClass(stClassName, szIconName))
			{
				CLogHelper::Instance().LogError("create_class fail!", true);
				return;
			}

			if (!CreateWindow(stClassName, stWindowName, rcRectangle))
			{
				CLogHelper::Instance().LogError("create_window fail!", true);
				return;
			}
			SetWindowLongPtr(GetHandle(), GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

			SetMessageHandlers();
		}

		void CWindow::Destroy()
		{
			if (!m_stClassName.empty())
			{
				UnregisterClass(m_stClassName.c_str(), GetInstance());
				m_stClassName.clear();
			}
			// DestroyWindow will call than 'widget' destructor
		}

		bool CWindow::HasMessage() const
		{
			MSG msg{};
			if (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
				return false;

			return true;
		}

		bool CWindow::PeekSingleMessage(MSG& msg) const
		{
			if (!PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
				return false;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
			return true;
		}
		void CWindow::PeekMessageLoop(std::function<void()> cb)
		{
			MSG msg{};
			while (true)
			{
				while (PeekSingleMessage(msg));
				if (msg.message == WM_QUIT)
					break;

				if (cb)
					cb();
			}
		}

		bool CWindow::CreateClass(const std::string& stClassName, char* szIconName)
		{
			WNDCLASSEXA wcEx{};
			ZeroMemory(&wcEx, sizeof(wcEx));

			wcEx.cbSize = sizeof(wcEx);
			wcEx.style = CS_CLASSDC;
			wcEx.lpfnWndProc = CWindow::WndProc;
			wcEx.hInstance = GetInstance();
			wcEx.hCursor = LoadCursorA(NULL, IDC_ARROW);
			wcEx.lpszClassName = stClassName.c_str();
			wcEx.hIcon = LoadIconA(GetInstance(), szIconName);
			wcEx.hIconSm = LoadIconA(GetInstance(), szIconName);

			return !!RegisterClassExA(&wcEx);
		}

		bool CWindow::CreateWindow(const std::string& stClassName, const std::string& stWindowName, rectangle& rcRectangle)
		{
			if (!rcRectangle.x && !rcRectangle.y)
			{
				RECT rc_system{ 0 };
				SystemParametersInfoA(SPI_GETWORKAREA, 0, &rc_system, 0);

				// center screen
				rcRectangle.x = (rc_system.right / 2) - (rcRectangle.width / 2);
				rcRectangle.y = (rc_system.bottom / 2) - (rcRectangle.height / 2);
			}

			return Create(stClassName, stWindowName, rcRectangle, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME, 0, NULL);
		}

		void CWindow::SetMessageHandlers()
		{
			AddMessageHandlers({
				TMessagePair(WM_CLOSE, [](HWND hWnd, WPARAM, LPARAM) -> LRESULT
				{
					DestroyWindow(hWnd);
					return 0;
				}),	
				TMessagePair(WM_DESTROY, [](HWND, WPARAM, LPARAM) -> LRESULT
				{
					PostQuitMessage(0);
					return 0;
				}),
				TMessagePair(WM_SIZE, [](HWND, WPARAM wParam, LPARAM lParam) -> LRESULT
				{
					if (wParam != SIZE_MINIMIZED)
					{
						CD3DManager::Instance().SetPosition(LOWORD(lParam), HIWORD(lParam));
					}
					return 0;
				})
			});
		}
	}
};
