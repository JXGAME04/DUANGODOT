#!/usr/bin/env python3
"""re_scan.py <elf> <command> - three sweeps over every function of the Linux game server that
answer "who touches this member", "which functions fill a table of member-function pointers" and
"where is this instruction shape", the way the M11 item work found KNpc::ProcessState,
KNpcAttribModify::LifePotionV and KItemList::EatMecidine without any symbol (docs/LINUX-SERVER.md §9).

  re_scan.py <elf> disp <hex>[,<hex>...] [mnemonic]     every instruction with [reg + disp] for those
                                                         displacements, grouped by function: the readers
                                                         and writers of a member at a known offset
  re_scan.py <elf> pmf [min]                             functions with many `mov [reg+disp], imm(code)`
                                                         stores: the constructors that fill ProcessFunc[]
                                                         tables (KNpcAttribModify, KProtocolProcess);
                                                         idx8 = (disp - 4) / 8 for a table at +4
  re_scan.py <elf> ins <regex> [max]                     instructions whose "mnemonic op_str" matches

Functions come from the re_calls.py cache (<elf>.calls.json - run `re_calls.py <elf> build` once).
"""
import json
import re
import sys

from capstone.x86 import X86_OP_IMM, X86_OP_MEM

from re_elf import Elf


def functions(elf):
    calls = json.load(open(elf.path + ".calls.json"))
    starts = sorted(calls["starts"])
    return [(starts[i], starts[i + 1]) for i in range(len(starts) - 1)]


def each_instruction(elf, limit=40000):
    for start, end in functions(elf):
        if end - start > limit:
            continue
        o = elf.off(start)
        if o is None:
            continue
        for ins in elf.md.disasm(elf.d[o:o + (end - start)], start):
            yield start, ins


def cmd_disp(elf, args):
    want = {int(x, 16) for x in args[0].split(",")}
    mn = args[1] if len(args) > 1 else None
    byfunc = {}
    for start, ins in each_instruction(elf):
        if mn and not ins.mnemonic.startswith(mn):
            continue
        for op in ins.operands:
            if op.type == X86_OP_MEM and op.mem.disp in want and op.mem.base != 0:
                byfunc.setdefault(start, []).append(f"{ins.address:#010x}  {ins.mnemonic} {ins.op_str}")
                break
    for start, lines in sorted(byfunc.items()):
        print(f"\n{start:#010x}  ({len(lines)})")
        for line in lines[:12]:
            print("   ", line)


def cmd_pmf(elf, args):
    minimum = int(args[0]) if args else 12
    hits = []
    for start, end in functions(elf):
        if end - start > 20000:
            continue
        o = elf.off(start)
        if o is None:
            continue
        stores = []
        for ins in elf.md.disasm(elf.d[o:o + (end - start)], start):
            if ins.mnemonic == "mov" and len(ins.operands) == 2 and ins.operands[0].type == X86_OP_MEM and ins.operands[1].type == X86_OP_IMM:
                imm = ins.operands[1].imm & 0xFFFFFFFF
                if elf.is_code(imm):
                    stores.append((ins.operands[0].mem.disp, imm, ins.address))
        if len(stores) >= minimum:
            hits.append((len(stores), start, end, stores))
    hits.sort(reverse=True)
    for n, start, end, stores in hits:
        print(f"\n{start:#010x}-{end:#010x} stores={n}")
        for disp, imm, at in stores:
            print(f"   [{disp:#7x}] = {imm:#010x}   @ {at:#010x}   idx8={(disp - 4) // 8}")


def cmd_ins(elf, args):
    rx = re.compile(args[0])
    limit = int(args[1]) if len(args) > 1 else 200
    n = 0
    for start, ins in each_instruction(elf):
        s = f"{ins.mnemonic} {ins.op_str}"
        if rx.search(s):
            print(f"{start:#010x}  {ins.address:#010x}  {s}")
            n += 1
            if n >= limit:
                return


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    sys.stdout.reconfigure(encoding="utf-8")
    elf = Elf(sys.argv[1])
    cmd, args = sys.argv[2], sys.argv[3:]
    if cmd == "disp":
        cmd_disp(elf, args)
    elif cmd == "pmf":
        cmd_pmf(elf, args)
    elif cmd == "ins":
        cmd_ins(elf, args)
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
