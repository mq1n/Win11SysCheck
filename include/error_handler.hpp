#pragma once
#include <Windows.h>

namespace Win11SysCheck
{
	class CErrorHandler
	{
		public:
			CErrorHandler(DWORD error_code = ::GetLastError()) :
				m_error_code(error_code), m_sys_err(std::system_error(std::error_code(m_error_code, std::system_category()))) {};
			CErrorHandler(const std::string& message, DWORD error_code = GetLastError()) :
				m_error_code(error_code), m_message(message), m_sys_err(std::system_error(std::error_code(m_error_code, std::system_category()), m_message)) {};
			~CErrorHandler() = default;

			CErrorHandler(const CErrorHandler&) = delete;
			CErrorHandler(CErrorHandler&&) noexcept = delete;
			CErrorHandler& operator=(const CErrorHandler&) = delete;
			CErrorHandler& operator=(CErrorHandler&&) noexcept = delete;
		
			auto code() const { return m_error_code; };
			auto msg() const { return m_message; };
			auto what() const { return m_sys_err.what(); }

		private:
			DWORD m_error_code;
			std::string m_message;
			std::system_error m_sys_err;
	};
};
