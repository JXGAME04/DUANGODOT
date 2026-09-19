# -*- coding: utf-8 -*-
"""Ghep ky nang JX1 (settings/skills.txt -> client/assets/skills.json) voi hieu ung 3D cua bo tham khao (skill_main /
skill_section / skill_event / skill_childobj / sfx_object) -> client/assets3d/sfx/skill_map.json cho KWorldView3D.

    python tools/scn3d/map_skills.py

Cach ghep: ten ky nang cua bo tham khao la ten Han cua chinh ky nang JX1 (怒雷指 = Nộ Lôi Chỉ); doc theo am Han-Viet
(hanviet.py, ke ca cach viet cua ban dich JX1) roi so KHOP voi SkillName cua skills.json sau chuan hoa (chu thuong, bo
khoang trang/so). Khong khop = khong ghep (liet ke trong "unmatched"), ky nang do dung hieu ung chung theo ngu hanh.

Tu bang su kien (skill_event, cot 8 loai su kien): 8 = sinh vat con (skill_childobj, cot 3 = khung 0..30 cua doan),
120 = hieu ung sfx_object; childobj: duong dan, khung song, cao (m), diem treo, kieu vi tri (0 nguoi thi trien 1 diem
muc tieu 2 muc tieu), dong bo (4 = bay toi muc tieu), toc do, hieu ung trung (cot 23 -> sfx_object).
"""
import io
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import UnityPy  # noqa: E402
from hanviet import readings  # noqa: E402

SECTION_FPS = 30.0   # 技能段持续时间(帧): 30 khung = 1 giay cua bo tham khao
# ngu hanh JX1 (Series 0 kim 1 moc 2 thuy 3 hoa 4 tho) -> hieu ung ra chieu / trung cua bo tham khao (sfx_object 1..4, 20..)
ELEMENT_CAST = {0: "Skill/shifa_jinxi", 1: "Skill/shifa_muxi", 2: "Skill/shifa_shuixi", 3: "Skill/shifa_huoxi", 4: "Skill/shifa_tuxi"}
ELEMENT_HIT = {0: "Skill/hit_jinxi01", 1: "Skill/hit_muxi01", 2: "Skill/hit_shuixi01", 3: "Skill/hit_huoxi01", 4: "Skill/hit_tuxi01"}


def norm(x):
    return re.sub(r"[^a-zàáảãạăằắẳẵặâầấẩẫậèéẻẽẹêềếểễệìíỉĩịòóỏõọôồốổỗộơờớởỡợùúủũụưừứửữựỳýỷỹỵđ]", "", x.lower())


def fnum(v, default=0.0):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def main():
    src = os.environ.get("JX_SCN3D_SRC", r"D:\game3gTQ_mo\pc\剑网江湖_Data\StreamingAssets")
    key = io.open(os.environ.get("JX_SCN3D_KEY", r"D:\game3gTQ_mo\khoa_bundle.txt"), encoding="utf-8").read().strip().split()[0]
    UnityPy.set_assetbundle_decrypt_key(key)
    from export_npc import Tables
    t = Tables(src)
    rows = lambda n: [r for r in t.t[n][1:] if r and r[0].strip() != "" and r[0].strip() != "#"]
    events = {r[0].strip(): r for r in rows("skill_event_cmn") if r[0].strip().isdigit()}
    sections = {r[0].strip(): r for r in rows("skill_section_cmn") if r[0].strip().isdigit()}
    childobjs = {r[0].strip(): r for r in rows("skill_childobj_cmn") if r[0].strip().isdigit()}
    sfx_objects = {r[0].strip(): r for r in rows("sfx_object_cmn") if r[0].strip().isdigit()}
    mains = [r for r in rows("skill_main_cmn") if len(r) > 4 and r[1].strip().isdigit()]

    def sfx_entry(sid):
        r = sfx_objects.get(str(sid))
        if r is None or not r[2].strip():
            return None
        return {"res": r[2].strip(), "life_s": fnum(r[5], 30.0) / SECTION_FPS, "hang": r[7].strip() if len(r) > 7 else ""}

    def child_entry(cid, at):
        r = childobjs.get(str(cid))
        if r is None or not r[5].strip():
            return None
        life = fnum(r[8], -1.0)
        sync = r[17].strip() if len(r) > 17 else ""
        speed = r[18].strip() if len(r) > 18 else ""
        e = {"res": r[5].strip(), "name": r[2].strip(), "at": at, "life_s": (life / SECTION_FPS) if life > 0 else 1.5,
             "radius": fnum((r[10].split("*") or ["0"])[0], 0.0), "pos_type": int(fnum(r[12], 0)), "height": fnum(r[13], 0.0),
             "hang": r[14].strip() if len(r) > 14 else "", "sync": int(fnum(sync, 0)), "fly": sync == "4",
             "speed": fnum((speed.split("*") or ["0"])[0], 0.0)}
        hit = r[23].strip() if len(r) > 23 else ""
        if hit.isdigit():
            h = sfx_entry(hit)
            if h:
                e["hit"] = h
        return e

    def section_effects(sec_id, depth=0):
        """-> (cast[], child[], next_sections[])"""
        casts, childs = [], []
        r = sections.get(str(sec_id))
        if r is None:
            return casts, childs
        frames = fnum(r[5], 30.0) or 30.0
        for eid in [x for x in r[3].split("*") if x.strip()]:
            ev = events.get(eid.strip())
            if ev is None:
                continue
            kind = ev[8].strip()
            trig = ev[2].strip()
            frame = fnum(ev[3], 0.0)
            at = max(0.0, min(1.0, frame / frames)) if trig in ("", "0") else 0.0
            if kind == "8" and ev[10].strip().isdigit():
                c = child_entry(ev[10].strip(), at)
                if c:
                    c["count"] = int(fnum(ev[11], 1)) if ev[11].strip() not in ("", "-1") else 1
                    childs.append(c)
            elif kind == "120" and ev[10].strip().isdigit():
                s = sfx_entry(ev[10].strip())
                if s:
                    s["at"] = at
                    s["on_hit"] = trig == "2"
                    casts.append(s)
        return casts, childs

    skills = json.load(io.open(os.path.join(NEXT, "client", "assets", "skills.json"), encoding="utf-8"))
    jx = {}
    jx_series = {}
    for r in skills["rows"]:
        cells = r.get("cells", {})
        n = norm(cells.get("SkillName", ""))
        if n:
            jx.setdefault(n, []).append(int(r["id"]))
        jx_series[int(r["id"])] = int(fnum(cells.get("Series", -1), -1))

    by_jx, by_ref, unmatched = {}, {}, []
    for r in mains:
        rid, name, sec = r[1].strip(), r[0].strip(), r[4].strip()
        if name.startswith("npc") or name.startswith("boss") or name.startswith("展示") or name.startswith("技能展示") or int(rid) >= 2000:
            continue
        base = re.sub(r"[•·].*$", "", name)
        rs = readings(base) or []
        hit = None
        for hv in rs:
            if norm(hv) in jx:
                hit = hv
                break
        casts, childs = section_effects(sec)
        entry = {"ref": int(rid), "cn": name, "vi": hit or (rs[0] if rs else ""), "element": r[2].strip(), "type": r[3].strip(),
                 "cast": casts, "child": childs}
        by_ref[rid] = entry
        if hit is None:
            unmatched.append({"ref": int(rid), "cn": name, "hanviet": rs[0] if rs else "?"})
            continue
        for jid in jx[norm(hit)]:
            # a levelled variant (天地无极•开天•初) does not replace the base skill's effects when the base is mapped too
            if str(jid) in by_jx and "•" in name:
                continue
            by_jx[str(jid)] = entry
    out = {"by_jx": by_jx, "element_cast": ELEMENT_CAST, "element_hit": ELEMENT_HIT, "series_of": {str(k): v for k, v in jx_series.items() if v >= 0},
           "unmatched": unmatched, "section_fps": SECTION_FPS}
    dst = os.path.join(NEXT, "client", "assets3d", "sfx", "skill_map.json")
    with io.open(dst, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    with_fx = sum(1 for e in by_jx.values() if e["cast"] or e["child"])
    print("ky nang tham khao %d, ghep JX1 %d ky nang (co hieu ung %d), khong ghep %d -> %s" % (len(by_ref), len(by_jx), with_fx, len(unmatched), dst))
    res = set()
    for e in by_jx.values():
        for c in e["cast"]:
            res.add(c["res"])
        for c in e["child"]:
            res.add(c["res"])
            if "hit" in c:
                res.add(c["hit"]["res"])
    have = {f[:-5] for f in os.listdir(os.path.join(NEXT, "client", "assets3d", "sfx")) if f.endswith(".json")}
    missing = sorted(p for p in res if p.replace("/", "_") not in have)
    print("hieu ung dung: %d, chua xuat: %d %s" % (len(res), len(missing), missing[:10]))


if __name__ == "__main__":
    main()
