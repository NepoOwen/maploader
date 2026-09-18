#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // !WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>

struct AuthConfig {
	const char* field1;
	const char* field2;
    const char* field3;
};

struct PatternConfig {
	AuthConfig* config;
};