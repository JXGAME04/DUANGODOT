#!/usr/bin/env python3
"""A call graph of a 32-bit ELF with the string constants each call passes - the ground the other
tools stand on.  Linear disassembly of a whole segment loses its footing in the data between
functions; this walks FUNCTIONS: every `call` target is a function start, every function is
disassembled from its start to the next start, and the pass is repeated until no new start turns
up.  Cached next to the binary as <elf>.calls.json (a minute for jx_linux_y, instant after that).

  re_calls.py <elf> build                         (re)build the cache
  re_calls.py <elf> callers <va-hex>              who calls this function
  re_calls.py <elf> strargs <va-hex> [n]          the strings passed to this function, most common first
  re_calls.py <elf> byarg <text>                  calls that pass a string containing <text>
"""
import bisect
import collections
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import re_elf  # noqa: E402

from capstone import CS_ARCH_X86, CS_MODE_32, Cs  # noqa: E402
from capstone.x86 import (X86_OP_IMM, X86_OP_MEM, X86_OP_REG, X86_REG_EAX, X86_REG_EBP, X86_REG_EBX,  # noqa: E402
                          X86_REG_ECX, X86_REG_EDI, X86_REG_EDX, X86_REG_ESI, X86_REG_ESP)

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


SAVED = (X86_REG_EBX, X86_REG_ESI, X86_REG_EDI)     # callee-saved: `this` usually lives here


def build(elf, seeds):
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    md.detail = True
    starts = set(seeds)
    code_segs = [s for s in elf.segments if s["flags"] & 1 and s["filesz"]]

    def seg_of(va):
        for s in code_segs:
            if s["va"] <= va < s["va"] + s["filesz"]:
                return s
        return None

    def end_of(start, ordered):
        # up to the next known function start, or 64 KB
        s = seg_of(start)
        i = bisect.bisect_right(ordered, start)
        return min(ordered[i] if i < len(ordered) else start + 0x10000, s["va"] + s["filesz"], start + 0x10000)

    # A function is walked from its start to the next known start.  A start found later inside
    # that range shortens it, so the function is walked again with the new end: in the end every
    # call is attributed to the function that really contains it, exactly once.
    walked = {}         # start -> (end it was walked with, its calls)
    while True:
        ordered = sorted(starts)
        todo = [s for s in ordered if seg_of(s) and (s not in walked or walked[s][0] != end_of(s, ordered))]
        if not todo:
            break
        for start in todo:
            s = seg_of(start)
            end = end_of(start, ordered)
            off = s["off"] + start - s["va"]
            found = []
            slots = {}      # [esp+N] -> constant stored there: the arguments of the next call
            regs = {}       # register -> constant it holds (mov reg, imm); ebx/esi/edi survive calls
            stack = {}      # [ebp-N] -> constant spilled there (mov [ebp-N], reg ... mov reg, [ebp-N])
            saved = {}      # [ebp-N] -> callee-saved register the prologue parked there
            inargs = {}     # [ebp+8+4k] -> value written over the function's own argument k
            targets = {}    # address -> (regs, stack) as they were at a forward jump to it
            nofall = False  # the previous instruction was ret / jmp: no fall-through here
            for ins in md.disasm(elf.d[off:off + (end - start)], start):
                ops = ins.operands
                if ins.address in targets:
                    # a block someone jumps to: its state is the jump's (replacing what is left
                    # after a ret / jmp, filling in what the fall-through does not know)
                    r2, s2 = targets[ins.address]
                    if nofall:
                        regs, stack = dict(r2), dict(s2)
                    else:
                        regs, stack = {**r2, **regs}, {**s2, **stack}
                nofall = False
                if ins.mnemonic.startswith("j") and ops and ops[0].type == X86_OP_IMM and (ops[0].imm & 0xFFFFFFFF) > ins.address:
                    t = ops[0].imm & 0xFFFFFFFF
                    r2, s2 = targets.get(t, ({}, {}))
                    targets[t] = ({**r2, **regs}, {**s2, **stack})
                if ins.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_MEM and ops[0].mem.base == X86_REG_ESP and ops[0].mem.index == 0:
                    if ops[1].type == X86_OP_IMM:
                        slots[ops[0].mem.disp] = ops[1].imm & 0xFFFFFFFF
                    elif ops[1].type == X86_OP_REG and ops[1].reg in regs:
                        slots[ops[0].mem.disp] = regs[ops[1].reg]
                    else:
                        slots.pop(ops[0].mem.disp, None)
                elif ins.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_MEM and ops[0].mem.base == X86_REG_EBP and ops[0].mem.index == 0 and ops[0].mem.disp >= 8:
                    # the arguments of a tail call (jmp) are written over the function's own
                    if ops[1].type == X86_OP_IMM:
                        inargs[ops[0].mem.disp] = ops[1].imm & 0xFFFFFFFF
                    elif ops[1].type == X86_OP_REG and ops[1].reg in regs:
                        inargs[ops[0].mem.disp] = regs[ops[1].reg]
                    else:
                        inargs[ops[0].mem.disp] = None
                elif ins.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_MEM and ops[0].mem.base == X86_REG_EBP and ops[0].mem.index == 0 and ops[0].mem.disp < 0:
                    disp = ops[0].mem.disp
                    if ops[1].type == X86_OP_IMM:
                        stack[disp] = ops[1].imm & 0xFFFFFFFF
                    elif ops[1].type == X86_OP_REG and ops[1].reg in regs:
                        stack[disp] = regs[ops[1].reg]
                    else:
                        stack.pop(disp, None)
                        if ops[1].type == X86_OP_REG and ops[1].reg in SAVED and disp not in saved:
                            saved[disp] = ops[1].reg     # prologue: mov [ebp-4], edi
                elif ins.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_REG:
                    if ops[1].type == X86_OP_IMM:
                        regs[ops[0].reg] = ops[1].imm & 0xFFFFFFFF
                    elif ops[1].type == X86_OP_MEM and ops[1].mem.base == X86_REG_EBP and ops[1].mem.index == 0 and ops[1].mem.disp >= 8:
                        # the function's own argument n: `this` of a method is argument 1
                        regs[ops[0].reg] = ("arg", (ops[1].mem.disp - 8) // 4 + 1, 0)
                    elif ops[1].type == X86_OP_MEM and ops[1].mem.base == X86_REG_EBP and ops[1].mem.index == 0 and ops[1].mem.disp in stack:
                        regs[ops[0].reg] = stack[ops[1].mem.disp]
                    elif ops[1].type == X86_OP_MEM and ops[1].mem.base == X86_REG_EBP and saved.get(ops[1].mem.disp) == ops[0].reg:
                        pass        # epilogue of an early return: mov edi, [ebp-4] - see `ret` below
                    elif ops[1].type == X86_OP_REG and ops[1].reg in regs:
                        regs[ops[0].reg] = regs[ops[1].reg]
                    else:
                        regs.pop(ops[0].reg, None)
                elif ins.mnemonic == "lea" and len(ops) == 2 and ops[0].type == X86_OP_REG and ops[1].type == X86_OP_MEM and ops[1].mem.index == 0:
                    base = regs.get(ops[1].mem.base)
                    if ops[1].mem.base == X86_REG_EBP:
                        # an object on this function's stack: only this function ever sees it
                        regs[ops[0].reg] = ("local", start, ops[1].mem.disp)
                    elif isinstance(base, tuple):
                        regs[ops[0].reg] = (base[0], base[1], base[2] + ops[1].mem.disp)
                    elif isinstance(base, int):
                        regs[ops[0].reg] = (base + ops[1].mem.disp) & 0xFFFFFFFF
                    else:
                        regs.pop(ops[0].reg, None)
                elif ins.mnemonic == "add" and len(ops) == 2 and ops[0].type == X86_OP_REG and ops[1].type == X86_OP_IMM and isinstance(regs.get(ops[0].reg), tuple):
                    b = regs[ops[0].reg]
                    regs[ops[0].reg] = (b[0], b[1], b[2] + ops[1].imm)
                elif ins.mnemonic == "push" and ops and ops[0].type == X86_OP_IMM:
                    slots[-1] = ops[0].imm & 0xFFFFFFFF
                elif ins.mnemonic == "call" and ops and ops[0].type == X86_OP_IMM:
                    target = ops[0].imm & 0xFFFFFFFF
                    # every constant an argument slot holds: strings (a string is whatever reads
                    # as one - .rodata shares the r-x segment with .text here), object addresses,
                    # and small numbers (a row number, a flag such as bColumnLab)
                    args = {d: v for d, v in slots.items() if isinstance(v, tuple) or v < 0x10000 or elf.is_mapped(v)}
                    found.append((start, ins.address, target, args))
                    if elf.is_code(target):
                        starts.add(target)
                    slots = {}
                    for r in (X86_REG_EAX, X86_REG_ECX, X86_REG_EDX):
                        regs.pop(r, None)
                elif ins.mnemonic in ("ret", "retn"):
                    # an early-return block: what follows is a block some jump leads to (see
                    # `targets`); failing that, ebx/esi/edi as they were before the epilogue
                    slots = {}
                    regs = {r: v for r, v in regs.items() if r in SAVED}
                    nofall = True
                elif ins.mnemonic == "pop" and ops and ops[0].type == X86_OP_REG and ops[0].reg in SAVED:
                    pass
                elif ins.mnemonic == "jmp" and ops and ops[0].type == X86_OP_IMM:
                    target = ops[0].imm & 0xFFFFFFFF
                    o2 = elf.off(target)
                    if o2 is not None and elf.d[o2:o2 + 3] == b"\x55\x89\xe5" and (target < start or target >= ins.address):
                        # a tail call: `jmp` to a function prologue (GCC's every function here
                        # begins push ebp; mov ebp, esp).  Its arguments are this function's own
                        # (or what was just written over them)
                        args = {}
                        for disp in sorted(set(range(8, 24, 4)) | set(inargs)):
                            v = inargs.get(disp, ("arg", (disp - 8) // 4 + 1, 0))
                            if v is not None:
                                args[disp - 8] = v
                        found.append((start, ins.address, target, args))
                        starts.add(target)
                    slots = {}
                    nofall = True
                elif ops and ops[0].type == X86_OP_REG and ins.mnemonic not in ("cmp", "test", "push"):
                    regs.pop(ops[0].reg, None)     # any other write to a register
            walked[start] = (end, found)
    calls = [c for start in sorted(walked) for c in walked[start][1]]
    return sorted(starts), calls


def cache_path(elf):
    return elf.path + ".calls.json"


def load(elf):
    p = cache_path(elf)
    if os.path.exists(p):
        with open(p, encoding="utf-8") as f:
            d = json.load(f)
        return d["starts"], [tuple(c[:3]) + ({int(k): (tuple(v) if isinstance(v, list) else v) for k, v in c[3].items()},) for c in d["calls"]]
    seeds = [elf.entry]
    for run in re_elf.luamap(elf):
        seeds += [fn for _a, _n, fn in run]
    for va in elf.plt():
        seeds.append(va)
    starts, calls = build(elf, seeds)
    with open(p, "w", encoding="utf-8") as f:
        json.dump({"starts": starts, "calls": [[a, b, c, {str(k): v for k, v in d.items()}] for a, b, c, d in calls]}, f)
    return starts, calls


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    elf = re_elf.Elf(sys.argv[1])
    cmd = sys.argv[2]
    if cmd == "build" and os.path.exists(cache_path(elf)):
        os.remove(cache_path(elf))
    starts, calls = load(elf)
    if cmd == "build":
        print(len(starts), "functions,", len(calls), "calls ->", cache_path(elf))
    elif cmd == "callers":
        va = int(sys.argv[3], 16)
        seen = collections.Counter(c[0] for c in calls if c[2] == va)
        for f, n in seen.most_common():
            print(f"{f:08X}  x{n}")
    elif cmd == "strargs":
        va = int(sys.argv[3], 16)
        n = int(sys.argv[4]) if len(sys.argv) > 4 else 40
        cnt = collections.Counter()
        for _f, _site, t, strs in calls:
            if t == va:
                for d, v in strs.items():
                    t = elf.text_at(v) if isinstance(v, int) else None
                    if t:
                        cnt[(d, t)] += 1
        for (d, s), k in cnt.most_common(n):
            print(f"[esp+{d}]  x{k:4d}  {s}")
    elif cmd == "byarg":
        want = sys.argv[3].encode("gbk", "ignore")
        cnt = collections.Counter()
        for f, _site, t, strs in calls:
            for d, v in strs.items():
                raw = elf.cstr(v, 80) if isinstance(v, int) else None
                if raw and elf.text_at(v) and want in raw:
                    cnt[(t, d)] += 1
        for (t, d), k in cnt.most_common(30):
            print(f"callee {t:08X}  [esp+{d}]  x{k}  {elf.plt().get(t, '')}")


if __name__ == "__main__":
    main()
