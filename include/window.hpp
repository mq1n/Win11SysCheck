#pragma once
#undef CreateWindow
#include "abstract_singleton.hpp"
#include "ui_types.hpp"
#include "widget.hpp"

namespace Win11SysCheck
{
	namespace gui
	{
		class CWindow : public CSingleton <CWindow>, public CWidget
		{
		public:
			CWindow();
			~CWindow();

			void Assemble(const std::string& stClassName, const std::string& stWindowName, rectangle rcRectangle, char* szIconName);
			void Destroy();

			bool HasMessage() const;
			bool PeekSingleMessage(MSG& msg) const;
			void PeekMessageLoop(std::function<void()> cb);

		protected:
			bool CreateClass(const std::string& stClassName, char* szIconName);
			bool CreateWindow(const std::string& stClassName, const std::string& stWindowName, rectangle& rcRectangle);

			void SetMessageHandlers();

		private:
			std::string m_stClassName;
		};
	}
};
