// Package jxlua converts the old game's Lua 4.0 scripts into real Lua 5.4 source.
//
// The zone used to run the old scripts unchanged by installing a Lua 4 compatibility layer
// (strfind, getn, floor ... re-exported as globals).  The owner's decision is that there is no
// Lua 4 base any more: the scripts themselves become Lua 5.4, so what runs on the server is what a
// Lua 5.4 developer reads, and the standard libraries behave exactly as documented.
//
// What the 5059 scripts of the reference server actually use (measured, not guessed):
//
//	format 4537  getn 4421  floor 4344  tinsert 2987  random 1743  mod 691
//	for..in without pairs 710  unpack 319  arg (vararg table) 200  strfind 197  strsub 192
//	write 66  call 61  ceil 56  strlen 55  openfile 49  sort 44  closefile 42  gsub 35 ...
//	plus 4045 uses of the Lua 4 upvalue syntax %name
//
// Every pass works on a *masked* copy of the source in which strings and comments are blanked out
// while keeping their exact byte length.  Matching happens on the mask, splicing happens on the
// real text, so the two always share the same indices: a pattern like "%d" inside a string, or a
// Chinese comment, is never rewritten, and a call whose arguments contain strings is still parsed.
package jxlua

import (
	"fmt"
	"regexp"
	"strings"
)

// Result of converting one file.
type Result struct {
	Source  string         // converted Lua 5.4 source
	Changes map[string]int // what was rewritten, for the report
	Notes   []string       // things a human should look at
}

func (r *Result) note(format string, args ...any) {
	r.Notes = append(r.Notes, fmt.Sprintf(format, args...))
}

func (r *Result) count(kind string, n int) {
	if n > 0 {
		r.Changes[kind] += n
	}
}

// Total number of rewrites, for the report.
func (r *Result) Total() int {
	n := 0
	for _, v := range r.Changes {
		n += v
	}
	return n
}

// ---- masking -------------------------------------------------------------------------------

// blank is what a string or comment byte becomes in the mask.  It is not a word character, not a
// bracket and not a quote, so no pattern can match across it.
const blank = '\x01'

// mask returns a copy of src, of the same byte length, with every byte that belongs to a string or
// a comment replaced by blank.  Newlines are kept so line-anchored patterns still behave.
func mask(src string) string {
	out := []byte(src)
	blankOut := func(from, to int) {
		for i := from; i < to && i < len(out); i++ {
			if out[i] != '\n' {
				out[i] = blank
			}
		}
	}
	i := 0
	for i < len(src) {
		switch c := src[i]; {
		case c == '-' && i+1 < len(src) && src[i+1] == '-':
			start := i
			i = endOfLine(src, i) // Lua 4 has no long comment: -- always runs to the newline
			blankOut(start, i)
		case c == '"' || c == '\'':
			start := i
			i = skipShortString(src, i)
			blankOut(start, i)
		case c == '[' && i+1 < len(src) && src[i+1] == '[':
			start := i
			i = findLongEnd(src, i+2)
			blankOut(start, i)
		default:
			i++
		}
	}
	return string(out)
}

func endOfLine(src string, i int) int {
	for i < len(src) && src[i] != '\n' {
		i++
	}
	return i
}

// skipShortString returns the index just past the short string starting at src[i].
func skipShortString(src string, i int) int {
	quote := src[i]
	i++
	for i < len(src) {
		switch src[i] {
		case '\\':
			i += 2
			continue
		case quote:
			return i + 1
		case '\n': // unterminated: Lua would reject it, so do not run past the line
			return i + 1
		}
		i++
	}
	return len(src)
}

// findLongEnd returns the index just past the ]] that closes the long string opened before `from`.
// Lua 4 long strings nest: an inner [[ has to be closed before the outer ]] counts (llex.c:200-210).
// Lua 5.4 does not nest, which is why a nested one is reported rather than silently converted.
func findLongEnd(src string, from int) int {
	depth := 0
	for i := from; i+1 < len(src); i++ {
		switch {
		case src[i] == '[' && src[i+1] == '[':
			depth++
			i++
		case src[i] == ']' && src[i+1] == ']':
			if depth == 0 {
				return i + 2
			}
			depth--
			i++
		}
	}
	return len(src)
}

// nestsLongString reports whether any [[ ]] string in the source holds another [[, which Lua 5.4
// would close at the first ]].
func nestsLongString(src string) bool {
	i := 0
	for i < len(src) {
		switch c := src[i]; {
		case c == '-' && i+1 < len(src) && src[i+1] == '-':
			i = endOfLine(src, i)
		case c == '"' || c == '\'':
			i = skipShortString(src, i)
		case c == '[' && i+1 < len(src) && src[i+1] == '[':
			end := findLongEnd(src, i+2)
			body := src[i+2 : max(i+2, end-2)]
			if strings.Contains(body, "[[") {
				return true
			}
			i = end
		default:
			i++
		}
	}
	return false
}

// ---- the rewrites --------------------------------------------------------------------------

// Lua 4 global functions that moved into a library in Lua 5.
var renames = []struct{ from, to string }{
	// string
	{"strfind", "string.find"}, {"strsub", "string.sub"}, {"strlen", "string.len"},
	{"strlower", "string.lower"}, {"strupper", "string.upper"}, {"strrep", "string.rep"},
	{"strbyte", "string.byte"}, {"strchar", "string.char"}, {"format", "string.format"},
	{"gsub", "string.gsub"}, {"gfind", "string.gmatch"},
	// table
	{"tinsert", "table.insert"}, {"tremove", "table.remove"}, {"sort", "table.sort"},
	{"unpack", "table.unpack"},
	// math.  Lua 4 mod() is C fmod, not the floor-modulo of Lua 5's % operator.
	{"floor", "math.floor"}, {"ceil", "math.ceil"}, {"abs", "math.abs"}, {"sqrt", "math.sqrt"},
	{"min", "math.min"}, {"max", "math.max"}, {"random", "math.random"},
	{"randomseed", "math.randomseed"}, {"sin", "math.sin"}, {"cos", "math.cos"},
	{"tan", "math.tan"}, {"exp", "math.exp"}, {"log", "math.log"}, {"mod", "math.fmod"},
	// io / os
	{"openfile", "io.open"}, {"write", "io.write"}, {"read", "io.read"},
	{"date", "os.date"}, {"clock", "os.clock"}, {"time", "os.time"},
	// base
	{"loadstring", "load"},
}

var (
	rxForIn     = regexp.MustCompile(`\bfor\s+([A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)*)\s+in\s+([^\n]*?)\s+do\b`)
	rxIterator  = regexp.MustCompile(`^(pairs|ipairs|next|string\.gmatch|gmatch|gfind)\b`)
	rxFuncVarg  = regexp.MustCompile(`\bfunction\b[^(\n]*\(([^)\n]*)\)`)
	rxArgUse    = regexp.MustCompile(`(^|[^\w.])arg\b`)
	rxArgPack   = regexp.MustCompile(`local\s+arg\s*=\s*table\.pack\s*\(`)
	rxTableGetn = regexp.MustCompile(`(^|[^\w.:])(?:table\.)?getn\s*\(`)
	rxSetn      = regexp.MustCompile(`(^|[^\w.:])(?:table\.)?setn\s*\(`)
	rxCall      = regexp.MustCompile(`(^|[^\w.:])call\s*\(`)
	rxClosefile = regexp.MustCompile(`(^|[^\w.:])closefile\s*\(`)
	rxPow       = regexp.MustCompile(`(^|[^\w.:])(?:math\.)?pow\s*\(`)
	rxGetGlobal = regexp.MustCompile(`(^|[^\w.:])getglobal\s*\(`)
	rxSetGlobal = regexp.MustCompile(`(^|[^\w.:])setglobal\s*\(`)
	rxDoString  = regexp.MustCompile(`(^|[^\w.:])dostring\s*\(`)
	rxBlockWord = regexp.MustCompile(`\b(function|if|for|while|do|end|repeat|until)\b`)
	rxWordIn    = regexp.MustCompile(`(^|[^\w.:])in\b`)
	rxLua4Only  = regexp.MustCompile(`(^|[^\w.:])(getn|setn|strfind|strsub|strlen|strlower|strupper|strrep|strbyte|strchar|tinsert|tremove|foreach|foreachi|gfind|openfile|closefile|readfrom|writeto|dostring|loadstring|setglobal|getglobal)\s*\(`)
)

// Convert turns one Lua 4 source file into Lua 5.4.
func Convert(src string) *Result {
	r := &Result{Changes: map[string]int{}}
	code := src
	code = convertLineComment(code, r)
	code = convertReservedIn(code, r)
	code = convertUpvalues(code, r)
	code = convertCallLike(code, r)
	code = convertRenames(code, r)
	code = convertForIn(code, r)
	code = convertVararg(code, r)
	code = convertTableSeparator(code, r)
	code = convertNumberGap(code, r)
	code = convertEscapes(code, r)
	code = convertCComment(code, r)
	r.Source = code
	if strings.Contains(mask(code), "/*") {
		r.note("contains an unterminated C style /* comment, which no Lua ever accepted")
	}
	if nestsLongString(code) {
		r.note("a [[ ]] string holds another [[; Lua 4 nested them, Lua 5.4 stops at the first ]]")
	}
	if m := rxLua4Only.FindStringSubmatch(mask(code)); m != nil {
		r.note("still contains the Lua 4 function %s(", m[2])
	}
	return r
}

// %name (Lua 4 upvalue) -> name.  Lua 5 closures capture locals directly, so the marker goes away.
//
// Lua 4 has no % operator at all - modulo is the function mod() - so in Lua 4 source every % in
// code is an upvalue marker, including the ones that follow a ')' or a ']'.  The single shape that
// a later edit could have meant as a modulo is `a%b` with the name touching the %, so that one is
// left alone and reported.
func convertUpvalues(code string, r *Result) string {
	m := mask(code)
	var out strings.Builder
	n, suspect := 0, 0
	for i := 0; i < len(code); i++ {
		if m[i] != '%' || !nameFollows(m, i+1) {
			out.WriteByte(code[i])
			continue
		}
		if i > 0 && isNameChar(m[i-1]) {
			out.WriteByte(code[i])
			suspect++
			continue
		}
		n++ // drop the % and keep the name
	}
	r.count("upvalue %name", n)
	if suspect > 0 {
		r.note("%d uses of %%name touch a name on the left and were kept as a modulo", suspect)
	}
	return out.String()
}

// nameFollows reports whether a name starts at or after i, past spaces and tabs.  The old scripts
// write both `%tbLog` and `= % tbLog`, and Lua 4 read them the same way.
func nameFollows(m string, i int) bool {
	for i < len(m) && (m[i] == ' ' || m[i] == '\t') {
		i++
	}
	return i < len(m) && isNameFirst(m[i])
}

// definesAFunction reports whether the name at i is the one being declared by a `function` right
// before it.  task/.../messenger_baoxiangtask.lua defines its own pow(), which must stay a
// definition instead of turning into the ^ operator.
func definesAFunction(m string, i int) bool {
	j := i
	for j > 0 && (m[j-1] == ' ' || m[j-1] == '\t') {
		j--
	}
	const kw = "function"
	if j < len(kw) || m[j-len(kw):j] != kw {
		return false
	}
	return j == len(kw) || !isNameChar(m[j-len(kw)-1])
}

func isNameFirst(c byte) bool {
	return c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
}

func isNameChar(c byte) bool {
	return isNameFirst(c) || (c >= '0' && c <= '9')
}

// Calls whose arguments have to be rearranged, not merely renamed.
func convertCallLike(code string, r *Result) string {
	code, n := rewriteCall(code, rxTableGetn, func(args []string) string {
		if len(args) != 1 {
			return ""
		}
		return "#(" + strings.TrimSpace(args[0]) + ")"
	})
	r.count("getn -> #", n)

	// Lua 5 has no setn: the length of a sequence is not stored any more.
	code, n = rewriteCall(code, rxSetn, func(args []string) string {
		if len(args) != 2 {
			return ""
		}
		return "nil --[[ setn removed in Lua 5 ]]"
	})
	r.count("setn removed", n)

	// call(f, args) -> f(table.unpack(args))
	code, n = rewriteCall(code, rxCall, func(args []string) string {
		if len(args) < 2 {
			return ""
		}
		return strings.TrimSpace(args[0]) + "(table.unpack(" + strings.TrimSpace(args[1]) + "))"
	})
	r.count("call -> unpack", n)

	// closefile(f) -> (f):close()
	code, n = rewriteCall(code, rxClosefile, func(args []string) string {
		if len(args) != 1 {
			return ""
		}
		return "(" + strings.TrimSpace(args[0]) + "):close()"
	})
	r.count("closefile -> :close", n)

	// pow(a, b) -> ((a)^(b)); math.pow is gone in 5.4.
	code, n = rewriteCall(code, rxPow, func(args []string) string {
		if len(args) != 2 {
			return ""
		}
		return "((" + strings.TrimSpace(args[0]) + ")^(" + strings.TrimSpace(args[1]) + "))"
	})
	r.count("pow -> ^", n)

	code, n = rewriteCall(code, rxGetGlobal, func(args []string) string {
		if len(args) != 1 {
			return ""
		}
		return "_G[" + strings.TrimSpace(args[0]) + "]"
	})
	r.count("getglobal -> _G[]", n)

	code, n = rewriteCall(code, rxSetGlobal, func(args []string) string {
		if len(args) != 2 {
			return ""
		}
		return "rawset(_G, " + strings.TrimSpace(args[0]) + ", " + strings.TrimSpace(args[1]) + ")"
	})
	r.count("setglobal -> rawset", n)

	code, n = rewriteCall(code, rxDoString, func(args []string) string {
		if len(args) != 1 {
			return ""
		}
		return "load(" + strings.TrimSpace(args[0]) + ")()"
	})
	r.count("dostring -> load()()", n)
	return code
}

// rewriteCall finds every match of head (whose match ends at the opening parenthesis and whose
// first group is the separator character before the name), extracts the balanced argument list and
// replaces the whole call with build(args).  build returns "" to leave that call alone.
func rewriteCall(code string, head *regexp.Regexp, build func(args []string) string) (string, int) {
	m := mask(code)
	var out strings.Builder
	count, pos := 0, 0
	for {
		loc := head.FindStringSubmatchIndex(m[pos:])
		if loc == nil {
			out.WriteString(code[pos:])
			break
		}
		nameStart := pos + loc[0]
		if loc[3] > 0 {
			nameStart = pos + loc[3] // just past the separator group
		}
		openParen := pos + loc[1] - 1 // the head match ends with '('
		end, args, ok := parseArgs(m, code, openParen)
		var replacement string
		if ok && !definesAFunction(m, nameStart) {
			replacement = build(args)
		}
		if replacement == "" {
			stop := openParen + 1
			out.WriteString(code[pos:stop])
			pos = stop
			continue
		}
		out.WriteString(code[pos:nameStart])
		out.WriteString(replacement)
		pos = end
		count++
	}
	return out.String(), count
}

// parseArgs reads a balanced argument list starting at the '(' at m[open].  Brackets are counted on
// the mask, so a parenthesis or a comma inside a string never counts; the argument text itself is
// cut from code.  It returns the index just past the ')' and the top level arguments.
func parseArgs(m, code string, open int) (end int, args []string, ok bool) {
	depth := 0
	last := open + 1
	for i := open; i < len(m); i++ {
		switch c := m[i]; c {
		case '(', '[', '{':
			depth++
		case ')', ']', '}':
			depth--
			if depth == 0 {
				if c != ')' {
					return 0, nil, false
				}
				args = append(args, code[last:i])
				if len(args) == 1 && strings.TrimSpace(args[0]) == "" {
					args = nil
				}
				return i + 1, args, true
			}
		case ',':
			if depth == 1 {
				args = append(args, code[last:i])
				last = i + 1
			}
		case '\n':
			return 0, nil, false // a call spanning lines is left to a human
		}
	}
	return 0, nil, false
}

func convertRenames(code string, r *Result) string {
	for _, ren := range renames {
		rx := regexp.MustCompile(`(^|[^\w.:])` + ren.from + `(\s*\()`)
		m := mask(code)
		var out strings.Builder
		n, pos := 0, 0
		for {
			loc := rx.FindStringSubmatchIndex(m[pos:])
			if loc == nil {
				out.WriteString(code[pos:])
				break
			}
			nameStart := pos + loc[0]
			if loc[3] > 0 {
				nameStart = pos + loc[3]
			}
			if definesAFunction(m, nameStart) { // `function pow(a, b)` declares one, it does not call it
				out.WriteString(code[pos : nameStart+len(ren.from)])
				pos = nameStart + len(ren.from)
				continue
			}
			out.WriteString(code[pos:nameStart])
			out.WriteString(ren.to)
			pos = nameStart + len(ren.from)
			n++
		}
		code = out.String()
		r.count(ren.from+" -> "+ren.to, n)
	}
	return code
}

// for k,v in t do  ->  for k,v in pairs(t) do
func convertForIn(code string, r *Result) string {
	m := mask(code)
	var out strings.Builder
	n, pos := 0, 0
	for {
		loc := rxForIn.FindStringSubmatchIndex(m[pos:])
		if loc == nil {
			out.WriteString(code[pos:])
			break
		}
		start, end := pos+loc[0], pos+loc[1]
		names := code[pos+loc[2] : pos+loc[3]]
		expr := strings.TrimSpace(code[pos+loc[4] : pos+loc[5]])
		maskedExpr := strings.TrimSpace(m[pos+loc[4] : pos+loc[5]])
		out.WriteString(code[pos:start])
		if expr == "" || rxIterator.MatchString(maskedExpr) {
			out.WriteString(code[start:end])
		} else {
			out.WriteString("for " + names + " in pairs(" + expr + ") do")
			n++
		}
		pos = end
	}
	r.count("for..in -> pairs()", n)
	return out.String()
}

// Lua 4 gave every vararg function an `arg` table; Lua 5.4 does not.  Where a function declares
// (...) and its body mentions arg, declare it: local arg = table.pack(...)
func convertVararg(code string, r *Result) string {
	if !strings.Contains(code, "arg") {
		return code
	}
	m := mask(code)
	var out strings.Builder
	pos, n := 0, 0
	for {
		loc := rxFuncVarg.FindStringIndex(m[pos:])
		if loc == nil {
			out.WriteString(code[pos:])
			break
		}
		end := pos + loc[1]
		header := m[pos+loc[0] : end]
		out.WriteString(code[pos:end])
		pos = end
		if !strings.Contains(header, "...") {
			continue
		}
		body := m[end:matchingEnd(m, end)]
		if !rxArgUse.MatchString(body) || rxArgPack.MatchString(body) {
			continue
		}
		// No comment of any kind: the header may sit on the same line as the whole body, and in
		// Lua 4 a -- comment swallows the rest of the line.
		out.WriteString(" local arg = table.pack(...) ")
		n++
	}
	r.count("vararg arg -> table.pack", n)
	return out.String()
}

// Lua 4 had no long comment: its lexer runs `--` to the end of the line whatever follows
// (llex.c:297-301).  So `--[[` opened nothing, and the lines under it were ordinary code or
// ordinary line comments.  Lua 5.4 would swallow everything up to the next ]], which in
// global/login_hint.lua leaves a stray comma behind, so the opener is broken up into `-- [[`.
func convertLineComment(code string, r *Result) string {
	if !strings.Contains(code, "--[") {
		return code
	}
	var out strings.Builder
	n, i := 0, 0
	for i < len(code) {
		switch c := code[i]; {
		case c == '-' && i+1 < len(code) && code[i+1] == '-':
			start := i
			i = endOfLine(code, i)
			text := code[start:i]
			if isLongCommentOpener(text) {
				out.WriteString("-- ")
				out.WriteString(text[2:])
				n++
			} else {
				out.WriteString(text)
			}
		case c == '"' || c == '\'':
			end := skipShortString(code, i)
			out.WriteString(code[i:end])
			i = end
		case c == '[' && i+1 < len(code) && code[i+1] == '[':
			end := findLongEnd(code, i+2)
			out.WriteString(code[i:end])
			i = end
		default:
			out.WriteByte(c)
			i++
		}
	}
	r.count("--[[ kept a line comment", n)
	return out.String()
}

// isLongCommentOpener reports whether a Lua 4 line comment would read as --[[ / --[=[ in Lua 5.4.
func isLongCommentOpener(text string) bool {
	if !strings.HasPrefix(text, "--[") {
		return false
	}
	i := 3
	for i < len(text) && text[i] == '=' {
		i++
	}
	return i < len(text) && text[i] == '['
}

// In Lua 4 a table constructor was `{ array part ; hash part }`, so `{182, 630,; }` was legal: the
// comma ended the array part and the semicolon opened an empty hash part.  In Lua 5 the two are one
// list and `,` and `;` are interchangeable separators, so a semicolon right after a comma (or right
// after the opening brace) is an empty field and a syntax error.  It is dropped.
func convertTableSeparator(code string, r *Result) string {
	if !strings.Contains(code, ";") {
		return code
	}
	m := mask(code)
	var out strings.Builder
	n := 0
	for i := 0; i < len(code); i++ {
		if m[i] == ';' && precededBy(m, i, ',', '{') {
			n++
			continue
		}
		out.WriteByte(code[i])
	}
	r.count("Lua 4 table separator", n)
	return out.String()
}

// precededBy reports whether the last character before i, past any whitespace, is one of want.
func precededBy(m string, i int, want ...byte) bool {
	for i--; i >= 0; i-- {
		switch m[i] {
		case ' ', '\t', '\r', '\n':
			continue
		}
		for _, w := range want {
			if m[i] == w {
				return true
			}
		}
		return false
	}
	return false
}

// Lua 4 stopped reading a number at the first character that could not extend it, so `1and` was
// the number 1 followed by the word `and`.  Lua 5.4 reads `1a` and calls it a malformed number, so
// the space the author left out is put back.
func convertNumberGap(code string, r *Result) string {
	m := mask(code)
	var out strings.Builder
	n := 0
	for i := 0; i < len(m); {
		if !isDigit(m[i]) || (i > 0 && (isNameChar(m[i-1]) || m[i-1] == '.')) {
			out.WriteByte(code[i])
			i++
			continue
		}
		end := endOfNumeral(m, i)
		out.WriteString(code[i:end])
		if end < len(m) && isNameFirst(m[end]) {
			out.WriteByte(' ')
			n++
		}
		i = end
	}
	r.count("number glued to a word", n)
	return out.String()
}

// endOfNumeral returns the index just past the Lua 5.4 numeral that starts at i.
func endOfNumeral(m string, i int) int {
	exponent := byte('e')
	if m[i] == '0' && i+1 < len(m) && (m[i+1] == 'x' || m[i+1] == 'X') {
		i += 2
		exponent = 'p'
		for i < len(m) && (isHexDigit(m[i]) || m[i] == '.') {
			i++
		}
	} else {
		for i < len(m) && (isDigit(m[i]) || m[i] == '.') {
			i++
		}
	}
	if i < len(m) && (m[i]|0x20) == exponent {
		j := i + 1
		if j < len(m) && (m[j] == '+' || m[j] == '-') {
			j++
		}
		if j < len(m) && isDigit(m[j]) {
			for i = j; i < len(m) && isDigit(m[i]); i++ {
			}
		}
	}
	return i
}

func isDigit(c byte) bool { return c >= '0' && c <= '9' }

func isHexDigit(c byte) bool {
	return isDigit(c) || (c|0x20) >= 'a' && (c|0x20) <= 'f'
}

// `in` is a reserved word in Lua 5.4 but not in this game's Lua 4: its lexer never listed it
// (llex.c:31-34) and the parser recognised the generic for by comparing the name to "in"
// (lparser.c:875).  So global/jingli.lua could name a parameter `in`, and here it becomes `_in`.
func convertReservedIn(code string, r *Result) string {
	if !strings.Contains(code, "in") {
		return code
	}
	m := mask(code)
	var out strings.Builder
	n, pos := 0, 0
	for {
		loc := rxWordIn.FindStringIndex(m[pos:])
		if loc == nil {
			out.WriteString(code[pos:])
			break
		}
		start, end := pos+loc[0], pos+loc[1]
		for start < end && m[start] != 'i' { // the pattern keeps the separator before the word
			start++
		}
		out.WriteString(code[pos:start])
		if isForIn(m, start) {
			out.WriteString(code[start:end])
		} else {
			out.WriteString("_in")
			n++
		}
		pos = end
	}
	r.count("reserved word in -> _in", n)
	return out.String()
}

// isForIn reports whether the `in` at i is the keyword of a generic for: walking back over names,
// commas and spaces has to land on `for`.
func isForIn(m string, i int) bool {
	j := i
	for j > 0 {
		c := m[j-1]
		if isNameChar(c) || c == ',' || c == ' ' || c == '\t' {
			j--
			continue
		}
		break
	}
	rest := m[j:i]
	return strings.HasPrefix(strings.TrimLeft(rest, " \t"), "for") && len(rest) > 3
}

// C style comments are not Lua in any version: this game's Lua 4 lexer has no case for '/'
// (llex.c:297-360), so the nine files that open a /* block never loaded at all.  The block is
// plainly meant to be disabled code, so it becomes a long comment and the rest of the file, which
// used to be unreachable, finally runs.
func convertCComment(code string, r *Result) string {
	if !strings.Contains(code, "/*") {
		return code
	}
	var out strings.Builder
	n, i := 0, 0
	for i < len(code) {
		switch c := code[i]; {
		case c == '-' && i+1 < len(code) && code[i+1] == '-':
			end := endOfLine(code, i)
			out.WriteString(code[i:end])
			i = end
		case c == '"' || c == '\'':
			end := skipShortString(code, i)
			out.WriteString(code[i:end])
			i = end
		case c == '[' && i+1 < len(code) && code[i+1] == '[':
			end := findLongEnd(code, i+2)
			out.WriteString(code[i:end])
			i = end
		case c == '/' && i+1 < len(code) && code[i+1] == '*':
			end := strings.Index(code[i+2:], "*/")
			if end < 0 || !startsTheLine(code, i) {
				out.WriteByte(c)
				i++
				continue
			}
			end += i + 4 // just past the */
			out.WriteString(commentEveryLine(code[i:end]))
			i = end
			n++
		default:
			out.WriteByte(c)
			i++
		}
	}
	r.count("/* */ block commented out", n)
	return out.String()
}

// startsTheLine reports whether only whitespace precedes i on its line.
func startsTheLine(code string, i int) bool {
	for i--; i >= 0 && code[i] != '\n'; i-- {
		if code[i] != ' ' && code[i] != '\t' && code[i] != '\r' {
			return false
		}
	}
	return true
}

// commentEveryLine prefixes each line with --.  A long comment would be shorter, but this shape
// survives running the converter twice: on a second pass the /* is already inside a line comment.
func commentEveryLine(block string) string {
	lines := strings.Split(block, "\n")
	for i, line := range lines {
		if i == len(lines)-1 && line == "" {
			continue
		}
		lines[i] = "--" + line
	}
	return strings.Join(lines, "\n")
}

// Lua 4 accepted any character after a backslash: an escape it did not know became the character
// itself, so "\script\\dailogsys" was the path \script\dailogsys.  Lua 5.4 rejects an unknown
// escape outright, so the backslash the old interpreter threw away is removed here.  The result is
// byte for byte the string the old server built, which is what the client was shown.
func convertEscapes(code string, r *Result) string {
	if !strings.Contains(code, `\`) {
		return code
	}
	var out strings.Builder
	n, i := 0, 0
	for i < len(code) {
		switch c := code[i]; {
		case c == '-' && i+1 < len(code) && code[i+1] == '-': // comment: copied whole
			start := i
			i = endOfLine(code, i)
			out.WriteString(code[start:i])
		case c == '[' && i+1 < len(code) && code[i+1] == '[': // long string: no escapes inside one
			start := i
			i = findLongEnd(code, i+2)
			out.WriteString(code[start:i])
		case c == '"' || c == '\'':
			end := skipShortString(code, i)
			out.WriteByte(code[i])
			for j := i + 1; j < end; j++ {
				if code[j] != '\\' || j+1 >= end {
					out.WriteByte(code[j])
					continue
				}
				if !knownEscape(code[j+1]) {
					n++ // drop the backslash, exactly as Lua 4 did
					continue
				}
				out.WriteByte(code[j])
				out.WriteByte(code[j+1])
				j++
			}
			i = end
		default:
			out.WriteByte(c)
			i++
		}
	}
	r.count("Lua 4 escape kept literal", n)
	return out.String()
}

// knownEscape reports whether Lua 5.4 gives \c the same meaning Lua 4 gave it.
func knownEscape(c byte) bool {
	switch c {
	case 'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '\n', '\r':
		return true
	}
	return c >= '0' && c <= '9'
}

// matchingEnd finds the `end` closing the block that was opened just before `from`.  It only has
// to bound a function body well enough to look for `arg`, so it counts keywords rather than
// parsing; a `do ... end` block inside makes it stop late, never early.
func matchingEnd(m string, from int) int {
	depth, pos := 1, from
	for depth > 0 {
		loc := rxBlockWord.FindStringIndex(m[pos:])
		if loc == nil {
			return len(m)
		}
		switch m[pos+loc[0] : pos+loc[1]] {
		case "function", "if", "for", "while", "repeat":
			depth++
		case "end", "until":
			depth--
		}
		pos += loc[1]
	}
	return pos
}
