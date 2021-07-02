#pragma once
#include "abstract_singleton.hpp"
#include "basic_log.hpp"
#include <mutex>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace Win11SysCheck
{
	static constexpr auto CUSTOM_LOG_FILENAME = "Win11SysCheck.log";
	static constexpr auto CUSTOM_LOG_ERROR_FILENAME = "Win11SysCheckError.log";

	enum ELogLevels
	{
		LL_SYS,
		LL_ERR,
		LL_CRI,
		LL_WARN,
		LL_DEV,
		LL_TRACE
	};

	class CLogHelper : public CSingleton <CLogHelper>
	{
	public:
		CLogHelper() = default;
		CLogHelper(const std::string& stLoggerName, const std::string& stFileName);

		void Log(int32_t nLevel, const std::string& stBuffer);
		void LogError(const std::string& stMessage, bool bFatal = true, bool bShowLastError = true);

	private:
		mutable std::recursive_mutex		m_pkMtMutex;

		std::shared_ptr <spdlog::logger>	m_pkLoggerImpl;
		std::string							m_stLoggerName;
		std::string							m_stFileName;
	};
}
