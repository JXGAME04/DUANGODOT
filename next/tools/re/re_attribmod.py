#!/usr/bin/env python3
"""KNpcAttribModify of the JX2 server: which KNpc field every magic attribute changes.

  re_attribmod.py <elf> [ctor-va] [table-tsv]      default ctor 0x08099600, names docs/linux/jx_linux_magicattrib.tsv

The constructor fills ProcessFunc[] (8-byte entries from this+4: idx = (disp - 4) / 8) with one
function per attribute id.  Every ProcessFunc(this, nIdx, pNpc, pMagic, extra) touches the npc
through the register loaded from [ebp+0x10] and reads the magic's values through the one from
[ebp+0x14] (+4, +8, +0xc = value[0..2]).  Walking each function and listing the [npc + offset]
operands it reads and writes names the KNpc members by the attribute that changes them - the
map of the KNpc structure (docs/LINUX-SERVER.md §10) without a single symbol.

Prints a TSV: id, name, function, npc offsets written (with the operation), npc offsets read,
magic values used.
"""
import os
import sys

from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG

from re_elf import Elf

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")


def table_of(elf, ctor):
    """{idx: function va} from the constructor's stores"""
    out = {}
    for ins in elf.dis(ctor, 900):
        if ins.mnemonic == "mov" and ins.operands[0].type == X86_OP_MEM and ins.operands[1].type == X86_OP_IMM:
            disp = ins.operands[0].mem.disp
            imm = ins.operands[1].imm & 0xFFFFFFFF
            if disp >= 4 and (disp - 4) % 8 == 0 and elf.is_code(imm):
                out[(disp - 4) // 8] = imm
        if ins.mnemonic in ("ret", "leave") and out:
            break
    return out


def walk(elf, va, end, limit=400):
    """(writes, reads, magic uses) of one ProcessFunc.  `end` is the next function's start: most
    of these are a few instructions ending in a tail jump (to the shared log/clamp helper), not
    in a ret, so the walk stops at the first jump out of [va, end)."""
    npc_regs, magic_regs = set(), set()
    writes, reads, magic = {}, set(), set()
    for ins in elf.dis(va, limit):
        if ins.address >= end:
            break
        ops = ins.operands
        if ins.mnemonic == "jmp" and ops and ops[0].type == X86_OP_IMM:
            if not (va <= (ops[0].imm & 0xFFFFFFFF) < end):
                break
            continue
        # register loads of the arguments (and copies of those registers)
        if ins.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_REG:
            src = ops[1]
            if src.type == X86_OP_MEM and ins.reg_name(src.mem.base) == "ebp" and src.mem.index == 0:
                if src.mem.disp == 0x10:
                    npc_regs.add(ins.reg_name(ops[0].reg))
                    continue
                if src.mem.disp == 0x14:
                    magic_regs.add(ins.reg_name(ops[0].reg))
                    continue
            if src.type == X86_OP_REG:
                name = ins.reg_name(src.reg)
                dst = ins.reg_name(ops[0].reg)
                if name in npc_regs:
                    npc_regs.add(dst)
                    continue
                if name in magic_regs:
                    magic_regs.add(dst)
                    continue
            # the register is overwritten by something else
            npc_regs.discard(ins.reg_name(ops[0].reg))
            magic_regs.discard(ins.reg_name(ops[0].reg))
        if ins.mnemonic == "lea" and ops[0].type == X86_OP_REG and ops[1].type == X86_OP_MEM:
            base = ins.reg_name(ops[1].mem.base)
            dst = ins.reg_name(ops[0].reg)
            if base in npc_regs and ops[1].mem.index == 0 and ops[1].mem.disp < 0x100:
                npc_regs.add(dst)   # lea reg, [npc + small]: still the npc (its member block)
            else:
                npc_regs.discard(dst)
                magic_regs.discard(dst)
        for i, op in enumerate(ops):
            if op.type != X86_OP_MEM or op.mem.base == 0:
                continue
            base = ins.reg_name(op.mem.base)
            if base in npc_regs:
                off = op.mem.disp
                if i == 0 and ins.mnemonic not in ("cmp", "test", "push"):
                    writes[off] = ins.mnemonic
                else:
                    reads.add(off)
            elif base in magic_regs and op.mem.disp in (4, 8, 0xC):
                magic.add((op.mem.disp - 4) // 4)
        if ins.mnemonic in ("ret",):
            break
    return writes, reads, magic


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return
    elf = Elf(sys.argv[1])
    ctor = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x08099600
    names = {}
    tsv = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.path.dirname(__file__), "..", "..", "docs", "linux", "jx_linux_magicattrib.tsv")
    if os.path.exists(tsv):
        for line in open(tsv, encoding="utf-8"):
            parts = line.rstrip("\n").split("\t")
            if len(parts) >= 2 and parts[0].isdigit():
                names[int(parts[0])] = parts[1]
    table = table_of(elf, ctor)
    starts = sorted(set(table.values()))
    print("id\tname\tfunction\twrites\treads\tmagic values")
    for idx in sorted(table):
        va = table[idx]
        after = [s for s in starts if s > va]
        writes, reads, magic = walk(elf, va, after[0] if after else va + 0x400)
        w = " ".join(f"{off:#x}:{op}" for off, op in sorted(writes.items()))
        r = " ".join(f"{off:#x}" for off in sorted(reads - set(writes)))
        m = " ".join(f"v{k}" for k in sorted(magic))
        print(f"{idx}\t{names.get(idx, '?')}\t{va:08X}\t{w}\t{r}\t{m}")


if __name__ == "__main__":
    main()
