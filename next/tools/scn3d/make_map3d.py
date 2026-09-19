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

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
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
    for p in npcs.get("placements", []):
        cha = int(p["cha"])
        tpl = CHA_TO_TEMPLATE.get(cha)
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
        placements.append({"template_id": tid, "name": t["name"], "x": x, "y": y, "kind": kind, "level": level,
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
    with io.open(os.path.join(out, "map3d.json"), "w", encoding="utf-8") as f:
        json.dump(map3d, f, ensure_ascii=False, indent=1)
    with io.open(os.path.join(out, "models.json"), "w", encoding="utf-8") as f:
        json.dump({"templates": models, "player": PLAYER_MODELS, "models_dir": "../../npc"}, f, ensure_ascii=False, indent=1)
    print("map %d %s: origin (%.2f, %.2f) m, %d x %d region, %d x %d o, di duoc %d o (%.1f%%), spawn %s, npc %d, exit %d (bay %d o)"
          % (a.id, name, origin[0], origin[1], region_cols, region_rows, cells_x, cells_y, walk, 100.0 * walk / len(grid), spawn, len(placements), len(exits), len(traps)))
    if missing:
        print("cha chua co template JX1 (bo qua):", sorted(missing))
    print("->", out)


if __name__ == "__main__":
    main()
