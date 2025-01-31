#pragma once

#include "Version.h"
#include <windows.h>

// Shows only basic program flow and errors
#define PANTHER_LL_BASIC 0
// Shows warnings and progress reports
#define PANTHER_LL_NORMAL 1
// Shows program flow and progress reports
#define PANTHER_LL_DETAILED 2
// Shows verbose progress informations
#define PANTHER_LL_VERBOSE 3

#define TRIM_CRLF(buffer) {int trim_crlf_len = lstrlenW(buffer); while (trim_crlf_len > 0 && (buffer[trim_crlf_len - 1] == L'\n' || buffer[trim_crlf_len - 1] == L'\r')) { buffer[--trim_crlf_len] = L'\0'; } }
#define wlogf(logger, level, buffersize, message, ...) { wchar_t wlogbuffer[buffersize]; swprintf_s(wlogbuffer, message, __VA_ARGS__); logger->Write(level, wlogbuffer); }
#define wloglerr(logger, level, buffersize, format) { wchar_t wlog_errbuffer[buffersize]; wchar_t* wlog_errmessage; size_t wlog_errsize = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&wlog_errmessage, 0, NULL); if (wlog_errsize != 0) { TRIM_CRLF(wlog_errmessage); swprintf_s(wlog_errbuffer, format, wlog_errmessage); LocalFree(wlog_errmessage); } else swprintf_s(wlog_errbuffer, L"Unable to format Win32 error message."); logger->Write(level, wlog_errbuffer); }
#define wlogerr(logger, level, buffersize, format, code, ...) { wchar_t wlog_errbuffer[buffersize]; wchar_t* wlog_errmessage; size_t wlog_errsize = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&wlog_errmessage, 0, NULL); if (wlog_errsize != 0) { TRIM_CRLF(wlog_errmessage); swprintf_s(wlog_errbuffer, format, wlog_errmessage, __VA_ARGS__); LocalFree(wlog_errmessage); } else swprintf_s(wlog_errbuffer, L"Unable to format Win32 error message."); logger->Write(level, wlog_errbuffer); }

namespace LibPanther
{
	class Logger
	{
	public:
		// Initialized the logger with the desired log level
		Logger(const wchar_t* fileName, int logLevel);
		~Logger();

		// Writes to the log file with formatted time and date information
		void Write(int level, const wchar_t* message);

		// Writes directly to the log file (uses as little memory as possible)
		void WriteDirect(int level, const wchar_t* message);

		// Retrieves the log level
		int GetLogLevel();
	private:
		wchar_t timeBuffer[100];
		wchar_t messageBuffer[512];
		HANDLE hLogFile;
		wchar_t szLogFile[MAX_PATH];
		int dwLogLevel;
		CRITICAL_SECTION cs;

		void formatTime();
	};
}

void* __cdecl safeMalloc(LibPanther::Logger* logger, size_t size);
void* __cdecl safeLocalAlloc(LibPanther::Logger* logger, size_t size);