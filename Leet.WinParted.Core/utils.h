#pragma once
#include "Windows.h"

namespace Leet
{
	namespace WinParted
	{
		namespace Utils
		{
			struct Win32Handle {
				HANDLE h;
				Win32Handle(HANDLE h) : h(h) {}
				~Win32Handle() { if (h != INVALID_HANDLE_VALUE && h != nullptr) CloseHandle(h); }
				operator HANDLE() const { return h; }
			};

			long CalculateCRC32(char* data, unsigned long long len);
		}
	}
}