# -*- coding: utf-8 -*-
"""Dung bo du lieu map 3D cho zone + client tu mot scene da xuat (export_scene.py + export_npc.py):

    python tools/scn3d/make_map3d.py world_baling --id 9053

Ghi client/assets3d/maps/<id>/:
    map.json      ban 2D toi thieu cho zone (id, luoi o 32, spawn, npcs theo template JX1, traps) - schema MAPS.md §3
    obstacle.bin  1 byte/o, hang truoc cot, 0 = di duoc: o di duoc khi tam o nam trong mot tam giac navmesh (chieu XZ)
    map3d.json    scene glTF, UNIT, origin (goc scene (0,0) cua zone nam o dau trong glTF), camera, y mat dat
    models.json   template JX1 -> model 3D (cha_pic id cua bo tham khao) cho KWorldView3D

Quy uoc toa do: docs/3D-QUY-UOC.md (UNIT = 0,02 m; X = x*UNIT + origin.X; Z = y*UNIT + origin.Z; yaw = 180 + angle mark).
Zone doc thu muc nay qua zone.maps_dir_extra = client/assets3d/maps.
"""
import argparse
import io
import json
import math
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
UNIT = 0.02
CELL = 32
REGION_W, REGION_H = 512, 1024
MARGIN_M = 8.0          # met trong quanh navmesh

# cha_pic (bo tham khao) -> template npcs.txt cua JX1 (client/assets/npcres/npcs.json), chi de thu tren map nay:
# (template, kind, level).  kind 3 = nguoi trong thanh (khong danh duoc), 0 = quai, 5 = thu nuoi trong thanh.
CHA_TO_TEMPLATE = {
    1004: (197, 3, 0),   # 巴陵铁匠 -> Thợ rèn 1
    1035: (198, 3, 0),   # 神秘铁匠 -> Thợ rèn 2
    1001: (203, 3, 0),   # 药店 -> Ông chủ dược điếm
    1000: (235, 3, 0),   # 车夫 -> Xa phu ất 1
    1002: (215, 3, 0),   # 杂货 -> Chủ tạp hóa ất
    1003: (291, 3, 0),   # 储物箱 -> Tạp hóa nhỏ 1 (JX1 khong co chu kho rieng)
    1020: (111, 3, 0),   # 天王将领 -> Thiên Vương bang chủ
    1022: (73, 3, 0),    # 武当 -> Võ Đang Chưởng môn
    1021: (82, 3, 0),    # 峨眉 -> Nga Mi Chưởng môn
    1024: (130, 3, 0),   # 天忍 -> Thiên Nhẫn giáo chủ
    1028: (104, 3, 0),   # 唐门 -> Đường Môn Chưởng môn
    1027: (118, 3, 0),   # 五毒 -> Ngũ Độc giáo chủ
    1023: (92, 3, 0),    # 昆仑 -> Côn Lôn Chưởng môn
    1029: (64, 3, 0),    # 少林 -> Thiếu Lâm Phương trượng
    1025: (98, 3, 0),    # 丐帮 -> Cái Bang bang chủ
    1026: (125, 3, 0),   # 翠烟 -> Thúy Yên Chưởng môn
    1010: (321, 3, 0),   # 野叟 -> Ăn mày
    1030: (47, 3, 0),    # 礼官 -> Tống Quan viên 1
    1011: (1983, 3, 0),  # 绝技 -> Đầu bếp mập (nguoi thuong)
    1031: (48, 3, 0),    # 称号 -> Tống Quan viên 2
    1032: (1905, 3, 0),  # 商城 -> Thương Buôn
    1033: (2149, 3, 0),  # 交易 -> Thương Nhân Chợ Đen
    1034: (49, 3, 0),    # 排行 -> Kim Quan viên 1
    1036: (50, 3, 0),    # 活动 -> Kim Quan viên 2
    1038: (294, 3, 0),   # 神秘商人 -> Bán thịt heo 1
    1039: (626, 3, 0),   # 龙武 -> Kỳ Binh Trung lập
    25: (43, 0, 10),     # 白猪 -> Heo trắng (quai cap 10)
    26: (42, 0, 10),     # 梅花鹿 -> Hươu đốm
    28: (429, 5, 0),     # 金毛 -> Hoàng cẩu 1 (thu trong thanh)
    31: (11, 0, 15),     # 小黄金BOSS -> Heo rừng cap 15
}
PLAYER_MODELS = {"0": 1, "1": 1}   # sex -> cha_pic (bo tham khao chi xuat nhan vat nam 1; nu dung tam nam)
# ngua: client 2.0 ve ngua theo HANG ANH (Settings\item\HorseRes.txt cot 2 - 2, KItemChangeRes::GetHorseRes; goi 0xad dong bo
# dung hang do cho nguoi khac) - nhieu vat pham ngua chung mot hang (vd hang 4 = ngua thanh cap 6..10 cua nhom 1 + cap 1..3 cua
# nhom 2). Cot 3 cua HorseRes.txt ta ten anh (普通黄马, 赤兔，顶级红...) -> cha_pic bo tham khao theo MAU trong ten [tự chọn
# cach ghep mau]: 白/雪白 -> 白马 1500 (顶级 -> 玉花骢 1510), 黑/黝黑 -> 黑马 1501 (顶级 -> 黑骐 1511, 绝地 -> 黑龙马 1561),
# 青/青白 -> 青马 1503, 黄/棕黄 -> 棕马 1502, 红/棕红 -> 红骊 1512, 血红/龙驹 -> 血龙马 1562; ho, su tu, lac da, huou, lua,
# soi, lon... bo tham khao khong co -> None (ngua mac dinh).
HORSE_RES = {   # hang anh: (ten trong HorseRes.txt [2.0], cha_pic)
    0: ("赤兔，顶级红", 1512), 1: ("照夜玉狮子，顶级白", 1510), 2: ("的卢，顶级黄", 1502), 3: ("普通黑马", 1501),
    4: ("普通青马", 1503), 5: ("乌云踏雪，顶级黑", 1511), 6: ("普通红马", 1512), 7: ("普通白马", 1500), 8: ("普通黄马", 1502),
    9: ("绝影，顶级青", 1503), 10: ("奔宵，顶级黝黑", 1511), 11: ("翻羽，顶级雪白 / 风云战马", 1510), 12: ("飞云，顶级青白", 1503),
    13: ("赤龙驹，顶级血红", 1562), 14: ("绝地，顶级黝黑", 1561), 15: ("逾辉，顶级血红", 1562), 16: ("腾雾，顶级棕黄", 1502),
    17: ("超光，顶级棕红", 1512), 18: ("虎王", None), 19: ("火睛金虎王", None), 20: ("金睛白虎王", None), 21: ("龙睛黑虎王", None),
    22: ("汗血龙驹", 1562), 23: ("狮子", None), 24: ("骆驼", None), 25: ("草泥马", None), 26: ("梅花鹿", None), 27: ("扬沙", None),
    28: ("御风", None), 29: ("追电", None), 30: ("流星", None), 31: ("蓝麟神鹿", None), 32: ("2015VIP·墨羽仙驴", None),
    34: ("2017VIP·战狼", None), 35: ("银甲玄彘", None),
}
HORSES = {"by_res": {str(k): v[1] for k, v in HORSE_RES.items() if v[1] is not None},
          "names": {str(k): v[0] for k, v in HORSE_RES.items()}, "default": 1500}


def yaw_to_dir(yaw_deg):
    """yaw Godot (do) -> dir 0..63 cua JX1 (KScene3DMath.dir_of_yaw)."""
    return int(round(32 - yaw_deg / 5.625)) % 64


def point_in_tri(px, pz, a, b, c):
    # dau cua tich co huong voi tung canh (chieu XZ)
    d1 = (px - b[0]) * (a[2] - b[2]) - (a[0] - b[0]) * (pz - b[2])
    d2 = (px - c[0]) * (b[2] - c[2]) - (b[0] - c[0]) * (pz - c[2])
    d3 = (px - a[0]) * (c[2] - a[2]) - (c[0] - a[0]) * (pz - a[2])
    neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
    pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
    return not (neg and pos)


def export_minimap(scene_id, origin, out):
    """Anh minimap cua bo tham khao [TK]: bang ui_map_view (id scene -> ten anh trong Assets/GUI/res/textures/map/, ti le,
    toa do the gioi Unity cua goc trai-duoi (x, z) va phai-tren) trong bundle UI 9431897d8969 (container = md5 duong dan).
    Bo xuat dao truc X (Unity -> Godot) nen anh xoay 180 do de toa do scene cua ta (x = (-x_unity - origin.X)/UNIT,
    y = (z_unity - origin.Z)/UNIT, y tang ve phia camera) van di theo hang/cot anh: sau khi xoay, goc trai-tren anh =
    (x_unity max, z_unity max) -> scene (x nho nhat, y ... ) - xem doc/3D-QUY-UOC. Tra ve khoi "minimap" cho map3d.json."""
    try:
        import hashlib
        import UnityPy
        from PIL import Image
    except ImportError:
        return None
    src = os.environ.get("JX_SCN3D_SRC", r"D:\game3gtQ_mo\pc\剑网江湖_Data\StreamingAssets")
    keyf = os.environ.get("JX_SCN3D_KEY", r"D:\game3gtQ_mo\khoa_bundle.txt")
    if not os.path.exists(keyf):
        return None
    UnityPy.set_assetbundle_decrypt_key(io.open(keyf, encoding="utf-8").read().strip().split()[0])
    from export_npc import Tables
    t = Tables(src)
    row = None
    for r in t.t.get("ui_map_view_cmn", [])[1:]:
        if r and r[0].strip() == str(scene_id):
            row = r
            break
    if row is None or len(row) < 9:
        return None
    fname = row[2].strip()
    x0, z0, x1, z1 = float(row[5]), float(row[6]), float(row[7]), float(row[8])   # trai-duoi, phai-tren (Unity x, z)
    env = UnityPy.load(os.path.join(src, "9431897d8969.bdd"))
    cont = {}
    for o in env.objects:
        if o.type.name == "AssetBundle":
            for k, v in o.read_typetree()["m_Container"]:
                cont[k] = v["asset"]["m_PathID"]
    pid = cont.get(hashlib.md5(("assets/gui/res/textures/map/" + fname).lower().encode("utf-8")).hexdigest()[:12])
    if pid is None:
        return None
    tex = None
    for o in env.objects:
        if o.path_id == pid and o.type.name == "Texture2D":
            tex = o.read()
            break
    if tex is None:
        return None
    img = tex.image.convert("RGBA").rotate(180)
    img.save(os.path.join(out, "minimap.png"))
    # scene units: x_scene = (-x_unity - origin.X)/UNIT -> the picture's Unity x range [x0, x1] becomes [(-x1 - oX)/U, (-x0 - oX)/U]
    left = (-x1 - origin[0]) / UNIT
    right = (-x0 - origin[0]) / UNIT
    top = (z0 - origin[1]) / UNIT
    bottom = (z1 - origin[1]) / UNIT
    return {"file": "minimap.png", "left": left, "top": top, "right": right, "bottom": bottom, "width": img.width, "height": img.height,
            "source": "ui_map_view %s %s" % (scene_id, fname)}


def rasterize(verts, tris, origin, cells_x, cells_y):
    """obstacle grid: 0 = o di duoc (tam o trong navmesh), 1 = vat can."""
    grid = bytearray([1]) * (cells_x * cells_y)
    cell_m = CELL * UNIT
    ox, oz = origin
    for t in tris:
        a, b, c = verts[t[0]], verts[t[1]], verts[t[2]]
        x0 = min(a[0], b[0], c[0]); x1 = max(a[0], b[0], c[0])
        z0 = min(a[2], b[2], c[2]); z1 = max(a[2], b[2], c[2])
        cx0 = max(0, int((x0 - ox) / cell_m) - 1); cx1 = min(cells_x - 1, int((x1 - ox) / cell_m) + 1)
        cy0 = max(0, int((z0 - oz) / cell_m) - 1); cy1 = min(cells_y - 1, int((z1 - oz) / cell_m) + 1)
        for cy in range(cy0, cy1 + 1):
            pz = oz + (cy + 0.5) * cell_m
            row = cy * cells_x
            for cx in range(cx0, cx1 + 1):
                if grid[row + cx] == 0:
                    continue
                px = ox + (cx + 0.5) * cell_m
                if point_in_tri(px, pz, a, b, c):
                    grid[row + cx] = 0
    return grid


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("map", help="thu muc scene da xuat trong client/assets3d (vd world_baling)")
    ap.add_argument("--id", type=int, required=True, help="map id cho zone (vd 9053)")
    ap.add_argument("--name", default="", help="ten hien thi (mac dinh name_vi cua scene + ' (3D)')")
    ap.add_argument("--spawn", default="BeginPoint01", help="mark lam diem sinh")
    ap.add_argument("--exit-to", type=int, default=1, help="map id ma cac ExitPoint dua ve (mac dinh 1 = Phuong Tuong)")
    ap.add_argument("--exit-x", type=int, default=72 * 512 + 12784, help="toa do scene tuyet doi (Mps: goc region + toa do map) tren map dich; mac dinh diem sinh Phuong Tuong")
    ap.add_argument("--exit-y", type=int, default=82 * 1024 + 16848)
    a = ap.parse_args()

    src = os.path.join(NEXT, "client", "assets3d", a.map)
    scene = json.load(io.open(os.path.join(src, "scene.json"), encoding="utf-8"))
    npcs = json.load(io.open(os.path.join(src, "npcs.json"), encoding="utf-8"))
    templates = json.load(io.open(os.path.join(NEXT, "client", "assets", "npcres", "npcs.json"), encoding="utf-8"))["templates"]
    nav = scene["marks"]["nav"]
    verts = nav["verts"]
    tris = nav["tris"]
    xs = [v[0] for v in verts]; zs = [v[2] for v in verts]; ys = [v[1] for v in verts]
    # goc scene (0,0) cua zone: goc tren-trai cua navmesh tru mot le, lam tron xuong theo region de origin cua region 0 dep
    origin = (math.floor((min(xs) - MARGIN_M) / (REGION_W * UNIT)) * REGION_W * UNIT,
              math.floor((min(zs) - MARGIN_M) / (REGION_H * UNIT)) * REGION_H * UNIT)
    w_units = (max(xs) + MARGIN_M - origin[0]) / UNIT
    h_units = (max(zs) + MARGIN_M - origin[1]) / UNIT
    region_cols = int(math.ceil(w_units / REGION_W))
    region_rows = int(math.ceil(h_units / REGION_H))
    cells_x = region_cols * REGION_W // CELL
    cells_y = region_rows * REGION_H // CELL
    grid = rasterize(verts, tris, origin, cells_x, cells_y)
    walk = sum(1 for b in grid if b == 0)

    def to_scene(pos):
        return (int(round((pos[0] - origin[0]) / UNIT)), int(round((pos[2] - origin[1]) / UNIT)))

    points = scene["marks"]["points"]
    spawn_mark = points.get(a.spawn) or next(iter(points.values()))
    spawn = to_scene(spawn_mark[0]["pos"])
    name = a.name or ("%s (3D)" % scene["table"].get("name_vi", a.map))

    placements = []
    models = {}
    missing = set()
    # bang ghep tu dong (map_npcs.py -> cha_templates.json: quai theo tu dien ten CN -> VI) sau bang tay CHA_TO_TEMPLATE
    auto_tpl = {}
    auto_path = os.path.join(HERE, "cha_templates.json")
    if os.path.exists(auto_path):
        auto_tpl = {int(k): tuple(v[:3]) for k, v in json.load(io.open(auto_path, encoding="utf-8")).items()}
        auto_name = {int(k): v[4] for k, v in json.load(io.open(auto_path, encoding="utf-8")).items() if len(v) > 4}
    for p in npcs.get("placements", []):
        cha = int(p["cha"])
        tpl = CHA_TO_TEMPLATE.get(cha) or auto_tpl.get(cha)
        if tpl is None:
            missing.add(cha)
            continue
        tid, kind, level = tpl
        t = templates.get(str(tid))
        if t is None:
            missing.add(cha)
            continue
        x, y = to_scene(p["pos"])
        yaw = 180.0 + float(p.get("angle", 0.0))
        placements.append({"template_id": tid, "name": auto_name.get(cha, t["name"]), "x": x, "y": y, "kind": kind, "level": level,
                           "camp": int(t.get("camp", 0)), "series": int(t.get("series", 0)), "dir": yaw_to_dir(yaw), "mark": p.get("mark", "")})
        models[str(tid)] = cha
    # the exits of the reference map become traps (KRegion::LoadServerTrap runs: cells x, y, n) whose script sends the
    # character back to the default map (--exit-to, Phuong Tuong's spawn by default): script root data/script/3d
    # (dev.py LUA_ROOT_NAMES), game path <bs>script<bs>3d<bs>exit_<map>_<n>.lua
    exits = []
    traps = []
    script_dir = os.path.join(NEXT, "data", "script", "3d", "script", "3d")
    os.makedirs(script_dir, exist_ok=True)
    n_exit = 0
    bs = chr(92)
    for mark, lst in sorted(points.items()):
        if mark.startswith("ExitPoint") or mark.startswith("EnterPoint"):
            for e in lst:
                x, y = to_scene(e["pos"])
                n_exit += 1
                cx, cy = x // CELL, y // CELL
                game_path = bs + "script" + bs + "3d" + bs + "exit_%d_%d.lua" % (a.id, n_exit)
                lines = ["-- %s cua map %d (%s): ve map %d" % (mark, a.id, name, a.exit_to),
                         "function main()", chr(9) + "NewWorld(%d, %d, %d)" % (a.exit_to, a.exit_x // CELL, a.exit_y // CELL), "end", ""]
                with io.open(os.path.join(script_dir, "exit_%d_%d.lua" % (a.id, n_exit)), "w", encoding="utf-8", newline=chr(10)) as f:
                    f.write(chr(10).join(lines))
                exits.append({"mark": mark, "x": x, "y": y, "trap": n_exit})
                for dy in (-1, 0, 1):
                    traps.append({"x": cx - 1, "y": cy + dy, "n": 3, "id": n_exit, "script": game_path})

    out = os.path.join(NEXT, "client", "assets3d", "maps", str(a.id))
    os.makedirs(out, exist_ok=True)
    map_json = {
        "id": a.id, "name": name, "source": "3d:" + a.map,
        "region_left": 0, "region_top": 0, "region_cols": region_cols, "region_rows": region_rows,
        "region_w": REGION_W, "region_h": REGION_H, "cell_size": CELL, "cells_x": cells_x, "cells_y": cells_y,
        "scene_w": cells_x * CELL, "scene_h": cells_y * CELL, "spawn": [spawn[0], spawn[1]], "indoor": False,
        "regions": [], "traps": traps, "npcs": placements, "exits": exits,
    }
    with io.open(os.path.join(out, "map.json"), "w", encoding="utf-8") as f:
        json.dump(map_json, f, ensure_ascii=False, indent=1)
    with open(os.path.join(out, "obstacle.bin"), "wb") as f:
        f.write(bytes(grid))
    cam = scene["table"].get("camera", {})
    map3d = {
        "id": a.id, "name": name, "scene": "../../%s/%s.gltf" % (a.map, a.map), "scene_json": "../../%s/scene.json" % a.map,
        "npcs_json": "../../%s/npcs.json" % a.map, "unit": UNIT, "origin": [origin[0], origin[1]], "scale": 1.0,
        "camera": cam, "ground_y": min(ys), "models": "models.json",
    }
    mm = export_minimap(scene["table"].get("id"), origin, out)
    if mm:
        map3d["minimap"] = mm
        print("minimap: %s %dx%d, scene x %.0f..%.0f y %.0f..%.0f" % (mm["source"], mm["width"], mm["height"], mm["left"], mm["right"], mm["top"], mm["bottom"]))
    with io.open(os.path.join(out, "map3d.json"), "w", encoding="utf-8") as f:
        json.dump(map3d, f, ensure_ascii=False, indent=1)
    with io.open(os.path.join(out, "models.json"), "w", encoding="utf-8") as f:
        json.dump({"templates": models, "player": PLAYER_MODELS, "horses": HORSES, "models_dir": "../../npc"}, f, ensure_ascii=False, indent=1)
    print("map %d %s: origin (%.2f, %.2f) m, %d x %d region, %d x %d o, di duoc %d o (%.1f%%), spawn %s, npc %d, exit %d (bay %d o)"
          % (a.id, name, origin[0], origin[1], region_cols, region_rows, cells_x, cells_y, walk, 100.0 * walk / len(grid), spawn, len(placements), len(exits), len(traps)))
    if missing:
        print("cha chua co template JX1 (bo qua):", sorted(missing))
    print("->", out)


if __name__ == "__main__":
    main()
