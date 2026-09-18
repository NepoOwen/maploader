#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // !WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h> // for printf()
#include <cstdlib> // for system()

struct AuthConfig {
	const char* field1;
	const char* field2;
	const char* field3;
};

struct PatternConfig {
	AuthConfig* config;
};

static void CreateDebugConsole() {
	AllocConsole();
	FILE* fp;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
	SetConsoleTitle(L"Debug Console");
}

static void DestroyDebugConsole() {
	FILE* fp;
	freopen_s(&fp, "NUL", "w", stdout);
	freopen_s(&fp, "NUL", "w", stderr);
	freopen_s(&fp, "NUL", "r", stdin);
	FreeConsole();
}

DWORD WINAPI startup(LPVOID lpParam) {
	auto* config = static_cast<PatternConfig*>(lpParam);

	CreateDebugConsole();
	printf("Field1: %s\n", config->config->field1);
	printf("Field2: %s\n", config->config->field2);
	printf("Field3: %s\n", config->config->field3);
	system("pause");
	DestroyDebugConsole();

    return 0;
}

extern "C" __declspec(dllexport) BOOL WINAPI clientdll(HMODULE hModule, void* config = nullptr) {
	HANDLE thread = CreateThread(nullptr, 0, startup, config, 0, nullptr);
	if (thread) { CloseHandle(thread); return 1; }
	return 0;
}