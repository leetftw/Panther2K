#include "Logger.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

const wchar_t* levelNames[4] = {
	L"  BASIC",
	L" NORMAL",
	L" DETAIL",
	L"VERBOSE"
};

namespace LibPanther 
{
	Logger::Logger(const wchar_t* fileName, int outputLevel)
	{
		DWORD chars;
		dwLogLevel = outputLevel;
		wcscpy_s(szLogFile, fileName);
		//lstrcpyW(szLogFile, fileName);

		hLogFile = CreateFileW(szLogFile, GENERIC_WRITE | FILE_GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hLogFile == INVALID_HANDLE_VALUE)
		{
			const wchar_t* output = L"Logger failed to initialize.";
			WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), output, lstrlenW(output), &chars, NULL);
			return;
		}

		InitializeCriticalSection(&cs);

		// Write LE UTF-16 byte order mark
		WriteFile(hLogFile, "ÿþ", 2, &chars, NULL);
	}

	Logger::~Logger()
	{
		if (hLogFile != INVALID_HANDLE_VALUE)
			CloseHandle(hLogFile);
		DeleteCriticalSection(&cs);
	}

	void Logger::Write(int level, const wchar_t* message)
	{
		if (dwLogLevel < level)
			return;
		
		//formatTime();
		swprintf(messageBuffer, 512, L"%s[%s] %s\r\n", timeBuffer, levelNames[level], message);
		WriteDirect(level, messageBuffer);
	}
	
	void Logger::WriteDirect(int level, const wchar_t* message)
	{
		DWORD chars;
		if (dwLogLevel < level)
			return;

		EnterCriticalSection(&cs);

		WriteFile(hLogFile, message, lstrlenW(message) * sizeof(wchar_t), &chars, NULL);
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), message, lstrlenW(message), &chars, NULL);

		LeaveCriticalSection(&cs);
	}

	int Logger::GetLogLevel()
	{
		return dwLogLevel;
	}

	void Logger::formatTime()
	{
		time_t tTime = time(NULL);
		tm lTime;
		localtime_s(&lTime, &tTime);
		wcsftime(timeBuffer, 100, L"", &lTime);
	}
}

// 'Safe' malloc implementation
// Terminates any execution if memory cannot be allocated
// If possible an error is logged
void *safeMalloc(LibPanther::Logger* logger, size_t size)
{
	void *returnValue = malloc(size);
	if (!returnValue) 
	{
		logger->WriteDirect(PANTHER_LL_BASIC, L"FATAL: OUT OF MEMORY.\r\n");
		ExitProcess(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY));
	}
	return returnValue;
}

void* safeLocalAlloc(LibPanther::Logger* logger, size_t size)
{
	void* returnValue = LocalAlloc(LPTR, size);
	if (!returnValue)
	{
		logger->WriteDirect(PANTHER_LL_BASIC, L"FATAL: OUT OF MEMORY.\r\n");
		ExitProcess(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY));
	}
	return returnValue;
}