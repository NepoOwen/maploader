# Maploader

A native Windows **embedded** DLL manual-mapping injector written in C++.

`Injector` loads a DLL into a running target process entirely from memory - without writing the payload to disk and without registering it with the OS loader. The target DLL is **embedded** (base64-encoded) directly into the executable at build time.

This project exists **for testing and educational purposes**: it injects an arbitrary DLL into a target process entirely in memory. The build is currently configured to target **Notepad.exe** as a convenience demo, but the same technique applies to any process you own or have permission to test.

---

## Quick links

- [How to use](#how-to-use)
- [Features](#features)
- [How it works](#how-it-works)
- [Project layout](#project-layout)
- [Requirements](#requirements)
- [Building](#building)
- [Regenerating the embedded DLL](#regenerating-the-embedded-dll)
- [Usage](#usage)
- [Exit codes](#exit-codes)
- [Notes](#notes)
- [Credits](#credits)
- [License](#license)

---

## How to use

A complete step-by-step guide to inject a DLL into a target process.

### 1. Build the DLL

Build the test payload (`client.dll`) in the `example-dll` project:

```
msbuild example-dll\client.sln /p:Configuration=Release /p:Platform=x64
```

This produces `build\client.dll`.

### 2. Embed the DLL into the injector

Run the embed script to encode `client.dll` into the injector's source and rebuild:

```
build\generate_embed.bat
```

This writes `project\src\embed.hpp` and rebuilds the injector (`build\Injector-x64.exe`).

### 3. Set the target process

Open `build\inject_process.bat` and update the process name to match your target:

```
set "process_name=Notepad.exe"
```

The default is `Notepad.exe` - change it to any process you own or have permission to test. If needed, also update the `field1`/`field2`/`field3` placeholder arguments in the same file.

### 4. Run the injection

Start the target process, then run the launcher:

```
build\inject_process.bat
```

The script locates your target process by name, finds its PID, and injects the embedded DLL into it.

> **Note:** Ensure the injector architecture matches the target process - use `Injector-x64.exe` for 64-bit targets and `Injector-x86.exe` for 32-bit targets (build with `Platform=Win32`).

---

## ⚠️ Disclaimer

This project is intended **for educational and research purposes only**. Manual DLL injection can be detected as malicious behavior by security software.

**Only use this against processes you own or have explicit permission to test** (such as `Notepad.exe`). You are solely responsible for how you use this code.

---

## Features

- **Manual mapping** - maps the payload into the target's address space without calling `LoadLibrary`.
- **Embedded payload** - the target DLL is compiled into the injector as base64, decoded at runtime.
- **Architecture checks** - aborts if the target process architecture does not match the injector's build.
- **SEH (x64) support** - registers exception handlers for the mapped image.
- **Section cleanup** - optionally wipes headers, `.rsrc`, `.reloc`, and `.pdata` from the mapped image.
- **Memory protection hardening** - re-applies minimal section protections after mapping.
- **Export-based initialization** - calls an exported `clientdll` entry point from the injected DLL when present.
- **Config passing** - allocates and passes config/auth strings to the injected module.
- **x86 and x64 builds** - separate executables for 32-bit and 64-bit targets.

---

## How it works

1. The injector decodes the embedded DLL (`EMBEDDED_DLL_BASE64`).
2. It opens the target process (`OpenProcess`) and verifies matching architecture via `IsWow64Process`.
3. It allocates a buffer in the remote process and copies the PE image sections into it.
4. It writes a `MANUAL_MAPPING_DATA` struct and a position-independent `Shellcode` stub into the target.
5. A remote thread runs the shellcode, which:
   - resolves imports,
   - applies base relocations,
   - registers SEH handlers (x64),
   - invokes the DLL entry point (or the exported init function, `clientdll`).
6. The injector then optionally wipes headers and unused sections and re-applies protection flags.

---

## Project layout

```
injector/
├── project/                      # The injector (executable)
│   ├── injector.vcxproj
│   ├── injector.vcxproj.filters
│   └── src/
│       ├── main.cpp              # Entry point, arg parsing, config allocation
│       ├── injector.hpp          # Manual-map declarations + shellcode signature
│       ├── injector.cpp          # ManualMapDll + Shellcode implementation
│       ├── config.hpp            # AuthConfig and PatternConfig structs
│       ├── embed.hpp             # Auto-generated embedded DLL (base64)
│       └── base64.hpp            # Base64 encode/decode (third-party)
├── example-dll/                  # The DLL that gets injected (test payload)
│   ├── client.vcxproj
│   └── src/
│       └── main.cpp
├── build/                        # Shared build output (both projects)
│   ├── generate_embed.bat        # Regenerates embed.hpp from client.dll
│   ├── inject_process.bat        # Convenience launcher script
│   └── lua/
│       └── dll_to_embed.lua      # Generates embed.hpp from a DLL
├── injector.sln                  # Visual Studio solution (injector)
├── LICENSE
└── readme.md
```

---

## Requirements

- Windows 10/11
- Visual Studio (MSVC toolchain)
- Optional: `luajit.exe` in `build\lua\` for regenerating the embedded DLL

---

## Building

The injector solution targets both `Release|x64` and `Release|Win32` (32-bit x86).

### Injector

Open `injector.sln` in Visual Studio and build the **Release | x64** (or **Release | Win32**) configuration.

Or build from the command line:

```
msbuild injector.sln /p:Configuration=Release /p:Platform=x64
msbuild injector.sln /p:Configuration=Release /p:Platform=Win32
```

Output binaries:
- `build\Injector-x64.exe`
- `build\Injector-x86.exe`

### Example DLL (test payload)

Open `example-dll\client.sln` in Visual Studio and build **Release | x64**, or:

```
msbuild example-dll\client.sln /p:Configuration=Release /p:Platform=x64
```

Output:
- `build\client.dll`

Both projects write their output and intermediate (`.obj`) files to the shared `build\` folder.

---

## Regenerating the embedded DLL

The payload is embedded via `src\embed.hpp`. To regenerate it from a newly built DLL:

1. Build the example DLL so that `build\client.dll` exists.
2. Ensure `build\lua\luajit.exe` is present.
3. Run:

```
build\generate_embed.bat
```

This encodes `build\client.dll` into `project\src\embed.hpp` and does a release rebuild.

> **Note:** `generate_embed.bat` hard-codes an MSBuild path. Edit the `MSBUILD` variable inside the script if your Visual Studio installation differs.

---

## Usage

```
Injector-x64.exe <PID> <field1> <field2> <field3>
```

| Argument  | Description                              |
| --------- | ---------------------------------------- |
| `PID`     | Process ID of the target process (e.g. Notepad) |
| `field1`  | Config string sent to the process        |
| `field2`  | Token string sent to the process         |
| `field3`  | Auth string sent to the process          |

The three string arguments are allocated in the remote process's memory and passed to the injected DLL as an `AuthConfig`.

### Example

Start Notepad, find its PID, then:

```
Injector-x64.exe 1234 "myConfig" "tokenValue" "authValue"
```

### Convenience launcher

`build\inject_process.bat` finds a `Notepad.exe` process and runs the injector automatically, forwarding the three field arguments. Edit the `process_name` variable (and the placeholder field values) to suit your use case. It expects the injector executable in the same directory.

---

## Exit codes

| Code | Meaning                                          |
| ---- | ------------------------------------------------ |
| `0`  | Success                                          |
| `-1` | Invalid arguments (missing PID or params)        |
| `-2` | Failed to open the target process                |
| `-3` | Architecture mismatch                            |
| `-4` | Embedded DLL size mismatch                       |
| `-5` | Manual mapping failed                            |

---

## Notes

- The shellcode is compiled with runtime checks and optimizations disabled (`#pragma runtime_checks("", off)` / `#pragma optimize("", off)`) to remain position-independent.
- The manual mapper looks for a `clientdll` export in the injected module; adapt `injector.cpp` if your payload uses a different contract.
- `base64.hpp` is third-party (`cpp-base64` by NepoOwen) and retains its original authorship header.
- Use the matching architecture: inject the x64 build into 64-bit targets and the x86 build into 32-bit targets.

---

## Credits

- Manual mapping shellcode and PE loader adapted from standard manual-mapping techniques.
- Base64 implementation: [NepoOwen/cpp-base64](https://github.com/NepoOwen/cpp-base64).
- [LuaJIT](https://luajit.org/) - used by `build\lua\dll_to_embed.lua` to generate the embedded DLL at build time.

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
