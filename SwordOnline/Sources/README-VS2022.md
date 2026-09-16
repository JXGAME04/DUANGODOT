# JXAll - Visual Studio 2022 build

`JXAll.sln` is the VS2022 (toolset **v143**, platform **Win32/x86**) replacement for the VC6
workspace `JXAll.dsw`. Every one of the 22 projects listed in `JXAll.dsw` was converted; the
~180 other `.dsp` files in the tree (tools, tests, old experiments) are not part of the game
build and were left untouched.

## Building

Open `JXAll.sln` in Visual Studio 2022 (Desktop C++ workload with MFC), or from a
*Developer Command Prompt*:

```
msbuild JXAll.sln /m /p:Configuration="Client Release" /p:Platform=Win32
msbuild JXAll.sln /m /p:Configuration="Server Release" /p:Platform=Win32
```

Solution configurations (the VC6 `Core` project had separate Client/Server targets, so the
solution mirrors that):

| Solution configuration | Builds | Deploys to |
|---|---|---|
| Client Release / Client Debug | client executables + shared DLLs, `Core` as **CoreClient.dll** | `..\..\bin\Client` |
| Server Release / Server Debug | server executables + shared DLLs, `Core` as **CoreServer.dll** | `..\..\bin\Server` |

The post-build copy steps of the original `.dsp` files were kept (made idempotent), so a build
refreshes `SwordOnline\Lib` (import/static libs) and `bin\Client` / `bin\Server` exactly like
the VC6 workspace did.

## Projects and what they produce

| Project | Output | Family |
|---|---|---|
| S3Client | Game.exe | client |
| Core | CoreClient.dll (client cfgs) / CoreServer.dll (server cfgs) | both |
| Engine | Engine.dll (+ engine.lib) | both |
| Represent2, Represent3 | Represent2.dll, Represent3.dll | client |
| KLVideo | KLVideo.dll | client |
| Autoupdate | JXOnline.exe (launcher) | client |
| UpdateDLL | UpdateDLL.dll | client |
| ExpandPackageStaticLib | ExpandPackageStaticLib.lib (used by UpdateDLL) | client |
| LuaLibDll | LuaLibDll.dll | both |
| ExpandPackage | ExpandPackage.dll | both |
| FilterText, FilterText_StaticLib | FilterText.dll, FilterText_StaticLib.lib (used by S3Client) | both |
| GameServer | GameServer.exe | server |
| Bishop, Goddess, S3Relay, Sword3PaySys | Bishop.exe, Goddess.exe, S3Relay.exe, Sword3PaySys.exe | server |
| Heaven, Rainbow | Heaven.dll, Rainbow.dll | server |
| Common, ESClient | common.lib, esclient.lib (static, linked by the servers / Core) | both |

## Shared settings - `JXAll.props`

Every project imports `JXAll.props`, which replaces the machine-wide VC6 directory settings that
`IncludePath.txt` documented:

* include dirs: `SwordOnline\Headers`, `Utility\Headers`, `Engine\Src`, `Core\Src`,
  `SwordOnline\sdk\dx9\Include` (the d3dx9 subset of the December 2006 DirectX SDK, checked in),
  `SwordOnline\ftlib` (FreeType headers; `freetype.dll` is loaded at run time),
  `Utility\Sources\Zip\zlib` (zlib.h matching `Lib\ZLib.lib`);
* library dirs: `SwordOnline\Lib`, `Utility\lib`, `SwordOnline\sdk\dx9\Lib`;
* `/source-charset:.1252 /execution-charset:.1252` - the sources mix GBK (Chinese) and TCVN3
  (Vietnamese) bytes and were always built by VC6 on a code-page-1252 machine, i.e. the bytes of
  every string literal are copied verbatim. **Do not change this to 936**: the single-byte TCVN3
  strings break under a GBK decode;
* `_CRT_SECURE_NO_WARNINGS`, `_CRT_NONSTDC_NO_WARNINGS`, `_WINSOCK_DEPRECATED_NO_WARNINGS`;
* `legacy_stdio_definitions.lib`, `/SAFESEH:NO` and `/NODEFAULTLIB` for `stlport_vc6*.lib` and
  `LIBC` - needed by the 2001-2003 prebuilt libraries (`mp3lib`, `decore`, `JpgLib`, `libdb41s`,
  `aplib`, `KAVPublic*`, USBKey) that are still linked from binaries;
* `VcpkgEnabled=false` so a machine-wide vcpkg integration cannot inject libraries.

## Editing the sources

The files are **not UTF-8**. Edit them with an editor that preserves bytes (VS: "Save with
encoding" - Western European 1252, no BOM); never let a tool re-encode them.

## Regenerating the projects

`Tools\dsp2vcxproj.py` (Python 3) parses `JXAll.dsw` / the `.dsp` files and rewrites the
`.vcxproj`, `.vcxproj.filters` and `.sln`. Project-specific knowledge that is not in the `.dsp`
files (implicit `#pragma comment(lib)` dependencies, dropped `freeimage.lib`, Debug post-build
copies) lives in the small tables at the top of the script.

## Source changes made for the new compiler

All changes are behaviour-preserving fixes for constructs VC6 accepted and modern C++ rejects
or warns about: loop variables that leaked out of `for` headers (hoisted), default arguments
repeated on constructor *definitions*, default-`int` declarations, `"literal"MACRO` string
splicing, member-function pointers taken without `&Class::`, `winsock.h`/`winsock2.h` include
order (`KWin32.h`, GameServer `StdAfx.h`), `fmax`/`fmin` macros vs `<cmath>`, kstring.h's CRT
overrides (VC6 only now), pooled `operator delete` (C++14 sized deallocation), explicit
narrowing casts, `MAKEINTRESOURCE(IDC_*)` double wrapping, and the CRC32 routine now uses the
portable C implementation (bit-identical to the removed `ebp`-based assembly, verified on 3612
cases). Release builds of both families compile with **0 errors / 0 warnings** at the original
warning levels (`/W3`, `/W2` for Core).

## Runtime fixes found by actually running the servers

* `Engine\Src\KPakFile.h` - `m_PackRef` was compiled out under `_SERVER`, but `engine.dll` (built
  without `_SERVER`) is shared by the client and the servers, so `CoreServer.dll` handed it a
  shorter object and `KPakFile::Close()` corrupted the caller's stack (GameServer crashed in
  `KRegion::LoadObject`). The member now exists in both builds.
* Goddess / S3Relay compile with `_USE_32BIT_TIME_T`: `libdb41s.lib` (Berkeley DB 4.1.25, VC6) has a
  `time_t` member in `DB_ENV` ahead of its method table; the 8-byte `time_t` of v143 shifted every
  method pointer by one slot (`set_lg_bsize()` called `set_lg_dir()` -> access violation on start).
* A `.def` listed in a project applies to every configuration (VC6 behaviour); Heaven/Rainbow Release
  otherwise exported nothing and GameServer exited silently.
* Bishop: the single-instance mutex/window-class name can be overridden with
  `BISHOP_INSTANCE_NAME` so two Bishops (e.g. two projects) can run on one machine.

## Test setup on one machine (optional `JXAll.local.props`)

`JXAll.props` imports `JXAll.local.props` (git-ignored) if present. Used here for a side-by-side
test with another JX server on the same PC: Bishop built with `NO_PAYSYS` (accepts any account, no
Sword3PaySys/MSSQL needed) and `BISHOP_INSTANCE_NAME="BishopClass_JXTEST"`, servers configured on
a private port block (Goddess 15001, Bishop 15622/15632, GameServer 16666; GameServer also listens
on the hard-coded 5006-5008) with `127.0.0.1` in `bin\Server\*.cfg/ini`, `Bishop.cfg`, and the
client's `config.ini` (`GameServPort=15622`) / `Settings\ServerList.ini` (`0_Address=127.0.0.1`).
Start order: Goddess (press *Start DB Service*), Bishop, GameServer, then `bin\Client\game.exe`.
S3Relay (chat/tong) and Sword3PaySys keep hard-coded ports from `Headers\ServerPort.h` and were not
part of the smoke test.
