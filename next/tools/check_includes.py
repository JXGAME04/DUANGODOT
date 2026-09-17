#!/usr/bin/env python3
"""check_includes.py [dir ...] - C++ files that use a standard name without including its header.

MSVC's headers pull each other in generously, libstdc++'s do not: `std::uint16_t` compiles on
Windows without <cstdint> and stops the Linux build (that is how server/core/.../Result.h broke the
first CI run).  This is the cheap half of include-what-you-use: for the names this code base
really uses, the header has to be included by the file itself or by one of THIS PROJECT's headers
it includes (that chain is the same on every compiler; what <string> drags in is not).  Exit code 1
when something is missing.

    python tools/check_includes.py                # server/
    python tools/check_includes.py server/zone
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# header -> the names that need it
NEEDS = {
    "cstdint": r"\bstd::u?int(8|16|32|64|ptr|max)_t\b|\bu?int(8|16|32|64)_t\b",
    "cstddef": r"\bstd::(size_t|ptrdiff_t|byte|nullptr_t)\b",
    "cstring": r"\bstd::(memcpy|memset|memcmp|memmove|strlen|strcmp|strncmp|strchr|strerror)\b",
    "cstdlib": r"\bstd::(abs|getenv|strtol|strtoul|strtoll|strtod|exit|abort|malloc|free|system)\b",
    "cmath": r"\bstd::(sqrt|floor|ceil|round|lround|fabs|hypot|atan2|sin|cos|pow|fmod|isfinite|isnan)\b",
    "cstdio": r"\bstd::(printf|snprintf|fprintf|fopen|fclose|fputs|FILE|remove|rename)\b",
    "string": r"\bstd::(string|to_string|stoi|stol|stoul|stoull|stod|getline)\b(?!_view)",
    "string_view": r"\bstd::string_view\b",
    "vector": r"\bstd::vector\b",
    "array": r"\bstd::array\b",
    "deque": r"\bstd::deque\b",
    "map": r"\bstd::(multi)?map\b",
    "set": r"\bstd::(multi)?set\b",
    "unordered_map": r"\bstd::unordered_(multi)?map\b",
    "unordered_set": r"\bstd::unordered_(multi)?set\b",
    "optional": r"\bstd::(optional|nullopt)\b",
    "variant": r"\bstd::(variant|get_if|holds_alternative|visit|monostate)\b",
    "memory": r"\bstd::(unique_ptr|shared_ptr|weak_ptr|make_unique|make_shared|enable_shared_from_this)\b",
    "functional": r"\bstd::(function|bind|ref|cref|hash|less|greater)\b",
    "utility": r"\bstd::(move|forward|pair|swap|exchange|make_pair)\b",
    "algorithm": r"\bstd::(min|max|sort|stable_sort|find|find_if|remove_if|remove|clamp|fill|copy|any_of|all_of|none_of|lower_bound|upper_bound|reverse|unique|count_if|nth_element|minmax|max_element|min_element|erase_if|transform)\b",
    "numeric": r"\bstd::(accumulate|iota|reduce)\b",
    "limits": r"\bstd::numeric_limits\b",
    "chrono": r"\bstd::chrono\b",
    "thread": r"\bstd::(thread|jthread|this_thread)\b",
    "mutex": r"\bstd::(mutex|lock_guard|unique_lock|scoped_lock|once_flag|call_once|recursive_mutex)\b",
    "condition_variable": r"\bstd::condition_variable\b",
    "atomic": r"\bstd::atomic\b",
    "span": r"\bstd::span\b",
    "filesystem": r"\bstd::filesystem\b",
    "fstream": r"\bstd::(ifstream|ofstream|fstream)\b",
    "sstream": r"\bstd::(stringstream|ostringstream|istringstream)\b",
    "iostream": r"\bstd::(cout|cerr|cin|clog)\b",
    "stdexcept": r"\bstd::(runtime_error|logic_error|invalid_argument|out_of_range|length_error)\b",
    "exception": r"\bstd::(exception|terminate|current_exception)\b(?!_ptr)",
    "system_error": r"\bstd::(error_code|system_error|errc)\b",
    "type_traits": r"\bstd::(is_same|is_same_v|enable_if|enable_if_t|decay_t|remove_cvref_t|is_trivially_copyable|is_trivially_copyable_v|underlying_type_t|is_integral_v|is_enum_v|conditional_t)\b",
    "tuple": r"\bstd::(tuple|tie|make_tuple|apply)\b",
    "bit": r"\bstd::(bit_cast|popcount|countl_zero|countr_zero|endian|byteswap)\b",
    "random": r"\bstd::(mt19937|mt19937_64|uniform_int_distribution|uniform_real_distribution|random_device)\b",
    "charconv": r"\bstd::(from_chars|to_chars)\b",
    "cassert": r"\bassert\s*\(",
    "cctype": r"\bstd::(isdigit|isalpha|isalnum|isspace|tolower|toupper|isprint)\b",
    "initializer_list": r"\bstd::initializer_list\b",
    "queue": r"\bstd::(queue|priority_queue)\b",
    "list": r"\bstd::list\b",
    "future": r"\bstd::(future|promise|async|packaged_task)\b",
    "format": r"\bstd::format\b",
    "source_location": r"\bstd::source_location\b",
    "compare": r"\bstd::(strong_ordering|weak_ordering|partial_ordering)\b",
    "cerrno": r"\berrno\b",
    "ctime": r"\bstd::(time_t|tm|time|gmtime|localtime|strftime|mktime)\b",
}
COMPILED = {h: re.compile(p) for h, p in NEEDS.items()}
INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
STRING = re.compile(r'"(?:\\.|[^"\\\n])*"')


INCLUDE_DIRS = [os.path.join(ROOT, "server", m, "include") for m in ("common", "core", "net", "zone")] + [
    os.path.join(ROOT, "server", m, "src") for m in ("common", "core", "net", "zone")]
_closure: dict[str, set[str]] = {}


def resolve(name: str, from_dir: str) -> str | None:
    for base in [from_dir, *INCLUDE_DIRS]:
        cand = os.path.normpath(os.path.join(base, name))
        if os.path.isfile(cand):
            return cand
    return None


def closure(path: str, seen: set[str] | None = None) -> set[str]:
    """Every header name reachable from `path` through THIS PROJECT's headers.  What a standard
    header drags in differs between compilers; what our own headers include does not."""
    path = os.path.normpath(path)
    if path in _closure:
        return _closure[path]
    seen = seen or set()
    if path in seen:
        return set()
    seen.add(path)
    try:
        text = open(path, encoding="utf-8", errors="replace").read()
    except OSError:
        return set()
    names = set(INCLUDE.findall(text))
    for name in list(names):
        target = resolve(name, os.path.dirname(path))
        if target:
            names |= closure(target, seen)
    _closure[path] = names
    return names


def check(path: str) -> list[str]:
    text = open(path, encoding="utf-8", errors="replace").read()
    have = closure(path)
    code = STRING.sub('""', COMMENT.sub("", text))
    missing = []
    for header, pattern in COMPILED.items():
        if header in have:
            continue
        m = pattern.search(code)
        if m:
            line = code.count(chr(10), 0, m.start()) + 1
            missing.append(f"{os.path.relpath(path, ROOT)}:{line}: uses {m.group(0)} without #include <{header}>")
    return missing


STD_INCLUDE = re.compile(r"^#include <([a-z_]+)>\s*$")


def add_include(path: str, header: str) -> None:
    """Puts `#include <header>` among the file's standard includes, in alphabetical order."""
    lines = open(path, encoding="utf-8").read().split("\n")
    std = [i for i, line in enumerate(lines) if STD_INCLUDE.match(line)]
    new = f"#include <{header}>"
    if std:
        at = std[-1] + 1
        for i in std:
            if STD_INCLUDE.match(lines[i]).group(1) > header:
                at = i
                break
        lines.insert(at, new)
    else:
        anchors = [i for i, line in enumerate(lines) if line.startswith("#include") or line.startswith("#pragma once")]
        at = (anchors[0] + 1) if anchors and lines[anchors[0]].startswith("#pragma once") else (anchors[-1] + 1 if anchors else 0)
        block = ["", new] if at and lines[at - 1].startswith("#pragma once") else [new]
        if not (at < len(lines) and lines[at].strip() == ""):
            block.append("")
        lines[at:at] = block
    open(path, "w", encoding="utf-8", newline="\n").write("\n".join(lines))


def main() -> int:
    fix = "--fix" in sys.argv
    dirs = [a for a in sys.argv[1:] if not a.startswith("--")] or ["server"]
    if fix:
        for _ in range(3):      # an added header can change what the next pass sees
            todo = []
            for d in dirs:
                for base, _dirs, files in os.walk(os.path.join(ROOT, d)):
                    for f in files:
                        if f.endswith((".h", ".hpp", ".cpp")) and not f.endswith((".pb.h", ".pb.cc")):
                            for line in check(os.path.join(base, f)):
                                todo.append((os.path.join(base, f), line.rsplit("<", 1)[1].rstrip(">")))
            if not todo:
                break
            for path, header in todo:
                add_include(path, header)
            _closure.clear()
            print(f"check_includes: added {len(todo)} include(s)")
    problems = []
    count = 0
    for d in dirs:
        for base, _, files in os.walk(os.path.join(ROOT, d)):
            if "generated" in base or os.sep + "build" in base:
                continue
            for f in files:
                if f.endswith((".h", ".hpp", ".cpp")) and not f.endswith((".pb.h", ".pb.cc")):
                    count += 1
                    problems += check(os.path.join(base, f))
    for p in problems:
        print(p)
    print(f"check_includes: {count} files, {len(problems)} missing include(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
