#pragma once

#include <fstream>
#include <iostream>
#include <filesystem>
#include <cinttypes>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <cstdarg>

#ifdef _WIN32
#include <wchar.h>
#include <windows.h>
#endif


/**
 * Check if we can enable virtual terminal (needed for ANSI escape sequences)
 * From: https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#example-of-enabling-virtual-terminal-processing
 */
static inline bool enableVirtualTerminal()
{
#ifdef _WIN32
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	if (hIn == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	DWORD dwOriginalOutMode = 0;
	DWORD dwOriginalInMode = 0;
	if (!GetConsoleMode(hOut, &dwOriginalOutMode))
	{
		return false;
	}
	if (!GetConsoleMode(hIn, &dwOriginalInMode))
	{
		return false;
	}

	DWORD dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
	DWORD dwRequestedInModes = ENABLE_VIRTUAL_TERMINAL_INPUT;

	DWORD dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
	if (!SetConsoleMode(hOut, dwOutMode))
	{
		// We failed to set both modes, try to step down mode gracefully.
		dwRequestedOutModes = ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		dwOutMode = dwOriginalOutMode | dwRequestedOutModes;
		if (!SetConsoleMode(hOut, dwOutMode))
		{
			// Failed to set any VT mode, can't do anything here.
			return false;
		}
	}

	DWORD dwInMode = dwOriginalInMode | dwRequestedInModes;
	if (!SetConsoleMode(hIn, dwInMode))
	{
		// Failed to set VT input mode, can't do anything here.
		return false;
	}
#endif
	return true;
}


namespace Styling
{
	static bool g_isVirtual = enableVirtualTerminal();

	static inline constexpr uint32_t reset				= 0;
	static inline constexpr uint32_t bold				= 1;
	static inline constexpr uint32_t dim				= 1 << 1;
	static inline constexpr uint32_t italic				= 1 << 2;
	static inline constexpr uint32_t underline			= 1 << 3;
	static inline constexpr uint32_t strikeout			= 1 << 4;
	static inline constexpr uint32_t normal				= 1 << 5;
	static inline constexpr uint32_t black				= 1 << 6;
	static inline constexpr uint32_t red				= 1 << 7;
	static inline constexpr uint32_t green				= 1 << 8;
	static inline constexpr uint32_t yellow				= 1 << 9;
	static inline constexpr uint32_t blue				= 1 << 10;
	static inline constexpr uint32_t magenta			= 1 << 11;
	static inline constexpr uint32_t cyan				= 1 << 12;
	static inline constexpr uint32_t white				= 1 << 13;
	static inline constexpr uint32_t brightBlack		= 1 << 14;
	static inline constexpr uint32_t brightRed			= 1 << 15;
	static inline constexpr uint32_t brightGreen		= 1 << 16;
	static inline constexpr uint32_t brightYellow		= 1 << 17;
	static inline constexpr uint32_t brightBlue			= 1 << 18;
	static inline constexpr uint32_t brightMagenta		= 1 << 19;
	static inline constexpr uint32_t brightCyan			= 1 << 20;
	static inline constexpr uint32_t brightWhite		= 1 << 21;

	static inline constexpr uint32_t debug		= brightBlack;
	static inline constexpr uint32_t info		= cyan;
	static inline constexpr uint32_t warning	= bold | yellow;
	static inline constexpr uint32_t error		= bold | red;
	static inline constexpr uint32_t success	= bold | green;


	static inline std::string style(uint32_t style = reset, bool noReset = false)
	{
		if (!g_isVirtual) return "";

		if (style == reset) return "\033[0m";

		std::string styleString = noReset ? "\033[" : "\033[0;";

		if (style & bold) styleString			+= "1;";
		if (style & dim) styleString			+= "2;";
		if (style & italic) styleString			+= "3;";
		if (style & underline) styleString		+= "4;";
		if (style & strikeout) styleString		+= "9;";
		if (style & normal) styleString			+= "22;";

		if (style & black) styleString			+= "30;";
		if (style & red) styleString			+= "31;";
		if (style & green) styleString			+= "32;";
		if (style & yellow) styleString			+= "33;";
		if (style & blue) styleString			+= "34;";
		if (style & magenta) styleString		+= "35;";
		if (style & cyan) styleString			+= "36;";
		if (style & white) styleString			+= "37;";

		if (style & brightBlack) styleString	+= "90;";
		if (style & brightRed) styleString		+= "91;";
		if (style & brightGreen) styleString	+= "92;";
		if (style & brightYellow) styleString	+= "93;";
		if (style & brightBlue) styleString		+= "94;";
		if (style & brightMagenta) styleString	+= "95;";
		if (style & brightCyan) styleString		+= "96;";
		if (style & brightWhite) styleString	+= "97;";

		if (!styleString.empty() && styleString.back() == ';')
			styleString.pop_back();

		return styleString + 'm';
	}
}


static inline std::string formattedDatetime(const char* fmt)
{
	std::stringstream buffer;
	time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm timeinfo;
#ifdef _WIN32
	localtime_s(&timeinfo, &t);
#else
	localtime_r(&t, &timeinfo);
#endif
	buffer << std::put_time(&timeinfo, fmt);
	return buffer.str();
}


namespace Logging
{
	enum class LogLevel
	{
		LOG_DEBUG,
		LOG_INFO,
		LOG_LOG,
		LOG_WARNING,
		LOG_ERROR,
	};

	static inline constexpr const char* getLogLevelName(LogLevel level)
	{
		switch (level)
		{
		case LogLevel::LOG_DEBUG:   return "DEBUG";
		case LogLevel::LOG_LOG:     return "";
		case LogLevel::LOG_INFO:    return "INFO";
		case LogLevel::LOG_WARNING: return "WARNING";
		case LogLevel::LOG_ERROR:   return "ERROR";
		}
		return "";
	}

#ifdef _DEBUG
	static inline constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::LOG_DEBUG;
#else
	static inline constexpr LogLevel DEFAULT_LOG_LEVEL = LogLevel::LOG_INFO;
#endif

	class LogHandler
	{
	protected:
		LogLevel m_loglevel;
	public:
		LogHandler() : m_loglevel(LogLevel::LOG_INFO) {}
		LogHandler(const LogLevel& loglevel) : m_loglevel(loglevel) {}
		void setLevel(const LogLevel& loglevel) { m_loglevel = loglevel; }
	};

	class ConsoleHandler : public LogHandler
	{
	public:
		using LogHandler::LogHandler;
		using LogHandler::setLevel;

		template <typename T> void log(const LogLevel& level, T message)
		{
			if (level < m_loglevel) { return; }

			std::ostream& os = (level > LogLevel::LOG_INFO) ? std::cerr : std::cout;
			auto* pOs = (level > LogLevel::LOG_INFO) ? stderr : stdout;

			const std::string levelName = std::string{ getLogLevelName(level) } + ':';

			if (!Styling::g_isVirtual)
			{
				if (level != LogLevel::LOG_LOG) fprintf(pOs, "%-9s", levelName.c_str());
				os << message << std::endl;
				return;
			}

			if (level != LogLevel::LOG_LOG)
			{
				os << Styling::style(Styling::bold);
				fprintf(pOs, "%-9s", levelName.c_str());
			}

			switch (level)
			{
			case LogLevel::LOG_LOG:
				break;
			case LogLevel::LOG_DEBUG:
				os << Styling::style(Styling::debug);
				break;
			case LogLevel::LOG_INFO:
				os << Styling::style(Styling::info);
				break;
			case LogLevel::LOG_WARNING:
				os << Styling::style(Styling::warning);
				break;
			case LogLevel::LOG_ERROR:
				os << Styling::style(Styling::error);
				break;
			}

			os << message << Styling::style() << std::endl;
		}
	};

	class FileHandler : public LogHandler
	{
	private:
		bool m_fileError = false;
		bool m_logdirChecked = false;
		bool m_logfileChecked = false;
		std::filesystem::path m_logdir;
		std::ofstream m_logfile;
	public:
		FileHandler(const std::filesystem::path& logdir, const LogLevel& loglevel) : m_logdir(logdir) { m_loglevel = loglevel; }
		FileHandler() : FileHandler("logs", LogLevel::LOG_WARNING) {}
		~FileHandler() { if (m_logfile.is_open()) m_logfile.close(); }
		using LogHandler::setLevel;
		void setLogDir(const std::filesystem::path& logdir) { m_logdir = logdir; }

		template <typename T> void log(const LogLevel& level, const std::string& loggerName, T message)
		{
			if (level < m_loglevel || m_fileError) { return; }

			if (!m_logdirChecked && !std::filesystem::exists(m_logdir) && !std::filesystem::is_directory(m_logdir))
			{
				if (!std::filesystem::create_directories(m_logdir)) {
					std::cerr << "###  Log Error: Could not create log directory \""
						<< std::filesystem::absolute(m_logdir).string() << "\"  ###" << std::endl;
					return;
				}
			}
			m_logdirChecked = true;

			if (!m_logfileChecked)
			{
				std::filesystem::path filepath = m_logdir / formattedDatetime("log_%F.txt");
				m_logfile.open(filepath, std::ios::app);
				if (!m_logfile.is_open() || !m_logfile.good())
				{
					m_logfile.close();
					m_fileError = true;
					std::cerr << "###  Log Error: Could not create/open log file \""
						<< std::filesystem::absolute(filepath).string() << "\"  ###" << std::endl;
				}
				m_logfileChecked = true;
			}

			m_logfile << formattedDatetime("[%FT%T]") << getLogLevelName(level) << "|"
				<< loggerName << "|" << message << std::endl;
		}
	};

	class Logger
	{
	public:
		Logger(const std::string& name) : m_name(name), m_loglevel(DEFAULT_LOG_LEVEL) {}
		Logger() : Logger("logger") {}
		Logger(const Logger&) = delete;

		void debug(const char* fmt, ...) { va_list args; va_start(args, fmt); _log(LogLevel::LOG_DEBUG, fmt, args); va_end(args); }
		void log(const char* fmt, ...) { va_list args; va_start(args, fmt); _log(LogLevel::LOG_LOG, fmt, args); va_end(args); }
		void info(const char* fmt, ...) { va_list args; va_start(args, fmt); _log(LogLevel::LOG_INFO, fmt, args); va_end(args); }
		void warning(const char* fmt, ...) { va_list args; va_start(args, fmt); _log(LogLevel::LOG_WARNING, fmt, args); va_end(args); }
		void warn(const char* fmt, ...) { va_list args; va_start(args, fmt); _log(LogLevel::LOG_WARNING, fmt, args); va_end(args); }
		void error(const char* fmt, ...) { va_list args; va_start(args, fmt); _log(LogLevel::LOG_ERROR, fmt, args); va_end(args); }
		template <typename T> void debug(T message) { _log(LogLevel::LOG_DEBUG, message); }
		template <typename T> void log(T message) { _log(LogLevel::LOG_LOG, message); }
		template <typename T> void info(T message) { _log(LogLevel::LOG_INFO, message); }
		template <typename T> void warning(T message) { _log(LogLevel::LOG_WARNING, message); }
		template <typename T> void warn(T message) { _log(LogLevel::LOG_WARNING, message); }
		template <typename T> void error(T message) { _log(LogLevel::LOG_ERROR, message); }

		std::string getName() const { return m_name; }
		void setLevel(const LogLevel& loglevel) { m_loglevel = loglevel; }
		void setConsoleHandlerLevel(const LogLevel& loglevel) { if (m_consoleHandler) m_consoleHandler->setLevel(loglevel); }
		void setFileHandlerLevel(const LogLevel& loglevel) { if (m_fileHandler) m_fileHandler->setLevel(loglevel); }
		void setFileHandlerLogDir(const std::filesystem::path& logDir) { m_fileHandler->setLogDir(logDir); }
		void setConsoleHandler(ConsoleHandler* handler) { m_consoleHandler = handler; }
		void setFileHandler(FileHandler* handler) { m_fileHandler = handler; }

		static Logger& getLogger(const std::string& loggerName)
		{
			if (Logger::s_loggers.contains(loggerName)) return *Logger::s_loggers[loggerName];
			Logger::s_loggers.insert(std::pair{ loggerName, std::make_unique<Logger>(loggerName) });
			return *Logger::s_loggers[loggerName];
		}
		static void setGlobalConsoleLevelDebug()
		{
			s_defaultConsoleHandler.setLevel(LogLevel::LOG_DEBUG);
			for (std::pair<const std::string, std::unique_ptr<Logger>>& kv : s_loggers)
			{
				kv.second->setLevel(LogLevel::LOG_DEBUG);
				kv.second->setConsoleHandlerLevel(LogLevel::LOG_DEBUG);
			}
		}
	private:
		static inline std::unordered_map<std::string, std::unique_ptr<Logger>> s_loggers{};
		static inline ConsoleHandler s_defaultConsoleHandler{ DEFAULT_LOG_LEVEL };
		static inline FileHandler s_defaultFileHandler;

		const std::string m_name;
		LogLevel m_loglevel = DEFAULT_LOG_LEVEL;
		ConsoleHandler* m_consoleHandler = &s_defaultConsoleHandler;
		FileHandler* m_fileHandler = &s_defaultFileHandler;

		void _log(const LogLevel& level, const char* fmt, va_list args)
		{
			if (level < m_loglevel) { return; }

			// Make a copy of the variable arguments list so we can test for length
			va_list args2;
			va_copy(args2, args);
			std::vector<char> buffer(vsnprintf(nullptr, 0, fmt, args2) + 1);
			va_end(args2);
			vsnprintf(&buffer[0], buffer.size(), fmt, args);
			std::string message{ &buffer[0] };

			if (m_consoleHandler) { m_consoleHandler->log(level, message); }
			if (m_fileHandler) { m_fileHandler->log(level, m_name, message); }
		}

		template <typename T> void _log(const LogLevel& level, T message)
		{
			if (level < m_loglevel) { return; }
			if (m_consoleHandler) { m_consoleHandler->log(level, message); }
			if (m_fileHandler) { m_fileHandler->log(level, m_name, message); }
		}
	};
}
