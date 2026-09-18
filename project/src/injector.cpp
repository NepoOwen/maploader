#include "injector.hpp"

#ifdef _WIN64
#define CURRENT_ARCH IMAGE_FILE_MACHINE_AMD64
#else
#define CURRENT_ARCH IMAGE_FILE_MACHINE_I386
#endif

bool ManualMapDll(HANDLE h, BYTE* s, SIZE_T sz, PatternConfig* pc, bool ch, bool cs, bool ap, bool seh, DWORD r, LPVOID lp) {
	IMAGE_NT_HEADERS* nh = nullptr;IMAGE_OPTIONAL_HEADER* oh = nullptr;IMAGE_FILE_HEADER* fh = nullptr;BYTE* b = nullptr;
	if (reinterpret_cast<IMAGE_DOS_HEADER*>(s)->e_magic != 0x5A4D)return false;
	nh = reinterpret_cast<IMAGE_NT_HEADERS*>(s + reinterpret_cast<IMAGE_DOS_HEADER*>(s)->e_lfanew);
	oh = &nh->OptionalHeader;fh = &nh->FileHeader;
	if (fh->Machine != CURRENT_ARCH)return false;
	b = reinterpret_cast<BYTE*>(VirtualAllocEx(h, nullptr, oh->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!b)return false;
	DWORD op = 0;VirtualProtectEx(h, b, oh->SizeOfImage, PAGE_EXECUTE_READWRITE, &op);
	MANUAL_MAPPING_DATA d{ 0 };
	d.pLoadLibraryA = LoadLibraryA; d.pGetProcAddress = GetProcAddress;
#ifdef _WIN64
	d.pRtlAddFunctionTable = (f_RtlAddFunctionTable)RtlAddFunctionTable;
#else 
	seh = false;
#endif
	d.pbase = b;d.fdwReasonParam = r;d.reservedParam = lp;d.SEHSupport = seh;d.pPatternConfig = pc;
	if (!WriteProcessMemory(h, b, s, 0x1000, nullptr)) { VirtualFreeEx(h, b, 0, MEM_RELEASE);return false; }
	IMAGE_SECTION_HEADER* sh = IMAGE_FIRST_SECTION(nh);
	for (UINT i = 0;i != fh->NumberOfSections;++i, ++sh) {
		if (sh->SizeOfRawData && !WriteProcessMemory(h, b + sh->VirtualAddress, s + sh->PointerToRawData, sh->SizeOfRawData, nullptr)) {
			VirtualFreeEx(h, b, 0, MEM_RELEASE);return false;
		}
	}
	BYTE* m = reinterpret_cast<BYTE*>(VirtualAllocEx(h, nullptr, sizeof(MANUAL_MAPPING_DATA), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if (!m) { VirtualFreeEx(h, b, 0, MEM_RELEASE); return false; }
	if (!WriteProcessMemory(h, m, &d, sizeof(MANUAL_MAPPING_DATA), nullptr)) { VirtualFreeEx(h, b, 0, MEM_RELEASE);VirtualFreeEx(h, m, 0, MEM_RELEASE); return false; }
	void* sc = VirtualAllocEx(h, nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!sc) { VirtualFreeEx(h, b, 0, MEM_RELEASE); VirtualFreeEx(h, m, 0, MEM_RELEASE); return false; }
	if (!WriteProcessMemory(h, sc, Shellcode, 0x1000, nullptr)) { VirtualFreeEx(h, b, 0, MEM_RELEASE);VirtualFreeEx(h, m, 0, MEM_RELEASE);VirtualFreeEx(h, sc, 0, MEM_RELEASE);return false; }
	HANDLE t = CreateRemoteThread(h, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(sc), m, 0, nullptr);
	if (!t) { VirtualFreeEx(h, b, 0, MEM_RELEASE); VirtualFreeEx(h, m, 0, MEM_RELEASE);VirtualFreeEx(h, sc, 0, MEM_RELEASE);return false; }
	CloseHandle(t);
	HINSTANCE hc = NULL;
	while (!hc) {
		DWORD ec = 0;GetExitCodeProcess(h, &ec);
		if (ec != STILL_ACTIVE)return false;
		MANUAL_MAPPING_DATA dc{ 0 }; ReadProcessMemory(h, m, &dc, sizeof(dc), nullptr); hc = dc.hMod;
		if (hc == (HINSTANCE)0x404040) { VirtualFreeEx(h, b, 0, MEM_RELEASE);VirtualFreeEx(h, m, 0, MEM_RELEASE); VirtualFreeEx(h, sc, 0, MEM_RELEASE); return false; }
		Sleep(10);
	}
	BYTE* e = (BYTE*)malloc(1024 * 1024 * 20);if (!e)return false;memset(e, 0, 1024 * 1024 * 20);
	if (ch)WriteProcessMemory(h, b, e, 0x1000, nullptr);
	if (cs) {
		sh = IMAGE_FIRST_SECTION(nh);
		for (UINT i = 0;i != fh->NumberOfSections;++i, ++sh) {
			if (sh->Misc.VirtualSize && ((seh ? 0 : strcmp((char*)sh->Name, ".pdata") == 0) || strcmp((char*)sh->Name, ".rsrc") == 0 || strcmp((char*)sh->Name, ".reloc") == 0))
				WriteProcessMemory(h, b + sh->VirtualAddress, e, sh->Misc.VirtualSize, nullptr);
		}
	}
	if (ap) {
		sh = IMAGE_FIRST_SECTION(nh);
		for (UINT i = 0;i != fh->NumberOfSections;++i, ++sh) {
			if (sh->Misc.VirtualSize) {
				DWORD o = 0, np = PAGE_READONLY;
				if ((sh->Characteristics & IMAGE_SCN_MEM_WRITE) > 0)np = PAGE_READWRITE;
				else if ((sh->Characteristics & IMAGE_SCN_MEM_EXECUTE) > 0)np = PAGE_EXECUTE;  // execute‑only - was PAGE_EXECUTE_READ
				VirtualProtectEx(h, b + sh->VirtualAddress, sh->Misc.VirtualSize, np, &o);
			}
		}
		DWORD o = 0;VirtualProtectEx(h, b, IMAGE_FIRST_SECTION(nh)->VirtualAddress, PAGE_READONLY, &o);
	}
	WriteProcessMemory(h, sc, e, 0x1000, nullptr);VirtualFreeEx(h, sc, 0, MEM_RELEASE);VirtualFreeEx(h, m, 0, MEM_RELEASE);
	return true;
}

#define RF32(r)((r>>0x0C)==IMAGE_REL_BASED_HIGHLOW)
#define RF64(r)((r>>0x0C)==IMAGE_REL_BASED_DIR64)
#ifdef _WIN64
#define RF RF64
#else
#define RF RF32
#endif
#pragma runtime_checks("",off)
#pragma optimize("",off)
void __stdcall Shellcode(MANUAL_MAPPING_DATA* d) {
	if (!d) { d->hMod = (HINSTANCE)0x404040;return; }
	BYTE* b = d->pbase;
	auto* o = &reinterpret_cast<IMAGE_NT_HEADERS*>(b + reinterpret_cast<IMAGE_DOS_HEADER*>((uintptr_t)b)->e_lfanew)->OptionalHeader;
	auto ll = d->pLoadLibraryA;auto gp = d->pGetProcAddress;
#ifdef _WIN64
	auto rt = d->pRtlAddFunctionTable;
#endif
	auto dm = reinterpret_cast<f_DLL_ENTRY_POINT>(b + o->AddressOfEntryPoint);
	BYTE* ld = b - o->ImageBase;
	if (ld && o->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) {
		auto* rd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(b + o->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
		const auto* re = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<uintptr_t>(rd) + o->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size);
		while (rd < re && rd->SizeOfBlock) {
			UINT a = (rd->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
			WORD* ri = reinterpret_cast<WORD*>(rd + 1);
			for (UINT i = 0;i != a;++i, ++ri)if (RF(*ri))*reinterpret_cast<UINT_PTR*>(b + rd->VirtualAddress + ((*ri) & 0xFFF)) += reinterpret_cast<UINT_PTR>(ld);
			rd = reinterpret_cast<IMAGE_BASE_RELOCATION*>(reinterpret_cast<BYTE*>(rd) + rd->SizeOfBlock);
		}
	}
	if (o->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size) {
		auto* id = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(b + o->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
		while (id->Name) {
			HINSTANCE hd = ll(reinterpret_cast<char*>(b + id->Name));
			ULONG_PTR* tr = reinterpret_cast<ULONG_PTR*>(b + id->OriginalFirstThunk);
			ULONG_PTR* fr = reinterpret_cast<ULONG_PTR*>(b + id->FirstThunk);
			if (!id->OriginalFirstThunk)tr = fr;
			for (;*tr;++tr, ++fr)*fr = IMAGE_SNAP_BY_ORDINAL(*tr) ? (ULONG_PTR)gp(hd, reinterpret_cast<char*>(*tr & 0xFFFF)) : (ULONG_PTR)gp(hd, reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(b + (*tr))->Name);
			++id;
		}
	}
	bool ef = false;
#ifdef _WIN64
	if (d->SEHSupport) {
		auto ex = o->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
		if (ex.Size && !rt(reinterpret_cast<IMAGE_RUNTIME_FUNCTION_ENTRY*>(b + ex.VirtualAddress), ex.Size / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY), (DWORD64)b))ef = true;
	}
#endif
	f_ManualMapInit mi = nullptr;
	if (o->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size) {
		auto* ed = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(b + o->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
		if (ed && ed->NumberOfNames > 0) {
			auto* na = reinterpret_cast<DWORD*>(b + ed->AddressOfNames);
			auto* aa = reinterpret_cast<DWORD*>(b + ed->AddressOfFunctions);
			auto* oa = reinterpret_cast<WORD*>(b + ed->AddressOfNameOrdinals);
			for (DWORD i = 0; i < ed->NumberOfNames; ++i) {
				char* fn = reinterpret_cast<char*>(b + na[i]);
				char* sp = fn;if (*sp == '_')sp++;
				char tn[] = { 'c','l','i','e','n','t','d','l','l',0}; // export name of the function to call in the injected DLL
				bool m = true;
				for (int j = 0;tn[j] != 0;j++) if (sp[j] != tn[j]) { m = false; break; }
				if (m) { mi = reinterpret_cast<f_ManualMapInit>(b + aa[oa[i]]); break; }
			}
		}
	}
	if (mi) mi(reinterpret_cast<HMODULE>(b), d->pPatternConfig);else dm(b, d->fdwReasonParam, d->reservedParam);
	d->hMod = ef ? reinterpret_cast<HINSTANCE>(0x505050) : reinterpret_cast<HINSTANCE>(b);
}
