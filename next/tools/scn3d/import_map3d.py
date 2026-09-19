# -*- coding: utf-8 -*-
"""Nhap mot map 3D TU LAM (glTF 2.0 theo docs/3D-HOA-SI.md) thanh bo du lieu cho zone + client (M3D-2 buoc 2.2):

    python tools/scn3d/import_map3d.py duong/dan/<ten>.gltf --id 9001 [--check]

Ghi client/assets3d/maps/<id>/: <ten>.gltf (+ .bin, anh) sao chep, scene.json (render, marks: points/nav), map3d.json,
map.json + obstacle.bin cho zone (tu mesh `walk*`), traps + Lua NewWorld tu empty `exit_<map>_<x>_<y>_<n>`, models.json rong
(NPC theo template JX1 dung model cua models.json chung neu co).  --check: chi kiem va in bao cao, khong ghi.

Quy uoc: met, Y len, -Z bac; goc tep = goc tay-bac cua map; ten node quyet dinh loai (terrain/walk/building/tree/grass/water/
spawn/npc_<tpl>_<n>/exit_<map>_<x>_<y>_<n>/light_sun/cam).
"""
import argparse
import io
import json
import math
import os
import shutil
import struct
import sys

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from make_map3d import rasterize, yaw_to_dir, UNIT, CELL, REGION_W, REGION_H, MARGIN_M, CHA_TO_TEMPLATE, PLAYER_MODELS, HORSES  # noqa: E402

COMP = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}
DTYPE = {5120: np.int8, 5121: np.uint8, 5122: np.int16, 5123: np.uint16, 5125: np.uint32, 5126: np.float32}


class Gltf:
    def __init__(self, path):
        self.dir = os.path.dirname(os.path.abspath(path))
        self.g = json.load(io.open(path, encoding="utf-8"))
        self.bufs = []
        for b in self.g.get("buffers", []):
            uri = b.get("uri", "")
            if uri.startswith("data:"):
                import base64
                self.bufs.append(base64.b64decode(uri.split(",", 1)[1]))
            else:
                self.bufs.append(open(os.path.join(self.dir, uri), "rb").read())

    def accessor(self, i):
        a = self.g["accessors"][i]
        bv = self.g["bufferViews"][a["bufferView"]]
        buf = self.bufs[bv.get("buffer", 0)]
        off = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
        n = a["count"]; c = COMP[a["type"]]; dt = DTYPE[a["componentType"]]
        stride = bv.get("byteStride", 0)
        if stride and stride != c * np.dtype(dt).itemsize:
            out = np.zeros((n, c), dtype=dt)
            for k in range(n):
                out[k] = np.frombuffer(buf, dtype=dt, count=c, offset=off + k * stride)
            return out
        return np.frombuffer(buf, dtype=dt, count=n * c, offset=off).reshape(n, c)

    def node_matrix(self, node):
        if "matrix" in node:
            return np.array(node["matrix"], dtype=np.float64).reshape(4, 4).T
        t = node.get("translation", [0, 0, 0]); q = node.get("rotation", [0, 0, 0, 1]); s = node.get("scale", [1, 1, 1])
        x, y, z, w = q
        R = np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                      [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                      [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])
        M = np.eye(4)
        M[:3, :3] = R * np.array(s)[None, :]
        M[:3, 3] = t
        return M

    def walk(self):
        """-> [(node index, node, world matrix)] moi node, duyet tu scene goc"""
        out = []
        scene = self.g["scenes"][self.g.get("scene", 0)]
        def rec(i, parent):
            node = self.g["nodes"][i]
            M = parent @ self.node_matrix(node)
            out.append((i, node, M))
            for c in node.get("children", []):
                rec(c, M)
        for r in scene.get("nodes", []):
            rec(r, np.eye(4))
        return out

    def mesh_triangles(self, mesh_index, M):
        """-> (verts (n,3) world, tris (m,3))"""
        m = self.g["meshes"][mesh_index]
        verts = []; tris = []; base = 0
        for p in m.get("primitives", []):
            pos = self.accessor(p["attributes"]["POSITION"]).astype(np.float64)
            w = (M @ np.concatenate([pos, np.ones((len(pos), 1))], axis=1).T).T[:, :3]
            if "indices" in p:
                idx = self.accessor(p["indices"]).reshape(-1).astype(np.int64)
            else:
                idx = np.arange(len(pos))
            mode = p.get("mode", 4)
            if mode == 4:
                t = idx.reshape(-1, 3)
            else:
                continue
            verts.append(w); tris.append(t + base); base += len(w)
        if not verts:
            return np.zeros((0, 3)), np.zeros((0, 3), dtype=np.int64)
        return np.concatenate(verts), np.concatenate(tris)


def kind_of(name):
    n = name.lower()
    for k in ("terrain", "walk", "building", "tree", "grass", "water", "stone", "prop", "spawn", "npc_", "exit_", "light_sun", "cam"):
        if n.startswith(k):
            return k.rstrip("_")
    return "prop"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("gltf")
    ap.add_argument("--id", type=int, required=True)
    ap.add_argument("--name", default="")
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()
    g = Gltf(a.gltf)
    desc_path = os.path.splitext(a.gltf)[0] + ".map3d.json"
    desc = json.load(io.open(desc_path, encoding="utf-8")) if os.path.exists(desc_path) else {}
    name = a.name or desc.get("name") or os.path.splitext(os.path.basename(a.gltf))[0]
    templates = json.load(io.open(os.path.join(NEXT, "client", "assets", "npcres", "npcs.json"), encoding="utf-8"))["templates"]

    counts = {}
    walk_v, walk_t = [], []
    terrain_v = []
    spawns, npcs, exits, errors = [], [], [], []
    cam = None; sun = None
    base = 0
    for i, node, M in g.walk():
        nm = node.get("name", "n%d" % i)
        k = kind_of(nm)
        counts[k] = counts.get(k, 0) + 1
        pos = M[:3, 3]
        fwd = -M[:3, 2]   # -Z cua node trong the gioi
        yaw = math.degrees(math.atan2(-fwd[0], -fwd[2]))
        if "mesh" in node:
            v, t = g.mesh_triangles(node["mesh"], M)
            if k == "walk":
                walk_v.append(v); walk_t.append(t + base); base += len(v)
            if k == "terrain":
                terrain_v.append(v)
        if k == "spawn":
            spawns.append((nm, pos, yaw))
        elif k == "npc":
            parts = nm.split("_")
            try:
                tid = int(parts[1])
            except (IndexError, ValueError):
                errors.append("npc node '%s': can npc_<template>_<n>" % nm); continue
            t = templates.get(str(tid))
            if t is None:
                errors.append("npc node '%s': template %d khong co trong npcs.txt" % (nm, tid)); continue
            npcs.append((tid, t, pos, yaw, nm))
        elif k == "exit":
            parts = nm.split("_")
            try:
                exits.append((int(parts[1]), int(parts[2]), int(parts[3]), pos, np.linalg.norm(M[:3, 0]), np.linalg.norm(M[:3, 2]), nm))
            except (IndexError, ValueError):
                errors.append("exit node '%s': can exit_<map>_<x>_<y>_<n>" % nm)
        elif k == "cam":
            cam = node.get("extras", {})
        elif k == "light_sun":
            sun = {"dir": [float(fwd[0]), float(fwd[1]), float(fwd[2])], "color": node.get("extras", {}).get("color", [1, 1, 0.95])}
    if not walk_v:
        errors.append("thieu mesh 'walk*' (vung di duoc)")
    if not terrain_v:
        errors.append("thieu mesh 'terrain*'")
    if not spawns:
        errors.append("thieu empty 'spawn'")
    allv = np.concatenate(walk_v + terrain_v) if (walk_v or terrain_v) else np.zeros((1, 3))
    xmin, zmin = float(allv[:, 0].min()), float(allv[:, 2].min())
    xmax, zmax = float(allv[:, 0].max()), float(allv[:, 2].max())
    if xmin < -0.5 or zmin < -0.5:
        errors.append("map nam ngoai goc tay-bac: x/z nho nhat %.1f / %.1f (phai >= 0)" % (xmin, zmin))
    origin = (0.0, 0.0)
    w_units = (xmax + MARGIN_M) / UNIT
    h_units = (zmax + MARGIN_M) / UNIT
    region_cols = max(1, int(math.ceil(w_units / REGION_W)))
    region_rows = max(1, int(math.ceil(h_units / REGION_H)))
    cells_x = region_cols * REGION_W // CELL
    cells_y = region_rows * REGION_H // CELL
    verts = np.concatenate(walk_v).tolist() if walk_v else []
    tris = np.concatenate(walk_t).tolist() if walk_t else []
    grid = rasterize(verts, tris, origin, cells_x, cells_y)
    walk_cells = sum(1 for b in grid if b == 0)

    def to_scene(p):
        return (int(round((p[0] - origin[0]) / UNIT)), int(round((p[2] - origin[1]) / UNIT)))

    print("map %d '%s': %.1f x %.1f m -> %d x %d region, %d x %d o, di duoc %d o (%.1f%%)" % (
        a.id, name, xmax - xmin, zmax - zmin, region_cols, region_rows, cells_x, cells_y, walk_cells, 100.0 * walk_cells / max(1, len(grid))))
    print("node:", ", ".join("%s %d" % kv for kv in sorted(counts.items())))
    print("spawn %d, npc %d, exit %d" % (len(spawns), len(npcs), len(exits)))
    for e in errors:
        print("LOI:", e)
    if a.check or errors:
        sys.exit(1 if errors else 0)

    out = os.path.join(NEXT, "client", "assets3d", "maps", str(a.id))
    os.makedirs(out, exist_ok=True)
    # sao chep glTF + bin + anh
    base_name = os.path.basename(a.gltf)
    shutil.copy2(a.gltf, os.path.join(out, base_name))
    for b in g.g.get("buffers", []) + g.g.get("images", []):
        uri = b.get("uri", "")
        if uri and not uri.startswith("data:"):
            src = os.path.join(g.dir, uri)
            dst = os.path.join(out, uri)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            if os.path.exists(src):
                shutil.copy2(src, dst)
    spawn_scene = to_scene(spawns[0][1])
    placements = []
    for tid, t, pos, yaw, nm in npcs:
        x, y = to_scene(pos)
        placements.append({"template_id": tid, "name": t["name"], "x": x, "y": y, "kind": int(t.get("kind", 0)), "level": 0,
                           "camp": int(t.get("camp", 0)), "series": int(t.get("series", 0)), "dir": yaw_to_dir(yaw), "mark": nm})
    traps = []
    script_dir = os.path.join(NEXT, "data", "script", "3d", "script", "3d")
    os.makedirs(script_dir, exist_ok=True)
    bs = chr(92)
    for n_exit, (map_to, ax, ay, pos, sx, sz, nm) in enumerate(exits, 1):
        x, y = to_scene(pos)
        cx, cy = x // CELL, y // CELL
        nx = max(1, int(round(sx / (CELL * UNIT)))); ny = max(1, int(round(sz / (CELL * UNIT))))
        game_path = bs + "script" + bs + "3d" + bs + "exit_%d_%d.lua" % (a.id, n_exit)
        lines = ["-- %s cua map %d (%s): ve map %d o (%d, %d)" % (nm, a.id, name, map_to, ax, ay), "function main()",
                 chr(9) + "NewWorld(%d, %d, %d)" % (map_to, ax, ay), "end", ""]
        with io.open(os.path.join(script_dir, "exit_%d_%d.lua" % (a.id, n_exit)), "w", encoding="utf-8", newline=chr(10)) as f:
            f.write(chr(10).join(lines))
        for dy in range(-(ny // 2), ny - ny // 2):
            traps.append({"x": cx - nx // 2, "y": cy + dy, "n": nx, "id": n_exit, "script": game_path})
    map_json = {"id": a.id, "name": name, "source": "3d:" + base_name, "region_left": 0, "region_top": 0, "region_cols": region_cols,
                "region_rows": region_rows, "region_w": REGION_W, "region_h": REGION_H, "cell_size": CELL, "cells_x": cells_x, "cells_y": cells_y,
                "scene_w": cells_x * CELL, "scene_h": cells_y * CELL, "spawn": [spawn_scene[0], spawn_scene[1]], "indoor": False,
                "regions": [], "traps": traps, "npcs": placements}
    with io.open(os.path.join(out, "map.json"), "w", encoding="utf-8") as f:
        json.dump(map_json, f, ensure_ascii=False, indent=1)
    with open(os.path.join(out, "obstacle.bin"), "wb") as f:
        f.write(bytes(grid))
    render = desc.get("render", {"ambient_light": [0.55, 0.55, 0.6], "fog": False})
    if sun:
        render["light"] = sun
    points = {"BeginPoint01": [{"pos": [float(spawns[0][1][0]), float(spawns[0][1][1]), float(spawns[0][1][2])], "angle": float(spawns[0][2]) - 180.0}]}
    scene_json = {"table": {"id": a.id, "name": name, "name_vi": name, "camera": cam or {}}, "render": render,
                  "lightmaps": [], "materials": [], "nodes": [], "marks": {"points": points, "areas": {}, "nav": {"verts": verts, "tris": tris}},
                  "own": True}
    with io.open(os.path.join(out, "scene.json"), "w", encoding="utf-8") as f:
        json.dump(scene_json, f, ensure_ascii=False)
    camera = cam or {"dist": 19.0, "dist_min": 10.0, "dist_max": 21.0, "yaw": 0.0, "pitch": 40.0, "pitch_min": 40.0, "pitch_max": 80.0}
    map3d = {"id": a.id, "name": name, "scene": base_name, "scene_json": "scene.json", "unit": UNIT, "origin": [0.0, 0.0], "scale": 1.0,
             "camera": camera, "ground_y": float(allv[:, 1].min()), "models": "models.json", "own": True}
    with io.open(os.path.join(out, "map3d.json"), "w", encoding="utf-8") as f:
        json.dump(map3d, f, ensure_ascii=False, indent=1)
    if not os.path.exists(os.path.join(out, "models.json")):
        # model 3D cho NPC: cua bo tham khao neu template co trong bang (CHA_TO_TEMPLATE), khong thi client ve khoi danh dau
        tpl_to_cha = {str(tid): cha for cha, (tid, _k, _l) in CHA_TO_TEMPLATE.items()}
        models = {str(tid): tpl_to_cha[str(tid)] for tid, _t, _p, _y, _n in npcs if str(tid) in tpl_to_cha}
        with io.open(os.path.join(out, "models.json"), "w", encoding="utf-8") as f:
            json.dump({"templates": models, "player": PLAYER_MODELS, "horses": HORSES, "models_dir": "../../npc"}, f, ensure_ascii=False, indent=1)
    print("->", out, "(spawn %s, %d npc, %d o bay)" % (spawn_scene, len(placements), len(traps)))


if __name__ == "__main__":
    main()
