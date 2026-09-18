#!/usr/bin/env python3
"""The column descriptors of the JX2 server's table readers (KBPT_*::ReadRow through the generic
reader 0x081ECF40): each reader builds an array of {type, destination, default} triples on its
stack - type 0 = integer (KTabFile::GetInteger(row, col, default) - an EMPTY cell gives the
default, KTabFile::GetValue fails on a zero-length cell), 1 = string (default = buffer size),
2 = the column is skipped - one triple per column of the file, in file order.

  re_tabdesc.py <elf> <reader-va-hex> [...]     print the triples: column, type, row offset, default

Row offsets are relative to the row base register (imul <row>, size; add <table base>).
"""
import sys

from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

from re_elf import Elf

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def descriptors(elf, va, max_ins=600):
    stack = {}        # ebp-relative offset -> ("imm", value) | ("row", offset)
    row_base = None   # the register holding the row pointer
    regs = {}         # register -> ("row", offset) for lea reg, [row + off]
    count = 0
    for ins in elf.dis(va, max_ins):
        count += 1
        if ins.mnemonic == "imul" and len(ins.operands) == 3 and ins.operands[2].type == X86_OP_IMM:
            row_base = ins.reg_name(ins.operands[0].reg)
            row_size = ins.operands[2].imm
        elif ins.mnemonic == "lea" and ins.operands[1].type == X86_OP_MEM and row_base and ins.reg_name(ins.operands[1].mem.base) == row_base:
            regs[ins.reg_name(ins.operands[0].reg)] = ("row", ins.operands[1].mem.disp)
        elif ins.mnemonic == "mov" and ins.operands[0].type == X86_OP_MEM and ins.reg_name(ins.operands[0].mem.base) == "ebp":
            off = ins.operands[0].mem.disp
            if ins.operands[1].type == X86_OP_IMM:
                stack[off] = ("imm", ins.operands[1].imm)
            elif ins.operands[1].type == X86_OP_REG:
                r = ins.reg_name(ins.operands[1].reg)
                if r in regs:
                    stack[off] = regs[r]
                elif r == row_base:
                    stack[off] = ("row", 0)
        elif ins.mnemonic == "call":
            break   # the descriptor is complete when the generic reader is called
    if not stack:
        return None, None
    base = min(stack)
    out = []
    col = 1
    while base + 12 * len(out) + 8 <= max(stack) + 8:
        i = len(out)
        t = stack.get(base + 12 * i, ("imm", 0))[1]
        dest = stack.get(base + 12 * i + 4)
        dflt = stack.get(base + 12 * i + 8, ("imm", 0))[1]
        out.append((col, t, dest, dflt))
        col += 1
        if base + 12 * (i + 1) > max(stack):
            break
    return row_size if row_base else None, out


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    elf = Elf(sys.argv[1])
    for a in sys.argv[2:]:
        va = int(a, 16)
        size, desc = descriptors(elf, va)
        print(f"== reader {va:08X}: row {size:#x} bytes" if size else f"== reader {va:08X}")
        if not desc:
            print("   (no descriptor found)")
            continue
        for col, t, dest, dflt in desc:
            kind = {0: "int", 1: "str", 2: "skip"}.get(t, str(t))
            d = f"row+{dest[1]:#x}" if dest and dest[0] == "row" else ("-" if dest is None else str(dest))
            dv = dflt if isinstance(dflt, int) and dflt < 0x80000000 else dflt - 0x100000000 if isinstance(dflt, int) else dflt
            print(f"   col {col:2d}  {kind:4s}  {d:10s}  default {dv}")


if __name__ == "__main__":
    main()
