# -*- coding: utf-8 -*-
"""Ghep ky nang JX1 (settings/skills.txt -> client/assets/skills.json) voi hieu ung 3D cua bo tham khao (skill_main /
skill_section / skill_event / skill_childobj / sfx_object) -> client/assets3d/sfx/skill_map.json cho KWorldView3D.

    python tools/scn3d/map_skills.py

Cach ghep: ten ky nang cua bo tham khao la ten Han cua chinh ky nang JX1 (怒雷指 = Nộ Lôi Chỉ); doc theo am Han-Viet
(hanviet.py, ke ca cach viet cua ban dich JX1) roi so KHOP voi SkillName cua skills.json sau chuan hoa (chu thuong, bo
khoang trang/so). Khong khop = khong ghep (liet ke trong "unmatched"), ky nang do dung hieu ung chung theo ngu hanh.

Tu bang su kien (skill_event, cot 8 loai su kien eSkillEventType [TK global-metadata, D:\game3gtQ_mo\enum_values.py]:
8 ChildOject = sinh vat con (skill_childobj), 120 SfxObject = hieu ung sfx_object, 2 Hit, 101 AnimPlay, 112 PlaySound,
103 CameraShake, 111 Ghost (bong mo), 26 COSetLineParticle (tia noi), 122 SkillWarning; cot 2 loai kich hoat
eSkillEventTriggerType: 0/"" TimeFrame (cot 3 = khung), 1 LoopFrame, 2 HitPass (sau khi trung), 3 ArriveTargetPos (den
dich), 7 TriggerEnd (het doan / het vat con)). skill_main cot 21 = hieu ung ra chieu (sfx_object) cua chinh ky nang.
childobj: duong dan, khung song, cao (m), diem treo, kieu vi tri (0 nguoi thi trien 1 diem muc tieu 2 muc tieu), dong bo
(3 bay theo muc tieu, 4 bay toi diem, 5/6 bay sat dat = "fly"), toc do, hieu ung trung (cot 23 -> sfx_object); su kien rieng cua vat con (cot 4) sinh vat con
long nhau ("children": when = time / arrive / hit / end) - vd dan bay toi roi no.
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
# sfx_object 1..4 = 土/火/水/木系出招 - bo tham khao KHONG co "kim" (ngoai cong dung vu khi, khong loe sang) [TK]
ELEMENT_CAST = {1: "Skill/shifa_muxi", 2: "Skill/shifa_shuixi", 3: "Skill/shifa_huoxi", 4: "Skill/shifa_tuxi"}
ELEMENT_HIT = {0: "Skill/hit_jinxi01", 1: "Skill/hit_muxi01", 2: "Skill/hit_shuixi01", 3: "Skill/hit_huoxi01", 4: "Skill/hit_tuxi01"}
# Ghep tay [TK skill_main id -> id JX1 skills.txt]: ten JX1 (ban dich) lech am Han-Viet chuan (Diên/Duyên, Ki/Kỳ, Tọa Vọng/Tọa
# Vong, Vô Tướng/Vô Tương, Kim Cang/Kim Cương, Đơn Chỉ/Đạn Chỉ, Diệm/Diễm, Lịch/Lệ, Phạn/Phạm, Trái ảnh/Thiến ảnh, Độc/Cổ,
# Tiêu Diêu/Tiêu Dao, Tích Lịch/Phích Lịch, "Mâu pháp"/"mâu thuật", 打狗阵 = Đả Cẩu bổng 209 (hào quang Cái Bang), 近身/远程武器普攻 =
# danh thuong tay gan 53/1/2 va am khi Duong Mon 43.  Ban "•" (nang cap) khong ghi: chi ban goc.  99 闪避 (ne) JX1 khong co.
MANUAL = {1: [53, 1, 2], 2: [43], 113: [157], 210: [174], 305: [119], 310: [209, 124], 313: [360], 400: [145, 247], 404: [132],
          410: [143], 413: [148], 514: [282, 335, 334], 604: [269], 705: [70], 710: [69, 203], 712: [73], 800: [45, 240],
          802: [347, 348], 1001: [10, 216], 1015: [321]}


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

    def child_entry(cid, at, depth=0):
        r = childobjs.get(str(cid))
        if r is None:
            return None
        # a row without a resource ("空子物体") is a logic carrier: it still spawns nested children / names a hit effect
        life = fnum(r[8], -1.0)
        sync = r[17].strip() if len(r) > 17 else ""
        speed = r[18].strip() if len(r) > 18 else ""
        e = {"res": r[5].strip(), "name": r[2].strip(), "at": at, "life_s": (life / SECTION_FPS) if life > 0 else 1.5,
             "radius": fnum((r[10].split("*") or ["0"])[0], 0.0), "pos_type": int(fnum(r[12], 0)), "height": fnum(r[13], 0.0),
             "hang": r[14].strip() if len(r) > 14 else "", "sync": int(fnum(sync, 0)), "fly": sync in ("3", "4", "5", "6"),
             "speed": fnum((speed.split("*") or ["0"])[0], 0.0)}
        hit = r[23].strip() if len(r) > 23 else ""
        if hit.isdigit():
            h = sfx_entry(hit)
            if h:
                e["hit"] = h
        if depth < 3:
            kids, fxs = child_events(r[4], e["life_s"], depth + 1)
            if kids:
                e["children"] = kids
            for f in fxs:
                # a sfx of the child's own events: at arrival / on hit it is the hit picture when the row names none
                if f.get("when") in ("arrive", "hit") and "hit" not in e:
                    e["hit"] = {"res": f["res"], "life_s": f["life_s"], "hang": f.get("hang", "")}
                else:
                    e.setdefault("fx", []).append(f)
        return e

    def child_events(ids, life_s, depth):
        """cac su kien cua mot vat con (cot 4): vat con long nhau (loai 8) va sfx (loai 120) -> (children[], fx[]) voi
        "when": time (at = khung / khung song), arrive (den dich), hit (sau khi trung), end (het)"""
        kids, fxs = [], []
        for eid in [x for x in str(ids).split("*") if x.strip()]:
            ev = events.get(eid.strip())
            if ev is None or len(ev) < 11:
                continue
            kind = ev[8].strip()
            trig = ev[2].strip()
            when = {"": "time", "0": "time", "1": "time", "2": "hit", "3": "arrive", "7": "end"}.get(trig, "time")
            frame = fnum(ev[3], 0.0)
            at = max(0.0, min(1.0, frame / (life_s * SECTION_FPS))) if when == "time" and life_s > 0 else 0.0
            if kind == "8" and ev[10].strip().isdigit():
                c = child_entry(ev[10].strip(), at, depth)
                if c:
                    c["when"] = when
                    c["count"] = int(fnum(ev[11], 1)) if ev[11].strip() not in ("", "-1") else 1
                    kids.append(c)
            elif kind == "120" and ev[10].strip().isdigit():
                s = sfx_entry(ev[10].strip())
                if s:
                    s["when"] = when
                    s["at"] = at
                    fxs.append(s)
        return kids, fxs

    states = {r[0].strip(): r for r in rows("state_list_cmn") if r[0].strip().isdigit()}

    def state_effects(state_id):
        """state_list: cot 13 vat con (id*giu hat*server), cot 14 sfx_object -> [{res, hang, loop}] (hao quang / trang thai, lap toi khi het)"""
        out = []
        r = states.get(str(state_id))
        if r is None:
            return out
        ch = (r[13].split("*") or [""])[0].strip() if len(r) > 13 else ""
        if ch.isdigit():
            c = child_entry(ch, 0.0)
            if c:
                out.append({"res": c["res"], "hang": c.get("hang", "sys_foot"), "height": c.get("height", 0.0), "loop": True})
        sf = r[14].strip() if len(r) > 14 else ""
        if sf.isdigit():
            e = sfx_entry(sf)
            if e:
                out.append({"res": e["res"], "hang": e.get("hang", ""), "height": 0.0, "loop": True})
        return out

    def section_effects(sec_id, kinds, states_added, depth=0):
        """-> (cast[], child[]); kinds[loai su kien] += 1 (thong ke cho MO-NHI-PHAN-3D); states_added += hao quang cua
        trang thai ma su kien 201 (AddState) them vao"""
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
            kinds[kind or "-"] = kinds.get(kind or "-", 0) + 1
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
            elif kind == "201" and ev[10].strip().isdigit():
                # AddState after the hit: a buff's picture is the state's own halo (state_list) while it holds
                for st in state_effects(ev[10].strip()):
                    st["state"] = int(ev[10].strip())
                    states_added.append(st)
        return casts, childs

    skills = json.load(io.open(os.path.join(NEXT, "client", "assets", "skills.json"), encoding="utf-8"))
    jx = {}
    jx_series = {}
    jx_name = {}
    for r in skills["rows"]:
        cells = r.get("cells", {})
        n = norm(cells.get("SkillName", ""))
        if n:
            jx.setdefault(n, []).append(int(r["id"]))
        jx_series[int(r["id"])] = int(fnum(cells.get("Series", -1), -1))
        jx_name[int(r["id"])] = cells.get("SkillName", "")

    by_jx, by_ref, unmatched = {}, {}, []
    event_kinds = {}
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
        manual = MANUAL.get(int(rid), [])
        kinds = {}
        casts, childs, states_added = [], [], []
        # skill_main col 4 may list several sections ("815*830", "106*107*108" = the moves of a combo); each section
        # chains on through col 11 (next section, col 5 = how the events mix)
        seen = set()
        for sec0 in [x.strip() for x in sec.split("*") if x.strip()]:
            nxt, hops = sec0, 0
            while nxt and nxt not in seen and nxt in sections and hops < 6:
                seen.add(nxt)
                c2, k2 = section_effects(nxt, kinds, states_added)
                casts += c2
                childs += k2
                nxt = sections[nxt][11].strip() if len(sections[nxt]) > 11 else ""
                hops += 1
        # skill_main col 21: the skill's own cast effect (sfx_object 1..4 = the element flash at the chest)
        own_cast = r[21].strip() if len(r) > 21 else ""
        if own_cast.isdigit():
            s0 = sfx_entry(own_cast)
            if s0:
                s0["at"] = 0.0
                s0["on_hit"] = False
                casts.insert(0, s0)
        for k, n in kinds.items():
            event_kinds[k] = event_kinds.get(k, 0) + n
        aura = []
        srow = sections.get(sec.split("*")[0].strip())
        if srow is not None and len(srow) > 4 and srow[4].strip().isdigit():
            aura = state_effects(srow[4].strip())   # "光环数据, 状态id": the halo the aura / buff keeps while it holds
        for st in states_added:
            if all(st["res"] != a0["res"] for a0 in aura):
                aura.append(st)
        entry = {"ref": int(rid), "cn": name, "vi": hit or (rs[0] if rs else ""), "element": r[2].strip(), "type": r[3].strip(),
                 "cast": casts, "child": childs, "aura": aura, "events": kinds}
        by_ref[rid] = entry
        if hit is None and not manual:
            unmatched.append({"ref": int(rid), "cn": name, "hanviet": rs[0] if rs else "?"})
            continue
        if manual:
            entry["vi"] = jx_name.get(manual[0], entry["vi"])
            entry["manual"] = True
        for jid in (manual or jx[norm(hit)]):
            # a levelled variant (天地无极•开天•初) does not replace the base skill's effects when the base is mapped too
            if str(jid) in by_jx and "•" in name:
                continue
            by_jx[str(jid)] = entry
    out = {"by_jx": by_jx, "element_cast": ELEMENT_CAST, "element_hit": ELEMENT_HIT, "series_of": {str(k): v for k, v in jx_series.items() if v >= 0},
           "unmatched": unmatched, "section_fps": SECTION_FPS, "event_kinds": event_kinds}
    dst = os.path.join(NEXT, "client", "assets3d", "sfx", "skill_map.json")
    with_fx = sum(1 for e in by_jx.values() if e["cast"] or e["child"] or e.get("aura"))
    # StateSpecialId cua moi ky nang JX1 (skills.json): the 0x7a packet names the states of other entities by it
    special = {}
    for r in skills["rows"]:
        sp = str(r.get("cells", {}).get("StateSpecialId", "")).strip()
        if sp.isdigit() and int(sp) > 0:
            special.setdefault(sp, int(r["id"]))
    out["skill_of_special"] = special
    with io.open(dst, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    print("ky nang tham khao %d, ghep JX1 %d ky nang (co hieu ung %d), khong ghep %d -> %s" % (len(by_ref), len(by_jx), with_fx, len(unmatched), dst))
    res = set()

    def walk(c):
        if c["res"]:
            res.add(c["res"])
        if "hit" in c:
            res.add(c["hit"]["res"])
        for f in c.get("fx", []):
            res.add(f["res"])
        for k in c.get("children", []):
            walk(k)
    for e in by_jx.values():
        for c in e["cast"] + e.get("aura", []):
            res.add(c["res"])
        for c in e["child"]:
            walk(c)
    print("loai su kien trong cac doan ky nang:", sorted(event_kinds.items(), key=lambda x: -x[1]))
    have = {f[:-5] for f in os.listdir(os.path.join(NEXT, "client", "assets3d", "sfx")) if f.endswith(".json")}
    missing = sorted(p for p in res if p.replace("/", "_") not in have)
    print("hieu ung dung: %d, chua xuat: %d %s" % (len(res), len(missing), missing[:10]))


if __name__ == "__main__":
    main()
