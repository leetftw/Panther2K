#pragma once

#include "PantherVersion.h"
#include <windows.h>

// Shows only basic program flow and errors
#define PANTHER_LL_BASIC 0
// Shows warnings and progress reports
#define PANTHER_LL_NORMAL 1
// Shows program flow and progress reports
#define PANTHER_LL_DETAILED 2
// Shows verbose progress informations
#define PANTHER_LL_VERBOSE 3

#define TRIM_CRLF(buffer) \
do { \
	int trim_crlf_len = lstrlenW(buffer); \
	while (trim_crlf_len > 0 && (buffer[trim_crlf_len - 1] == L'\n' || buffer[trim_crlf_len - 1] == L'\r')) \
		buffer[--trim_crlf_len] = L'\0'; } while(0)

#ifdef _DEBUG
#define wlogf(logger, level, buffersize, message, ...) \
do { \
	wchar_t wlogbuffer[buffersize]; \
	swprintf_s(wlogbuffer, message, __VA_ARGS__); \
	wchar_t wlogbuffer2[buffersize]; \
	swprintf_s(wlogbuffer2, L"%s [%s]", wlogbuffer, __FUNCTIONW__); \
	logger->Write(level, wlogbuffer2); } while(0)
#define wlogc(logger, level, message)  \
do { \
	const wchar_t* functionName = __FUNCTIONW__; \
	int bufferSize = (lstrlenW(message) + lstrlenW(functionName) + 4); \
	wchar_t* wlogbuffer = static_cast<wchar_t*>(safeMalloc(logger, bufferSize * sizeof(wchar_t))); \
	swprintf_s(wlogbuffer, bufferSize, L"%s [%s]", message, functionName); \
	logger->Write(level, wlogbuffer); \
	safeFree(logger, wlogbuffer); \
} while (0)

#else
#define wlogc(logger, level, message) logger->Write(level, message);
#define wlogf(logger, level, buffersize, message, ...) \
do { \
	wchar_t wlogbuffer[buffersize]; \
	swprintf_s(wlogbuffer, message, __VA_ARGS__); \
	logger->Write(level, wlogbuffer); } while(0)
#endif

#define wlogerr(logger, level, buffersize, format, code, ...) \
do { \
	wchar_t wlog_errbuffer[buffersize]; \
	wchar_t* wlog_errmessage; \
	size_t wlog_errsize = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, \
										code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPWSTR)&wlog_errmessage, 0, NULL); \
	if (wlog_errsize != 0) { \
		TRIM_CRLF(wlog_errmessage); \
		wlogf(logger, level, buffersize, format, wlog_errmessage, __VA_ARGS__); \
		LocalFree(wlog_errmessage); \
	} else logger->Write(level, L"Unable to format Win32 error message."); } while(0)

#define wloglerr(logger, level, buffersize, format, ...) wlogerr(logger, level, buffersize, format, GetLastError(), __VA_ARGS__)

namespace Leet
{
	namespace Panther2K
	{
		namespace Util
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

				// Sets the log level
				void SetLogLevel(int level);
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
	}
}

#if PANTHER_RELEASE_TYPE != PANTHER_RT_RELEASE
void* _stdcall safeMallocImpl(Leet::Panther2K::Util::Logger* logger, size_t size, const wchar_t* file, int line, const wchar_t* function);
#define safeMalloc(logger, size) safeMallocImpl(logger, size, __FILEW__, __LINE__, __FUNCTIONW__)
void* _stdcall safeLocalAllocImpl(Leet::Panther2K::Util::Logger* logger, size_t size, const wchar_t* file, int line, const wchar_t* function);
#define safeLocalAlloc(logger, size) safeLocalAllocImpl(logger, size, __FILEW__, __LINE__, __FUNCTIONW__)

void _stdcall safeRegisterNewImpl(Leet::Panther2K::Util::Logger* logger, void* ptr, size_t size, const wchar_t* file, int line, const wchar_t* function);
#define safeRegisterNew(logger) safeRegisterNewImpl(logger, this, sizeof(this), __FILEW__, __LINE__, __FUNCTIONW__)
void _stdcall safeRegisterDelete(Leet::Panther2K::Util::Logger* logger, void* ptr);


void _stdcall safeFree(Leet::Panther2K::Util::Logger* logger, void* ptr);
void _stdcall safeCleanup(Leet::Panther2K::Util::Logger* logger);
#else
#define safeMalloc(logger, size) malloc(size)
#define safeLocalAlloc(logger, size) LocalAlloc(LMEM_FIXED, size)

#define safeRegisterNew(logger) ((void)0)
#define safeRegisterDelete(logger, ptr) ((void)0)

#define safeFree(logger, ptr) free(ptr)
#define safeCleanup(logger) wlogc(logger, PANTHER_LL_BASIC, L"[Memory Manager] Production build, not performing memory cleanup.")
#endif