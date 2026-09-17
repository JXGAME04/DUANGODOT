#!/usr/bin/env python3
"""Signatures of the Lua script functions a JX server binary registers (Lua 4.0 C API).

  re_luasig.py <elf> callees [n]        the functions the registered functions call most: the
                                        Lua C API is among them (name them in API below)
  re_luasig.py <elf> sig <name|va>      one function: arguments read, values pushed, results
  re_luasig.py <elf> all <out.tsv>      every registered function -> tab separated table

How a signature is read (Lua 4.0, what JX1's Script.cpp did and this binary still does):
  int f(lua_State* L)              the only C argument, at [ebp+8]
  lua_tonumber(L, i) / lua_tostring(L, i)   read argument i: `mov [esp+4], i` before the call
  lua_gettop(L)                    the function looks at how many arguments it got (optional ones)
  lua_pushnumber(L, x) / lua_pushstring(L, s) / lua_pushnil(L) / lua_newtable(L) ...   results
  mov eax, N ... ret               the C function returns how many values it pushed
The helper that many functions call first with only L is the "current player of this script"
lookup (Lua_GetPlayerIndex of JX1): a function that calls it needs a player behind the script.
"""
import collections
import re
import struct
import sys

sys.path.insert(0, __import__("os").path.dirname(__file__))
import re_elf  # noqa: E402

from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG, X86_REG_EAX, X86_REG_EBP, X86_REG_ESP  # noqa: E402

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")

# The Lua 4.0 C API and the game's helpers, by address in jx_linux_y (named by reading their
# bodies: see docs/LINUX-SERVER.md §5).  Fill in per binary.
API = {
    "jx_linux_y": {
        # Lua 4.0 (statically linked): lua_State.top +0, .Cbase +0x10; TObject 12 bytes,
        # ttype 1 nil, 2 number, 3 string, 4 table, 7 userdata in this build
        0x8232490: "lua_gettop",          # (top - Cbase) / 12
        0x82338B0: "lua_tonumber",        # o = Cbase + (i-1)*12; ttype 2 -> value.n, else luaV_tonumber
        0x82339B0: "lua_tonumber_int",    # lua_tonumber then double -> int (fistp)
        0x8233850: "lua_tostring",
        0x8232590: "lua_type",            # ttype of the value at i
        0x8232690: "lua_touserdata",      # ttype 7 -> value
        0x8232D40: "lua_pushnumber",      # top->ttype = 2, value.n = x, top += 12
        0x82337A0: "lua_pushstring",      # strlen + lua_pushlstring
        0x8233730: "lua_pushlstring",
        0x8232E70: "lua_pushnil",         # ttype 1
        0x8232BE0: "lua_newtable",        # luaH_new, ttype 4
        0x8232F20: "lua_pushvalue",
        0x8233C10: "lua_settop",
        0x8233360: "lua_rawseti",
        0x8233620: "lua_getglobal",
        # KGLua argument checkers (luaL_check_* of the game, wrappers of the above)
        0x8245AF0: "check_number",        # lua_tonumber, compared against 0
        0x8245BB0: "check_string",        # lua_tostring, non-null
        0x8245C90: "check_type",          # lua_type(L,i) == wanted
        0x8245CE0: "has_arg",             # lua_type(L,i) != -1
        0x8245B60: "opt_number",          # (L, i, default double)
        # the script's context: Lua globals the engine sets before calling a script
        0x8107860: "GetPlayerIndex",      # global "PlayerIndex" -> g_pPlayer[i].m_nIndex (Npc index), -1 when nil
        0x8107910: "GetPlayerIndex",      # tail-jumps to the one above
        0x8106A40: "GetSubWorldIndex",    # global "SubWorld"
    },
}


# what each reader says about the argument it reads
READERS = {
    "lua_tonumber": "number", "lua_tonumber_int": "number", "check_number": "number", "opt_number": "number?",
    "lua_tostring": "string", "check_string": "string",
    "lua_touserdata": "userdata",
    "lua_type": "?", "check_type": "?", "has_arg": "?",
}


def registered(elf):
    out = {}
    for run in re_elf.luamap(elf):
        for _at, name, fn in run:
            o = elf.off(fn)
            if o and elf.d[o] == 0x55:
                out[name] = fn
    return out


def body(elf, va, limit=600, hops=2):
    """Instructions of one function: from va to its last ret.  GCC puts blocks after the epilogue
    (`if (lua_gettop(L) == 2) jmp body; ... ret; body:`), so a ret ends the function only when no
    jump seen so far leads past it.  A tail jump to another function (a thin wrapper such as
    AddItem -> the real one) is followed, twice at most."""
    ins = []
    furthest = va
    for i in elf.dis(va, limit, stop_at_ret=False):
        ins.append(i)
        if i.mnemonic.startswith("j") and i.operands and i.operands[0].type == X86_OP_IMM:
            target = i.operands[0].imm & 0xFFFFFFFF
            if va <= target < va + 0x2000:
                furthest = max(furthest, target)
            elif i.mnemonic == "jmp":
                if hops > 0 and elf.is_code(target) and furthest <= i.address:
                    ins += body(elf, target, limit, hops - 1)
                if furthest <= i.address:
                    break
        if i.mnemonic in ("ret", "retn") and furthest <= i.address:
            break
    return ins


def callees(elf, funcs, top=40):
    count = collections.Counter()
    for name, va in funcs.items():
        seen = set()
        for i in body(elf, va):
            if i.mnemonic == "call" and i.operands and i.operands[0].type == X86_OP_IMM:
                seen.add(i.operands[0].imm & 0xFFFFFFFF)
        for c in seen:
            count[c] += 1
    return count.most_common(top)


def signature(elf, va, api, depth=1):
    """{"args": {index: type}, "gettop": bool, "player": bool, "pushes": [types], "returns": int|None,
    "calls": [names]}.  A function that reads nothing itself but hands L to another function (a
    wrapper: SetPos -> the real one) takes that function's signature, one level down."""
    args, pushes, calls = {}, [], []
    gettop = player = False
    returns = None
    ins = body(elf, va)
    # what was stored at [esp+4] / [esp+8] just before a call: the Lua index / value
    slot4 = slot8 = None
    slot0_is_L = False
    lregs = set()               # registers holding L = [ebp+8]
    fpu_pushed = False
    last_eax = None
    inner = []                  # non-API functions called with L
    for i in ins:
        ops = i.operands
        if i.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_REG and ops[1].type == X86_OP_MEM \
                and ops[1].mem.base == X86_REG_EBP and ops[1].mem.disp == 8 and ops[1].mem.index == 0:
            lregs.add(ops[0].reg)
            last_eax = None if ops[0].reg == X86_REG_EAX else last_eax
        elif i.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_MEM and ops[0].mem.base != 0:
            disp = ops[0].mem.disp
            if ops[1].type == X86_OP_IMM:
                if disp == 4:
                    slot4 = ops[1].imm
                elif disp == 8:
                    slot8 = ops[1].imm
            elif ops[1].type == X86_OP_REG and disp in (4, 8):
                if disp == 4:
                    slot4 = "reg"
                else:
                    slot8 = "reg"
            elif ops[1].type == X86_OP_REG and disp == 0 and ops[0].mem.base == X86_REG_ESP:
                slot0_is_L = ops[1].reg in lregs
        elif i.mnemonic in ("fstp",) and ops and ops[0].type == X86_OP_MEM and ops[0].mem.disp == 4:
            fpu_pushed = True
        elif i.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_REG and ops[0].reg == X86_REG_EAX and ops[1].type == X86_OP_IMM:
            last_eax = ops[1].imm
        elif i.mnemonic == "xor" and len(ops) == 2 and ops[0].type == X86_OP_REG and ops[1].type == X86_OP_REG and ops[0].reg == ops[1].reg == X86_REG_EAX:
            last_eax = 0
        elif i.mnemonic == "mov" and len(ops) == 2 and ops[0].type == X86_OP_REG and ops[0].reg == X86_REG_EAX:
            last_eax = None           # eax now holds something computed, not a result count
        elif i.mnemonic == "call" and ops and ops[0].type == X86_OP_IMM:
            target = ops[0].imm & 0xFFFFFFFF
            name = api.get(target)
            if name:
                calls.append(name)
            elif slot0_is_L and elf.is_code(target) and target not in elf.plt():
                inner.append(target)
            slot0_is_L = False
            if name == "lua_gettop":
                gettop = True
            elif name == "GetPlayerIndex":
                player = True
            elif name in READERS:
                kind = READERS[name]
                if isinstance(slot4, int) and 0 < slot4 < 64:
                    # a check only says "there is an argument"; a real read says what it is
                    if kind != "?" or slot4 not in args:
                        args[slot4] = kind
                else:
                    args.setdefault("?", kind)
            elif name and name.startswith("lua_push"):
                pushes.append(name[len("lua_push"):])
            elif name == "lua_newtable":
                pushes.append("table")
            slot4 = slot8 = None
            fpu_pushed = False
        elif i.mnemonic in ("ret", "retn"):
            if isinstance(last_eax, int) and 0 <= last_eax <= 8:
                returns = last_eax if returns is None else max(returns, last_eax)
    if depth and not args and not pushes:
        # a wrapper: the real function is the one it hands L to
        for target in inner[:2]:
            sub = signature(elf, target, api, depth - 1)
            for k, v in sub["args"].items():
                args.setdefault(k, v)
            pushes += sub["pushes"]
            gettop, player = gettop or sub["gettop"], player or sub["player"]
            calls += ["-> " + f"{target:08X}"] + sub["calls"]
            if sub["returns"] is not None:
                returns = sub["returns"] if returns is None else max(returns, sub["returns"])
    return {"args": args, "gettop": gettop, "player": player, "pushes": pushes, "returns": returns, "calls": calls,
            "size": sum(i.size for i in ins)}


def describe(name, sig):
    parts = []
    for k in sorted(sig["args"], key=lambda x: (isinstance(x, str), x)):
        parts.append(f"arg{k}:{sig['args'][k]}")
    a = ", ".join(parts) if parts else "(không đọc đối số)"
    opt = " +gettop" if sig["gettop"] else ""
    who = " [cần nhân vật]" if sig["player"] else ""
    ret = f"-> {sig['returns']} giá trị" if sig["returns"] is not None else "-> ?"
    push = (" đẩy " + ",".join(sig["pushes"])) if sig["pushes"] else ""
    return f"{name}({a}){opt}{who} {ret}{push}"


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return
    path, cmd = sys.argv[1], sys.argv[2]
    elf = re_elf.Elf(path)
    key = next((k for k in API if k in path.replace("\\", "/").split("/")[-1]), None)
    api = API.get(key, {})
    funcs = registered(elf)
    if cmd == "callees":
        n = int(sys.argv[3]) if len(sys.argv) > 3 else 40
        for target, cnt in callees(elf, funcs, n):
            print(f"{target:08X}  called by {cnt:5d} functions  {api.get(target, '')}")
    elif cmd == "sig":
        want = sys.argv[3]
        va = funcs.get(want) or int(want, 16)
        sig = signature(elf, va, api)
        print(describe(want, sig))
        print("  calls:", " ".join(sig["calls"]), " size:", sig["size"])
    elif cmd == "all":
        out = sys.argv[3]
        with open(out, "w", encoding="utf-8") as f:
            f.write("name\tva\targs\tgettop\tplayer\tpushes\treturns\tsize\n")
            for name in sorted(funcs):
                sig = signature(elf, funcs[name], api)
                args = " ".join(f"{k}:{v}" for k, v in sorted(sig["args"].items(), key=lambda x: (isinstance(x[0], str), x[0])))
                f.write(f"{name}\t{funcs[name]:08X}\t{args}\t{int(sig['gettop'])}\t{int(sig['player'])}\t{','.join(sig['pushes'])}\t{sig['returns'] if sig['returns'] is not None else ''}\t{sig['size']}\n")
        print("wrote", out, len(funcs), "functions")


if __name__ == "__main__":
    main()
