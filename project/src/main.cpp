#include "injector.hpp"
#include "base64.hpp"
#include "embed.hpp"
#include <vector>

bool IsCorrectTargetArchitecture(HANDLE h) {
	BOOL t = FALSE, s = FALSE;
	return IsWow64Process(h, &t) && IsWow64Process(GetCurrentProcess(), &s) && (t == s);
}

AuthConfig* AllocConfig(HANDLE h, std::vector<LPVOID>& m, const char* config, const char* token, const char* auth) {
	SIZE_T configLen = config ? strlen(config) + 1 : 0;
	SIZE_T tokenLen = token ? strlen(token) + 1 : 0;
	SIZE_T authLen = auth ? strlen(auth) + 1 : 0;
	SIZE_T totalSize = configLen + tokenLen + authLen;

	if (totalSize == 0) return nullptr;

	LPVOID stringBuffer = VirtualAllocEx(h, nullptr, totalSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!stringBuffer) return nullptr;
	m.push_back(stringBuffer);

	AuthConfig cfg = { 0 };
	SIZE_T offset = 0;

	if (configLen > 0) {
		cfg.field1 = (const char*)((BYTE*)stringBuffer + offset);
		WriteProcessMemory(h, (LPVOID)cfg.field1, config, configLen, nullptr);
		offset += configLen;
	}

	if (tokenLen > 0) {
		cfg.field2 = (const char*)((BYTE*)stringBuffer + offset);
		WriteProcessMemory(h, (LPVOID)cfg.field2, token, tokenLen, nullptr);
		offset += tokenLen;
	}

	if (authLen > 0) {
		cfg.field3 = (const char*)((BYTE*)stringBuffer + offset);
		WriteProcessMemory(h, (LPVOID)cfg.field3, auth, authLen, nullptr);
		offset += authLen;
	}

	AuthConfig* remoteCfg = (AuthConfig*)VirtualAllocEx(h, nullptr, sizeof(AuthConfig), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteCfg) return nullptr;
	m.push_back(remoteCfg);

	WriteProcessMemory(h, remoteCfg, &cfg, sizeof(AuthConfig), nullptr);
	return remoteCfg;
}

PatternConfig* AllocPatterns(HANDLE h, std::vector<LPVOID>& m, const char* config, const char* token, const char* auth) {
	AuthConfig* cfg = AllocConfig(h, m, config, token, auth);

	PatternConfig c = { cfg };
	PatternConfig* rc = (PatternConfig*)VirtualAllocEx(h, nullptr, sizeof(PatternConfig), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (rc) { WriteProcessMemory(h, rc, &c, sizeof(PatternConfig), nullptr); m.push_back(rc); return rc; }
	return nullptr;
}

int wmain(int argc, wchar_t* argv[]) {
	// Usage: Injector.exe <PID> <config> <token> <auth>
	if (argc < 5) return -1;
	DWORD pid = _wtoi(argv[1]);
	if (!pid) return -1;
	char cB[32768];
	char tB[1028];
	char aB[32768];
	WideCharToMultiByte(CP_UTF8, 0, argv[2], -1, cB, sizeof(cB), NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, argv[3], -1, tB, sizeof(tB), NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, argv[4], -1, aB, sizeof(aB), NULL, NULL);
	const char* config = cB;
	const char* token = tB;
	const char* auth = aB;
	TOKEN_PRIVILEGES p = { 0 };
	HANDLE t = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &t)) {
		p.PrivilegeCount = 1;
		p.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &p.Privileges[0].Luid))
			AdjustTokenPrivileges(t, FALSE, &p, 0, NULL, NULL);
		CloseHandle(t);
	}
	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!h) return -2;
	if (!IsCorrectTargetArchitecture(h)) { CloseHandle(h); return -3; }
	std::string d = base64::decode(std::string(EMBEDDED_DLL_BASE64));
	if (d.size() != EMBEDDED_DLL_SIZE) { CloseHandle(h); return -4; }
	std::vector<LPVOID> m;
	PatternConfig* c = AllocPatterns(h, m, config, token, auth);
	bool s = ManualMapDll(h, (BYTE*)d.data(), d.size(), c);
	CloseHandle(h);
	return s ? 0 : -5;
}
