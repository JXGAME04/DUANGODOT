# -*- coding: utf-8 -*-
"""Xuat hieu ung ky nang (particles.bdd) ra JSON + glTF de client Godot dung lai bang CPUParticles3D + mesh + den.

    python tools/scn3d/export_sfx.py Skill/wd_nuleizhi Skill/shifa_huoxi      # theo duong dan (tuong doi Particles/)
    python tools/scn3d/export_sfx.py --childobj 1 2 5 --sfx 1 2 3 4 5 290     # theo id bang skill_childobj / sfx_object
    python tools/scn3d/export_sfx.py --all                                     # moi prefab trong particles.bdd (558)

Moi prefab -> assets3d/sfx/<ten>.gltf (cay node + mesh + animation bien doi) + <ten>.json:
  nodes[]: {name, ps?, mesh_material?, light?}; ps = ParticleSystem (module chinh), mesh_material = texture/blend/mau cho MeshRenderer,
  light = den diem. Godot: Scn3DSfx.gd doc JSON, tim node theo ten trong glTF, them CPUParticles3D / doi vat lieu / OmniLight3D.
Toa do Unity -> Godot: dao truc X (giong export_scene.py).
"""
import argparse, io, json, math, os, struct, sys, collections

import numpy as np
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
from export_scene import Raw, h12, bundle_file, quat_to_mat, FLIP  # noqa: E402
from export_npc import Tables, euler_to_quat  # noqa: E402


def mmc(v, default=0.0):
    """MinMaxCurve -> {'min', 'max', 'curve': [[t, v]...] hoac None}
    minMaxState: 0 hang so (scalar), 1 duong cong (maxCurve * scalar), 2 hai hang so ngau nhien (minScalar..scalar), 3 hai duong cong"""
    if not isinstance(v, dict) or "scalar" not in v:
        return {"min": default, "max": default, "curve": None}
    state = v.get("minMaxState", 0); s = float(v.get("scalar", default)); mn = float(v.get("minScalar", s))
    def keys(c):
        return [[float(k["time"]), float(k["value"]) * s] for k in c.get("m_Curve", [])]
    if state == 0:
        return {"min": s, "max": s, "curve": None}
    if state == 2:
        return {"min": min(mn, s), "max": max(mn, s), "curve": None}
    if state == 1:
        ks = keys(v.get("maxCurve", {}))
        vals = [k[1] for k in ks] or [s]
        return {"min": min(vals), "max": max(vals), "curve": ks}
    ks = keys(v.get("maxCurve", {})); ks2 = keys(v.get("minCurve", {}))
    vals = [k[1] for k in ks + ks2] or [s]
    return {"min": min(vals), "max": max(vals), "curve": ks}


def gradient(g):
    """MinMaxGradient -> [[t, r, g, b, a]...] (dung maxGradient)"""
    if not isinstance(g, dict):
        return None
    mg = g.get("maxGradient", g)
    nc = int(mg.get("m_NumColorKeys", 2)); na = int(mg.get("m_NumAlphaKeys", 2))
    cols = []
    for i in range(nc):
        k = mg.get("key%d" % i); t = mg.get("ctime%d" % i, 0)
        if k is None:
            continue
        cols.append([t / 65535.0, k["r"], k["g"], k["b"]])
    alphas = []
    for i in range(na):
        k = mg.get("key%d" % i); t = mg.get("atime%d" % i, 0)
        if k is None:
            continue
        alphas.append([t / 65535.0, k["a"]])
    if not cols:
        return None
    # gop tai cac moc thoi gian cua ca hai
    times = sorted(set([c[0] for c in cols] + [a[0] for a in alphas] + [0.0, 1.0]))
    def lerp_keys(keys, t, n):
        if not keys:
            return [1.0] * n
        if t <= keys[0][0]:
            return keys[0][1:]
        if t >= keys[-1][0]:
            return keys[-1][1:]
        for a, b in zip(keys, keys[1:]):
            if a[0] <= t <= b[0]:
                f = (t - a[0]) / max(1e-6, b[0] - a[0])
                return [x + (y - x) * f for x, y in zip(a[1:], b[1:])]
        return keys[-1][1:]
    out = []
    for t in times:
        c = lerp_keys(cols, t, 3); a = lerp_keys(alphas, t, 1)
        out.append([round(t, 4), round(c[0], 4), round(c[1], 4), round(c[2], 4), round(a[0], 4)])
    return out


class SfxExporter:
    def __init__(self, src, key, out):
        UnityPy.set_assetbundle_decrypt_key(key)
        self.src = src; self.out = out
        os.makedirs(os.path.join(out, "tex"), exist_ok=True)
        self.env = UnityPy.load(bundle_file(src, "particles"), bundle_file(src, "shader"))
        self.objs = {o.path_id: o for o in self.env.objects}
        self.cont = {}
        for o in self.env.objects:
            if o.type.name == "AssetBundle":
                for k, v in o.read_typetree()["m_Container"]:
                    self.cont[k] = v["asset"]["m_PathID"]
        self.texcache = {}
        self.log = []

    def texture(self, pid):
        if pid in self.texcache:
            return self.texcache[pid]
        o = self.objs.get(pid); fn = None
        if o is not None and o.type.name == "Texture2D":
            t = o.read(); fn = "tex/%s.png" % t.m_Name
            try:
                t.image.save(os.path.join(self.out, fn))
            except Exception as e:
                self.log.append("texture loi %s: %s" % (t.m_Name, e)); fn = None
        self.texcache[pid] = fn
        return fn

    def material(self, pid):
        o = self.objs.get(pid)
        if o is None or o.type.name != "Material":
            return None
        m = o.read()
        try:
            shader = m.m_Shader.read().m_ParsedForm.m_Name
        except Exception:
            shader = "?"
        floats = {k: float(v) for k, v in m.m_SavedProperties.m_Floats}
        colors = {k: [c.r, c.g, c.b, c.a] for k, c in m.m_SavedProperties.m_Colors}
        tex = None; st = [1, 1, 0, 0]
        for k, t in m.m_SavedProperties.m_TexEnvs:
            if k == "_MainTex" and t.m_Texture.path_id:
                tex = self.texture(t.m_Texture.path_id); st = [t.m_Scale.x, t.m_Scale.y, t.m_Offset.x, t.m_Offset.y]
        dst = floats.get("_BlendDst", floats.get("_DstBlend", 10))
        # 1 = One (cong), 10 = OneMinusSrcAlpha (alpha), 0 = Zero (che)
        blend = "add" if dst == 1 else ("mul" if dst == 0 and floats.get("_SrcBlend", 1) == 2 else "alpha")
        if "add" in shader.lower() or "_add" in m.m_Name.lower():
            blend = "add"
        tint = colors.get("_TintColor", colors.get("_Color", [1, 1, 1, 1]))
        return {"name": m.m_Name, "shader": shader, "blend": blend, "tex": tex, "st": st, "tint": tint,
                "enhance": floats.get("_Enhance", 1.0), "cutoff": floats.get("_Cutoff", 0.0)}

    # ---------- glTF ghi mesh ----------
    def export(self, rel):
        """rel: vd 'Skill/wd_nuleizhi' -> (json, gltf) hoac None"""
        pid = None
        for cnd in ("assets/particles/%s.prefab" % rel, "assets/particles/%s" % rel):
            pid = self.cont.get(h12(cnd))
            if pid is not None:
                break
        if pid is None or self.objs[pid].type.name != "GameObject":
            self.log.append("khong co prefab: " + rel); return None
        name = rel.replace("/", "_").replace("\\", "_")
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
        gnodes = []; meshes = []; jnodes = []; by_pid = {}; anims = []
        mesh_cache = {}

        def mesh_index(mesh_pid, uvmod=None):
            key = (mesh_pid, json.dumps(uvmod, sort_keys=True) if uvmod else "")
            if key in mesh_cache:
                return mesh_cache[key]
            mo = self.objs.get(mesh_pid)
            if mo is None or mo.type.name != "Mesh":
                mesh_cache[key] = None; return None
            m = mo.read(); hd = MeshHandler(m); hd.process()
            nv = m.m_VertexData.m_VertexCount
            def arr(a, want):
                if not a:
                    return None
                x = np.array(a, dtype=np.float32).reshape(nv, -1)
                if x.shape[1] < want:
                    x = np.concatenate([x, np.zeros((nv, want - x.shape[1]), dtype=np.float32)], axis=1)
                return x[:, :want]
            pos = arr(hd.m_Vertices, 3); nor = arr(hd.m_Normals, 3); uv0 = arr(hd.m_UV0, 2); col = arr(getattr(hd, "m_Colors", None), 4)
            P = pos.copy(); P[:, 0] = -P[:, 0]
            attrs = {"POSITION": add_accessor(P, 5126, "VEC3", 34962, True)}
            if nor is not None:
                N = nor.copy(); N[:, 0] = -N[:, 0]; attrs["NORMAL"] = add_accessor(N, 5126, "VEC3", 34962)
            if uv0 is not None:
                uv = uv0.copy()
                if uvmod:
                    # SFXMeshModify: o atlas (uvNum_X/Y luoi, uvOffset_X/Y o) - o dem tu GOC TREN-TRAI cua anh (kiem tra tren 362 mesh:
                    # o dem tu duoi cho vung den o 208 mesh, dem tu tren cho hinh), nen v glTF (tu tren xuong) = (oy + (1 - v_unity)) / ny
                    nx, ny, ox, oy = uvmod
                    uv[:, 0] = (uv[:, 0] + ox) / max(1, nx); uv[:, 1] = (oy + (1.0 - uv[:, 1])) / max(1, ny)
                else:
                    uv[:, 1] = 1.0 - uv[:, 1]
                attrs["TEXCOORD_0"] = add_accessor(uv, 5126, "VEC2", 34962)
            if col is not None:
                attrs["COLOR_0"] = add_accessor(col, 5126, "VEC4", 34962)
            prims = []
            for tri in hd.get_triangles():
                t = np.array(tri, dtype=np.uint32).reshape(-1, 3)
                if len(t):
                    prims.append({"attributes": attrs, "indices": add_accessor(t[:, [0, 2, 1]].reshape(-1), 5125, "SCALAR", 34963), "mode": 4})
            if not prims:
                mesh_cache[key] = None; return None
            meshes.append({"name": m.m_Name, "primitives": prims})
            mesh_cache[key] = len(meshes) - 1
            return mesh_cache[key]

        def walk(go_pid, parent_gi, depth):
            g = self.objs[go_pid].read()
            tr = None; comps = collections.defaultdict(list)
            for c in g.m_Components:
                co = self.objs.get(c.path_id)
                if co is None:
                    continue
                if co.type.name == "Transform":
                    tr = co.read()
                else:
                    comps[co.type.name].append(co)
            if tr is None:
                return
            p = tr.m_LocalPosition; q = tr.m_LocalRotation; s = tr.m_LocalScale
            gi = len(gnodes)
            gnodes.append({"name": g.m_Name, "translation": [-p.x, p.y, p.z], "rotation": [q.x, -q.y, -q.z, q.w], "scale": [s.x, s.y, s.z]})
            by_pid[go_pid] = gi
            if parent_gi is not None:
                gnodes[parent_gi].setdefault("children", []).append(gi)
            jn = {"name": g.m_Name, "active": bool(getattr(g, "m_IsActive", True)), "depth": depth}
            # SFXMeshModify: o atlas + mau
            uvmod = None; sfxcol = None
            for co in comps.get("MonoBehaviour", []):
                raw = co.get_raw_data(); sc = self.objs.get(struct.unpack_from("<q", raw, 20)[0])
                cls = sc.read().m_ClassName if sc is not None and sc.type.name == "MonoScript" else "?"
                if cls == "SFXMeshModify":
                    r = Raw(raw); r.header(); r.pptr()
                    color = [r.f32(), r.f32(), r.f32(), r.f32()]; emissive = [r.f32(), r.f32(), r.f32(), r.f32()]
                    nx = r.u8(); r.align(); ny = r.u8(); r.align(); ox = r.u8(); r.align(); oy = r.u8(); r.align()
                    uvgrow = r.f32(); enhance = r.f32()
                    uvmod = [max(1, nx), max(1, ny), ox, oy]
                    sfxcol = {"color": color, "emissive": emissive, "uvgrow": uvgrow, "enhance": enhance}
                jn.setdefault("scripts", []).append(cls)
            # mesh
            if comps.get("MeshFilter") and comps.get("MeshRenderer"):
                mf = comps["MeshFilter"][0].read(); mr = comps["MeshRenderer"][0].read()
                mpid = mf.m_Mesh.path_id
                if not mpid and sfxcol is not None:
                    # SFXMeshModify.shareMesh (PPtr dau tien sau header)
                    raw = comps["MonoBehaviour"][0].get_raw_data(); r = Raw(raw); r.header(); mpid = r.pptr()[1]
                mi = mesh_index(mpid, uvmod) if mpid else None
                if mi is not None:
                    gnodes[gi]["mesh"] = mi
                    mats = [self.material(m.path_id) for m in mr.m_Materials]
                    jn["mesh_material"] = mats[0] if mats and mats[0] else None
                    if sfxcol:
                        jn["mesh_sfx"] = sfxcol
            # particle system
            for co in comps.get("ParticleSystem", []):
                d = co.read_typetree()
                im = d.get("InitialModule", {}); em = d.get("EmissionModule", {}); sh = d.get("ShapeModule", {})
                ps = {
                    "duration": float(d.get("lengthInSec", 1.0)), "looping": bool(d.get("looping", False)), "prewarm": bool(d.get("prewarm", False)),
                    "start_delay": mmc(d.get("startDelay"))["max"], "sim_speed": float(d.get("simulationSpeed", 1.0)),
                    "world_space": bool(d.get("moveWithTransform", 0) == 0 and d.get("InitialModule", {}).get("useLocalSpace", None) is False) if "useLocalSpace" in im else bool(d.get("simulationSpace", 0) == 1),
                    "max_particles": int(im.get("maxNumParticles", 100)),
                    "lifetime": mmc(im.get("startLifetime"), 1.0), "speed": mmc(im.get("startSpeed"), 1.0), "size": mmc(im.get("startSize"), 1.0),
                    "rotation": mmc(im.get("startRotation"), 0.0), "gravity": mmc(im.get("gravityModifier"), 0.0),
                    "start_color": gradient(im.get("startColor")),
                    "rate": mmc(em.get("rateOverTime"), 0.0) if em.get("enabled", True) else {"min": 0, "max": 0, "curve": None},
                    "bursts": [{"time": float(b.get("time", 0)), "count": mmc(b.get("countCurve"), 0.0)["max"] if isinstance(b.get("countCurve"), dict) else float(b.get("count", 0)), "cycles": int(b.get("cycleCount", 1)), "interval": float(b.get("repeatInterval", 0.01))} for b in em.get("m_Bursts", [])],
                    "shape": {"type": int(sh.get("type", 0)), "radius": float(sh.get("radius", {}).get("value", 0.0)) if isinstance(sh.get("radius"), dict) else float(sh.get("radius", 0.0)),
                              "angle": float(sh.get("angle", 25.0)), "length": float(sh.get("length", 5.0)), "scale": [sh.get("m_Scale", {}).get("x", 1), sh.get("m_Scale", {}).get("y", 1), sh.get("m_Scale", {}).get("z", 1)],
                              "pos": [-sh.get("m_Position", {}).get("x", 0), sh.get("m_Position", {}).get("y", 0), sh.get("m_Position", {}).get("z", 0)],
                              "rot": [sh.get("m_Rotation", {}).get("x", 0), sh.get("m_Rotation", {}).get("y", 0), sh.get("m_Rotation", {}).get("z", 0)],
                              "enabled": bool(sh.get("enabled", True)), "align_dir": bool(sh.get("alignToDirection", False))},
                }
                cm = d.get("ColorModule", {})
                ps["color_over_life"] = gradient(cm.get("gradient")) if cm.get("enabled") else None
                sm = d.get("SizeModule", {})
                ps["size_over_life"] = mmc(sm.get("curve"), 1.0)["curve"] if sm.get("enabled") else None
                rm = d.get("RotationModule", {})
                ps["rot_over_life"] = mmc(rm.get("curve"), 0.0) if rm.get("enabled") else None
                vm = d.get("VelocityModule", {})
                ps["velocity"] = {"x": mmc(vm.get("x"))["max"], "y": mmc(vm.get("y"))["max"], "z": mmc(vm.get("z"))["max"], "local": bool(vm.get("inWorldSpace", 0) == 0)} if vm.get("enabled") else None
                uvm = d.get("UVModule", {})
                ps["uv"] = {"tiles_x": int(uvm.get("tilesX", 1)), "tiles_y": int(uvm.get("tilesY", 1)), "start_frame": mmc(uvm.get("startFrame"), 0.0),
                            "frame_over_time": mmc(uvm.get("frameOverTime"), 0.0), "cycles": float(uvm.get("cycles", 1.0)), "anim_type": int(uvm.get("animationType", 0)),
                            "row": int(uvm.get("rowIndex", 0)), "row_mode": int(uvm.get("rowMode", 0))} if uvm.get("enabled") else None
                # renderer
                rd = None
                for rco in comps.get("ParticleSystemRenderer", []):
                    rd = rco.read_typetree()
                if rd:
                    mats = [self.material(m["m_PathID"]) for m in rd.get("m_Materials", [])]
                    mesh_pid = rd.get("m_Mesh", {}).get("m_PathID", 0)
                    mi = mesh_index(mesh_pid) if mesh_pid else None
                    ps["render"] = {"mode": int(rd.get("m_RenderMode", 0)), "material": mats[0] if mats and mats[0] else None,
                                    "mesh": (meshes[mi]["name"] if mi is not None else None), "mesh_index": mi,
                                    "min_size": float(rd.get("m_MinParticleSize", 0)), "max_size": float(rd.get("m_MaxParticleSize", 0.5)),
                                    "length_scale": float(rd.get("m_LengthScale", 2.0)), "velocity_scale": float(rd.get("m_VelocityScale", 0.0)),
                                    "alignment": int(rd.get("m_RenderAlignment", 0)), "sort": int(rd.get("m_SortMode", 0))}
                jn["ps"] = ps
            # light
            for lco in comps.get("Light", []):
                l = lco.read()
                jn["light"] = {"type": int(l.m_Type), "color": [l.m_Color.r, l.m_Color.g, l.m_Color.b], "intensity": float(l.m_Intensity), "range": float(getattr(l, "m_Range", 10.0))}
            # animation (legacy) tren node nay
            for aco in comps.get("Animation", []):
                a = aco.read()
                for clip_ptr in a.m_Animations:
                    if clip_ptr.path_id in self.objs and self.objs[clip_ptr.path_id].type.name == "AnimationClip":
                        anims.append((gi, go_pid, clip_ptr.path_id))
                dflt = a.m_Animation
                if dflt.path_id in self.objs and self.objs[dflt.path_id].type.name == "AnimationClip":
                    anims.append((gi, go_pid, dflt.path_id))
                jn["has_animation"] = True
            jnodes.append(jn)
            for ch in tr.m_Children:
                co = self.objs.get(ch.path_id)
                if co is not None and co.type.name == "Transform":
                    walk(co.read().m_GameObject.path_id, gi, depth + 1)
        walk(pid, None, 0)

        # animations: clip paths tuong doi node co Animation -> node index
        def node_index_by_path(root_gi, path):
            gi = root_gi
            for part in [x for x in path.split("/") if x]:
                found = None
                for c in gnodes[gi].get("children", []):
                    if gnodes[c]["name"] == part:
                        found = c; break
                if found is None:
                    return None
                gi = found
            return gi
        gltf_anims = []; seen = set()
        for root_gi, go_pid, clip_pid in anims:
            if clip_pid in seen:
                continue
            seen.add(clip_pid)
            d = self.objs[clip_pid].read_typetree()
            samplers = []; channels = []
            def add_channel(path, prop, keys, conv):
                ni = node_index_by_path(root_gi, path)
                if ni is None or not keys:
                    return
                keys = sorted(keys, key=lambda k: k[0])
                times = np.array([k[0] for k in keys], dtype=np.float32).reshape(-1, 1)
                vals = [conv(k[1]) for k in keys]
                if prop == "rotation":
                    for i in range(1, len(vals)):
                        if np.dot(vals[i], vals[i - 1]) < 0:
                            vals[i] = [-x for x in vals[i]]
                a_in = add_accessor(times, 5126, "SCALAR", None, True)
                a_out = add_accessor(np.array(vals, dtype=np.float32), 5126, "VEC4" if prop == "rotation" else "VEC3")
                samplers.append({"input": a_in, "output": a_out, "interpolation": "LINEAR"})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": ni, "path": prop}})
            for c in d.get("m_RotationCurves", []):
                add_channel(c["path"], "rotation", [(k["time"], (k["value"]["x"], k["value"]["y"], k["value"]["z"], k["value"]["w"])) for k in c["curve"]["m_Curve"]], lambda q: [q[0], -q[1], -q[2], q[3]])
            for c in d.get("m_EulerCurves", []):
                add_channel(c["path"], "rotation", [(k["time"], euler_to_quat(k["value"]["x"], k["value"]["y"], k["value"]["z"])) for k in c["curve"]["m_Curve"]], lambda q: [q[0], -q[1], -q[2], q[3]])
            for c in d.get("m_PositionCurves", []):
                add_channel(c["path"], "translation", [(k["time"], (k["value"]["x"], k["value"]["y"], k["value"]["z"])) for k in c["curve"]["m_Curve"]], lambda v: [-v[0], v[1], v[2]])
            for c in d.get("m_ScaleCurves", []):
                add_channel(c["path"], "scale", [(k["time"], (k["value"]["x"], k["value"]["y"], k["value"]["z"])) for k in c["curve"]["m_Curve"]], lambda v: [v[0], v[1], v[2]])
            # float curves (vd m_LocalScale.x, material _MainTex_ST) -> ghi vao json de Godot xu ly neu can
            floats = []
            for c in d.get("m_FloatCurves", []):
                floats.append({"path": c.get("path", ""), "attr": c.get("attribute", ""), "keys": [[round(k["time"], 4), round(k["value"], 4)] for k in c["curve"]["m_Curve"]][:64]})
            if channels:
                gltf_anims.append({"name": d.get("m_Name", "anim"), "samplers": samplers, "channels": channels})
            if floats:
                jnodes[0].setdefault("float_curves", []).append({"clip": d.get("m_Name", "anim"), "root": gnodes[root_gi]["name"], "curves": floats})
        gltf = {"asset": {"version": "2.0", "generator": "jxnext scn3d export_sfx"}, "scene": 0, "scenes": [{"nodes": [0]}], "nodes": gnodes}
        if meshes:
            gltf["meshes"] = meshes
        if gltf_anims:
            gltf["animations"] = gltf_anims
        if len(buf):
            with open(os.path.join(self.out, name + ".bin"), "wb") as f:
                f.write(bytes(buf))
            gltf.update({"accessors": accessors, "bufferViews": views, "buffers": [{"uri": name + ".bin", "byteLength": len(buf)}]})
        elif os.path.exists(os.path.join(self.out, name + ".bin")):
            os.remove(os.path.join(self.out, name + ".bin"))
        with io.open(os.path.join(self.out, name + ".gltf"), "w", encoding="utf-8") as f:
            json.dump(gltf, f)
        desc = {"name": name, "rel": rel, "nodes": jnodes, "gltf": name + ".gltf", "mesh_names": [m["name"] for m in meshes]}
        with io.open(os.path.join(self.out, name + ".json"), "w", encoding="utf-8") as f:
            json.dump(desc, f, ensure_ascii=False, indent=1)
        nps = sum(1 for j in jnodes if "ps" in j); nm = sum(1 for j in jnodes if "mesh_material" in j)
        print("  sfx %-32s node %2d  particle %2d  mesh %2d  light %d  anim %d" % (rel, len(jnodes), nps, nm, sum(1 for j in jnodes if "light" in j), len(gltf_anims)))
        return desc


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="*")
    ap.add_argument("--childobj", nargs="*", type=int, default=[])
    ap.add_argument("--sfx", nargs="*", type=int, default=[])
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--src", default=os.environ.get("JX_SCN3D_SRC", r"D:\game3gTQ_mo\pc\剑网江湖_Data\StreamingAssets"))
    ap.add_argument("--key-file", default=os.environ.get("JX_SCN3D_KEY", r"D:\game3gTQ_mo\khoa_bundle.txt"))
    ap.add_argument("--out", default=os.path.join(NEXT, "client", "assets3d", "sfx"))
    a = ap.parse_args()
    key = open(a.key_file, "r", encoding="utf-8").read().strip()
    ex = SfxExporter(a.src, key, a.out)
    tables = Tables(a.src)
    paths = list(a.paths)
    index = {"childobj": {}, "sfx": {}}
    for r in tables.t.get("skill_childobj", [])[1:]:
        if len(r) > 5 and r[0].strip().isdigit():
            index["childobj"][int(r[0])] = {"name": r[2].strip(), "path": r[5].strip(), "time": r[8].strip() if len(r) > 8 else "", "hang": r[14].strip() if len(r) > 14 else ""}
    for r in tables.t.get("sfx_object", [])[1:]:
        if len(r) > 2 and r[0].strip().isdigit():
            index["sfx"][int(r[0])] = {"name": r[1].strip(), "path": r[2].strip(), "time": r[5].strip() if len(r) > 5 else "", "hang": r[7].strip() if len(r) > 7 else ""}
    for cid in a.childobj:
        if cid in index["childobj"] and index["childobj"][cid]["path"]:
            paths.append(index["childobj"][cid]["path"])
    for sid in a.sfx:
        if sid in index["sfx"] and index["sfx"][sid]["path"]:
            paths.append(index["sfx"][sid]["path"])
    if a.all:
        for k in ex.cont:
            pass
        # duong dan goc khong co (container da bam) -> lay tu hai bang
        paths = sorted(set([v["path"] for v in index["childobj"].values() if v["path"]] + [v["path"] for v in index["sfx"].values() if v["path"]]))
    done = {}
    for p in dict.fromkeys(paths):
        d = ex.export(p)
        if d:
            done[p] = d["name"]
    with io.open(os.path.join(a.out, "sfx_index.json"), "w", encoding="utf-8") as f:
        json.dump({"exported": done, "childobj": {str(k): v for k, v in index["childobj"].items()}, "sfx": {str(k): v for k, v in index["sfx"].items()}}, f, ensure_ascii=False, indent=1)
    with io.open(os.path.join(a.out, "export.log"), "w", encoding="utf-8") as f:
        f.write("\n".join(ex.log))
    print("xuat %d hieu ung, %d canh bao -> %s" % (len(done), len(ex.log), a.out))


if __name__ == "__main__":
    main()
