package jxlua

import (
	"strings"
	"testing"
)

func convert(t *testing.T, src string) string {
	t.Helper()
	return Convert(src).Source
}

func mustContain(t *testing.T, got, want string) {
	t.Helper()
	if !strings.Contains(got, want) {
		t.Fatalf("expected the output to contain %q, got:\n%s", want, got)
	}
}

func mustNotContain(t *testing.T, got, want string) {
	t.Helper()
	if strings.Contains(got, want) {
		t.Fatalf("expected the output NOT to contain %q, got:\n%s", want, got)
	}
}

func TestUpvalueMarkerIsRemoved(t *testing.T) {
	// The real line from script/debug.lua:41
	got := convert(t, `szVar, szExp = %Match(szExp, "([a-zA-Z0-9_]+)(.*)")`)
	mustContain(t, got, `szVar, szExp = Match(szExp, "([a-zA-Z0-9_]+)(.*)")`)
}

func TestUpvalueMarkerIsNotRemovedFromPatternsOrComments(t *testing.T) {
	got := convert(t, "local s = string.gsub(x, \"%s+\", \"\")\n-- %note stays\n")
	mustContain(t, got, `"%s+"`)
	mustContain(t, got, "-- %note stays")
}

func TestUpvalueMarkerAfterAClosingParenIsRemoved(t *testing.T) {
	// The real shape from activitysys/config/1002/bronzechest.lua:17.  Lua 4 has no % operator,
	// so a % here is an upvalue however the token before it looks.
	got := convert(t, `CallBack = function(nItemIdx) %tbLog:Write("x", 1) end`)
	mustContain(t, got, "function(nItemIdx) tbLog:Write(")
}

func TestModuloTouchingItsOperandSurvives(t *testing.T) {
	// Not Lua 4 syntax, but a file edited later against Lua 5 must not lose its operator.
	r := Convert("local r = a%b\n")
	mustContain(t, r.Source, "a%b")
	if len(r.Notes) == 0 {
		t.Fatal("expected a note about the kept %")
	}
}

func TestStringLibraryIsQualified(t *testing.T) {
	got := convert(t, `local s = format("%d-%s", n, strsub(name, 1, strlen(name)))`)
	mustContain(t, got, "string.format(")
	mustContain(t, got, "string.sub(")
	mustContain(t, got, "string.len(")
	mustContain(t, got, `"%d-%s"`) // the pattern inside the string is untouched
}

func TestAlreadyQualifiedCallsAreLeftAlone(t *testing.T) {
	got := convert(t, "local s = string.format(x)  local t = self.format(y)")
	mustNotContain(t, got, "string.string.format")
	mustNotContain(t, got, "self.string.format")
}

func TestTableAndMathLibrariesAreQualified(t *testing.T) {
	got := convert(t, "tinsert(t, 1)  tremove(t)  sort(t)  local v = floor(random(1, 10))")
	mustContain(t, got, "table.insert(")
	mustContain(t, got, "table.remove(")
	mustContain(t, got, "table.sort(")
	mustContain(t, got, "math.floor(math.random(1, 10))")
}

func TestModBecomesFmodNotPercent(t *testing.T) {
	// Lua 4 mod() is C fmod; Lua 5's % is a floor-modulo, which differs on negatives.
	got := convert(t, "local r = mod(nCount, 4)")
	mustContain(t, got, "math.fmod(nCount, 4)")
}

func TestGetnBecomesLengthOperator(t *testing.T) {
	got := convert(t, "local n = getn(tbl)\nlocal m = table.getn(a.b[1])")
	mustContain(t, got, "local n = #(tbl)")
	mustContain(t, got, "local m = #(a.b[1])")
}

func TestUnpackIsQualified(t *testing.T) {
	mustContain(t, convert(t, "f(unpack(args))"), "table.unpack(args)")
}

func TestCallBecomesUnpack(t *testing.T) {
	got := convert(t, "call(%DoIt, {1, 2, 3})")
	mustContain(t, got, "DoIt(table.unpack({1, 2, 3}))")
}

func TestPowBecomesExponent(t *testing.T) {
	mustContain(t, convert(t, "local v = pow(2, n)"), "((2)^(n))")
}

func TestGlobalAccessorsAreRewritten(t *testing.T) {
	got := convert(t, "local v = getglobal(name)\nsetglobal(name, 5)")
	mustContain(t, got, "_G[name]")
	mustContain(t, got, "rawset(_G, name, 5)")
}

func TestDostringBecomesLoad(t *testing.T) {
	mustContain(t, convert(t, "dostring(chunk)"), "load(chunk)()")
}

func TestFileFunctionsBecomeIoLibrary(t *testing.T) {
	got := convert(t, "local f = openfile(path, \"w\")\nwrite(f, s)\ncloseFILE\nclosefile(f)")
	mustContain(t, got, "io.open(path,")
	mustContain(t, got, "io.write(f, s)")
	mustContain(t, got, "(f):close()")
}

func TestGenericForGetsPairs(t *testing.T) {
	got := convert(t, "for k, v in tbl do\n\tprint(k)\nend")
	mustContain(t, got, "for k, v in pairs(tbl) do")
}

func TestGenericForKeepsExistingIterator(t *testing.T) {
	src := "for k, v in pairs(tbl) do end\nfor i, v in ipairs(t) do end\nfor w in string.gmatch(s, \"%a+\") do end"
	got := convert(t, src)
	mustNotContain(t, got, "pairs(pairs(")
	mustNotContain(t, got, "pairs(ipairs(")
	mustNotContain(t, got, "pairs(string.gmatch(")
}

func TestVarargFunctionDeclaresArg(t *testing.T) {
	got := convert(t, "function Log(...)\n\tlocal n = arg.n\nend")
	mustContain(t, got, "local arg = table.pack(...)")
}

func TestOneLineVarargFunctionStaysWhole(t *testing.T) {
	// The real line from activitysys/activitydetail.lua:75.  A -- comment here would swallow the
	// body, so the declaration has to be inserted with a long comment.
	got := convert(t, "local _pack_ = function(...) return arg end")
	mustContain(t, got, "table.pack(...)")
	mustContain(t, got, "return arg end")
}

func TestVarargFunctionWithoutArgIsUntouched(t *testing.T) {
	mustNotContain(t, convert(t, "function Log(...)\n\treturn select(1, ...)\nend"), "table.pack")
}

func TestNonVarargFunctionIsUntouched(t *testing.T) {
	mustNotContain(t, convert(t, "function Log(a, b)\n\treturn a\nend"), "table.pack")
}

func TestLongStringsAreUntouched(t *testing.T) {
	got := convert(t, "local s = [[ format(getn(x)) %up ]]\nlocal n = getn(y)")
	mustContain(t, got, "[[ format(getn(x)) %up ]]")
	mustContain(t, got, "#(y)")
}

func TestLineCommentsAreUntouched(t *testing.T) {
	got := convert(t, "-- tinsert(t) %up\nlocal n = getn(y)")
	mustContain(t, got, "-- tinsert(t) %up")
	mustContain(t, got, "#(y)")
}

func TestLongCommentOpenerIsBrokenUp(t *testing.T) {
	// Lua 4's lexer runs -- to the end of the line whatever follows, so these three lines were
	// three separate comments and the code after them was live.  Lua 5.4 would swallow it all.
	got := convert(t, "--[[ mo ta\n-- dong hai\n-- het ]],\nlocal n = getn(y)")
	mustContain(t, got, "-- [[ mo ta")
	mustContain(t, got, "#(y)")
}

func TestCodeUnderALongCommentOpenerStaysCode(t *testing.T) {
	// Under Lua 4 semantics the second line is code, so it must still be converted.
	got := convert(t, "--[[\nlocal n = getn(y)\n]]")
	mustContain(t, got, "#(y)")
}

func TestNestedLongStringIsReported(t *testing.T) {
	r := Convert("local s = [[ outer [[ inner ]] still outer ]]\n")
	if len(r.Notes) == 0 {
		t.Fatal("expected a note about the nested long string")
	}
}

func TestEscapedQuoteInsideStringDoesNotLeakIntoCode(t *testing.T) {
	got := convert(t, `local s = "a \" getn(x)"  local n = getn(y)`)
	mustContain(t, got, `"a \" getn(x)"`)
	mustContain(t, got, "#(y)")
}

func TestCallArgumentsContainingStringsAreParsed(t *testing.T) {
	got := convert(t, `local n = getn(Split(s, ","))`)
	mustContain(t, got, `#(Split(s, ","))`)
}

func TestMultilineCallIsLeftForAHuman(t *testing.T) {
	// parseArgs refuses to reason across lines, so getn keeps its old shape and is reported.
	src := "local n = getn(\n\ttbl)"
	got := convert(t, src)
	mustContain(t, got, "getn(")
}

func TestUpvalueMarkerSeparatedFromItsNameIsRemoved(t *testing.T) {
	// The real lines from activitysys/config/1007/extend.lua:66 and .../1026/extend.lua:43.
	got := convert(t, "local a =% tbLimit\nlocal b = % tbBox:Value()")
	mustContain(t, got, "local a = tbLimit")
	mustContain(t, got, "local b =  tbBox:Value()")
}

func TestLua4TableSeparatorIsDropped(t *testing.T) {
	// The real line from misc/daiyitoushi/toushi_head.lua:684.
	got := convert(t, "local t = {\n\t182,\n\t630,;\t-- ghi chu\n}")
	mustContain(t, got, "630,\t-- ghi chu")
	mustNotContain(t, got, ",;")
}

func TestSemicolonBetweenStatementsSurvives(t *testing.T) {
	mustContain(t, convert(t, "local a = 1; local b = 2"), "local a = 1; local b = 2")
}

func TestCStyleCommentBecomesALongComment(t *testing.T) {
	// Nine files in the reference tree open a /* block.  The embedded Lua 4 lexer has no case for
	// '/', so those files never compiled at all; as a long comment the block stays disabled and
	// the rest of the file finally loads.
	got := convert(t, "local t = {}\n/*\nlocal x = 1\n*/\nlocal n = getn(t)")
	mustContain(t, got, "--/*")
	mustContain(t, got, "--local x = 1")
	mustContain(t, got, "--*/")
	mustContain(t, got, "#(t)")
	if convert(t, got) != got {
		t.Fatalf("converting the result again changed it:\n%s", convert(t, got))
	}
}

func TestCStyleCommentInTheMiddleOfALineIsLeftAlone(t *testing.T) {
	// Only a block that owns its lines can be commented out line by line without moving code.
	mustContain(t, convert(t, "local a = 1 /* junk */"), "/* junk */")
}

func TestCStyleCommentInsideAStringIsNotTouched(t *testing.T) {
	mustContain(t, convert(t, `local s = "a /* b */ c"`), `"a /* b */ c"`)
}

func TestVarargDeclarationIsNotAddedTwice(t *testing.T) {
	once := convert(t, "function Log(...)\n\tlocal n = arg.n\nend")
	twice := convert(t, once)
	if once != twice {
		t.Fatalf("a second pass added another declaration:\n%s", twice)
	}
}

func TestUnterminatedCStyleCommentIsReported(t *testing.T) {
	r := Convert("local t = {}\n/*\nlocal x = 1\n")
	if len(r.Notes) == 0 {
		t.Fatal("expected a note about the unterminated comment")
	}
}

func TestNumberGluedToAWordIsSeparated(t *testing.T) {
	// The real line from tagnewplayer/func_check.lua:66.
	got := convert(t, "if (nDay >= 1and nDay <= 4) then end")
	mustContain(t, got, "1 and nDay")
}

func TestNumbersThatAreAlreadyValidAreNotTouched(t *testing.T) {
	got := convert(t, "local a = 0x1F  local b = 1e5  local c = 1.5e-3  local d = tb2  local e = 12")
	for _, want := range []string{"0x1F", "1e5", "1.5e-3", "tb2", "= 12"} {
		mustContain(t, got, want)
	}
	mustNotContain(t, got, "0x1 F")
	mustNotContain(t, got, "1 e5")
}

func TestFunctionDefinitionIsNotRewrittenAsACall(t *testing.T) {
	// The real line from task/tollgate/messenger/qianbaoku/messenger_baoxiangtask.lua:67.
	got := convert(t, "function pow(nBase, nFang)\n\treturn 1\nend\nfunction format(s)\n\treturn s\nend")
	mustContain(t, got, "function pow(nBase, nFang)")
	mustContain(t, got, "function format(s)")
}

func TestCallsAreStillRewrittenNextToADefinition(t *testing.T) {
	got := convert(t, "function pow(a, b)\n\treturn 1\nend\nlocal v = pow(2, 3)")
	mustContain(t, got, "function pow(a, b)")
	mustContain(t, got, "((2)^(3))")
}

func TestReservedWordInBecomesAName(t *testing.T) {
	// The real lines from global/jingli.lua:530.
	got := convert(t, "function WorldEntrance(playerindex, in)\n\tif (in == 1) then return 1 end\nend")
	mustContain(t, got, "function WorldEntrance(playerindex, _in)")
	mustContain(t, got, "if (_in == 1)")
}

func TestForInKeywordIsNotRenamed(t *testing.T) {
	got := convert(t, "for k, v in tbl do end\nfor i = 1, 3 do end")
	mustContain(t, got, "for k, v in pairs(tbl) do")
	mustNotContain(t, got, "_in")
}

func TestUnknownEscapeLosesItsBackslashLikeLua4(t *testing.T) {
	// The real line from activitysys/config/2/extend.lua:14.  Lua 4 turned \d into d, so the path
	// it built was \script\dailogsys\dailogsay.lua; Lua 5.4 refuses to compile \d at all.
	got := convert(t, `Include("\\script\\\dailogsys\\\dailogsay.lua")`)
	mustContain(t, got, `Include("\\script\\dailogsys\\dailogsay.lua")`)
}

func TestKnownEscapesAreKept(t *testing.T) {
	got := convert(t, `local s = "a\tb\nc\\d\"e\65"`)
	mustContain(t, got, `"a\tb\nc\\d\"e\65"`)
}

func TestEscapesInLongStringsAreNotTouched(t *testing.T) {
	mustContain(t, convert(t, `local s = [[\script\x]]`), `[[\script\x]]`)
}

func TestChangeCountsAreReported(t *testing.T) {
	r := Convert("local n = getn(t)  local s = format(\"%d\", n)  local v = %up")
	if r.Changes["getn -> #"] != 1 {
		t.Fatalf("getn count = %d, want 1", r.Changes["getn -> #"])
	}
	if r.Changes["format -> string.format"] != 1 {
		t.Fatalf("format count = %d, want 1", r.Changes["format -> string.format"])
	}
	if r.Changes["upvalue %name"] != 1 {
		t.Fatalf("upvalue count = %d, want 1", r.Changes["upvalue %name"])
	}
	if r.Total() != 3 {
		t.Fatalf("total = %d, want 3", r.Total())
	}
}

func TestAlreadyConvertedSourceIsStable(t *testing.T) {
	// Running the converter twice must not change anything the second time.
	once := convert(t, "local n = getn(t)\nfor k, v in t do end\nlocal s = format(\"%d\", n)\n")
	twice := convert(t, once)
	if once != twice {
		t.Fatalf("conversion is not idempotent:\nfirst:\n%s\nsecond:\n%s", once, twice)
	}
}

func TestRealScriptShape(t *testing.T) {
	src := `-- kiem tra cap do
function GetLevel(nId)
	local tbl = {}
	for i = 1, getn(LevelTable) do
		tinsert(tbl, format("%d", LevelTable[i]))
	end
	for k, v in tbl do
		if mod(k, 2) == 0 then
			Say(v)
		end
	end
	return floor(getn(tbl) / 2)
end`
	got := convert(t, src)
	for _, want := range []string{
		"#(LevelTable)", "table.insert(", `string.format("%d"`,
		"for k, v in pairs(tbl) do", "math.fmod(k, 2)", "math.floor(#(tbl) / 2)",
	} {
		mustContain(t, got, want)
	}
	mustContain(t, got, "-- kiem tra cap do")
}
