#include "../include/pch.hpp"
#include "../include/log_helper.hpp"
#include "../include/basic_log.hpp"
#include "../include/error_handler.hpp"

namespace Win11SysCheck
{
	static void LogErrorHandler(const std::string& stMessage)
	{
		Logf(CUSTOM_LOG_ERROR_FILENAME, "Log error handled: %s\n", stMessage.c_str());
		MessageBoxA(0, stMessage.c_str(), "Log manager initilization fail!", 0);
		abort();
	}

	CLogHelper::CLogHelper(const std::string& stLoggerName, const std::string& stFileName) :
		m_stLoggerName(stLoggerName), m_stFileName(stFileName)
	{
		try
		{
			auto sinks = std::vector<spdlog::sink_ptr>();

			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
			sinks.push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(m_stFileName.c_str()));

			m_pkLoggerImpl = std::make_shared<spdlog::logger>(m_stLoggerName.c_str(), sinks.begin(), sinks.end());
			m_pkLoggerImpl->set_error_handler(LogErrorHandler);
		}
		catch (const spdlog::spdlog_ex & ex)
		{
			Logf(CUSTOM_LOG_ERROR_FILENAME, "Exception throw on InitLogger (spdlog::spdlog_ex): %s\n", ex.what());
			abort();
		}
		catch (uint32_t dwNumber)
		{
			Logf(CUSTOM_LOG_ERROR_FILENAME, "Exception throw on InitLogger (w/ number): %p\n", dwNumber);
			abort();
		}
		catch (...)
		{
			Logf(CUSTOM_LOG_ERROR_FILENAME, "Exception throw on InitLogger (w/o information!)\n");
			abort();
		}
	}

	void CLogHelper::Log(int32_t nLevel, const std::string& stBuffer)
	{
		try
		{
			switch (nLevel)
			{
			case LL_SYS:
				m_pkLoggerImpl->info(stBuffer.c_str());
				break;
			case LL_CRI:
				m_pkLoggerImpl->critical(stBuffer.c_str());
				break;
			case LL_ERR:
				m_pkLoggerImpl->error(stBuffer.c_str());
				break;
			case LL_DEV:
				m_pkLoggerImpl->debug(stBuffer.c_str());
				break;
			case LL_TRACE:
				m_pkLoggerImpl->trace(stBuffer.c_str());
				break;
			case LL_WARN:
				m_pkLoggerImpl->warn(stBuffer.c_str());
				break;
			}
			m_pkLoggerImpl->flush();
		}
		catch (const spdlog::spdlog_ex & ex)
		{
			Logf(CUSTOM_LOG_ERROR_FILENAME, "Exception throw on sys_log (spdlog::spdlog_ex %s\n", ex.what());
			abort();
		}
		catch (uint32_t dwNumber)
		{
			Logf(CUSTOM_LOG_ERROR_FILENAME, "Exception throw on sys_log (w/ number): %p\n", dwNumber);
			abort();
		}
		catch (...)
		{
			Logf(CUSTOM_LOG_ERROR_FILENAME, "Exception throw on sys_log (w/o information!");
			abort();
		}
	}

	void CLogHelper::LogError(const std::string& stMessage, bool bFatal, bool bShowLastError)
	{
		const auto err = CErrorHandler{ stMessage, ::GetLastError() };

		auto msg = ""s;
		if (bShowLastError)
			msg = fmt::format("{0} (Code: {1})", err.what(), err.code());
		else
			msg = fmt::format("{0}", err.msg());

		Log(LL_ERR, msg.c_str());
		MessageBoxA(0, msg.c_str(), 0, MB_ICONERROR);

		if (bFatal)
			std::exit(EXIT_FAILURE);
	}
}