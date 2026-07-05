#pragma once
#include <Windows.h>
struct Win32Handle {
    HANDLE h;
    Win32Handle(HANDLE h) : h(h) {}
    ~Win32Handle() { if (h != INVALID_HANDLE_VALUE && h != nullptr) CloseHandle(h); }
    operator HANDLE() const { return h; }
};