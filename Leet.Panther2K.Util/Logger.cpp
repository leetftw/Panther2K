#include "include/PantherLogger.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

const wchar_t* levelNames[4] = {
	L"  BASIC",
	L" NORMAL",
	L" DETAIL",
	L"VERBOSE"
};

using namespace Leet::Panther2K::Util;

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

void Logger::SetLogLevel(int level)
{
	if (level < PANTHER_LL_BASIC || level > PANTHER_LL_VERBOSE || level == dwLogLevel)
		return;

	switch (level)
	{
		case PANTHER_LL_BASIC:
			WriteDirect(PANTHER_LL_BASIC, L"[Logger] Setting log level to BASIC.\r\n");
			break;
		case PANTHER_LL_NORMAL:
			WriteDirect(PANTHER_LL_BASIC, L"[Logger] Setting log level to NORMAL.\r\n");
			break;
		case PANTHER_LL_DETAILED:
			WriteDirect(PANTHER_LL_BASIC, L"[Logger] Setting log level to DETAILED.\r\n");
			break;
		case PANTHER_LL_VERBOSE:
			WriteDirect(PANTHER_LL_BASIC, L"[Logger] Setting log level to VERBOSE.\r\n");
			break;
	}
	dwLogLevel = level;
}

void Logger::formatTime()
{
	time_t tTime = time(NULL);
	tm lTime;
	localtime_s(&lTime, &tTime);
	wcsftime(timeBuffer, 100, L"", &lTime);
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
	const wchar_t* allocator;
} AllocationInfo;

std::unordered_map<void*, AllocationInfo> allocations;
std::mutex allocMutex;

// 'Safe' malloc implementation
// Terminates any execution if memory cannot be allocated
// If possible an error is logged
void* _stdcall safeMallocImpl(Logger* logger, size_t size, const wchar_t* file, int line, const wchar_t* function)
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
		allocations[returnValue] = { file, function, line, size, L"malloc" };
	}

	return returnValue;
}

#if PANTHER_RELEASE_TYPE != PANTHER_RT_RELEASE
void _stdcall safeFree(Logger* logger, void* ptr)
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
			logger->Write(PANTHER_LL_BASIC, L"[Memory Manager] WARNING: Attempt to free untracked pointer!");
		}
	}

	free(ptr);
}
#endif

void* _stdcall safeLocalAllocImpl(Logger* logger, size_t size, const wchar_t* file, int line, const wchar_t* function)
{
	void* returnValue = LocalAlloc(LPTR, size);
	if (!returnValue)
	{
		logger->WriteDirect(PANTHER_LL_BASIC, L"FATAL: OUT OF MEMORY.\r\n");
		ExitProcess(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_ENOUGH_MEMORY));
	}

	{
		std::lock_guard<std::mutex> lock(allocMutex);
		allocations[returnValue] = { file, function, line, size, L"LocalAlloc" };
	}

	return returnValue;
}

void _stdcall safeRegisterNewImpl(Leet::Panther2K::Util::Logger* logger, void* ptr, size_t size, const wchar_t* file, int line, const wchar_t* function)
{
	std::lock_guard<std::mutex> lock(allocMutex);
	allocations[ptr] = { file, function, line, size, L"new" };
}

#if PANTHER_RELEASE_TYPE != PANTHER_RT_RELEASE
void _stdcall safeRegisterDelete(Leet::Panther2K::Util::Logger* logger, void* ptr)
{
	std::lock_guard<std::mutex> lock(allocMutex);
	auto it = allocations.find(ptr);
	if (it != allocations.end())
	{
		allocations.erase(it);
	}
	else if (logger)
	{
		logger->Write(PANTHER_LL_BASIC, L"[Memory Manager] WARNING: Attempt to delete untracked class!");
	}
}

void _stdcall safeCleanup(Logger* logger)
{
	wlogc(logger, PANTHER_LL_BASIC, L"[Memory Manager] Safe allocation report");
	wlogc(logger, PANTHER_LL_BASIC, L"========================================");
	for (const auto& alloc : allocations) 
	{
		void* ptr = alloc.first;
		AllocationInfo info = alloc.second;

		if (wcscmp(info.allocator, L"new"))
		{
#if defined(_M_X64)
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"Pointer not freed: Ptr: %016llX | Size: %08llX | Allocated at: %s:%d in function %s | Allocator: %s", (unsigned long long)ptr, info.size, info.file, info.line, info.function, info.allocator);
#elif defined(_M_IX86)
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"Pointer not freed: Ptr: %08X | Size: %08X | Allocated at: %s:%d in function %s | Allocator: %s", (unsigned long)ptr, info.size, info.file, info.line, info.function, info.allocator);
#else
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"Pointer not freed: Ptr/size: (architecture ptr and size_t width unknown) | Allocated at: %s:%d in function %s | Allocator: %s", info.file, info.line, info.function, info.allocator);
#endif
		}
		else
		{
#if defined(_M_X64)
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"Object not freed: Ptr: %016llX | Size: %08llX | Type: %s", (unsigned long long)ptr, info.size, info.function);
#elif defined(_M_IX86)
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"Object not freed: Ptr: %08X | Size: %08X | Type: %s", (unsigned long long)ptr, info.size, info.function);
#else
			wlogf(logger, PANTHER_LL_BASIC, MAX_PATH * 2, L"Object not freed: Ptr/size: (architecture ptr and size_t width unknown) | Type: %s", info.function);
#endif
		}
	}
}
#endif