# -*- coding: utf-8 -*-
"""Sinh map mau theo docs/3D-HOA-SI.md (mot thon nho, glTF 2.0 viet tay bang numpy - khong can Blender) de:
  1) hoa si mo ra xem dung quy uong ten node / don vi / huong;  2) kiem bo nhap import_map3d.py tu dau den cuoi.

    python tools/scn3d/make_sample_map.py            -> docs/3d-sample/sample_map.gltf (+ .bin, .map3d.json)
    python tools/scn3d/import_map3d.py docs/3d-sample/sample_map.gltf --id 9001

Thon 64 x 48 m: terrain (mat dat co dinh, hai buoc cao), walk (duong + san), hai nha (building), cay (tree), ho nuoc (water),
spawn, hai NPC JX1 (npc_197_1 tho ren, npc_43_1 heo trang), cong ve Phuong Tuong (exit_1_1551_3150_1), nang.
"""
import io
import json
import math
import os
import struct

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
OUT = os.path.join(NEXT, "docs", "3d-sample")


class Builder:
    def __init__(self):
        self.bin = bytearray()
        self.views = []; self.accessors = []; self.meshes = []; self.nodes = []; self.materials = []
        self.mat_index = {}

    def material(self, name, rgba, alpha_mode="OPAQUE"):
        if name in self.mat_index:
            return self.mat_index[name]
        m = {"name": name, "pbrMetallicRoughness": {"baseColorFactor": list(rgba), "metallicFactor": 0.0, "roughnessFactor": 0.9},
             "doubleSided": alpha_mode != "OPAQUE"}
        if alpha_mode != "OPAQUE":
            m["alphaMode"] = alpha_mode
        self.materials.append(m)
        self.mat_index[name] = len(self.materials) - 1
        return self.mat_index[name]

    def _acc(self, arr, ctype, atype, target, minmax=False):
        data = np.ascontiguousarray(arr).tobytes()
        while len(self.bin) % 4:
            self.bin.append(0)
        off = len(self.bin); self.bin += data
        self.views.append({"buffer": 0, "byteOffset": off, "byteLength": len(data), "target": target})
        acc = {"bufferView": len(self.views) - 1, "componentType": ctype, "count": len(arr), "type": atype}
        if minmax:
            acc["min"] = np.min(arr, axis=0).tolist(); acc["max"] = np.max(arr, axis=0).tolist()
        self.accessors.append(acc)
        return len(self.accessors) - 1

    def mesh(self, name, verts, tris, normals, uvs, mat):
        p = {"attributes": {"POSITION": self._acc(np.array(verts, np.float32), 5126, "VEC3", 34962, True),
                            "NORMAL": self._acc(np.array(normals, np.float32), 5126, "VEC3", 34962),
                            "TEXCOORD_0": self._acc(np.array(uvs, np.float32), 5126, "VEC2", 34962)},
             "indices": self._acc(np.array(tris, np.uint32).reshape(-1), 5125, "SCALAR", 34963), "material": mat}
        self.meshes.append({"name": name, "primitives": [p]})
        return len(self.meshes) - 1

    def node(self, name, mesh=None, translation=(0, 0, 0), rotation=None, scale=None, extras=None):
        n = {"name": name, "translation": list(translation)}
        if mesh is not None:
            n["mesh"] = mesh
        if rotation is not None:
            n["rotation"] = list(rotation)
        if scale is not None:
            n["scale"] = list(scale)
        if extras:
            n["extras"] = extras
        self.nodes.append(n)
        return len(self.nodes) - 1

    def write(self, path):
        g = {"asset": {"version": "2.0", "generator": "jxnext make_sample_map"}, "scene": 0, "scenes": [{"nodes": list(range(len(self.nodes)))}],
             "nodes": self.nodes, "meshes": self.meshes, "materials": self.materials, "accessors": self.accessors, "bufferViews": self.views,
             "buffers": [{"byteLength": len(self.bin), "uri": os.path.basename(path)[:-5] + ".bin"}]}
        with io.open(path, "w", encoding="utf-8") as f:
            json.dump(g, f, indent=1)
        with open(path[:-5] + ".bin", "wb") as f:
            f.write(bytes(self.bin))


def grid_mesh(w, d, nx, nz, height_fn):
    verts, norms, uvs = [], [], []
    for j in range(nz + 1):
        for i in range(nx + 1):
            x = w * i / nx; z = d * j / nz
            verts.append([x, height_fn(x, z), z]); norms.append([0, 1, 0]); uvs.append([i / nx * 8, j / nz * 6])
    tris = []
    for j in range(nz):
        for i in range(nx):
            a = j * (nx + 1) + i; b = a + 1; c = a + nx + 1; d2 = c + 1
            tris += [[a, c, b], [b, c, d2]]
    return verts, tris, norms, uvs


def box(w, h, d):
    """hop goc tai chan (y 0..h), tam x/z"""
    x0, x1, y0, y1, z0, z1 = -w / 2, w / 2, 0.0, h, -d / 2, d / 2
    faces = [((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0, 1)), ((x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (0, 0, -1)),
             ((x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (1, 0, 0)), ((x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (-1, 0, 0)),
             ((x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0), (0, 1, 0))]
    verts, tris, norms, uvs = [], [], [], []
    for f in faces:
        b = len(verts)
        for k in range(4):
            verts.append(list(f[k])); norms.append(list(f[4])); uvs.append([[0, 0], [1, 0], [1, 1], [0, 1]][k])
        tris += [[b, b + 1, b + 2], [b, b + 2, b + 3]]
    return verts, tris, norms, uvs


def quad(w, d, y=0.0, uv_scale=1.0):
    verts = [[-w / 2, y, -d / 2], [w / 2, y, -d / 2], [w / 2, y, d / 2], [-w / 2, y, d / 2]]
    return verts, [[0, 2, 1], [0, 3, 2]], [[0, 1, 0]] * 4, [[0, 0], [uv_scale, 0], [uv_scale, uv_scale], [0, uv_scale]]


def main():
    os.makedirs(OUT, exist_ok=True)
    b = Builder()
    W, D = 64.0, 48.0
    ground = b.material("ground", (0.36, 0.42, 0.24, 1))
    road = b.material("road", (0.55, 0.5, 0.42, 1))
    wall = b.material("wall", (0.8, 0.76, 0.66, 1))
    roof = b.material("roof", (0.45, 0.22, 0.15, 1))
    leaf = b.material("leaf", (0.2, 0.5, 0.18, 1), "MASK")
    trunk = b.material("trunk", (0.35, 0.25, 0.15, 1))
    water = b.material("water", (0.2, 0.45, 0.6, 0.7), "BLEND")
    # dat: hai buoc cao (0 m tren, 1 m duoi tu z = 30)
    def h(x, z):
        return 0.0 if z < 30 else min(1.0, (z - 30) / 4.0)
    v, t, n, uv = grid_mesh(W, D, 32, 24, h)
    b.node("terrain_main", b.mesh("terrain_main", v, t, n, uv, ground))
    # duong di duoc: mot dai duong bac-nam + san giua (mesh walk, phang tren mat dat)
    v, t, n, uv = quad(8.0, 44.0, 0.05)
    b.node("walk_road", b.mesh("walk_road", v, t, n, uv, road), translation=(32, 0, 24))
    v, t, n, uv = quad(28.0, 16.0, 0.05)
    b.node("walk_square", b.mesh("walk_square", v, t, n, uv, road), translation=(32, 0, 16))
    v, t, n, uv = quad(28.0, 10.0, 1.05)
    b.node("walk_lower", b.mesh("walk_lower", v, t, n, uv, road), translation=(32, 0, 40))
    # nha: khoi tuong + mai
    for k, (x, z, w, d) in enumerate([(14, 12, 8, 6), (50, 12, 8, 6)]):
        v, t, n, uv = box(w, 3.2, d)
        b.node("building_house_%d" % (k + 1), b.mesh("house_%d" % (k + 1), v, t, n, uv, wall), translation=(x, 0, z))
        v, t, n, uv = box(w + 1.0, 1.2, d + 1.0)
        b.node("building_roof_%d" % (k + 1), b.mesh("roof_%d" % (k + 1), v, t, n, uv, roof), translation=(x, 3.2, z))
    # cay: than + tan
    for k, (x, z) in enumerate([(8, 30), (56, 30), (10, 42), (54, 44)]):
        v, t, n, uv = box(0.5, 3.0, 0.5)
        b.node("tree_trunk_%d" % (k + 1), b.mesh("trunk_%d" % (k + 1), v, t, n, uv, trunk), translation=(x, h(x, z), z))
        v, t, n, uv = box(4.0, 3.0, 4.0)
        b.node("tree_crown_%d" % (k + 1), b.mesh("crown_%d" % (k + 1), v, t, n, uv, leaf), translation=(x, h(x, z) + 3.0, z))
    # ho nuoc
    v, t, n, uv = quad(10.0, 8.0, 0.02, 4.0)
    b.node("water_pond", b.mesh("pond", v, t, n, uv, water), translation=(14, 0, 32))
    # diem sinh, NPC, cong, nang, camera
    b.node("spawn", translation=(32, 0, 20))
    b.node("npc_197_1", translation=(20, 0, 16), rotation=(0, math.sin(math.radians(45)), 0, math.cos(math.radians(45))))   # tho ren, nhin dong-bac
    b.node("npc_43_1", translation=(44, 0, 18))     # heo trang
    b.node("exit_1_1551_3150_1", translation=(32, 0, 45), scale=(6, 1, 2))   # ve diem sinh Phuong Tuong
    b.node("light_sun", rotation=(-0.3, 0.2, 0.0, 0.93), extras={"color": [1.0, 0.98, 0.9]})
    b.node("cam", extras={"dist": 19.0, "dist_min": 10.0, "dist_max": 21.0, "yaw": 0.0, "pitch": 40.0, "pitch_min": 40.0, "pitch_max": 80.0})
    path = os.path.join(OUT, "sample_map.gltf")
    b.write(path)
    with io.open(os.path.join(OUT, "sample_map.map3d.json"), "w", encoding="utf-8") as f:
        json.dump({"id": 9001, "name": "Thôn thử 3D", "render": {"ambient_light": [0.55, 0.55, 0.6], "fog": True, "fog_mode": 1,
                   "fog_start": 60.0, "fog_end": 220.0, "fog_color": [0.75, 0.82, 0.9]}}, f, ensure_ascii=False, indent=1)
    print("->", path, "(%d node, %d mesh)" % (len(b.nodes), len(b.meshes)))


if __name__ == "__main__":
    main()
