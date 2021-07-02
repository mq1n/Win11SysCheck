#pragma once
#include <Windows.h>
#include "ui_types.hpp"

namespace Win11SysCheck
{
	namespace gui
	{
		using TMessageFunction	= std::function <LRESULT(HWND, WPARAM, LPARAM)>;
		using TMessagePair		= std::pair <uint32_t, TMessageFunction>;

		class CWidget
		{
		public:
			CWidget(HWND hParentWnd = nullptr, HINSTANCE hInstance = nullptr, bool bIsWindow = false);
			~CWidget();

			bool Create(
				const std::string& stClassName, const std::string& stWidgetName, rectangle& rcRectangle,
				DWORD style, DWORD ex_style = 0, HMENU hMenu = nullptr
			);

			void AddMessageHandlers(const std::vector <TMessagePair>& vMessages);
			void RemoveMessageHandler(uint32_t message);

			bool SetText(const std::string& stText) const;
			void SetInstance(HINSTANCE hInstance);
			void SetVisible(bool bIsVisible);
			void SetPosition(int x, int y);
			void SetCenterPosition();
			void Show();
			void Hide();

			HWND GetHandle() const;
			HINSTANCE GetInstance() const;
			bool IsVisible() const;
			int GetScreenWidth() const;
			int GetScreenHeight() const;
			void GetWindowRect(RECT* pRC);
			void GetClientRect(RECT* pRC);
			void GetMousePosition(POINT* pPT);
			auto GetRect() const { return m_rcCurrRect; };

		public:
			static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
			static std::unordered_map <HWND, CWidget*> m_upWidgets;

		private:
			std::unordered_map <uint32_t, TMessageFunction> m_umCustomMessages;
			HFONT m_hFont;
			HWND m_hWnd;
			HWND m_hParentWnd;
			HINSTANCE m_hInstance;
			rectangle m_rcCurrRect;
			WNDPROC m_pOriginalWndProc;
			bool m_bIsWindow;
			bool m_bVisible;
		};
	}
};
