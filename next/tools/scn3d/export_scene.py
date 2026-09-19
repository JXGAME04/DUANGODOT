# -*- coding: utf-8 -*-
"""Xuat mot scene 3D cua bo tham khao (剑网江湖, Unity 2022.3 bundle) ra glTF + PNG + JSON de client Godot nap thu.

Chi dung de thu nghiem noi bo (tai san thuoc ban quyen game goc): tai san xuat ra nam ngoai git
(client/assets3d/, da .gitignore), khoa giai ma doc tu tep ngoai repo (--key-file).

    python tools/scn3d/export_scene.py world_baling
    python tools/scn3d/export_scene.py world_baling --src "D:/game3gTQ_mo/pc/剑网江湖_Data/StreamingAssets" --out client/assets3d/world_baling

Cach bundle to chuc (xem D:/game3gTQ_mo/BAO-CAO-MAP-3D.md muc 7):
  scenes_<scene>.bdd : prefab (GameObject/Transform/MeshFilter/MeshRenderer bi tuoc tham chieu) + TextAsset index
                       (cay node + sceneInfo.{meshUrlList, matRefUrlList, lightTexList}) + mesh gop [cmbm] + lightmap
  Scene_Ref (MonoBehaviour, typetree bi tuoc): mData[i] = thanh phan dich, mType[i]: 2 MeshFilter->mesh, 3 MeshCollider->mesh,
                       0 MeshRenderer->material, 1 MeshRenderer->lightmap (+ mLightmap[i] = scale/offset)
  expscnmesh.bdd     : mesh thu vien, container = md5(lower('assets/art/scenemakeres/mesh/' + url))[:12]
  expscnmat.bdd      : Material + MaterialRef{TextureData[]{key, path}}, container = md5(lower('assets/art/scenemakeres/mat/' + url))[:12]
  scenes.bdd         : texture dung chung, container = md5(lower(path))[:12]
"""
import argparse, hashlib, io, json, math, os, struct, sys, collections

import numpy as np
import UnityPy
from UnityPy.helpers.MeshHelper import MeshHandler

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))


def h12(s):
    return hashlib.md5(s.lower().encode("utf-8")).hexdigest()[:12]


def bundle_file(src, name):
    return os.path.join(src, hashlib.md5((name + ".bdd").encode()).hexdigest()[:12] + ".bdd")


class Raw:
    """Doc byte tho cua MonoBehaviour (typetree bi tuoc)."""

    def __init__(self, b):
        self.b = b
        self.p = 0

    def i32(self):
        v = struct.unpack_from("<i", self.b, self.p)[0]; self.p += 4; return v

    def i64(self):
        v = struct.unpack_from("<q", self.b, self.p)[0]; self.p += 8; return v

    def u8(self):
        v = self.b[self.p]; self.p += 1; return v

    def i16(self):
        v = struct.unpack_from("<h", self.b, self.p)[0]; self.p += 2; return v

    def f32(self):
        v = struct.unpack_from("<f", self.b, self.p)[0]; self.p += 4; return v

    def align(self):
        self.p = (self.p + 3) & ~3

    def pptr(self):
        return (self.i32(), self.i64())

    def string(self):
        n = self.i32(); s = self.b[self.p:self.p + n].decode("utf-8", "replace"); self.p += n; self.align(); return s

    def header(self):
        """m_GameObject, m_Enabled, m_Script, m_Name -> tra ve (go_pathid, script_pathid)"""
        go = self.pptr(); self.u8(); self.align(); sc = self.pptr(); self.string()
        return go[1], sc[1]


def quat_to_mat(q):
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def trs(p, q, s):
    M = np.eye(4)
    M[:3, :3] = quat_to_mat(q) @ np.diag(s)
    M[:3, 3] = p
    return M


def mat_to_quat(R):
    """ma tran quay 3x3 (cot da chuan hoa) -> quaternion [x, y, z, w]"""
    m = R
    tr = m[0, 0] + m[1, 1] + m[2, 2]
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        return [(m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s]
    if m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = math.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2
        return [0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s]
    if m[1, 1] > m[2, 2]:
        s = math.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2
        return [(m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s]
    s = math.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2
    return [(m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s]


def euler_unity(rx, ry, rz):
    """Unity Transform.eulerAngles (do) -> ma tran quay (thu tu Z, X, Y nhu Unity: R = Ry * Rx * Rz)."""
    ax, ay, az = math.radians(rx), math.radians(ry), math.radians(rz)
    Rx = np.array([[1, 0, 0], [0, math.cos(ax), -math.sin(ax)], [0, math.sin(ax), math.cos(ax)]])
    Ry = np.array([[math.cos(ay), 0, math.sin(ay)], [0, 1, 0], [-math.sin(ay), 0, math.cos(ay)]])
    Rz = np.array([[math.cos(az), -math.sin(az), 0], [math.sin(az), math.cos(az), 0], [0, 0, 1]])
    return Ry @ Rx @ Rz


FLIP = np.diag([-1.0, 1.0, 1.0, 1.0])  # Unity (tay trai) -> glTF/Godot (tay phai): dao truc X


class Exporter:
    def __init__(self, scene, src, key, out):
        self.scene = scene
        self.src = src
        self.out = out
        UnityPy.set_assetbundle_decrypt_key(key)
        os.makedirs(os.path.join(out, "tex"), exist_ok=True)
        self.log = []

    # ---------- nap bundle ----------
    def load(self):
        s = self.scene
        self.env_scene = UnityPy.load(bundle_file(self.src, "scenes_" + s))
        self.env_mesh = UnityPy.load(bundle_file(self.src, "expscnmesh"))
        self.env_mat = UnityPy.load(bundle_file(self.src, "expscnmat"), bundle_file(self.src, "shader"))
        self.env_tex = UnityPy.load(bundle_file(self.src, "scenes"))
        self.cont_scene = self._containers(self.env_scene)
        self.cont_mesh = self._containers(self.env_mesh)
        self.cont_mat = self._containers(self.env_mat)
        self.cont_tex = self._containers(self.env_tex)
        self.objs_scene = {o.path_id: o for o in self.env_scene.objects}
        self.objs_mesh = {o.path_id: o for o in self.env_mesh.objects}
        self.objs_mat = {o.path_id: o for o in self.env_mat.objects}
        self.objs_tex = {o.path_id: o for o in self.env_tex.objects}
        idx_pid = self.cont_scene.get(h12("assets/art/scnsload/%s/index.json" % s))
        if idx_pid is None:
            raise SystemExit("khong thay index.json cua scene " + s)
        raw = self._textasset(self.objs_scene[idx_pid])
        self.index = json.loads(raw.decode("utf-8-sig"))
        self.info = self.index["sceneInfo"]
        print("scene %s: %d node, %d mesh url, %d mat url, %d lightmap" % (
            s, len(self.index["nodes"]), len(self.info["meshUrlList"]), len(self.info["matRefUrlList"]), len(self.info["lightTexList"])))

    @staticmethod
    def _containers(env):
        cont = {}
        for o in env.objects:
            if o.type.name == "AssetBundle":
                for k, v in o.read_typetree()["m_Container"]:
                    cont[k] = v["asset"]["m_PathID"]
        return cont

    @staticmethod
    def _textasset(o):
        d = o.read()
        raw = d.m_Script
        if isinstance(raw, str):
            raw = raw.encode("utf-8", "surrogateescape")
        return raw

    # ---------- Scene_Ref ----------
    def parse_scene_refs(self):
        """target component path_id -> {'mesh': idx, 'mat': [idx...], 'lm': (idx, sx, sy, ox, oy)}"""
        self.bind = collections.defaultdict(dict)
        n = 0
        for o in self.env_scene.objects:
            if o.type.name != "MonoBehaviour":
                continue
            r = Raw(o.get_raw_data())
            go_pid, sc_pid = r.header()
            sc = self.objs_scene.get(sc_pid)
            cls = sc.read().m_ClassName if sc is not None and sc.type.name == "MonoScript" else "?"
            if cls != "Scene_Ref":
                continue
            cnt = r.i32(); data = [r.pptr()[1] for _ in range(cnt)]
            cnt2 = r.i32(); types = [r.u8() for _ in range(cnt2)]; r.align()
            cnt3 = r.i32(); index = [r.i16() for _ in range(cnt3)]; r.align()
            cnt4 = r.i32(); lm = [(r.f32(), r.f32(), r.f32(), r.f32()) for _ in range(cnt4)]
            for i in range(cnt):
                t, ix = types[i], index[i]
                b = self.bind[data[i]]
                if t == 2:
                    b["mesh"] = ix
                elif t == 3:
                    b["colmesh"] = ix
                elif t == 0:
                    b.setdefault("mat", []).append(ix)
                elif t == 1:
                    v = lm[i] if i < cnt4 else (1, 1, 0, 0)
                    b["lm"] = (ix, v[0], v[1], v[2], v[3])
            n += 1
        print("Scene_Ref:", n, "components,", len(self.bind), "targets")

    # ---------- mesh ----------
    def resolve_mesh(self, url):
        """-> (env, ObjectReader Mesh) hoac None"""
        pid = self.cont_scene.get(h12(url))
        if pid is not None:
            return self._mesh_from(self.env_scene, self.objs_scene, pid)
        pid = self.cont_mesh.get(h12("assets/art/scenemakeres/mesh/" + url))
        if pid is not None:
            return self._mesh_from(self.env_mesh, self.objs_mesh, pid)
        return None

    def _mesh_from(self, env, objs, pid):
        o = objs.get(pid)
        if o is None:
            return None
        if o.type.name == "Mesh":
            return o
        if o.type.name == "GameObject":
            # prefab thu vien: lay Mesh cua MeshFilter dau tien trong cay
            go = o.read()
            for c in go.m_Components:
                co = objs.get(c.path_id)
                if co is not None and co.type.name == "MeshFilter":
                    mf = co.read()
                    if mf.m_Mesh.path_id:
                        mo = objs.get(mf.m_Mesh.path_id)
                        if mo is not None:
                            return mo
            # tim trong con
            for c in go.m_Components:
                co = objs.get(c.path_id)
                if co is not None and co.type.name == "Transform":
                    for ch in co.read().m_Children:
                        cho = objs.get(ch.path_id)
                        if cho is None:
                            continue
                        r = self._mesh_from(env, objs, cho.read().m_GameObject.path_id)
                        if r is not None:
                            return r
        return None

    def mesh_data(self, mo):
        """-> dict(pos, nor, uv0, uv1, submeshes=[np.array idx]) trong toa do Unity"""
        m = mo.read()
        hd = MeshHandler(m); hd.process()
        nv = m.m_VertexData.m_VertexCount
        def arr(a, want):
            if not a:
                return None
            x = np.array(a, dtype=np.float32).reshape(nv, -1)
            return x[:, :want]
        pos = arr(hd.m_Vertices, 3)
        nor = arr(hd.m_Normals, 3)
        uv0 = arr(hd.m_UV0, 2)
        uv1 = arr(hd.m_UV1, 2)
        subs = []
        for tri, sm in zip(hd.get_triangles(), m.m_SubMeshes):
            a = np.array(tri, dtype=np.uint32).reshape(-1, 3)
            base = getattr(sm, "baseVertex", 0) or 0
            if base:
                a = a + base
            subs.append(a)
        return {"name": m.m_Name, "pos": pos, "nor": nor, "uv0": uv0, "uv1": uv1, "subs": subs}

    # ---------- material ----------
    def resolve_material(self, url):
        pid = self.cont_mat.get(h12("assets/art/scenemakeres/mat/" + url))
        if pid is None:
            return None
        mo = self.objs_mat.get(pid)
        texpaths = {}
        if mo is not None and mo.type.name == "MonoBehaviour":
            # container tro toi MaterialRef {mMaterial, mTextures}; lay Material qua mMaterial
            mat_pid, texpaths = self.matref.get(pid, (0, {}))
            mo = self.objs_mat.get(mat_pid)
        if mo is None or mo.type.name != "Material":
            return None
        m = mo.read()
        try:
            shader = m.m_Shader.read().m_ParsedForm.m_Name
        except Exception:
            shader = "?"
        floats = {k: v for k, v in m.m_SavedProperties.m_Floats}
        colors = {k: (c.r, c.g, c.b, c.a) for k, c in m.m_SavedProperties.m_Colors}
        st = {k: (t.m_Scale.x, t.m_Scale.y, t.m_Offset.x, t.m_Offset.y) for k, t in m.m_SavedProperties.m_TexEnvs}
        if not texpaths:
            texpaths = self.matref_by_mat.get(mo.path_id, {})
        return {"name": m.m_Name, "shader": shader, "floats": floats, "colors": colors, "st": st, "tex": texpaths}

    def parse_matrefs(self):
        """Material path_id -> {key: path}"""
        self.matref = {}          # MaterialRef path_id -> (Material path_id, {key: path})
        self.matref_by_mat = {}   # Material path_id -> {key: path}
        for o in self.env_mat.objects:
            if o.type.name != "MonoBehaviour":
                continue
            r = Raw(o.get_raw_data())
            r.header()
            mat = r.pptr()[1]
            cnt = r.i32()
            d = {}
            for _ in range(cnt):
                k = r.string(); v = r.string(); d[k] = v
            self.matref[o.path_id] = (mat, d)
            self.matref_by_mat[mat] = d

    # ---------- texture ----------
    def export_texture(self, path):
        """path Unity -> ten tep PNG tuong doi (tex/xxx.png) hoac None"""
        key = h12(path)
        if key in self.texcache:
            return self.texcache[key]
        pid = self.cont_tex.get(key); env = self.objs_tex
        if pid is None:
            pid = self.cont_scene.get(key); env = self.objs_scene
        if pid is None:
            self.texcache[key] = None
            self.log.append("texture thieu: " + path)
            return None
        o = env.get(pid)
        if o is None or o.type.name != "Texture2D":
            self.texcache[key] = None
            return None
        t = o.read()
        fn = "tex/%s.png" % key
        try:
            t.image.save(os.path.join(self.out, fn))
        except Exception as e:
            self.log.append("texture loi %s: %s" % (path, e))
            self.texcache[key] = None
            return None
        self.texcache[key] = fn
        self.texinfo[fn] = {"path": path, "w": t.m_Width, "h": t.m_Height}
        return fn

    # ---------- hierarchy ----------
    def walk(self):
        """Duyet cay: tra ve danh sach instance {mesh_url_idx, mats, lm, world(np 4x4 Unity), group, name, active}"""
        roots = {}
        for o in self.env_scene.objects:
            if o.type.name == "Transform":
                t = o.read()
                if not t.m_Father or t.m_Father.path_id == 0:
                    go = t.m_GameObject.read()
                    roots.setdefault(go.m_Name, []).append(t)
        self.roots = roots
        nodes = self.index["nodes"]; parents = self.index["parents"]
        inst = []
        self.effects = []
        # ma tran the gioi cua node index (px.. la world position/euler/lossyScale)
        for ni, nd in enumerate(nodes):
            url = nd.get("url", "")
            if not url or url in ("Config",):
                continue
            cand = roots.get(url) or roots.get(nd.get("name", ""))
            if not cand:
                self.log.append("node %s: khong thay prefab goc" % url)
                continue
            NM = np.eye(4)
            NM[:3, :3] = euler_unity(nd["rx"], nd["ry"], nd["rz"]) @ np.diag([nd["lsx"], nd["lsy"], nd["lsz"]])
            NM[:3, 3] = [nd["px"], nd["py"], nd["pz"]]
            root = cand[0]
            # neu transform goc trong bundle da mang dung vi tri node thi khong nhan hai lan
            rp = root.m_LocalPosition
            root_has_pos = abs(rp.x - nd["px"]) < 1e-3 and abs(rp.z - nd["pz"]) < 1e-3 and (abs(rp.x) + abs(rp.z) > 1e-3)
            base = np.eye(4) if root_has_pos else NM
            group = url
            self._walk_transform(root, base, group, inst, is_root=True, root_has_pos=root_has_pos)
        return inst

    def _walk_transform(self, t, parentM, group, inst, is_root=False, root_has_pos=False):
        p = t.m_LocalPosition; q = t.m_LocalRotation; s = t.m_LocalScale
        L = trs([p.x, p.y, p.z], [q.x, q.y, q.z, q.w], [s.x, s.y, s.z])
        if is_root and not root_has_pos:
            L = np.eye(4)  # goc prefab: vi tri lay tu node index
        M = parentM @ L
        go = t.m_GameObject.read()
        comps = {}
        for c in go.m_Components:
            co = self.objs_scene.get(c.path_id)
            if co is not None:
                comps.setdefault(co.type.name, []).append(co)
        active = bool(getattr(go, "m_IsActive", 1))
        # PrefabRef [TK Execute 0x87e890 / SetPrefabData 0x87d6a0]: the scene's own effects (torches cmn_huoyan*, stone lamps
        # cmn_shideng01, fountains, incense smoke cmn_yanwu, waterfall spray cmn_pubu_shuihua*): objData[] {obj, url, type 11 =
        # Prefab, data ["1" = SetTransform, px, py, pz, rx, ry, rz, sx, sy, sz]} instantiated under this object
        for co in comps.get("MonoBehaviour", []):
            try:
                r = Raw(co.get_raw_data()); go_pid, sc_pid = r.header()
                sc = self.objs_scene.get(sc_pid)
                if sc is None or sc.type.name != "MonoScript" or sc.read().m_ClassName != "PrefabRef":
                    continue
                cnt = r.i32()
                for _ in range(cnt):
                    r.pptr(); url = r.string(); typ = r.i32(); nd = r.i32(); data = [r.string() for _ in range(nd)]
                    if typ != 11 or not url.startswith("Assets/Particles/") or not url.endswith(".prefab"):
                        continue
                    rel = url[len("Assets/Particles/"):-len(".prefab")]
                    lp = [0.0, 0.0, 0.0]; le = [0.0, 0.0, 0.0]; ls = [1.0, 1.0, 1.0]
                    if len(data) >= 10 and data[0].strip() == "1":
                        try:
                            lp = [float(x) for x in data[1:4]]; le = [float(x) for x in data[4:7]]; ls = [float(x) for x in data[7:10]]
                        except ValueError:
                            pass
                    LM = np.eye(4); LM[:3, :3] = euler_unity(*le) @ np.diag(ls); LM[:3, 3] = lp
                    W = M @ LM
                    sc3 = [float(np.linalg.norm(W[:3, i])) for i in range(3)]
                    R = W[:3, :3] / np.array([max(s_, 1e-6) for s_ in sc3])
                    q = mat_to_quat(R)
                    self.effects.append({"prefab": rel, "host": go.m_Name, "pos": [-float(W[0, 3]), float(W[1, 3]), float(W[2, 3])],
                                         "quat": [q[0], -q[1], -q[2], q[3]], "scale": sc3, "active": active})
            except (struct.error, IndexError, UnicodeDecodeError):
                self.log.append("PrefabRef khong doc duoc: " + go.m_Name)
        if "MeshRenderer" in comps and "MeshFilter" in comps:
            mr = comps["MeshRenderer"][0]; mf = comps["MeshFilter"][0]
            bf = self.bind.get(mf.path_id, {}); br = self.bind.get(mr.path_id, {})
            if "mesh" in bf:
                mrd = mr.read()
                inst.append({"mesh": bf["mesh"], "mats": br.get("mat", []), "lm": br.get("lm"), "world": M,
                             "group": group, "name": go.m_Name, "active": active and bool(getattr(mrd, "m_Enabled", 1)),
                             "layer": int(getattr(go, "m_Layer", 0))})
        for ch in t.m_Children:
            co = self.objs_scene.get(ch.path_id)
            if co is not None and co.type.name == "Transform":
                self._walk_transform(co.read(), M, group, inst)

    # ---------- anh sang / suong ----------
    def render_setting(self):
        rs = {}
        for o in self.env_scene.objects:
            if o.type.name == "Light":
                l = o.read(); go = l.m_GameObject.read()
                tr = None
                for c in go.m_Components:
                    co = self.objs_scene.get(c.path_id)
                    if co is not None and co.type.name == "Transform":
                        tr = co.read()
                if l.m_Type == 1 and tr is not None:
                    q = tr.m_LocalRotation
                    fwd = quat_to_mat([q.x, q.y, q.z, q.w]) @ np.array([0, 0, 1.0])
                    rs["light"] = {"name": go.m_Name, "dir": [-float(fwd[0]), float(fwd[1]), float(fwd[2])],
                                   "color": [l.m_Color.r, l.m_Color.g, l.m_Color.b], "intensity": float(l.m_Intensity)}
            elif o.type.name == "Camera":
                c = o.read()
                rs["camera_unity"] = {"fov": float(c.field_of_view), "near": float(c.near_clip_plane), "far": float(c.far_clip_plane)}
                # the reference clears to a solid colour per scene (m_ClearFlags 2, no skybox: SkyBoxCollection unused, the one
                # cubemap is customReflection) - Ba Lang (0.635, 0.96, 1.0), the caves dark
                bg = getattr(c, "m_BackGroundColor", None)
                if bg is not None:
                    rs["camera_bg"] = [float(bg.r), float(bg.g), float(bg.b)]
                    rs["camera_clear"] = int(getattr(c, "m_ClearFlags", 2))
            elif o.type.name == "MonoBehaviour":
                r = Raw(o.get_raw_data()); go_pid, sc_pid = r.header()
                sc = self.objs_scene.get(sc_pid)
                cls = sc.read().m_ClassName if sc is not None and sc.type.name == "MonoScript" else "?"
                if cls == "CullDistances":
                    # CullDistances [TK OnEnable 0x4a91d0]: cull_dist_ly14/15/16 (the only serialized fields, 12 bytes) go to
                    # Camera.layerCullDistances[layer 14 / 15 / 16] with layerCullSpherical = true; 0 = the far plane.  10 scenes:
                    # 25 / 50 / 0 (Ba Lang, Thanh Do, ...), 40 / 55 / 0 (Thanh Thanh), 25 / 50 / 100 (Bien Kinh)
                    b = r.b[r.p:]
                    if len(b) >= 12:
                        v = struct.unpack_from("<3f", b, 0)
                        rs["cull_layers"] = {"14": float(v[0]), "15": float(v[1]), "16": float(v[2])}
                if cls == "SceneRenderSetting":
                    b = r.b[r.p:]
                    f = lambda off: struct.unpack_from("<f", b, off)[0]
                    rs["ambient_equator"] = [f(0), f(4), f(8)]
                    rs["ambient_ground"] = [f(16), f(20), f(24)]
                    rs["ambient_intensity"] = f(32)
                    rs["ambient_light"] = [f(36), f(40), f(44)]
                    rs["ambient_mode"] = struct.unpack_from("<i", b, 52)[0]
                    rs["ambient_sky"] = [f(164), f(168), f(172)]
                    rs["fog"] = bool(b[180])
                    rs["fog_color"] = [f(184), f(188), f(192)]
                    rs["fog_mode"] = struct.unpack_from("<i", b, 200)[0]
                    rs["fog_density"] = f(204); rs["fog_start"] = f(208); rs["fog_end"] = f(212)
        return rs

    def scn_table(self):
        """scn_list_cmn (excel.bdd): dong dau tien co sl_path == scene -> cameraInit, sl_markpath, ten"""
        env = UnityPy.load(bundle_file(self.src, "excel"))
        row = None
        for o in env.objects:
            if o.type.name == "TextAsset":
                d = o.read()
                if d.m_Name == "scn_list_cmn":
                    raw = self._textasset(o)
                    if raw[:2] == b"\xff\xfe":
                        txt = raw.decode("utf-16")
                        for line in txt.splitlines():
                            c = line.split("\t")
                            if len(c) > 20 and c[4].strip() == self.scene:
                                row = c; break
        if row is None:
            return {}
        cam = [float(x) for x in row[18].split("*")] if row[18].strip() else [19, 10, 21, 0, 40, 40, 80]
        try:
            tv = json.load(io.open(os.path.join(HERE, "ten_viet.json"), encoding="utf-8")).get("map", {})
        except Exception:
            tv = {}
        return {"id": int(row[0]), "name": row[1], "name_vi": tv.get(row[1].strip(), ""), "note": row[2], "sl_markpath": row[5], "sl_type": row[7],
                "camera": {"dist": cam[0], "dist_min": cam[1], "dist_max": cam[2], "yaw": cam[3], "pitch": cam[4], "pitch_min": cam[5], "pitch_max": cam[6]},
                "fog_type": row[27].strip() if len(row) > 27 else ""}

    def marks(self, markpath):
        env = UnityPy.load(bundle_file(self.src, "scenesnav"))
        cont = self._containers(env); objs = {o.path_id: o for o in env.objects}
        pid = cont.get(h12("assets/scenes/scn/mark/%s.json" % markpath))
        out = {"points": {}, "areas": []}
        if pid is not None:
            j = json.loads(self._textasset(objs[pid]).decode("utf-8-sig"))
            for p in j.get("point", []):
                out["points"][p["n"]] = [{"pos": [-v["pos"]["x"], v["pos"]["y"], v["pos"]["z"]], "angle": -v.get("angle", 0.0)} for v in p.get("v", [])]
            for a in j.get("areas", []):
                out["areas"].append({"name": a.get("n"), "nodes": [[-n["x"], n["y"], n["z"]] for n in a.get("nodes", [])]})
        pid = cont.get(h12("assets/scenes/scn/navi/%s.bytes" % markpath))
        if pid is not None:
            raw = self._textasset(objs[pid])
            # AIS: "AIS", 5 x u32 (1,1,0,0,1 = sections present), then sections of {u16 n, u16 0xCD02, n items} separated by
            # u32 0: nav verts (3 float), nav tris (u16 x 3), then the HEIGHT MESH verts + tris (GXAIScene_GetHeight: the y a
            # creature stands at - NavContext.GetNearHeight called by TaskMoveHelper.LogicTick/New, ChildObject.InitPosAndAngle
            # [TK]; the village squares lie on it ~10 cm above the terrain), building / stone meshes when present
            p = 24
            secs = []
            while p + 4 <= len(raw):
                n, tag = struct.unpack_from("<HH", raw, p)
                if tag != 0xCD02:
                    break
                p += 4
                if len(secs) % 2 == 0:
                    secs.append(np.frombuffer(raw, dtype=np.float32, count=n * 3, offset=p).reshape(-1, 3)); p += n * 12
                else:
                    secs.append(np.frombuffer(raw, dtype=np.uint16, count=n, offset=p).reshape(-1, 3)); p += n * 2
                p += 4   # the u32 0 between sections
            v = secs[0]; idx = secs[1]
            v2 = v.copy(); v2[:, 0] = -v2[:, 0]
            out["nav"] = {"verts": v2.round(3).tolist(), "tris": idx.tolist(),
                          "bbox": [float(v2[:, 0].min()), float(v2[:, 2].min()), float(v2[:, 0].max()), float(v2[:, 2].max())]}
            if len(secs) >= 4 and len(secs[2]) >= 3:
                hv = secs[2].copy(); hv[:, 0] = -hv[:, 0]
                out["nav"]["height"] = {"verts": hv.round(3).tolist(), "tris": secs[3].tolist()}
        return out

    # ---------- glTF ----------
    def export(self):
        self.parse_scene_refs()
        self.parse_matrefs()
        self.texcache = {}; self.texinfo = {}
        inst = self.walk()
        print("instances:", len(inst), "(active %d)" % sum(1 for i in inst if i["active"]))
        mesh_urls = self.info["meshUrlList"]; mat_urls = self.info["matRefUrlList"]; lm_urls = self.info["lightTexList"]

        # materials
        materials = []; mat_index = {}
        def get_material(mi):
            if mi in mat_index:
                return mat_index[mi]
            url = mat_urls[mi] if 0 <= mi < len(mat_urls) else None
            info = self.resolve_material(url) if url else None
            gi = len(materials)
            if info is None:
                materials.append({"name": "m%d" % gi, "shader": "?", "url": url, "base": None, "alpha": "OPAQUE", "double": False, "kind": "missing"})
                self.log.append("material thieu: %s" % url)
            else:
                sh = info["shader"]
                kind = "terrain" if "地形" in sh else ("water" if "水体" in sh else ("particle" if "particle" in sh.lower() else "static"))
                if "Alpha" in sh or "剪裁" in sh or "测试" in sh:
                    alpha = "MASK"
                elif "透明" in sh or kind in ("water", "particle") or "叠加" in sh:
                    alpha = "BLEND"
                else:
                    alpha = "OPAQUE"
                double = kind in ("water",) or "树叶" in sh or "草" in sh or alpha == "MASK"
                texs = {}
                for k, pth in info["tex"].items():
                    fn = self.export_texture(pth)
                    if fn:
                        texs[k] = {"file": fn, "st": info["st"].get(k, (1, 1, 0, 0))}
                base_key = "_MainTex" if "_MainTex" in texs else ("_Splat0" if "_Splat0" in texs else (next(iter(texs)) if texs else None))
                materials.append({"name": "m%d" % gi, "shader": sh, "url": url, "unity_name": info["name"], "kind": kind, "alpha": alpha,
                                  "cutoff": float(info["floats"].get("_Cutoff", 0.5)), "double": double,
                                  "color": list(info["colors"].get("_Color", (1, 1, 1, 1))), "base": base_key, "tex": texs,
                                  "floats": {k: float(v) for k, v in info["floats"].items()}})
            mat_index[mi] = gi
            return gi

        # lightmaps
        lightmaps = []
        for i, u in enumerate(lm_urls):
            fn = self.export_texture(u)
            lightmaps.append(fn)

        # meshes: key (mesh url idx, tuple(mats))
        meshes = []; mesh_index = {}; mesh_cache = {}
        buf = bytearray(); views = []; accessors = []
        def add_view(arr, target):
            arr = np.ascontiguousarray(arr)
            off = len(buf)
            buf.extend(arr.tobytes())
            while len(buf) % 4:
                buf.append(0)
            views.append({"buffer": 0, "byteOffset": off, "byteLength": arr.nbytes, "target": target})
            return len(views) - 1
        def add_accessor(arr, ctype, comp, target, minmax=False):
            vi = add_view(arr, target)
            acc = {"bufferView": vi, "componentType": ctype, "count": int(arr.shape[0]), "type": comp}
            if minmax:
                acc["min"] = [float(x) for x in arr.min(axis=0)]; acc["max"] = [float(x) for x in arr.max(axis=0)]
            accessors.append(acc)
            return len(accessors) - 1
        def get_mesh(mi, mats):
            key = (mi, tuple(mats))
            if key in mesh_index:
                return mesh_index[key]
            if mi not in mesh_cache:
                url = mesh_urls[mi] if 0 <= mi < len(mesh_urls) else None
                mo = self.resolve_mesh(url) if url else None
                if mo is None:
                    self.log.append("mesh thieu: %s" % url)
                    mesh_cache[mi] = None
                else:
                    try:
                        mesh_cache[mi] = self.mesh_data(mo)
                    except Exception as e:
                        self.log.append("mesh loi %s: %s" % (url, e))
                        mesh_cache[mi] = None
            md = mesh_cache[mi]
            if md is None:
                mesh_index[key] = None
                return None
            pos = md["pos"].copy(); pos[:, 0] = -pos[:, 0]
            prims = []
            a_pos = add_accessor(pos.astype(np.float32), 5126, "VEC3", 34962, True)
            attrs = {"POSITION": a_pos}
            if md["nor"] is not None and len(md["nor"]) == len(pos):
                nor = md["nor"].copy(); nor[:, 0] = -nor[:, 0]
                attrs["NORMAL"] = add_accessor(nor.astype(np.float32), 5126, "VEC3", 34962)
            if md["uv0"] is not None and len(md["uv0"]) == len(pos):
                uv = md["uv0"].copy(); uv[:, 1] = 1.0 - uv[:, 1]
                attrs["TEXCOORD_0"] = add_accessor(uv.astype(np.float32), 5126, "VEC2", 34962)
            if md["uv1"] is not None and len(md["uv1"]) == len(pos):
                attrs["TEXCOORD_1"] = add_accessor(md["uv1"].astype(np.float32), 5126, "VEC2", 34962)  # giu nguyen: shader lightmap tu lat V
            for si, tri in enumerate(md["subs"]):
                if len(tri) == 0:
                    continue
                t = tri[:, [0, 2, 1]].astype(np.uint32)  # dao chieu vi da lat truc X
                a_idx = add_accessor(t.reshape(-1), 5125, "SCALAR", 34963)
                mat_i = mats[si] if si < len(mats) else (mats[0] if mats else None)
                prim = {"attributes": attrs, "indices": a_idx, "mode": 4}
                if mat_i is not None:
                    prim["material"] = get_material(mat_i)
                prims.append(prim)
            gi = len(meshes)
            meshes.append({"name": md["name"], "primitives": prims})
            mesh_index[key] = gi
            return gi

        nodes = []; node_meta = []
        for it in inst:
            if not it["active"]:
                continue
            gm = get_mesh(it["mesh"], it["mats"])
            if gm is None:
                continue
            W = FLIP @ it["world"] @ FLIP
            ni = len(nodes)
            nodes.append({"name": "g%d" % ni, "mesh": gm, "matrix": [float(x) for x in W.T.reshape(-1)]})
            meta = {"group": it["group"], "src": it["name"]}
            if it.get("layer", 0):
                # the Unity layer: CullDistances of the scene cuts layers 14 (small props) / 15 (trees) / 16 (canopies) by distance
                meta["layer"] = int(it["layer"])
            if it["lm"] is not None and it["lm"][0] < len(lightmaps) and lightmaps[it["lm"][0]]:
                meta["lm"] = [int(it["lm"][0])] + [round(float(x), 6) for x in it["lm"][1:]]
            node_meta.append(meta)
        root = {"name": self.scene, "children": list(range(len(nodes)))}
        nodes.append(root)

        # images/textures/materials glTF
        images = []; textures = []; img_index = {}
        def tex_index(fn):
            if fn not in img_index:
                images.append({"uri": fn.replace("\\", "/")})
                textures.append({"source": len(images) - 1, "sampler": 0})
                img_index[fn] = len(textures) - 1
            return img_index[fn]
        gmats = []
        for m in materials:
            gm = {"name": m["name"], "pbrMetallicRoughness": {"metallicFactor": 0.0, "roughnessFactor": 1.0, "baseColorFactor": [1, 1, 1, 1]},
                  "doubleSided": bool(m["double"]), "alphaMode": m["alpha"]}
            if m["alpha"] == "MASK":
                gm["alphaCutoff"] = m.get("cutoff", 0.5)
            if m.get("base") and m["tex"].get(m["base"]):
                gm["pbrMetallicRoughness"]["baseColorTexture"] = {"index": tex_index(m["tex"][m["base"]]["file"])}
            if m["kind"] == "water":
                gm["pbrMetallicRoughness"]["baseColorFactor"] = [0.35, 0.55, 0.75, 0.55]
            gmats.append(gm)

        bin_name = self.scene + ".bin"
        with open(os.path.join(self.out, bin_name), "wb") as f:
            f.write(bytes(buf))
        gltf = {"asset": {"version": "2.0", "generator": "jxnext scn3d export"}, "scene": 0, "scenes": [{"nodes": [len(nodes) - 1]}],
                "nodes": nodes, "meshes": meshes, "materials": gmats, "images": images, "textures": textures,
                "samplers": [{"magFilter": 9729, "minFilter": 9987, "wrapS": 10497, "wrapT": 10497}],
                "accessors": accessors, "bufferViews": views, "buffers": [{"uri": bin_name, "byteLength": len(buf)}]}
        with io.open(os.path.join(self.out, self.scene + ".gltf"), "w", encoding="utf-8") as f:
            json.dump(gltf, f)
        table = self.scn_table()
        marks = self.marks(table.get("sl_markpath", self.scene)) if table else {}
        rs = self.render_setting()
        scene_json = {"scene": self.scene, "table": table, "render": rs, "lightmaps": lightmaps, "materials": materials,
                      "nodes": node_meta, "marks": marks, "textures": self.texinfo, "effects": getattr(self, "effects", []),
                      "stats": {"instances": len(nodes) - 1, "meshes": len(meshes), "materials": len(materials), "buffer_bytes": len(buf)}}
        with io.open(os.path.join(self.out, "scene.json"), "w", encoding="utf-8") as f:
            json.dump(scene_json, f, ensure_ascii=False, indent=1)
        with io.open(os.path.join(self.out, "export.log"), "w", encoding="utf-8") as f:
            f.write("\n".join(self.log))
        print("xuat: %d node, %d mesh, %d material, %d texture, buffer %.1f MB, %d canh bao -> %s" % (
            len(nodes) - 1, len(meshes), len(materials), len(self.texinfo), len(buf) / 1e6, len(self.log), self.out))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scene", help="vd world_baling")
    ap.add_argument("--src", default=os.environ.get("JX_SCN3D_SRC", r"D:\game3gTQ_mo\pc\剑网江湖_Data\StreamingAssets"))
    ap.add_argument("--key-file", default=os.environ.get("JX_SCN3D_KEY", r"D:\game3gTQ_mo\khoa_bundle.txt"))
    ap.add_argument("--out", default=None)
    ap.add_argument("--marks-only", action="store_true", help="chi doc lai mark/navmesh (marks cua scene.json), giu glTF")
    a = ap.parse_args()
    key = open(a.key_file, "r", encoding="utf-8").read().strip()
    out = a.out or os.path.join(NEXT, "client", "assets3d", a.scene)
    ex = Exporter(a.scene, a.src, key, out)
    if a.marks_only:
        sj = os.path.join(out, "scene.json")
        scene_json = json.load(io.open(sj, encoding="utf-8"))
        table = scene_json.get("table") or ex.scn_table()
        scene_json["marks"] = ex.marks(table.get("sl_markpath", a.scene)) if table.get("sl_markpath") else scene_json.get("marks", {})
        with io.open(sj, "w", encoding="utf-8") as f:
            json.dump(scene_json, f, ensure_ascii=False, indent=1)
        h = scene_json["marks"].get("nav", {}).get("height", {})
        print("marks: %d diem, %d vung, nav %d dinh, height mesh %d dinh / %d tam giac -> %s" % (
            len(scene_json["marks"].get("points", {})), len(scene_json["marks"].get("areas", [])), len(scene_json["marks"].get("nav", {}).get("verts", [])),
            len(h.get("verts", [])), len(h.get("tris", [])), sj))
        return
    ex.load()
    ex.export()


if __name__ == "__main__":
    main()
