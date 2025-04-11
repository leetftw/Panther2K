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

/*
 * MEMORY TRACKING
 * Terrible memory management tools
 * Mainly to debug leaks
 */

#include <unordered_map>
#include <mutex>

typedef struct AllocationInfo
{
	const wchar_t* file;
	const wchar_t* function;
	int line;
	size_t size;
} AllocationInfo;

std::unordered_map<void*, AllocationInfo> allocations;
std::mutex allocMutex;

// 'Safe' malloc implementation
// Terminates any execution if memory cannot be allocated
// If possible an error is logged
void *safeMallocImpl(LibPanther::Logger* logger, size_t size, const wchar_t* file, int line, const wchar_t* function)
{
	void *returnValue = malloc(size);
	if (!returnValue) 
	{
		DWORD chars;
		const wchar_t* outOfMem = L"FATAL: OUT OF MEMORY.\r\n";
		if (logger) logger->WriteDirect(PANTHER_LL_BASIC, outOfMem);
		else WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), outOfMem, lstrlenW(outOfMem), &chars, NULL);
		ExitProcess(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY));
	}

	{
		std::lock_guard<std::mutex> lock(allocMutex);
		allocations[returnValue] = { file, function, line, size };
	}

	return returnValue;
}

void safeFree(LibPanther::Logger* logger, void* ptr)
{
	if (!ptr) return;
	{
		std::lock_guard<std::mutex> lock(allocMutex);
		auto it = allocations.find(ptr);
		if (it != allocations.end()) 
		{
			allocations.erase(it);
		}
		else if (logger) 
		{
			logger->Write(PANTHER_LL_BASIC, L"[Memory] WARNING: Attempt to free untracked pointer!");
		}
	}

	free(ptr);
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

void __cdecl safeCleanup(LibPanther::Logger* logger)
{
	wlogc(logger, PANTHER_LL_BASIC, L"[Memory] Safe allocation report.");
	wlogc(logger, PANTHER_LL_BASIC, L"================================");
	for (const auto& alloc : allocations) 
	{
		void* ptr = alloc.first;
		AllocationInfo info = alloc.second;

		wlogf(logger, PANTHER_LL_BASIC, MAX_PATH, L"Pointer not freed: Ptr: %016llX | Size: %08llX | Allocated at: %s:%d in function %s", (unsigned long long)ptr, info.size, info.file, info.line, info.function);
	}
}
