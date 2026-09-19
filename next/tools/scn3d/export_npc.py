# -*- coding: utf-8 -*-
"""Xuat nhan vat/NPC 3D cua bo tham khao (剑网江湖) ra glTF co skin + animation, va bang dat NPC cho mot map.

    python tools/scn3d/export_npc.py --map world_baling        # NPC + quai o Ba Lang + nhan vat chinh
    python tools/scn3d/export_npc.py --cha 1 25 1004           # chi vai cha_pic id

Chuoi du lieu (mo o D:/game3gTQ_mo/BAO-CAO-MAP-3D.md muc 7 + phien 2026-09-19):
  excel.bdd  cha_pic_cmn: id, ten, xuong (bone), skins "body*head*shoes", model id, scale, defAnimGroup
             anim_group: xx (nghi), zp (di), gjxx, ... -> animation_list.name -> clip
  exprolesbone  assets/art/rolesmakeres/bone/<bone>.prefab                : cay xuong (GameObject/Transform)
  exprolesskin  assets/art/rolesmakeres/skinpart/<bone>/<part>.asset      : ChaResourceRef{meshUrl, matUrl, clothUrl, bones[]}
  exprolesmesh  assets/art/rolesmakeres/mesh/<meshUrl>                    : Mesh da (trong so xuong kenh 12/13, m_BindPose)
  exprolesmat   assets/art/rolesmakeres/mat/<matUrl>                      : MaterialRef -> Material + texture path
  models        md5(lower(texture path))                                   : Texture2D
  exprolesanim  assets/art/rolesmakeres/animation/<bone>/<clip>.asset     : AnimationClip legacy (duong cong quaternion/pos/scale theo path xuong)
Toa do Unity -> glTF: dao truc X (vi tri -x, quaternion (x,-y,-z,w), ma tran F*M*F), dao chieu tam giac, lat V cua UV.
"""
import argparse, hashlib, io, json, math, os, struct, sys, collections

import numpy as np
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from export_scene import Raw, h12, bundle_file, quat_to_mat, trs, FLIP  # noqa: E402

# diem dung tren map (mark JSON) -> cha_pic id. Bang refresh cua server khong co trong client, nen ghep theo ten.
MARK_TO_CHA = {
    "world_baling": {
        "n_tiejiang": 1004, "n_tiejiang2": 1035, "n_yaodian": 1001, "n_yizhan": 1000, "n_zahuo": 1002, "n_chuwuxiang": 1003,
        "n_tianwangjl": 1020, "n_wudangdr": 1022, "n_emei": 1021, "n_tianren": 1024, "n_tangmen": 1028, "n_wudu": 1027,
        "n_kunlun": 1023, "n_shaolin": 1029, "n_gaibang": 1025, "n_cuiyan": 1026, "n_yesou": 1010, "n_liguan": 1030,
        "n_jueji": 1011, "n_chenghao": 1031, "n_shangcheng": 1032, "n_jiaoyi": 1033, "n_paihang": 1034, "n_huodong": 1036,
        "n_smshangren": 1038, "n_longwu": 1039,
        "baizhu": 25, "meihualu": 26, "jinmao": 28, "小黄金BOSS": 31,
    },
}
PLAYER_CHA = 1


def load_bundle(src, name):
    env = UnityPy.load(bundle_file(src, name))
    cont = {}
    for o in env.objects:
        if o.type.name == "AssetBundle":
            for k, v in o.read_typetree()["m_Container"]:
                cont[k] = v["asset"]["m_PathID"]
    return env, cont, {o.path_id: o for o in env.objects}


def textasset(o):
    d = o.read()
    raw = d.m_Script
    if isinstance(raw, str):
        raw = raw.encode("utf-8", "surrogateescape")
    return raw


class Tables:
    def __init__(self, src):
        env = UnityPy.load(bundle_file(src, "excel"))
        self.t = {}
        for o in env.objects:
            if o.type.name == "TextAsset":
                d = o.read()
                raw = textasset(o)
                if raw[:2] == b"\xff\xfe":
                    rows = [l.split("\t") for l in raw.decode("utf-16").replace("\r\n", "\n").split("\n")]
                    # bang du lieu co the trung ten voi schema (cha_pic) hoac mang hau to _cmn (scn_list_cmn)
                    self.t[d.m_Name] = rows
                    if d.m_Name.endswith("_cmn"):
                        self.t.setdefault(d.m_Name[:-4], rows)
                    else:
                        self.t.setdefault(d.m_Name + "_cmn", rows)
        self.cha_pic = {}
        for r in self.t.get("cha_pic_cmn", [])[1:]:
            if len(r) > 14 and r[0].strip().isdigit():
                self.cha_pic[int(r[0])] = {"id": int(r[0]), "name": r[1].strip(), "bone": r[3].strip(), "sex": r[4].strip(),
                                           "scale": float(r[6]) if r[6].strip() else 1.0, "anim_group": int(r[9]) if r[9].strip() else -1,
                                           "skins": r[12].strip(), "model": r[13].strip(), "sizeY": None}
        self.anim_group = {}
        for r in self.t.get("anim_group_cmn", [])[1:]:
            if len(r) > 10 and r[0].strip().isdigit():
                self.anim_group[int(r[0])] = {"xx": r[2].strip(), "zp": r[3].strip(), "gjxx": r[4].strip(), "gjpb": r[5].strip(),
                                              "xdz": r[6].strip(), "ss": r[7].strip(), "sw": r[8].strip(), "weapon": r[9].strip(), "magic": r[10].strip()}
        self.animation_list = {}
        for r in self.t.get("animation_list_cmn", [])[1:]:
            if len(r) > 1 and r[0].strip().isdigit():
                self.animation_list[int(r[0])] = r[1].strip()
        self.model_list = {}
        for r in self.t.get("model_list_cmn", [])[1:]:
            if len(r) > 2 and r[0].strip().isdigit():
                self.model_list[int(r[0])] = {"skins": r[2].strip(), "hangs": r[3].strip() if len(r) > 3 else ""}
        self.model_view = {}
        for r in self.t.get("cha_model_view_cmn", [])[1:]:
            if len(r) > 2 and r[0].strip().isdigit():
                try:
                    self.model_view[int(r[0])] = float(r[2])
                except ValueError:
                    pass

    def anim_names(self, group):
        """-> {'xx': 'xx01', 'zp': 'zp01', ...} theo anim_group (id -> animation_list)"""
        g = self.anim_group.get(group)
        if not g:
            g = self.anim_group.get(1, {})
        out = {}
        for key in ("xx", "zp", "gjxx", "gjpb"):
            v = g.get(key, "")
            if v.isdigit() and int(v) in self.animation_list:
                out[key] = self.animation_list[int(v)]
        for key in ("xdz", "ss", "sw"):
            v = g.get(key, "")
            first = v.split("*")[0] if v else ""
            if first.isdigit() and int(first) in self.animation_list:
                out[key] = self.animation_list[int(first)]
        return out


class NpcExporter:
    def __init__(self, src, key, out):
        UnityPy.set_assetbundle_decrypt_key(key)
        self.src = src
        self.out = out
        os.makedirs(os.path.join(out, "tex"), exist_ok=True)
        self.tables = Tables(src)
        self.bone = load_bundle(src, "exprolesbone")
        self.skin = load_bundle(src, "exprolesskin")
        self.mesh = load_bundle(src, "exprolesmesh")
        self.mat = load_bundle(src, "exprolesmat")
        self.tex = load_bundle(src, "models")
        self.anim = load_bundle(src, "exprolesanim")
        self.texcache = {}
        self.log = []
        self.matref = {}
        for o in self.mat[0].objects:
            if o.type.name == "MonoBehaviour":
                r = Raw(o.get_raw_data()); r.header()
                mat = r.pptr()[1]; cnt = r.i32()
                self.matref[o.path_id] = (mat, {r.string(): r.string() for _ in range(cnt)})

    # ---------- xuong ----------
    def skeleton(self, bone):
        env, cont, objs = self.bone
        pid = cont.get(h12("assets/art/rolesmakeres/bone/%s.prefab" % bone))
        if pid is None:
            return None
        root_go = objs[pid].read()
        root_tr = None
        for c in root_go.m_Components:
            co = objs.get(c.path_id)
            if co is not None and co.type.name == "Transform":
                root_tr = co.read()
        nodes = []      # glTF nodes (Unity-space TRS, doi sau)
        by_name = {}
        by_path = {}

        def walk(tr, parent_idx, path):
            go = tr.m_GameObject.read()
            p = tr.m_LocalPosition; q = tr.m_LocalRotation; s = tr.m_LocalScale
            idx = len(nodes)
            nodes.append({"name": go.m_Name, "t": [p.x, p.y, p.z], "q": [q.x, q.y, q.z, q.w], "s": [s.x, s.y, s.z], "children": [], "parent": parent_idx})
            if parent_idx is not None:
                nodes[parent_idx]["children"].append(idx)
            by_name.setdefault(go.m_Name, idx)
            by_path[path] = idx
            for ch in tr.m_Children:
                co = objs.get(ch.path_id)
                if co is not None and co.type.name == "Transform":
                    cht = co.read()
                    chname = cht.m_GameObject.read().m_Name
                    walk(cht, idx, (path + "/" + chname) if path else chname)
        walk(root_tr, None, "")
        return {"root_name": root_go.m_Name, "nodes": nodes, "by_name": by_name, "by_path": by_path}

    # ---------- da ----------
    def skin_part(self, bone, part):
        env, cont, objs = self.skin
        pid = cont.get(h12("assets/art/rolesmakeres/skinpart/%s/%s.asset" % (bone, part)))
        if pid is None:
            return None
        r = Raw(objs[pid].get_raw_data()); r.header()
        meshUrl = r.string(); matUrl = r.string(); clothUrl = r.string()
        nb = r.i32(); bones = [r.string() for _ in range(nb)]
        return {"part": part, "meshUrl": meshUrl, "matUrl": matUrl, "bones": bones}

    def skinned_mesh(self, meshUrl):
        env, cont, objs = self.mesh
        pid = cont.get(h12("assets/art/rolesmakeres/mesh/" + meshUrl))
        if pid is None:
            return None
        m = objs[pid].read()
        hd = MeshHandler(m); hd.process()
        nv = m.m_VertexData.m_VertexCount

        def arr(a, want, dtype=np.float32):
            if not a:
                return None
            x = np.array(a, dtype=dtype).reshape(nv, -1)
            if x.shape[1] < want:   # kenh trong so/chi so xuong co the chi 1-2 cot -> dem du 4
                pad = np.zeros((nv, want - x.shape[1]), dtype=dtype)
                x = np.concatenate([x, pad], axis=1)
            return x[:, :want]
        pos = arr(hd.m_Vertices, 3); nor = arr(hd.m_Normals, 3); uv0 = arr(hd.m_UV0, 2)
        bw = arr(getattr(hd, "m_BoneWeights", None), 4)
        bi = arr(getattr(hd, "m_BoneIndices", None), 4, np.int64)
        if bw is None and m.m_Skin:
            bw = np.array([[w.weight_0_, w.weight_1_, w.weight_2_, w.weight_3_] for w in m.m_Skin], dtype=np.float32)
            bi = np.array([[w.boneIndex_0_, w.boneIndex_1_, w.boneIndex_2_, w.boneIndex_3_] for w in m.m_Skin], dtype=np.int64)
        subs = []
        for tri, sm in zip(hd.get_triangles(), m.m_SubMeshes):
            a = np.array(tri, dtype=np.uint32).reshape(-1, 3)
            base = getattr(sm, "baseVertex", 0) or 0
            if base:
                a = a + base
            subs.append(a)
        bind = []
        for bp in m.m_BindPose:
            M = np.array([[getattr(bp, "e%d%d" % (i, j)) for j in range(4)] for i in range(4)], dtype=np.float64)
            bind.append(M)
        return {"name": m.m_Name, "pos": pos, "nor": nor, "uv0": uv0, "bw": bw, "bi": bi, "subs": subs, "bind": bind}

    def material(self, matUrl):
        env, cont, objs = self.mat
        pid = cont.get(h12("assets/art/rolesmakeres/mat/" + matUrl))
        if pid is None:
            return None
        o = objs.get(pid)
        texs = {}
        if o is not None and o.type.name == "MonoBehaviour":
            mat_pid, texs = self.matref.get(pid, (0, {}))
            o = objs.get(mat_pid)
        if o is None or o.type.name != "Material":
            return None
        m = o.read()
        floats = {k: v for k, v in m.m_SavedProperties.m_Floats}
        colors = {k: (c.r, c.g, c.b, c.a) for k, c in m.m_SavedProperties.m_Colors}
        return {"name": m.m_Name, "tex": texs, "floats": floats, "colors": colors}

    def texture(self, path):
        key = h12(path)
        if key in self.texcache:
            return self.texcache[key]
        fn = None
        for env, cont, objs in (self.tex, self.mat, self.skin):
            pid = cont.get(key)
            if pid is not None and pid in objs and objs[pid].type.name == "Texture2D":
                t = objs[pid].read()
                fn = "tex/%s.png" % key
                try:
                    t.image.save(os.path.join(self.out, fn))
                except Exception as e:
                    self.log.append("texture loi %s: %s" % (path, e)); fn = None
                break
        if fn is None:
            self.log.append("texture thieu: " + path)
        self.texcache[key] = fn
        return fn

    # ---------- animation ----------
    def clip(self, bone, name):
        env, cont, objs = self.anim
        pid = cont.get(h12("assets/art/rolesmakeres/animation/%s/%s.asset" % (bone, name)))
        if pid is None:
            return None
        d = objs[pid].read_typetree()
        out = {"name": d.get("m_Name", name), "rot": {}, "pos": {}, "scl": {}, "rate": d.get("m_SampleRate", 30.0)}
        for c in d.get("m_RotationCurves", []):
            keys = [(k["time"], (k["value"]["x"], k["value"]["y"], k["value"]["z"], k["value"]["w"])) for k in c["curve"]["m_Curve"]]
            out["rot"][c["path"]] = keys
        for c in d.get("m_PositionCurves", []):
            out["pos"][c["path"]] = [(k["time"], (k["value"]["x"], k["value"]["y"], k["value"]["z"])) for k in c["curve"]["m_Curve"]]
        for c in d.get("m_ScaleCurves", []):
            out["scl"][c["path"]] = [(k["time"], (k["value"]["x"], k["value"]["y"], k["value"]["z"])) for k in c["curve"]["m_Curve"]]
        # euler curves (neu co) -> quaternion
        for c in d.get("m_EulerCurves", []):
            keys = []
            for k in c["curve"]["m_Curve"]:
                ex, ey, ez = k["value"]["x"], k["value"]["y"], k["value"]["z"]
                keys.append((k["time"], euler_to_quat(ex, ey, ez)))
            out["rot"].setdefault(c["path"], keys)
        return out

    # ---------- glTF ----------
    def export_cha(self, cha_id):
        cp = self.tables.cha_pic.get(cha_id)
        if not cp or not cp["bone"]:
            self.log.append("cha_pic %s khong co" % cha_id); return None
        bone = cp["bone"]
        sk = self.skeleton(bone)
        if sk is None:
            self.log.append("xuong thieu: %s" % bone); return None
        skins_str = cp["skins"]
        if cp["model"].isdigit() and int(cp["model"]) in self.tables.model_list and self.tables.model_list[int(cp["model"])]["skins"]:
            skins_str = self.tables.model_list[int(cp["model"])]["skins"]
        parts = [p for p in skins_str.split("*") if p]
        anim_names = self.tables.anim_names(cp["anim_group"])

        buf = bytearray(); views = []; accessors = []
        def add_view(arr, target=None):
            arr = np.ascontiguousarray(arr)
            off = len(buf); buf.extend(arr.tobytes())
            while len(buf) % 4:
                buf.append(0)
            v = {"buffer": 0, "byteOffset": off, "byteLength": arr.nbytes}
            if target:
                v["target"] = target
            views.append(v); return len(views) - 1
        def add_accessor(arr, ctype, comp, target=None, minmax=False, normalized=False):
            vi = add_view(arr, target)
            acc = {"bufferView": vi, "componentType": ctype, "count": int(arr.shape[0]), "type": comp}
            if minmax:
                acc["min"] = [float(x) for x in arr.min(axis=0)]; acc["max"] = [float(x) for x in arr.max(axis=0)]
            accessors.append(acc); return len(accessors) - 1

        # nodes: 0 = goc nhan vat, 1.. = xuong (theo thu tu duyet), sau do mesh nodes
        gnodes = [{"name": "cha_%d" % cha_id, "children": []}]
        joint_index = {}
        for i, n in enumerate(sk["nodes"]):
            q = n["q"]; t = n["t"]
            gn = {"name": n["name"], "translation": [-t[0], t[1], t[2]], "rotation": [q[0], -q[1], -q[2], q[3]], "scale": list(n["s"])}
            gnodes.append(gn)
            joint_index[i] = len(gnodes) - 1
        for i, n in enumerate(sk["nodes"]):
            gi = joint_index[i]
            gnodes[gi]["children"] = [joint_index[c] for c in n["children"]]
            if gnodes[gi]["children"] == []:
                del gnodes[gi]["children"]
        gnodes[0]["children"].append(joint_index[0])
        skeleton_root = joint_index[0]

        images = []; textures = []; materials = []; meshes = []; skins = []; img_index = {}
        def tex_index(fn):
            if fn not in img_index:
                images.append({"uri": fn.replace("\\", "/")}); textures.append({"source": len(images) - 1, "sampler": 0})
                img_index[fn] = len(textures) - 1
            return img_index[fn]
        stats = {"parts": 0, "verts": 0, "tris": 0, "anims": 0, "missing_bones": 0}
        for part in parts:
            sp = self.skin_part(bone, part)
            if sp is None:
                self.log.append("skin part thieu: %s/%s" % (bone, part)); continue
            md = self.skinned_mesh(sp["meshUrl"])
            if md is None:
                self.log.append("mesh thieu: %s" % sp["meshUrl"]); continue
            mat = self.material(sp["matUrl"])
            # material
            gm = {"name": "%s" % part, "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0}, "doubleSided": True, "alphaMode": "OPAQUE"}
            if mat:
                mt = mat["tex"].get("_MainTex")
                if mt:
                    fn = self.texture(mt)
                    if fn:
                        gm["pbrMetallicRoughness"]["baseColorTexture"] = {"index": tex_index(fn)}
                if "alpha" in part.lower() or "alpha" in (mat.get("name") or "").lower() or mat["floats"].get("_Cutoff", 0) > 0:
                    gm["alphaMode"] = "MASK"; gm["alphaCutoff"] = float(mat["floats"].get("_Cutoff", 0.5) or 0.5)
            materials.append(gm); mat_i = len(materials) - 1
            # skin joints theo thu tu bones[] cua ChaResourceRef
            joints = []; ibm = []
            for bi_, bname in enumerate(sp["bones"]):
                ni = sk["by_name"].get(bname)
                if ni is None:
                    stats["missing_bones"] += 1; self.log.append("xuong '%s' khong co trong %s" % (bname, bone)); ni = 0
                joints.append(joint_index[ni])
                M = md["bind"][bi_] if bi_ < len(md["bind"]) else np.eye(4)
                Mg = FLIP @ M @ FLIP
                ibm.append(Mg.T.reshape(-1))
            ibm_arr = np.array(ibm, dtype=np.float32)
            a_ibm = add_accessor(ibm_arr, 5126, "MAT4")
            skins.append({"joints": joints, "inverseBindMatrices": a_ibm, "skeleton": skeleton_root, "name": part})
            skin_i = len(skins) - 1
            # mesh
            pos = md["pos"].copy(); pos[:, 0] = -pos[:, 0]
            attrs = {"POSITION": add_accessor(pos.astype(np.float32), 5126, "VEC3", 34962, True)}
            if md["nor"] is not None:
                nor = md["nor"].copy(); nor[:, 0] = -nor[:, 0]
                attrs["NORMAL"] = add_accessor(nor.astype(np.float32), 5126, "VEC3", 34962)
            if md["uv0"] is not None:
                uv = md["uv0"].copy(); uv[:, 1] = 1.0 - uv[:, 1]
                attrs["TEXCOORD_0"] = add_accessor(uv.astype(np.float32), 5126, "VEC2", 34962)
            if md["bw"] is not None and md["bi"] is not None:
                w = md["bw"].astype(np.float32)
                s = w.sum(axis=1, keepdims=True); s[s == 0] = 1.0
                w = w / s
                attrs["WEIGHTS_0"] = add_accessor(w, 5126, "VEC4", 34962)
                ji = np.clip(md["bi"], 0, max(0, len(joints) - 1)).astype(np.uint16)
                attrs["JOINTS_0"] = add_accessor(ji, 5123, "VEC4", 34962)
            prims = []
            for tri in md["subs"]:
                if len(tri) == 0:
                    continue
                t = tri[:, [0, 2, 1]].astype(np.uint32)
                prims.append({"attributes": attrs, "indices": add_accessor(t.reshape(-1), 5125, "SCALAR", 34963), "material": mat_i, "mode": 4})
                stats["tris"] += len(tri)
            meshes.append({"name": md["name"], "primitives": prims})
            gnodes.append({"name": "mesh_" + part, "mesh": len(meshes) - 1, "skin": skin_i})
            gnodes[0]["children"].append(len(gnodes) - 1)
            stats["parts"] += 1; stats["verts"] += int(pos.shape[0])

        # animations
        animations = []
        for key, cname in anim_names.items():
            c = self.clip(bone, cname)
            if c is None:
                self.log.append("clip thieu: %s/%s" % (bone, cname)); continue
            samplers = []; channels = []
            def add_channel(path, prop, keys, conv):
                if not keys:
                    return
                ni = sk["by_path"].get(path)
                if ni is None:
                    # thu theo ten xuong cuoi cung
                    ni = sk["by_name"].get(path.split("/")[-1])
                if ni is None:
                    return
                keys = sorted(keys, key=lambda k: k[0])
                times = np.array([k[0] for k in keys], dtype=np.float32)
                vals = [conv(k[1]) for k in keys]
                if prop == "rotation":
                    # giu lien tuc dau quaternion
                    for i in range(1, len(vals)):
                        if np.dot(vals[i], vals[i - 1]) < 0:
                            vals[i] = [-x for x in vals[i]]
                arr = np.array(vals, dtype=np.float32)
                a_in = add_accessor(times.reshape(-1, 1), 5126, "SCALAR", None, True)
                accessors[a_in]["min"] = [float(times.min())]; accessors[a_in]["max"] = [float(times.max())]
                a_out = add_accessor(arr, 5126, "VEC4" if prop == "rotation" else "VEC3")
                samplers.append({"input": a_in, "output": a_out, "interpolation": "LINEAR"})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": joint_index[ni], "path": prop}})
            for path, keys in c["rot"].items():
                add_channel(path, "rotation", keys, lambda q: [q[0], -q[1], -q[2], q[3]])
            for path, keys in c["pos"].items():
                add_channel(path, "translation", keys, lambda v: [-v[0], v[1], v[2]])
            for path, keys in c["scl"].items():
                add_channel(path, "scale", keys, lambda v: [v[0], v[1], v[2]])
            if channels:
                animations.append({"name": key, "samplers": samplers, "channels": channels})
                stats["anims"] += 1

        fname = "cha_%d_%s" % (cha_id, bone)
        with open(os.path.join(self.out, fname + ".bin"), "wb") as f:
            f.write(bytes(buf))
        gltf = {"asset": {"version": "2.0", "generator": "jxnext scn3d export_npc"}, "scene": 0, "scenes": [{"nodes": [0]}],
                "nodes": gnodes, "meshes": meshes, "materials": materials, "skins": skins, "images": images, "textures": textures,
                "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
                "animations": animations, "accessors": accessors, "bufferViews": views,
                "buffers": [{"uri": fname + ".bin", "byteLength": len(buf)}]}
        with io.open(os.path.join(self.out, fname + ".gltf"), "w", encoding="utf-8") as f:
            json.dump(gltf, f)
        info = {"cha": cha_id, "name": cp["name"], "bone": bone, "file": fname + ".gltf", "scale": cp["scale"], "anims": anim_names,
                "sizeY": self.tables.model_view.get(cha_id), "stats": stats}
        print("  cha %d %s [%s]: %d phan, %d dinh, %d tam giac, %d animation, xuong thieu %d" % (
            cha_id, cp["name"], bone, stats["parts"], stats["verts"], stats["tris"], stats["anims"], stats["missing_bones"]))
        return info


def euler_to_quat(x, y, z):
    ax, ay, az = math.radians(x), math.radians(y), math.radians(z)
    cx, sx = math.cos(ax / 2), math.sin(ax / 2); cy, sy = math.cos(ay / 2), math.sin(ay / 2); cz, sz = math.cos(az / 2), math.sin(az / 2)
    # Unity: Z * X * Y
    qz = (0, 0, sz, cz); qx = (sx, 0, 0, cx); qy = (0, sy, 0, cy)
    def mul(a, b):
        ax_, ay_, az_, aw = a; bx, by, bz, bw = b
        return (aw * bx + ax_ * bw + ay_ * bz - az_ * by, aw * by - ax_ * bz + ay_ * bw + az_ * bx,
                aw * bz + ax_ * by - ay_ * bx + az_ * bw, aw * bw - ax_ * bx - ay_ * by - az_ * bz)
    return mul(mul(qy, qx), qz)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--map", default=None, help="vd world_baling: xuat NPC cua map + bang dat")
    ap.add_argument("--cha", nargs="*", type=int, default=[], help="cha_pic id")
    ap.add_argument("--src", default=os.environ.get("JX_SCN3D_SRC", r"D:\game3gTQ_mo\pc\剑网江湖_Data\StreamingAssets"))
    ap.add_argument("--key-file", default=os.environ.get("JX_SCN3D_KEY", r"D:\game3gTQ_mo\khoa_bundle.txt"))
    ap.add_argument("--out", default=os.path.join(NEXT, "client", "assets3d", "npc"))
    a = ap.parse_args()
    key = open(a.key_file, "r", encoding="utf-8").read().strip()
    ex = NpcExporter(a.src, key, a.out)
    ids = list(a.cha)
    placements = []
    if a.map:
        mapping = MARK_TO_CHA.get(a.map, {})
        scene_json = os.path.join(NEXT, "client", "assets3d", a.map, "scene.json")
        marks = {}
        if os.path.exists(scene_json):
            marks = json.load(io.open(scene_json, encoding="utf-8")).get("marks", {}).get("points", {})
        else:
            print("chua co", scene_json, "- chay export_scene.py truoc")
        for mark, pts in marks.items():
            cha = mapping.get(mark)
            if cha is None:
                continue
            for p in pts:
                placements.append({"mark": mark, "cha": cha, "pos": p["pos"], "angle": p.get("angle", 0.0)})
            if cha not in ids:
                ids.append(cha)
        if PLAYER_CHA not in ids:
            ids.append(PLAYER_CHA)
    infos = {}
    for cid in ids:
        info = ex.export_cha(cid)
        if info:
            infos[str(cid)] = info
    with io.open(os.path.join(a.out, "npc_models.json"), "w", encoding="utf-8") as f:
        json.dump(infos, f, ensure_ascii=False, indent=1)
    if a.map:
        with io.open(os.path.join(NEXT, "client", "assets3d", a.map, "npcs.json"), "w", encoding="utf-8") as f:
            json.dump({"player": PLAYER_CHA, "placements": placements}, f, ensure_ascii=False, indent=1)
        print("dat NPC:", len(placements), "vi tri ->", os.path.join(a.out, "..", a.map, "npcs.json"))
    with io.open(os.path.join(a.out, "export.log"), "w", encoding="utf-8") as f:
        f.write("\n".join(ex.log))
    print("xuat %d nhan vat, %d canh bao -> %s" % (len(infos), len(ex.log), a.out))


if __name__ == "__main__":
    main()
