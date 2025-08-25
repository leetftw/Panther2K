#pragma once

#define PANTHER_RT_ALPHA 0
#define PANTHER_RT_BETA 1
#define PANTHER_RT_RELEASE 2

#define PANTHER_VERSION_MAJOR 2
#define PANTHER_VERSION_MINOR 0
#define PANTHER_VERSION_PATCH 0
#define PANTHER_VERSION_BUILD 4

#define PANTHER_RELEASE_TYPE PANTHER_RT_BETA

// Thanks GCC!
// https://gcc.gnu.org/onlinedocs/gcc-3.4.3/cpp/Stringification.html
#define exp_to_str(s) #s // Converts macro name to string
#define macro_to_str(s) exp_to_str(s) // Converts macro result to string

#if PANTHER_RELEASE_TYPE == PANTHER_RT_ALPHA
#define BASE_VER_STRING macro_to_str(PANTHER_VERSION_MAJOR) "." \
                        macro_to_str(PANTHER_VERSION_MINOR) "." \
                        macro_to_str(PANTHER_VERSION_PATCH) "a" \
                        macro_to_str(PANTHER_VERSION_BUILD)
#elif PANTHER_RELEASE_TYPE == PANTHER_RT_BETA
#define BASE_VER_STRING macro_to_str(PANTHER_VERSION_MAJOR) "." \
                        macro_to_str(PANTHER_VERSION_MINOR) "." \
                        macro_to_str(PANTHER_VERSION_PATCH) "b" \
                        macro_to_str(PANTHER_VERSION_BUILD)
#else
#define BASE_VER_STRING macro_to_str(PANTHER_VERSION_MAJOR) "." \
                        macro_to_str(PANTHER_VERSION_MINOR) "." \
                        macro_to_str(PANTHER_VERSION_PATCH)
#endif

#define PANTHER_VER_STRING L"Leet's Panther2K " BASE_VER_STRING
#define WINPARTED_VER_STRING L"Leet's WinParted " BASE_VER_STRING
