#!/usr/bin/env python3
"""ci_annotate.py <ctest preset> - after a failed ctest: run the failed tests again and say WHAT
failed as GitHub annotations.

A red step only says "exit code 8".  The log that names the assertion needs a login to read, and a
person looking at the pull request should not have to open it anyway.  An annotation
(`::error file=...,line=...::text`) shows up on the run's summary page and is public through the
checks API, so the failing REQUIRE, with its expansion, is one click - or one curl - away:

    curl -s https://api.github.com/repos/<owner>/<repo>/check-runs/<id>/annotations

Catch2 prints `path(line): FAILED:` (MSVC style) or `path:line: FAILED:` followed by the expression
and "with expansion:"; a test that crashed or timed out has no such line, so the tail of its
output is reported instead.  GitHub keeps ten error annotations per step: the first ten are sent.
Always exits 1 - it only runs when the tests have already failed.

    ci_annotate.py --tail <file>...     the end of each file (a script step's output, zone.log,
                                        gateway.log) as one annotation each, error lines first
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAILED = re.compile(r"^(?P<file>.+?)[(:](?P<line>\d+)\)?: FAILED:\s*$")
TEST_START = re.compile(r"^\s*Start\s+\d+:\s+(?P<name>.+?)\s*$")
TEST_END = re.compile(r"^\s*\d+/\d+\s+Test\s+#\d+:\s+(?P<name>.+?)\s+\.+\s*(?P<result>\*\*\*\S.*|Passed|\S+)")


def escape(text: str) -> str:
    return text.replace("%", "%25").replace("\r", "").replace("\n", "%0A")


def relative(path: str) -> str:
    path = path.strip().replace("\\", "/")
    marker = "/next/"
    at = path.rfind(marker)
    return "next/" + path[at + len(marker):] if at >= 0 else path


def tails(files: list[str]) -> int:
    """`ci_annotate.py --tail <file>...`: after a failed script step (the end-to-end run), the end of
    each file as one annotation - a log file's error/fatal/warn lines first, else its last lines."""
    for path in files:
        full = path if os.path.isabs(path) or os.path.exists(path) else os.path.join(ROOT, path)   # as given, else under next/
        try:
            with open(full, encoding="utf-8", errors="replace") as f:
                lines = [l.rstrip() for l in f if l.strip()]
        except OSError:
            print(f"::warning title={escape(path)}::not written")
            continue
        loud = [l for l in lines if re.search(r'"lvl":"(error|fatal|warn)"|FAILED|Error|error:|SCRIPT ERROR', l)]
        chosen = (loud[-12:] if loud else []) + [l for l in lines[-15:] if l not in loud[-12:]]
        text = "\n".join(l[:300] for l in chosen)[-3500:]
        print(f"::error title={escape(path)}::" + escape(text or "(empty)"))
    return 1


def main() -> int:
    if len(sys.argv) > 2 and sys.argv[1] == "--tail":
        return tails(sys.argv[2:])
    preset = sys.argv[1]
    run = subprocess.run(["ctest", "--preset", preset, "--rerun-failed", "--output-on-failure"],
                         cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
    lines = (run.stdout + "\n" + run.stderr).split("\n")
    sent = 0
    current = ""
    body: list[str] = []
    found_in_test = 0
    for i, line in enumerate(lines):
        start = TEST_START.match(line)
        if start:
            current, body, found_in_test = start.group("name"), [], 0
            continue
        body.append(line)
        failed = FAILED.match(line.strip())
        if failed and sent < 10:
            detail = [l for l in lines[i + 1:i + 9] if l.strip()]
            print(f"::error file={relative(failed.group('file'))},line={failed.group('line')},title={escape(current or 'test')}::"
                  + escape("\n".join(detail)))
            sent += 1
            found_in_test += 1
            continue
        end = TEST_END.match(line)
        if end and "Passed" not in end.group("result") and found_in_test == 0 and sent < 10:
            tail = [l for l in body if l.strip()][-25:]
            print(f"::error title={escape(end.group('name') + ' ' + end.group('result'))}::" + escape("\n".join(tail)))
            sent += 1
    if sent == 0:
        print("::error title=ctest failed::" + escape("\n".join([l for l in lines if l.strip()][-30:])))
    print(run.stdout[-6000:])
    return 1


if __name__ == "__main__":
    sys.exit(main())
