#!/usr/bin/env python3
"""dsp2vcxproj.py - convert the 22 VC6 projects listed in JXAll.dsw into
VS2022 (v143 / Win32) .vcxproj + .vcxproj.filters files and a JXAll.sln.

All input files are read as latin-1 (lossless byte map) because the .dsp
files and source names are GBK (codepage 936).  Output XML is UTF-8, so
GBK group / file names are decoded with 'gbk' for the XML only.
"""
import io, os, re, sys, uuid

ROOT   = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))   # SwordOnline\Sources
DSW    = os.path.join(ROOT, "JXAll.dsw")
SLN    = os.path.join(ROOT, "JXAll.sln")
PROPS  = "JXAll.props"                      # lives next to the .sln
NS     = "http://schemas.microsoft.com/developer/msbuild/2003"
CPPGUID= "{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}"
PLAT   = "Win32"

# which projects build in which solution configuration family
CLIENT_ONLY = {"S3Client","Represent2","Represent3","KLVideo","Autoupdate","UpdateDLL","ExpandPackageStaticLib"}
SERVER_ONLY = {"Bishop","Goddess","Heaven","Rainbow","S3Relay","GameServer","Common","Sword3PaySys"}
# build-order dependencies that only exist as #pragma comment(lib, ...) in the sources
EXTRA_DEPS = {"S3Client": ["FilterText_StaticLib"], "Core": ["ESClient"]}
# link inputs to drop: freeimage.lib is never referenced (S3Client's "FreeImage" is a method) and was
# built against STLport, which no longer exists for this toolchain
DROP_LIBS = {"freeimage.lib"}
# bare Common.lib / Engine.lib on the GameServer link line resolve to the Release copies that the Common
# post-build drops into MultiServer/GameServer; the per-config Lib/release|debug items already cover them
DROP_LIBS_PROJECT = {"GameServer": {"common.lib", "engine.lib"}}
# VC6 post-build steps that were missing for the Debug side (Engine Debug links Lib\debug\lualibdll.lib)
EXTRA_POSTBUILD = {("LuaLibDll", "Debug"): ["copy debug\\lualibdll.lib ..\\..\\..\\Lib\\debug\\lualibdll.lib"]}
# prebuilt libs built against another CRT flavour: drop their default-lib directive in that config
#   libdb41s.lib (/MT) in Goddess Debug (/MTd); ZLib.lib (/MD) in Engine Debug (/MDd);
#   KavPublicD.lib (/MTd) in UpdateDLL Debug (/MDd) - the VC6 Release line did the same for LIBCMT
EXTRA_NODEFAULT = {("Goddess", "Debug"): ["libcmt.lib"], ("S3Relay", "Debug"): ["libcmt.lib"],
                   ("Engine", "Debug"): ["msvcrt.lib"], ("Engine", "OutRead Debug"): ["msvcrt.lib"],
                   ("UpdateDLL", "Debug"): ["libcmtd.lib"]}
# link inputs the VC6 Debug lines forgot when zlib was introduced (only the Release line got zlib.lib)
EXTRA_LIBS = {("Engine", "Debug"): ["zlib.lib"], ("Engine", "OutRead Debug"): ["zlib.lib"]}
# prebuilt KavPublicD.lib ships without its PDB: same /ignore:4099 the other projects already use
EXTRA_LINKOPTS = {("UpdateDLL", "Debug"): ["/ignore:4099"]}
# libdb41s.lib (Berkeley DB 4.1.25) was built by VC6 with a 4-byte time_t; struct DB_ENV carries a
# time_t member ahead of its method table, so the 8-byte time_t of v143 shifts every method pointer
# by one slot (set_lg_bsize() ended up calling set_lg_dir()).  _USE_32BIT_TIME_T restores the VC6 ABI.
EXTRA_DEFINES = {"Goddess": ["_USE_32BIT_TIME_T"], "S3Relay": ["_USE_32BIT_TIME_T"]}
# GameServer.dsp compiled its Debug config with /MT (release CRT) yet links the /MTd Debug libs; VC6
# tolerated that mix, the v143 linker does not (debug-CRT symbols unresolved) - use the debug CRT
EXTRA_RUNTIME = {("GameServer", "Debug"): "MultiThreadedDebug"}

def rd(p):
    with io.open(p, "r", encoding="latin-1", newline="") as f:
        return f.read()

def gbk(s):
    """latin-1 str carrying GBK bytes -> real unicode for XML."""
    try:    return s.encode("latin-1").decode("gbk")
    except: return s

def xml(s):
    return (s.replace("&","&amp;").replace("<","&lt;").replace(">","&gt;").replace('"',"&quot;"))

def guid_for(name):
    return "{%s}" % str(uuid.uuid5(uuid.NAMESPACE_URL, "jxall/"+name)).upper()

def tokenize(args):
    """split a VC6 option string into tokens, keeping quoted parts."""
    return re.findall(r'"[^"]*"|\S+', args)

def unq(s):
    return s[1:-1] if len(s) >= 2 and s[0] == '"' and s[-1] == '"' else s

# ----------------------------------------------------------------------------
# parsing
# ----------------------------------------------------------------------------
def parse_dsp(path):
    t = rd(path)
    P = {"path": path, "dir": os.path.dirname(path)}
    P["name"] = re.search(r'Name="([^"]+)"', t).group(1)
    tt = re.search(r'# TARGTYPE "([^"]+)" (0x[0-9a-fA-F]+)', t)
    P["targtype"] = int(tt.group(2), 16)
    head, _, body = t.partition("# Begin Target")
    # ---- configurations -------------------------------------------------
    cfgs = {}
    order = []
    cur = None
    for line in head.splitlines():
        m = re.match(r'!(?:ELSE)?IF\s+"\$\(CFG\)"\s*==\s*"([^"]+)"', line)
        if m:
            cur = m.group(1); order.append(cur)
            cfgs[cur] = {"props": {}, "cpp": [], "cppsub": [], "rsc": [], "link": [], "linksub": [], "lib": [], "post": []}
            continue
        if line.startswith("!ENDIF"): cur = None; continue
        if cur is None: continue
        c = cfgs[cur]
        m = re.match(r'# PROP (?!BASE )(\w+) (.*)$', line)
        if m: c["props"][m.group(1)] = unq(m.group(2).strip()); continue
        m = re.match(r'# (ADD|SUBTRACT) (CPP|RSC|LINK32|LIB32) (.*)$', line)
        if m:
            kind, tool, args = m.groups()
            key = {"CPP":"cpp","RSC":"rsc","LINK32":"link","LIB32":"lib"}[tool] + ("sub" if kind=="SUBTRACT" else "")
            if key in c: c[key] += tokenize(args.strip())
            continue
        m = re.match(r'(PostBuild_Cmds|PreLink_Cmds|PreBuild_Cmds)=(.*)$', line)
        if m: c["post"] += [x for x in m.group(2).split("\t") if x.strip()]
    P["cfgorder"] = order
    P["cfgs"] = cfgs
    # ---- files ----------------------------------------------------------
    files = []
    groups = []            # stack of group names
    lines = body.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        m = re.match(r'# Begin Group "([^"]*)"', line)
        if m: groups.append(m.group(1)); i += 1; continue
        if line.startswith("# End Group"): groups.pop(); i += 1; continue
        if line.startswith("# Begin Source File"):
            j = i + 1
            block = []
            while not lines[j].startswith("# End Source File"):
                block.append(lines[j]); j += 1
            src = None; percfg = {}; cur = None; common = {"excl": False, "cpp": [], "cppsub": []}
            for b in block:
                m = re.match(r'SOURCE=(.*)$', b)
                if m: src = unq(m.group(1).strip()); continue
                m = re.match(r'!(?:ELSE)?IF\s+"\$\(CFG\)"\s*==\s*"([^"]+)"', b)
                if m: cur = m.group(1); percfg[cur] = {"excl": False, "cpp": [], "cppsub": []}; continue
                if b.startswith("!ENDIF"): cur = None; continue
                tgt = percfg[cur] if cur else common
                if "# PROP Exclude_From_Build 1" in b: tgt["excl"] = True
                m = re.match(r'# (ADD|SUBTRACT) CPP (.*)$', b)
                if m: tgt["cpp" if m.group(1)=="ADD" else "cppsub"] += tokenize(m.group(2).strip())
            files.append({"src": src, "group": "\\".join(groups), "common": common, "percfg": percfg})
            i = j + 1; continue
        i += 1
    P["files"] = files
    return P

# ----------------------------------------------------------------------------
# option mapping
# ----------------------------------------------------------------------------
def cpp_settings(toks, subs):
    s = {"defines": [], "includes": [], "rt": None, "warn": "Level3", "opt": "Disabled",
         "dbg": None, "eh": False, "rtc": False, "pchmode": None, "pchfile": None}
    it = iter(toks)
    for tk in it:
        if tk == "/D":
            s["defines"].append(unq(next(it)))
        elif tk.startswith("/D") and len(tk) > 2 and tk[2] != '"':
            s["defines"].append(unq(tk[2:]))
        elif tk == "/I":
            s["includes"].append(unq(next(it)).rstrip("\\"))
        elif tk in ("/MT","/MTd","/MD","/MDd","/ML","/MLd"):
            s["rt"] = {"/MT":"MultiThreaded","/MTd":"MultiThreadedDebug","/MD":"MultiThreadedDLL",
                       "/MDd":"MultiThreadedDebugDLL","/ML":"MultiThreaded","/MLd":"MultiThreadedDebug"}[tk]
        elif re.match(r'/W[0-4]$', tk): s["warn"] = ("Level" + tk[2]) if tk[2] != "0" else "TurnOffAllWarnings"
        elif tk == "/O2": s["opt"] = "MaxSpeed"
        elif tk == "/O1": s["opt"] = "MinSpace"
        elif tk == "/Ox": s["opt"] = "Full"
        elif tk == "/Od": s["opt"] = "Disabled"
        elif tk == "/ZI": s["dbg"] = "EditAndContinue"
        elif tk == "/Zi": s["dbg"] = "ProgramDatabase"
        elif tk == "/Z7": s["dbg"] = "OldStyle"
        elif tk == "/GX": s["eh"] = True
        elif tk == "/GZ": s["rtc"] = True
        elif tk.startswith("/Yu"): s["pchmode"] = "Use"; s["pchfile"] = unq(tk[3:]) or None
        elif tk.startswith("/Yc"): s["pchmode"] = "Create"; s["pchfile"] = unq(tk[3:]) or None
        elif tk.startswith("/YX"): s["pchmode"] = "Auto"; s["pchfile"] = unq(tk[3:]) or None
        # ignored: /nologo /Gm /FR /FD /LD /c /FAs /Zl ...
    return s

def link_settings(toks, subs):
    s = {"libs": [], "libpaths": [], "nodefault": [], "subsystem": None, "out": None, "deffile": None,
         "implib": None, "map": False, "debug": False, "incremental": None, "extra": []}
    for tk in toks:
        low = tk.lower()
        if low.endswith(".lib") and not tk.startswith("/"): s["libs"].append(unq(tk))
        elif low.startswith("/subsystem:"): s["subsystem"] = {"windows":"Windows","console":"Console"}.get(low.split(":")[1], "Windows")
        elif low.startswith("/out:"): s["out"] = unq(tk[5:])
        elif low.startswith("/def:"): s["deffile"] = unq(tk[5:])
        elif low.startswith("/implib:"): s["implib"] = unq(tk[8:])
        elif low.startswith("/libpath:"): s["libpaths"].append(unq(tk[9:]).rstrip("\\/"))
        elif low.startswith("/nodefaultlib:"): s["nodefault"].append(unq(tk[14:]))
        elif low == "/map": s["map"] = True
        elif low == "/debug": s["debug"] = True
        elif low == "/incremental:no": s["incremental"] = False
        elif low == "/incremental:yes": s["incremental"] = True
        elif low.startswith("/ignore:"): s["extra"].append(tk)
        # ignored: /nologo /dll /machine /pdbtype /profile
    return s

def rsc_settings(toks):
    s = {"defines": [], "culture": None}
    it = iter(toks)
    for tk in it:
        if tk == "/d": s["defines"].append(unq(next(it)))
        elif tk == "/l":
            v = next(it); n = int(v, 16); s["culture"] = "0x%04x" % n
    return s

def cfg_short(name, proj):
    # "Core - Win32 Server Debug" -> "Server Debug"
    return re.sub(r'^.*?-\s*Win32\s+', '', name).strip()

def cfg_type(P):
    return {0x0101:"Application", 0x0102:"DynamicLibrary", 0x0103:"Application", 0x0104:"StaticLibrary"}[P["targtype"]]

def is_console(P):
    return P["targtype"] == 0x0103

def relpath(frm, to):
    r = os.path.relpath(to, frm)
    return r.replace("/", "\\")

def fix_postbuild(cmds):
    """make VC6 post-build steps robust: idempotent md, /Y copy, skip missing."""
    out = []
    for c in cmds:
        c = c.strip()
        if not c: continue
        m = re.match(r'(?i)md\s+(.+)$', c)
        if m:
            d = m.group(1).strip().rstrip("\\")
            out.append('if not exist "%s" md "%s"' % (d, d)); continue
        m = re.match(r'(?i)copy\s+(\S+)\s+(\S+)$', c)
        if m:
            src, dst = m.group(1), m.group(2)
            ddir = os.path.dirname(dst.rstrip("\\"))
            if ddir: out.append('if not exist "%s" md "%s"' % (ddir, ddir))
            out.append('if exist "%s" copy /Y "%s" "%s" >nul' % (src, src, dst)); continue
        out.append(c)
    return out

# ----------------------------------------------------------------------------
# generation
# ----------------------------------------------------------------------------
def auto_pch_ok(P, hdr):
    """True if every compiled C++ file of the project #includes hdr (VC6 /YX tolerance check)."""
    rx = re.compile(r'^\s*#\s*include\s*[<"]' + re.escape(os.path.basename(hdr)) + r'[>"]', re.I | re.M)
    for f in P["files"]:
        src = f["src"]
        if os.path.splitext(src)[1].lower() not in (".cpp", ".cxx", ".cc"): continue
        if f["common"]["excl"]: continue
        disk = os.path.normpath(os.path.join(P["dir"], gbk(src)))
        if not os.path.exists(disk): continue
        if not rx.search(rd(disk)): return False
    return True

def gen_project(P, allproj, prodlib):
    name = P["name"]; d = P["dir"]
    ctype = cfg_type(P)
    cfgs  = [(c, cfg_short(c, P)) for c in P["cfgorder"]]
    # de-dup shorts while keeping order
    seen = set(); cfgs = [x for x in cfgs if not (x[1] in seen or seen.add(x[1]))]
    props_rel = relpath(d, os.path.join(ROOT, PROPS))
    usemfc = any(P["cfgs"][c]["props"].get("Use_MFC") in ("2","6") for c,_ in cfgs)
    staticmfc = any(P["cfgs"][c]["props"].get("Use_MFC") in ("1","5") for c,_ in cfgs)
    L = []
    A = L.append
    A('<?xml version="1.0" encoding="utf-8"?>')
    A('<Project DefaultTargets="Build" xmlns="%s">' % NS)
    A('  <ItemGroup Label="ProjectConfigurations">')
    for c, sh in cfgs:
        A('    <ProjectConfiguration Include="%s|%s">' % (sh, PLAT))
        A('      <Configuration>%s</Configuration>' % sh)
        A('      <Platform>%s</Platform>' % PLAT)
        A('    </ProjectConfiguration>')
    A('  </ItemGroup>')
    A('  <PropertyGroup Label="Globals">')
    A('    <VCProjectVersion>17.0</VCProjectVersion>')
    A('    <ProjectGuid>%s</ProjectGuid>' % guid_for(name))
    A('    <RootNamespace>%s</RootNamespace>' % name)
    A('    <Keyword>%s</Keyword>' % ("MFCProj" if usemfc or staticmfc else "Win32Proj"))
    A('    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>')
    A('  </PropertyGroup>')
    A('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props" />')
    for c, sh in cfgs:
        cf = P["cfgs"][c]; pr = cf["props"]
        cond = "'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT)
        A('  <PropertyGroup Condition="%s" Label="Configuration">' % cond)
        A('    <ConfigurationType>%s</ConfigurationType>' % ctype)
        A('    <UseDebugLibraries>%s</UseDebugLibraries>' % ("true" if pr.get("Use_Debug_Libraries") == "1" else "false"))
        A('    <PlatformToolset>v143</PlatformToolset>')
        A('    <CharacterSet>MultiByte</CharacterSet>')
        if pr.get("Use_MFC") in ("2","6"): A('    <UseOfMfc>Dynamic</UseOfMfc>')
        elif pr.get("Use_MFC") in ("1","5"): A('    <UseOfMfc>Static</UseOfMfc>')
        A('    <WholeProgramOptimization>false</WholeProgramOptimization>')
        A('  </PropertyGroup>')
    A('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.props" />')
    A('  <ImportGroup Label="ExtensionSettings">')
    A('  </ImportGroup>')
    A('  <ImportGroup Label="Shared">')
    A('  </ImportGroup>')
    for c, sh in cfgs:
        cond = "'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT)
        A('  <ImportGroup Label="PropertySheets" Condition="%s">' % cond)
        A('    <Import Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props" Condition="exists(\'$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\')" Label="LocalAppDataPlatform" />')
        A('    <Import Project="%s" />' % props_rel)
        A('  </ImportGroup>')
    A('  <PropertyGroup Label="UserMacros" />')
    # per-config properties + item definitions
    percfg_cpp = {}
    percfg_link = {}
    tn_by_cfg = {}
    for c, sh in cfgs:
        cf = P["cfgs"][c]; pr = cf["props"]
        cond = "'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT)
        cpp = cpp_settings(cf["cpp"], cf["cppsub"]); percfg_cpp[c] = cpp
        lnk = link_settings(cf["link"], cf["linksub"]); percfg_link[c] = lnk
        outdir = pr.get("Output_Dir", sh.replace(" ", "")).rstrip("\\") + "\\"
        intdir = pr.get("Intermediate_Dir", outdir).rstrip("\\") + "\\"
        # target name / ext
        if lnk["out"]:
            tn, te = os.path.splitext(os.path.basename(lnk["out"]))
        else:
            tn = name; te = {"Application":".exe","DynamicLibrary":".dll","StaticLibrary":".lib"}[ctype]
        tn_by_cfg[c] = tn
        A('  <PropertyGroup Condition="%s">' % cond)
        A('    <OutDir>%s</OutDir>' % xml(outdir))
        A('    <IntDir>%s</IntDir>' % xml(intdir))
        A('    <TargetName>%s</TargetName>' % xml(tn))
        A('    <TargetExt>%s</TargetExt>' % xml(te))
        if ctype != "StaticLibrary":
            inc = lnk["incremental"]
            if inc is None: inc = (cpp["opt"] == "Disabled")
            A('    <LinkIncremental>%s</LinkIncremental>' % ("true" if inc else "false"))
        A('  </PropertyGroup>')
    # project-level PCH decision: does any file create a pch?
    yc_files = {}
    for f in P["files"]:
        for tk in f["common"]["cpp"]:
            if tk.startswith("/Yc"): yc_files[f["src"]] = unq(tk[3:])
        for c, cc in f["percfg"].items():
            for tk in cc["cpp"]:
                if tk.startswith("/Yc"): yc_files[f["src"]] = unq(tk[3:])
    for c, sh in cfgs:
        cf = P["cfgs"][c]; pr = cf["props"]
        cond = "'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT)
        cpp = percfg_cpp[c]; lnk = percfg_link[c]; rsc = rsc_settings(cf["rsc"])
        A('  <ItemDefinitionGroup Condition="%s">' % cond)
        A('    <ClCompile>')
        A('      <WarningLevel>%s</WarningLevel>' % cpp["warn"])
        A('      <Optimization>%s</Optimization>' % cpp["opt"])
        if cpp["opt"] != "Disabled":
            A('      <FunctionLevelLinking>true</FunctionLevelLinking>')
            A('      <IntrinsicFunctions>true</IntrinsicFunctions>')
        A('      <SDLCheck>false</SDLCheck>')
        A('      <ConformanceMode>false</ConformanceMode>')
        A('      <PreprocessorDefinitions>%s;%%(PreprocessorDefinitions)</PreprocessorDefinitions>' % xml(";".join(cpp["defines"] + EXTRA_DEFINES.get(name, []))))
        incs = [i for i in cpp["includes"] if not (os.path.isabs(i) and not os.path.isdir(i))]
        if incs:
            A('      <AdditionalIncludeDirectories>%s;%%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>' % xml(";".join(incs)))
        rt = EXTRA_RUNTIME.get((name, sh)) or cpp["rt"] or ("MultiThreadedDebug" if pr.get("Use_Debug_Libraries") == "1" else "MultiThreaded")
        A('      <RuntimeLibrary>%s</RuntimeLibrary>' % rt)
        # PCH
        pchfile = cpp["pchfile"]
        if cpp["pchmode"] in ("Use", "Create") and pchfile:
            A('      <PrecompiledHeader>Use</PrecompiledHeader>')
            A('      <PrecompiledHeaderFile>%s</PrecompiledHeaderFile>' % xml(pchfile))
        elif cpp["pchmode"] == "Auto" and yc_files and auto_pch_ok(P, pchfile or list(yc_files.values())[0]):
            # VC6 /YX was lenient; only keep a PCH when every C++ unit really includes the header
            hdr = pchfile or list(yc_files.values())[0]
            A('      <PrecompiledHeader>Use</PrecompiledHeader>')
            A('      <PrecompiledHeaderFile>%s</PrecompiledHeaderFile>' % xml(hdr))
        else:
            A('      <PrecompiledHeader>NotUsing</PrecompiledHeader>')
        dbg = cpp["dbg"] or "ProgramDatabase"
        A('      <DebugInformationFormat>%s</DebugInformationFormat>' % dbg)
        if cpp["rtc"]: A('      <BasicRuntimeChecks>EnableFastChecks</BasicRuntimeChecks>')
        A('      <ExceptionHandling>%s</ExceptionHandling>' % ("Sync" if cpp["eh"] else "false"))
        A('    </ClCompile>')
        A('    <ResourceCompile>')
        if rsc["defines"]:
            A('      <PreprocessorDefinitions>%s;%%(PreprocessorDefinitions)</PreprocessorDefinitions>' % xml(";".join(rsc["defines"])))
        if rsc["culture"]: A('      <Culture>%s</Culture>' % rsc["culture"])
        A('    </ResourceCompile>')
        if ctype == "StaticLibrary":
            A('    <Lib>')
            A('      <OutputFile>$(OutDir)$(TargetName)$(TargetExt)</OutputFile>')
            A('    </Lib>')
        else:
            A('    <Link>')
            A('      <SubSystem>%s</SubSystem>' % (lnk["subsystem"] or ("Console" if is_console(P) else "Windows")))
            A('      <GenerateDebugInformation>true</GenerateDebugInformation>')
            if lnk["map"]:
                A('      <GenerateMapFile>true</GenerateMapFile>')
                A('      <MapFileName>$(OutDir)$(TargetName).map</MapFileName>')
            libs = [l for l in lnk["libs"] if l.lower() not in DROP_LIBS and l.lower() not in DROP_LIBS_PROJECT.get(name, set())] + EXTRA_LIBS.get((name, sh), [])
            if libs:
                A('      <AdditionalDependencies>%s;%%(AdditionalDependencies)</AdditionalDependencies>' % xml(";".join(libs)))
            if lnk["libpaths"]:
                A('      <AdditionalLibraryDirectories>%s;%%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>' % xml(";".join(lnk["libpaths"])))
            nodefault = lnk["nodefault"] + EXTRA_NODEFAULT.get((name, sh), [])
            if nodefault:
                A('      <IgnoreSpecificDefaultLibraries>%s;%%(IgnoreSpecificDefaultLibraries)</IgnoreSpecificDefaultLibraries>' % xml(";".join(nodefault)))
            # VC6 applied a .def listed among the project files to every configuration, even though only
            # some link lines spelled out /def: (Heaven/Rainbow Release would otherwise export nothing)
            listed_def = [f["src"] for f in P["files"] if f["src"].lower().endswith(".def")]
            deffile = lnk["deffile"] or (listed_def[0] if listed_def else None)
            if deffile:
                A('      <ModuleDefinitionFile>%s</ModuleDefinitionFile>' % xml(deffile))
            if lnk["implib"]:
                A('      <ImportLibrary>%s</ImportLibrary>' % xml(lnk["implib"]))
            extra = lnk["extra"] + [o for o in EXTRA_LINKOPTS.get((name, sh), []) if o not in lnk["extra"]]
            if extra:
                A('      <AdditionalOptions>%s %%(AdditionalOptions)</AdditionalOptions>' % xml(" ".join(extra)))
            A('      <TargetMachine>MachineX86</TargetMachine>')
            A('    </Link>')
        post = fix_postbuild(cf["post"] + EXTRA_POSTBUILD.get((name, sh), []))
        if post:
            A('    <PostBuildEvent>')
            A('      <Command>%s</Command>' % xml("\r\n".join(post)))
            A('      <Message>Deploying %s</Message>' % xml(tn_by_cfg[c]))
            A('    </PostBuildEvent>')
        A('  </ItemDefinitionGroup>')
    # ---- items ----------------------------------------------------------
    compile_items = []; include_items = []; rc_items = []; none_items = []; lib_items = []; img_items = []
    for f in P["files"]:
        src = f["src"]; ext = os.path.splitext(src)[1].lower()
        disk = os.path.normpath(os.path.join(d, gbk(src)))
        exists = os.path.exists(disk)
        ent = (src, f, exists)
        if ext in (".cpp", ".c", ".cxx", ".cc"):
            if exists: compile_items.append(ent)
            else:
                print("   !! missing compile unit %s (kept as None)" % src); none_items.append(ent)
        elif ext in (".h", ".hpp", ".inl", ".hxx", ".tlh", ".tli"): include_items.append(ent)
        elif ext == ".rc": rc_items.append(ent)
        elif ext == ".lib": lib_items.append(ent)
        elif ext in (".ico", ".bmp", ".cur"): img_items.append(ent)
        else: none_items.append(ent)

    def excl_conds(f):
        """return list of (cond, excluded) for per-config exclusion."""
        res = []
        if f["common"]["excl"]:
            return [(None, True)]
        low = f["src"].lower().replace("/", "\\")
        for c, sh in cfgs:
            cc = f["percfg"].get(c)
            ex = bool(cc and cc["excl"])
            # Lib\debug\x.lib belongs to Debug configs only, Lib\release\x.lib to Release ones only
            if low.endswith(".lib") and "\\debug\\" in low and "debug" not in sh.lower(): ex = True
            if low.endswith(".lib") and "\\release\\" in low and "debug" in sh.lower(): ex = True
            if ex:
                res.append(("'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT), True))
        return res

    A('  <ItemGroup>')
    for src, f, exists in compile_items:
        inc = gbk(src)
        meta = []
        for cond, ex in excl_conds(f):
            meta.append('      <ExcludedFromBuild%s>true</ExcludedFromBuild>' % ((' Condition="%s"' % cond) if cond else ''))
        # per-file pch
        ext = os.path.splitext(src)[1].lower()
        tk_common = f["common"]["cpp"]; sub_common = f["common"]["cppsub"]
        if any(t.startswith("/Yc") for t in tk_common):
            hdr = [unq(t[3:]) for t in tk_common if t.startswith("/Yc")][0]
            for c, sh in cfgs:
                cond = "'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT)
                cpc = percfg_cpp[c]
                if cpc["pchmode"] in ("Use", "Create", "Auto") and (cpc["pchfile"] or yc_files):
                    meta.append('      <PrecompiledHeader Condition="%s">Create</PrecompiledHeader>' % cond)
                    if hdr: meta.append('      <PrecompiledHeaderFile Condition="%s">%s</PrecompiledHeaderFile>' % (cond, xml(hdr)))
                else:
                    meta.append('      <PrecompiledHeader Condition="%s">NotUsing</PrecompiledHeader>' % cond)
        elif any(t.startswith(("/YX","/Yc","/Yu")) for t in sub_common) or ext == ".c":
            meta.append('      <PrecompiledHeader>NotUsing</PrecompiledHeader>')
        else:
            # per-config /Yc or subtract
            for c, sh in cfgs:
                cc = f["percfg"].get(c)
                if not cc: continue
                cond = "'$(Configuration)|$(Platform)'=='%s|%s'" % (sh, PLAT)
                pchbase = os.path.splitext(os.path.basename(percfg_cpp[c]["pchfile"] or ""))[0].lower()
                if any(t.startswith("/Yc") for t in cc["cpp"]):
                    meta.append('      <PrecompiledHeader Condition="%s">Create</PrecompiledHeader>' % cond)
                elif any(t.startswith("/YX") for t in cc["cpp"]) and pchbase and os.path.splitext(os.path.basename(src))[0].lower() == pchbase:
                    # VC6 "automatic" PCH on the stdafx.cpp itself == create it
                    meta.append('      <PrecompiledHeader Condition="%s">Create</PrecompiledHeader>' % cond)
                elif any(t.startswith(("/YX","/Yc","/Yu")) for t in cc["cppsub"]):
                    meta.append('      <PrecompiledHeader Condition="%s">NotUsing</PrecompiledHeader>' % cond)
        # per-file include dirs
        fi = [unq(tk_common[i+1]) for i,t in enumerate(tk_common) if t == "/I" and i+1 < len(tk_common)]
        if fi:
            meta.append('      <AdditionalIncludeDirectories>%s;%%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>' % xml(";".join(fi)))
        if meta:
            A('    <ClCompile Include="%s">' % xml(inc)); L.extend(meta); A('    </ClCompile>')
        else:
            A('    <ClCompile Include="%s" />' % xml(inc))
    A('  </ItemGroup>')
    if include_items:
        A('  <ItemGroup>')
        for src, f, exists in include_items: A('    <ClInclude Include="%s" />' % xml(gbk(src)))
        A('  </ItemGroup>')
    if rc_items:
        A('  <ItemGroup>')
        for src, f, exists in rc_items:
            conds = excl_conds(f)
            if conds:
                A('    <ResourceCompile Include="%s">' % xml(gbk(src)))
                for cond, ex in conds:
                    A('      <ExcludedFromBuild%s>true</ExcludedFromBuild>' % ((' Condition="%s"' % cond) if cond else ''))
                A('    </ResourceCompile>')
            else:
                A('    <ResourceCompile Include="%s" />' % xml(gbk(src)))
        A('  </ItemGroup>')
    if lib_items:
        A('  <ItemGroup>')
        for src, f, exists in lib_items:
            conds = excl_conds(f)
            inc = gbk(src)
            # a prebuilt Lib\debug\x.lib that never existed (JpgLib, mp3lib): fall back to Lib\x.lib
            if not exists and "\\debug\\" in inc.lower() and os.path.splitext(os.path.basename(inc))[0].lower() not in prodlib:
                alt = re.sub(r'(?i)\\debug\\', '\\\\', inc)
                if os.path.exists(os.path.normpath(os.path.join(d, alt))):
                    inc = alt
            if conds:
                A('    <Library Include="%s">' % xml(inc))
                for cond, ex in conds:
                    A('      <ExcludedFromBuild%s>true</ExcludedFromBuild>' % ((' Condition="%s"' % cond) if cond else ''))
                A('    </Library>')
            else:
                A('    <Library Include="%s" />' % xml(inc))
        A('  </ItemGroup>')
    if img_items:
        A('  <ItemGroup>')
        for src, f, exists in img_items: A('    <Image Include="%s" />' % xml(gbk(src)))
        A('  </ItemGroup>')
    if none_items:
        A('  <ItemGroup>')
        for src, f, exists in none_items: A('    <None Include="%s" />' % xml(gbk(src)))
        A('  </ItemGroup>')
    # ---- project references (build order only) --------------------------
    deps = set()
    for c, sh in cfgs:
        for l in percfg_link[c]["libs"]:
            base = os.path.splitext(os.path.basename(l))[0].lower()
            if base in prodlib and prodlib[base] != name: deps.add(prodlib[base])
    for src, f, exists in lib_items:
        base = os.path.splitext(os.path.basename(src))[0].lower()
        if base in prodlib and prodlib[base] != name: deps.add(prodlib[base])
    deps |= set(EXTRA_DEPS.get(name, []))
    if deps:
        A('  <ItemGroup>')
        for dn in sorted(deps):
            dp = allproj[dn]
            A('    <ProjectReference Include="%s">' % xml(relpath(d, os.path.join(dp["dir"], dn + ".vcxproj"))))
            A('      <Project>%s</Project>' % guid_for(dn))
            A('      <LinkLibraryDependencies>false</LinkLibraryDependencies>')
            A('      <UseLibraryDependencyInputs>false</UseLibraryDependencyInputs>')
            A('    </ProjectReference>')
        A('  </ItemGroup>')
    A('  <Import Project="$(VCTargetsPath)\\Microsoft.Cpp.targets" />')
    A('  <ImportGroup Label="ExtensionTargets">')
    A('  </ImportGroup>')
    A('</Project>')
    P["deps"] = sorted(deps); P["cfgs_short"] = cfgs
    # ---- filters --------------------------------------------------------
    F = []; B = F.append
    B('<?xml version="1.0" encoding="utf-8"?>')
    B('<Project ToolsVersion="4.0" xmlns="%s">' % NS)
    groups = sorted(set(gbk(f["group"]) for f in P["files"] if f["group"]))
    allgroups = set()
    for g in groups:
        parts = g.split("\\")
        for i in range(1, len(parts)+1): allgroups.add("\\".join(parts[:i]))
    B('  <ItemGroup>')
    for g in sorted(allgroups):
        B('    <Filter Include="%s">' % xml(g))
        B('      <UniqueIdentifier>{%s}</UniqueIdentifier>' % str(uuid.uuid5(uuid.NAMESPACE_URL, "jxall/%s/%s" % (name, g))).upper())
        B('    </Filter>')
    B('  </ItemGroup>')
    def emit(items, tag):
        if not items: return
        B('  <ItemGroup>')
        for src, f, exists in items:
            g = gbk(f["group"])
            if g:
                B('    <%s Include="%s">' % (tag, xml(gbk(src)))); B('      <Filter>%s</Filter>' % xml(g)); B('    </%s>' % tag)
            else:
                B('    <%s Include="%s" />' % (tag, xml(gbk(src))))
        B('  </ItemGroup>')
    emit(compile_items, "ClCompile"); emit(include_items, "ClInclude"); emit(rc_items, "ResourceCompile")
    emit(lib_items, "Library"); emit(img_items, "Image"); emit(none_items, "None")
    B('</Project>')
    return "\n".join(L) + "\n", "\n".join(F) + "\n"

def gen_sln(projs):
    L = []; A = L.append
    A("")
    A("Microsoft Visual Studio Solution File, Format Version 12.00")
    A("# Visual Studio Version 17")
    A("VisualStudioVersion = 17.0.31903.59")
    A("MinimumVisualStudioVersion = 10.0.40219.1")
    for P in projs:
        rel = relpath(ROOT, os.path.join(P["dir"], P["name"] + ".vcxproj"))
        A('Project("%s") = "%s", "%s", "%s"' % (CPPGUID, P["name"], rel, guid_for(P["name"])))
        A("EndProject")
    A("Global")
    A("\tGlobalSection(SolutionConfigurationPlatforms) = preSolution")
    slncfgs = ["Client Debug", "Client Release", "Server Debug", "Server Release"]
    for s in slncfgs: A("\t\t%s|%s = %s|%s" % (s, PLAT, s, PLAT))
    A("\tEndGlobalSection")
    A("\tGlobalSection(ProjectConfigurationPlatforms) = postSolution")
    for P in projs:
        shorts = [sh for c, sh in P["cfgs_short"]]
        g = guid_for(P["name"])
        for s in slncfgs:
            fam, kind = s.split()             # Client/Server, Debug/Release
            if P["name"] == "Core":
                pc = "%s %s" % (fam, kind)
            else:
                pc = kind if kind in shorts else shorts[0]
            build = True
            if fam == "Client" and P["name"] in SERVER_ONLY: build = False
            if fam == "Server" and P["name"] in CLIENT_ONLY: build = False
            A("\t\t%s.%s|%s.ActiveCfg = %s|%s" % (g, s, PLAT, pc, PLAT))
            if build: A("\t\t%s.%s|%s.Build.0 = %s|%s" % (g, s, PLAT, pc, PLAT))
    A("\tEndGlobalSection")
    A("\tGlobalSection(SolutionProperties) = preSolution")
    A("\t\tHideSolutionNode = FALSE")
    A("\tEndGlobalSection")
    A("\tGlobalSection(ExtensibilityGlobals) = postSolution")
    A("\t\tSolutionGuid = {%s}" % str(uuid.uuid5(uuid.NAMESPACE_URL, "jxall/sln")).upper())
    A("\tEndGlobalSection")
    A("EndGlobal")
    return "\r\n".join(L) + "\r\n"

def main():
    projs = []
    for m in re.finditer(r'Project:\s*"([^"]+)"=([^\s]+)\s*-\s*Package Owner', rd(DSW)):
        path = os.path.normpath(os.path.join(ROOT, m.group(2).replace("\\", "/").lstrip("./")))
        P = parse_dsp(path); P["name"] = m.group(1)
        projs.append(P)
    allproj = {P["name"]: P for P in projs}
    # map produced lib base names -> project
    prodlib = {}
    for P in projs:
        for c in P["cfgorder"]:
            lnk = link_settings(P["cfgs"][c]["link"], P["cfgs"][c]["linksub"])
            if lnk["out"]: prodlib[os.path.splitext(os.path.basename(lnk["out"]))[0].lower()] = P["name"]
        prodlib.setdefault(P["name"].lower(), P["name"])
    for P in projs:
        print("-> %s" % P["name"])
        vc, flt = gen_project(P, allproj, prodlib)
        with io.open(os.path.join(P["dir"], P["name"] + ".vcxproj"), "w", encoding="utf-8-sig", newline="\r\n") as f: f.write(vc)
        with io.open(os.path.join(P["dir"], P["name"] + ".vcxproj.filters"), "w", encoding="utf-8-sig", newline="\r\n") as f: f.write(flt)
        print("   cfgs=%s deps=%s" % ([sh for c, sh in P["cfgs_short"]], P["deps"]))
    with io.open(SLN, "w", encoding="utf-8-sig", newline="") as f: f.write(gen_sln(projs))
    print("wrote %s" % SLN)

if __name__ == "__main__":
    main()
