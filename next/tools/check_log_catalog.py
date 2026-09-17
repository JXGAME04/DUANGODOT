#!/usr/bin/env python3
"""check_log_catalog.py [language] - every sentence the servers can print has its Vietnamese.

The console of jx_zone and the gateway speaks through config/log.<language>.json (docs/LOGGING.md):
the code logs an English `msg`, the console looks it up.  A message that is not in the catalogue is
printed in English - harmless, but the owner asked for a console that reads the same all the way
down, so a new log line has to come with its sentence.  This lists what is missing:

    messages    exit code 1      (the sentences)
    categories  exit code 1      (their first part: "zone" of "zone.fight")
    fields      reported only    (a field name reads fine in English; translate the common ones)

It reads the C++ under server/ (tests left out) and the Go of the gateway and what it uses; the
tools (jxassets, jxbot, jxrecord...) talk to a developer and are not checked.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CPP_CALL = re.compile(r'log::(?:trace|debug|info|warn|error|fatal|write)\((?:[^"();]*,)?\s*"([^"]+)"\s*,\s*"([^"]+)"')
CPP_KEY = re.compile(r'kv\("([^"]+)"')
GO_CALL = re.compile(r'log\.(?:Trace|Debug|Info|Warn|Error|Fatal)(?:Ctx)?\(\s*(?:[a-zA-Z_.()]+,\s*)?"([^"]+)"\s*,\s*"([^"]+)"')
GO_KEY = re.compile(r'log\.F\("([^"]+)"')
GO_DIRS = ["services/cmd/gateway", "services/internal", "services/pkg/auth", "services/pkg/persist", "services/pkg/wire"]


def scan(root, exts, call, key, skip):
    found = {"messages": {}, "categories": {}, "fields": {}}
    for base, _, files in os.walk(os.path.join(ROOT, root)):
        rel = os.path.relpath(base, ROOT).replace("\\", "/")
        if any(part in rel for part in skip):
            continue
        for name in files:
            if not name.endswith(exts) or name.endswith("_test.go"):
                continue
            text = open(os.path.join(base, name), encoding="utf-8", errors="replace").read()
            for m in call.finditer(text):
                found["categories"].setdefault(m.group(1).split(".")[0], f"{rel}/{name}")
                found["messages"].setdefault(m.group(2), f"{rel}/{name}")
            for m in key.finditer(text):
                found["fields"].setdefault(m.group(1), f"{rel}/{name}")
    return found


def main() -> int:
    language = sys.argv[1] if len(sys.argv) > 1 else "vi"
    path = os.path.join(ROOT, "config", f"log.{language}.json")
    catalog = json.load(open(path, encoding="utf-8"))
    used = scan("server", (".cpp", ".h", ".hpp"), CPP_CALL, CPP_KEY, ("tests",))
    for d in GO_DIRS:
        if os.path.isdir(os.path.join(ROOT, d)):
            part = scan(d, (".go",), GO_CALL, GO_KEY, ())
            for kind in used:
                for k, v in part[kind].items():
                    used[kind].setdefault(k, v)
    bad = 0
    for kind, fatal in (("messages", True), ("categories", True), ("fields", False)):
        missing = {k: v for k, v in used[kind].items() if not catalog.get(kind, {}).get(k)}
        for k, where in sorted(missing.items()):
            print(f"{'MISSING' if fatal else 'untranslated'} {kind[:-1]}: \"{k}\"   ({where})")
        if fatal:
            bad += len(missing)
        unused = [k for k in catalog.get(kind, {}) if k not in used[kind]]
        print(f"check_log_catalog: {kind}: {len(used[kind])} used, {len(missing)} without a translation, {len(unused)} in the catalogue but no longer in the code")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
