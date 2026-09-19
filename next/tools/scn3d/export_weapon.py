# -*- coding: utf-8 -*-
"""Xuat vu khi / vat treo (model_hang_list -> smodels.bdd) ra glTF tinh + weapons.json, va diem treo (HangItemMgr) cua xuong.

    python tools/scn3d/export_weapon.py            # tat ca vu khi (74) -> client/assets3d/weapon/
    python tools/scn3d/export_weapon.py --ids 1 12 # vai id

Du lieu:
  excel model_hang_list: id, ten, mh_def_hang_ids (hinge id: 10 daojian, 11 qianggun, 12/13 song dao-chuy phai/trai, 14/15 quyen),
                         res (Assets/StaticModels/weapons/<res>.prefab), mh_type (1 kiem 2 dao 3 thuong 4 con 5 song dao 6 song chuy
                         7 quyen 8 phi tieu 9 phi dao 10 no 20 ao choang), mh_xweapon_sfxobj, mh_animgrp (anim_group), mh_attach_skill
  smodels.bdd: prefab con nguyen tham chieu: GameObject(Transform, MeshFilter->Mesh, MeshRenderer->Material->Texture2D), con start/end
               (diem ve vet dao), huiguang (mesh vien sang, tao luc chay).
  HangItemMgr (tren goc xuong): name (=hinge name), path (duong dan xuong), position/rotation(euler)/scale lech, boneIsHang, defaultShow
"""
import argparse, io, json, math, os, struct, sys

import numpy as np
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from export_scene import Raw, h12, bundle_file, quat_to_mat, FLIP  # noqa: E402
from export_npc import load_bundle, Tables, euler_to_quat, load_ten_viet, hang_items, hang_godot  # noqa: E402

HINGE = {10: "daojian", 11: "qianggun", 12: "ssdaochui_r", 13: "ssdaochui_l", 14: "ssqt_r", 15: "ssqt_l", 2: "jian_xuanren",
         30: "npc_wq_r", 31: "npc_wq_l", 50: "ma_qi1", 51: "ma_qi2"}
TYPE_VI = {1: "Kiếm", 2: "Đao", 3: "Thương", 4: "Côn", 5: "Song đao", 6: "Song chùy", 7: "Quyền", 8: "Phi tiêu", 9: "Phi đao", 10: "Nỏ", 20: "Áo choàng"}


class WeaponExporter:
    def __init__(self, src, key, out):
        UnityPy.set_assetbundle_decrypt_key(key)
        self.src = src; self.out = out
        os.makedirs(os.path.join(out, "tex"), exist_ok=True)
        self.sm = load_bundle(src, "smodels")
        self.tables = Tables(src)
        self.texcache = {}
        self.log = []
        self.ten_viet = load_ten_viet()

    def texture(self, pid):
        env, cont, objs = self.sm
        if pid in self.texcache:
            return self.texcache[pid]
        o = objs.get(pid)
        fn = None
        if o is not None and o.type.name == "Texture2D":
            t = o.read(); fn = "tex/%s.png" % t.m_Name
            try:
                t.image.save(os.path.join(self.out, fn))
            except Exception as e:
                self.log.append("texture loi %s: %s" % (t.m_Name, e)); fn = None
        self.texcache[pid] = fn
        return fn

    def export(self, row):
        """row: dict(id, name, hang_ids, res, type, sfx, animgrp, skill)"""
        env, cont, objs = self.sm
        res = row["res"]
        base = "assets/staticmodels/cloak/" if row["type"] == 20 else "assets/staticmodels/weapons/"
        pid = None
        for cnd in (base + res + ".prefab", "assets/staticmodels/weapons/" + res + ".prefab", res + ".prefab"):
            pid = cont.get(h12(cnd))
            if pid is not None:
                break
        if pid is None or objs[pid].type.name != "GameObject":
            self.log.append("khong co prefab: %s (%s)" % (res, row["name"])); return None
        buf = bytearray(); views = []; accessors = []
        def add_view(arr, target=None):
            arr = np.ascontiguousarray(arr); off = len(buf); buf.extend(arr.tobytes())
            while len(buf) % 4:
                buf.append(0)
            v = {"buffer": 0, "byteOffset": off, "byteLength": arr.nbytes}
            if target:
                v["target"] = target
            views.append(v); return len(views) - 1
        def add_accessor(arr, ctype, comp, target=None, minmax=False):
            acc = {"bufferView": add_view(arr, target), "componentType": ctype, "count": int(arr.shape[0]), "type": comp}
            if minmax:
                acc["min"] = [float(x) for x in arr.min(axis=0)]; acc["max"] = [float(x) for x in arr.max(axis=0)]
            accessors.append(acc); return len(accessors) - 1
        gnodes = [{"name": "wp_%d" % row["id"], "children": []}]
        meshes = []; materials = []; images = []; textures = []; img_index = {}; anchors = {}; mat_index = {}
        def tex_index(fn):
            if fn not in img_index:
                images.append({"uri": fn}); textures.append({"source": len(images) - 1, "sampler": 0}); img_index[fn] = len(textures) - 1
            return img_index[fn]
        def material(mpid):
            if mpid in mat_index:
                return mat_index[mpid]
            mo = objs.get(mpid)
            gm = {"name": "m%d" % len(materials), "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 0.6}, "doubleSided": True, "alphaMode": "OPAQUE"}
            if mo is not None and mo.type.name == "Material":
                m = mo.read(); gm["name"] = m.m_Name
                for k, t in m.m_SavedProperties.m_TexEnvs:
                    if k == "_MainTex" and t.m_Texture.path_id:
                        fn = self.texture(t.m_Texture.path_id)
                        if fn:
                            gm["pbrMetallicRoughness"]["baseColorTexture"] = {"index": tex_index(fn)}
                if "alpha" in m.m_Name.lower():
                    gm["alphaMode"] = "MASK"; gm["alphaCutoff"] = 0.5
            materials.append(gm); mat_index[mpid] = len(materials) - 1
            return mat_index[mpid]
        stats = {"meshes": 0, "tris": 0}

        def walk(go_pid, parent_M, depth):
            g = objs[go_pid].read()
            tr = None; mf = None; mr = None
            for c in g.m_Components:
                co = objs.get(c.path_id)
                if co is None:
                    continue
                if co.type.name == "Transform":
                    tr = co.read()
                elif co.type.name == "MeshFilter":
                    mf = co.read()
                elif co.type.name == "MeshRenderer":
                    mr = co.read()
            if tr is None:
                return
            p = tr.m_LocalPosition; q = tr.m_LocalRotation; s = tr.m_LocalScale
            L = np.eye(4); L[:3, :3] = quat_to_mat([q.x, q.y, q.z, q.w]) @ np.diag([s.x, s.y, s.z]); L[:3, 3] = [p.x, p.y, p.z]
            M = parent_M @ L
            if g.m_Name in ("start", "end"):
                anchors[g.m_Name] = [-float(M[0, 3]), float(M[1, 3]), float(M[2, 3])]
            if mf is not None and mr is not None and mf.m_Mesh.path_id in objs and objs[mf.m_Mesh.path_id].type.name == "Mesh":
                m = objs[mf.m_Mesh.path_id].read(); hd = MeshHandler(m); hd.process()
                nv = m.m_VertexData.m_VertexCount
                def arr(a, want):
                    if not a:
                        return None
                    x = np.array(a, dtype=np.float32).reshape(nv, -1)
                    if x.shape[1] < want:
                        x = np.concatenate([x, np.zeros((nv, want - x.shape[1]), dtype=np.float32)], axis=1)
                    return x[:, :want]
                pos = arr(hd.m_Vertices, 3); nor = arr(hd.m_Normals, 3); uv0 = arr(hd.m_UV0, 2)
                # bake transform (toa do Unity) roi dao truc X
                Ph = np.concatenate([pos, np.ones((nv, 1), dtype=np.float32)], axis=1) @ M.T.astype(np.float32)
                P = Ph[:, :3].copy(); P[:, 0] = -P[:, 0]
                attrs = {"POSITION": add_accessor(P.astype(np.float32), 5126, "VEC3", 34962, True)}
                if nor is not None:
                    N = nor @ np.linalg.inv(M[:3, :3]).T.astype(np.float32); N = N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-6); N[:, 0] = -N[:, 0]
                    attrs["NORMAL"] = add_accessor(N.astype(np.float32), 5126, "VEC3", 34962)
                if uv0 is not None:
                    uv = uv0.copy(); uv[:, 1] = 1.0 - uv[:, 1]
                    attrs["TEXCOORD_0"] = add_accessor(uv.astype(np.float32), 5126, "VEC2", 34962)
                prims = []
                mats = [mm.path_id for mm in mr.m_Materials]
                for si, tri in enumerate(hd.get_triangles()):
                    t = np.array(tri, dtype=np.uint32).reshape(-1, 3)
                    if len(t) == 0:
                        continue
                    t = t[:, [0, 2, 1]]
                    prim = {"attributes": attrs, "indices": add_accessor(t.reshape(-1), 5125, "SCALAR", 34963), "mode": 4}
                    mp = mats[si] if si < len(mats) else (mats[0] if mats else None)
                    if mp is not None:
                        prim["material"] = material(mp)
                    prims.append(prim); stats["tris"] += len(t)
                meshes.append({"name": g.m_Name, "primitives": prims})
                gnodes.append({"name": g.m_Name, "mesh": len(meshes) - 1}); gnodes[0]["children"].append(len(gnodes) - 1)
                stats["meshes"] += 1
            for ch in tr.m_Children:
                co = objs.get(ch.path_id)
                if co is not None and co.type.name == "Transform":
                    walk(co.read().m_GameObject.path_id, M, depth + 1)
        walk(pid, np.eye(4), 0)
        if stats["meshes"] == 0:
            self.log.append("prefab khong co mesh: %s" % res); return None
        fname = "wp_%d_%s" % (row["id"], os.path.basename(res))
        with open(os.path.join(self.out, fname + ".bin"), "wb") as f:
            f.write(bytes(buf))
        gltf = {"asset": {"version": "2.0", "generator": "jxnext scn3d export_weapon"}, "scene": 0, "scenes": [{"nodes": [0]}],
                "nodes": gnodes, "meshes": meshes, "materials": materials, "images": images, "textures": textures,
                "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
                "accessors": accessors, "bufferViews": views, "buffers": [{"uri": fname + ".bin", "byteLength": len(buf)}]}
        with io.open(os.path.join(self.out, fname + ".gltf"), "w", encoding="utf-8") as f:
            json.dump(gltf, f)
        info = dict(row); info.update({"file": fname + ".gltf", "anchors": anchors, "type_vi": TYPE_VI.get(row["type"], ""),
                                       "hangs": [HINGE.get(h, str(h)) for h in row["hang_ids"]], "stats": stats,
                                       "name_vi": self.ten_viet.get("weapon", {}).get(row["name"], "")})
        return info


def read_rows(tables):
    rows = []
    for r in tables.t.get("model_hang_list", [])[1:]:
        if len(r) < 5 or not r[0].strip().isdigit():
            continue
        hang = [int(x) for x in r[2].replace("*", " ").split() if x.strip().isdigit()]
        rows.append({"id": int(r[0]), "name": r[1].strip(), "hang_ids": hang, "res": r[3].strip(), "type": int(r[4]) if r[4].strip().lstrip("-").isdigit() else 0,
                     "sfx": int(r[5]) if len(r) > 5 and r[5].strip().isdigit() else 0, "animgrp": int(r[6]) if len(r) > 6 and r[6].strip().isdigit() else 0,
                     "skill": int(r[7]) if len(r) > 7 and r[7].strip().isdigit() else 0})
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ids", nargs="*", type=int, default=[])
    ap.add_argument("--src", default=os.environ.get("JX_SCN3D_SRC", r"D:\game3gTQ_mo\pc\剑网江湖_Data\StreamingAssets"))
    ap.add_argument("--key-file", default=os.environ.get("JX_SCN3D_KEY", r"D:\game3gTQ_mo\khoa_bundle.txt"))
    ap.add_argument("--out", default=os.path.join(NEXT, "client", "assets3d", "weapon"))
    a = ap.parse_args()
    key = open(a.key_file, "r", encoding="utf-8").read().strip()
    ex = WeaponExporter(a.src, key, a.out)
    rows = read_rows(ex.tables)
    if a.ids:
        rows = [r for r in rows if r["id"] in a.ids]
    out = {}
    for r in rows:
        if not r["res"] or r["res"].startswith("Assets/Particles"):
            continue
        info = ex.export(r)
        if info:
            out[str(r["id"])] = info
            print("  wp %d %s (%s) [%s] mesh %d tri %d hang %s" % (r["id"], r["name"], info["type_vi"], r["res"], info["stats"]["meshes"], info["stats"]["tris"], info["hangs"]))
    with io.open(os.path.join(a.out, "weapons.json"), "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False, indent=1)
    with io.open(os.path.join(a.out, "export.log"), "w", encoding="utf-8") as f:
        f.write("\n".join(ex.log))
    print("xuat %d vu khi, %d canh bao -> %s" % (len(out), len(ex.log), a.out))


if __name__ == "__main__":
    main()
