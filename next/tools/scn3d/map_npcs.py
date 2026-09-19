# -*- coding: utf-8 -*-
"""Ghep NPC/quai cua bo tham khao (cha_list: ten Han, cha_pic, loai, cap) voi template JX1 (client/assets/npcres/npcs.json:
ten tieng Viet da dich) -> tools/scn3d/cha_templates.json  {cha_pic: [template_id, kind, level, "ly do"]} cho make_map3d.py
(CHA_TO_TEMPLATE tay van uu tien).

Cach ghep [tự chọn, ten JX1 la ban dich nen khong doc Han-Viet duoc]: tu dien tu khoa CN -> VI (con vat, mau, vai tro),
template JX1 phai chua du tu khoa; nhieu ung vien -> template co cap gan cap tham khao nhat (cha_list cot 9), roi id nho.
    python tools/scn3d/map_npcs.py            # in bang ghep + ghi cha_templates.json
"""
import io
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

# tu khoa: moi muc = (cac chu Han, [tu tieng Viet phai co trong ten JX1], loai_uu_tien) ; thu tu = uu tien
WORDS = [
    ("野猪", ["heo rừng"]), ("白猪", ["heo trắng"]), ("花猪", ["heo rừng"]), ("刺猬", ["nhím"]), ("梅花鹿", ["hươu"]),
    ("灰狼", ["sói xám"]), ("苍狼", ["sói xám"]), ("青狼", ["sói xám"]), ("火狼", ["sói đỏ"]), ("雪狼", ["sói tuyết"]), ("变异狼", ["sói đỏ"]), ("狼", ["sói"]),
    ("大象", ["voi châu"]), ("黄河象", ["voi hoàng"]), ("白象", ["voi châu"]), ("非洲象", ["voi châu"]),
    ("黑熊", ["gấu đen"]), ("棕熊", ["gấu nâu"]), ("变异熊", ["gấu đen"]), ("巨型熊王", ["gấu nâu"]), ("熊", ["gấu"]),
    ("金钱豹", ["kim tiền báo"]), ("穿山豹", ["báo"]), ("豹", ["báo"]),
    ("白玉虎", ["bạch hổ"]), ("黄炎虎", ["hoa nam hổ"]), ("火云虎", ["hoa nam hổ"]), ("玄雷虎王", ["hổ"]), ("虎王", ["hổ"]), ("虎", ["hổ"]),
    ("苍鹰", ["thương ưng"]), ("金雕", ["thương ưng"]), ("鹰", ["ưng"]),
    ("金毛", ["hoàng cẩu"]), ("蜘蛛", ["nhện"]), ("蝙蝠", ["dơi"]), ("蛇", ["rắn"]), ("蟾蜍", ["cóc"]), ("青蛙", ["cóc"]),
    ("狼棒", ["sơn tặc 1"]), ("秃鹫", ["thương ưng"]),
    ("土匪头目", ["sơn tặc đầu"]), ("恶霸统领", ["sơn tặc đầu"]), ("绿林大盗", ["sơn tặc đầu"]), ("金锤王", ["sơn tặc đầu"]),
    ("村霸", ["sơn tặc 1"]), ("草寇", ["sơn tặc 1"]), ("土匪", ["sơn tặc 1"]), ("莽汉", ["sơn tặc 1"]), ("浪汉", ["sơn tặc 1"]),
    # biet danh theo vu khi cua bon cuop (金枪, 铜锤, 长刀, 短戟, 利斧, 双锋, 铁爪, 青盾, 黑巾, 快腿, 灵印, 风鸣, 雷啸, 夺命镰, 飞镰, 髻匕, 刀客, 钝斧汉)
    ("枪", ["sơn tặc 2"]), ("锤", ["sơn tặc 2"]), ("刀", ["sơn tặc 1"]), ("戟", ["sơn tặc 2"]), ("斧", ["sơn tặc 2"]), ("锋", ["sơn tặc 1"]),
    ("爪", ["sơn tặc 1"]), ("盾", ["sơn tặc 2"]), ("巾", ["sơn tặc 1"]), ("腿", ["sơn tặc 1"]), ("印", ["sơn tặc 2"]), ("鸣", ["sơn tặc 1"]),
    ("啸", ["sơn tặc 2"]), ("镰", ["sơn tặc 2"]), ("匕", ["sơn tặc 1"]),
    ("魔女", ["nữ thích khách"]),
    ("骷髅", ["thủy quỷ"]),   # JX1 khong co xuong kho: Thuy Quy dung tam [tự chọn]
]
SOLDIERS = [
    ("暗河杀手", ["thích khách"]), ("杀手", ["thích khách"]), ("刺客", ["thích khách"]),
    ("弓兵", ["cung kỵ binh"]), ("枪兵", ["thương kỵ binh"]), ("刀兵", ["binh sĩ"]), ("守卫", ["vệ binh"]),
    ("峨眉弟子", ["nga mi"]), ("武当弟子", ["võ đang"]), ("少林弟子", ["thiếu lâm"]),
]
WORDS = SOLDIERS + WORDS


def norm(x):
    return re.sub(r"\s+", " ", x.lower()).strip()


def main():
    import UnityPy
    key = io.open(os.environ.get("JX_SCN3D_KEY", r"D:\game3gtQ_mo\khoa_bundle.txt"), encoding="utf-8").read().strip().split()[0]
    UnityPy.set_assetbundle_decrypt_key(key)
    from export_npc import Tables
    from make_map3d import CHA_TO_TEMPLATE
    t = Tables(os.environ.get("JX_SCN3D_SRC", r"D:\game3gtQ_mo\pc\剑网江湖_Data\StreamingAssets"))
    cha_list = [r for r in t.t["cha_list_cmn"][1:] if r and r[0].strip().isdigit() and len(r) > 9]
    tpl = json.load(io.open(os.path.join(NEXT, "client", "assets", "npcres", "npcs.json"), encoding="utf-8"))["templates"]
    monsters = []
    for k, v in tpl.items():
        if int(v.get("kind", 0)) == 0 and int(v.get("camp", 0)) == 5 and v.get("res"):
            name = norm(v.get("name", ""))
            if "(" in name or "boss" in name or "nhiệm vụ" in name or "npc" in name:
                continue
            cells = v.get("cells", {})
            # the zone's life = LifeParam x level (KNpcTemplate.cpp:249) + the level-band extras LifeParam1..3 of the old
            # npcs.txt: a boss row (Thiên Ưng 1450: LifeParam3 20 000 000) must not stand in for a field monster
            def num(x):
                try:
                    return float(str(x).split("|")[0] or 0)
                except ValueError:
                    return 0.0
            big = any(num(cells.get(k2, "0")) >= 100000 for k2 in ("LifeParam1", "LifeParam2", "LifeParam3"))
            if big or num(cells.get("LifeParam", "100")) > 1000:
                continue
            lvl = int(v.get("level", 0)) if str(v.get("level", "")).lstrip("-").isdigit() else 0
            monsters.append((int(k), name, lvl))
    out = {}
    report = []
    for r in cha_list:
        pic = r[3].strip()
        name = r[1].strip()
        kind = r[6].strip()
        if not pic.isdigit() or int(pic) in CHA_TO_TEMPLATE or int(pic) in out:
            continue
        if kind not in ("3",):   # 3 = NPC/quai (1 nhan vat, 2 ngua, 4 bay, 5 thu cung)
            continue
        level = int(r[9].strip()) if r[9].strip().isdigit() else 10
        base = re.sub(r"[_|•（(].*$", "", name)
        chosen = None
        # 1) the Han-Viet reading itself names a JX1 monster (飞沙 Phi Sa, 狼棒 Lang Bổng, 寒枪 Hàn Thương): the JX1 build kept
        #    those names for the desert / cave monsters
        from hanviet import readings
        for hv in readings(base) or []:
            hvn = norm(hv)
            cands = [m for m in monsters if hvn and (m[1] == hvn or m[1].startswith(hvn + " ") or (" " + hvn + " ") in (" " + m[1] + " "))]
            if cands:
                cands.sort(key=lambda m: (abs(m[2] - level), m[0]))
                chosen = (cands[0][0], 0, level, "Han-Viet %s (%s)" % (hv, cands[0][1]))
                break
        for cn, vi in WORDS:
            if chosen:
                break
            if cn in base:
                cands = [m for m in monsters if all(w in m[1] for w in vi)]
                if cands:
                    cands.sort(key=lambda m: (abs(m[2] - level), m[0]))
                    chosen = (cands[0][0], 0, level, "%s -> %s (%s)" % (cn, "+".join(vi), cands[0][1]))
                    break
        if chosen is None:
            # a dialogue npc (cha_list col 5 bit 2 "不能动的": stands still, bit 4 the big minimap dot) without a JX1 role:
            # a villager template stands in (kind 3), named by the Han-Viet reading when hanviet.py knows every character
            flag = r[5].strip()
            try:
                flag_v = int(flag, 2) if flag.startswith("0b") else int(flag or "0")
            except ValueError:
                flag_v = 0
            if flag_v & 0b100 and not base.startswith(("DPS", "旗帜", "测试", "占位")):
                from hanviet import readings
                rs = readings(base) or []
                hv = rs[0] if rs and "?" not in rs[0] else ""
                chosen = (322, 3, 0, "npc doi thoai khong co vai JX1 -> Nam thanh nien 3" + (" [%s]" % hv if hv else ""))
                if hv:
                    chosen = chosen + (hv.title(),)
        if chosen:
            out[int(pic)] = list(chosen)
        report.append((r[0].strip(), name, pic, level, chosen))
    dst = os.path.join(HERE, "cha_templates.json")
    with io.open(dst, "w", encoding="utf-8") as f:
        json.dump({str(k): v for k, v in sorted(out.items())}, f, ensure_ascii=False, indent=1)
    n_ok = sum(1 for x in report if x[4])
    print("cha_list loai 3 chua ghep tay: %d, ghep tu dong duoc %d -> %s" % (len(report), n_ok, dst))
    for x in report:
        print("  %5s %-14s pic %-4s cap %-3d %s" % (x[0], x[1][:14], x[2], x[3], ("-> %d %s" % (x[4][0], x[4][3])) if x[4] else "?"))


if __name__ == "__main__":
    main()
