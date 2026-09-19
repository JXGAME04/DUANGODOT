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


# NGUI UITweener (ban sua cua bo tham khao) - bo cuc byte doc theo thu tu khai bao truong trong global-metadata [TK]:
# method (Linear 0, EaseIn 1, EaseOut 2, EaseInOut 3, BounceIn 4, BounceOut 5), style (Once 0, Loop 1, PingPong 2),
# ignoreTimeScale, delay, duration, steeperCurves, tweenGroup, onFinished (List<EventDelegate>), eventReceiver (PPtr),
# callWhenFinished (string), animationCurve (Keyframe[] {time, value, in, out, weightedMode, inW, outW} + pre/post + order);
# roi lop con: TweenRotation from/to Vector3 + quaternionLerp; TweenScale from/to + updateTable; TweenPosition from/to +
# sourceFrom/targetTo PPtr + worldSpace; TweenAlpha from/to float; TweenColor from/to Color + bEmissive; TweenEnhance from/to.
# Kiem: TweenScale 180 byte, TweenRotation 180, TweenPosition 204, TweenColor 188 - dung bang tong cac truong.
TWEEN_KIND = {"TweenRotation": "rotation", "TweenScale": "scale", "TweenPosition": "position", "TweenAlpha": "alpha",
              "TweenColor": "color", "TweenEnhance": "enhance"}


def read_tween(cls, raw):
    r = Raw(raw); r.header()
    method = r.i32(); style = r.i32(); r.u8(); r.align(); delay = r.f32(); duration = r.f32(); r.u8(); r.align(); r.i32()
    n_fin = r.i32()
    if n_fin != 0:
        return None   # EventDelegate list: bo cuc dai, khong can cho hieu ung
    r.pptr(); r.string()
    nk = r.i32()
    keys = []
    for _ in range(nk):
        t = r.f32(); v = r.f32(); r.f32(); r.f32(); r.i32(); r.f32(); r.f32()
        keys.append([round(t, 4), round(v, 4)])
    r.i32(); r.i32(); r.i32()
    kind = TWEEN_KIND[cls]
    if kind in ("rotation", "scale", "position"):
        a = [r.f32(), r.f32(), r.f32()]; b = [r.f32(), r.f32(), r.f32()]
        if kind == "rotation":
            a = [a[0], -a[1], -a[2]]; b = [b[0], -b[1], -b[2]]   # doi truc X: goc quay Y/Z dao dau
        elif kind == "position":
            a = [-a[0], a[1], a[2]]; b = [-b[0], b[1], b[2]]
    elif kind == "color":
        a = [r.f32(), r.f32(), r.f32(), r.f32()]; b = [r.f32(), r.f32(), r.f32(), r.f32()]
    else:
        a = r.f32(); b = r.f32()
    out = {"type": kind, "method": method, "style": style, "delay": delay, "duration": duration, "from": a, "to": b}
    if kind == "color":
        out["emissive"] = bool(r.u8())
    linear = len(keys) == 2 and keys[0] == [0.0, 0.0] and keys[1] == [1.0, 1.0]
    if keys and not linear:
        out["curve"] = keys
    return out


# SFXMixerMesh [TK]: mColor + List<MixLayer> (enable, supportAnim, mtype, mArtColor, TexTrans (nx, ny, ox, oy - o tu tren),
# mPosOffset, mRotSpeed (rad/s), mRotCurve, mScaleType (0 tinh, 1 sin, 2 rang cua), mScaleSpeed, mScalePos, mScaleDis,
# mScaleData, mEnhance, mEnhanceCurve); cac truong meshFilter/mesh/... la runtime. Bo cuc kiem bang tong byte (168 = 1 lop).
# Mesh dung luc chay: moi lop mot quad (+-0.5, 0, +-0.5) trong mat XZ, lop sau thap hon 0,01 m; shader sfx_mixer_mesh_rs
# quay/scale (D:\game3gtQ_mo\mixer_mesh.py, disasm .cctor 0x702130, InitMeshData 0x7012d0, FillMeshSquare 0x6ffb30).
# XftWeapon.XWeaponTrail (ban sua cua bo tham khao) - bo cuc byte [TK, kiem 156 byte]: Version (string), UseWith2D + Enabled
# (2 byte), PointStart/PointEnd (PPtr), MaxFrame, Granularity, Fps, AdjustColor, MyColor, EmissiveColor, MyMaterial (PPtr),
# TexTransSplit (int2), TexTransOffset (int2), mTrailWidth (float). Vet keo dai MaxFrame khung o Fps khung/giay.
def read_ptrail(raw, material):
    """PigeonCoopToolkit.Effects.Trails.Trail (vet dai theo dan bay, 5 prefab): TrailRenderer_Base.TrailData {TrailMaterial,
    Lifetime, UsingSimpleSize, SimpleSizeOverLifeStart/End, SizeOverLife (AnimationCurve), UsingSimpleColor, SimpleColorOverLife
    Start/End, ColorOverLife (Gradient 8 mau + 8 ctime + 8 atime + 4 byte dem), StretchSizeToFit, StretchColorToFit,
    MaterialTileLength, UseForwardOverride, ForwardOverride, ForwardOverrideRelative}, Emit, TexTransSplit, TexTransOffset;
    Trail: MinVertexDistance, MaxNumberOfPoints (392 byte, het byte = dung)"""
    r = Raw(raw); r.header()
    mat = r.pptr()
    life = r.f32()
    use_simple_size = r.u8(); r.align()
    ss0 = r.f32(); ss1 = r.f32()
    n = r.i32(); size_keys = []
    for _ in range(n):
        kt = r.f32(); kv = r.f32(); r.f32(); r.f32(); r.i32(); r.f32(); r.f32()
        size_keys.append([round(kt, 4), round(kv, 4)])
    r.i32(); r.i32(); r.i32()
    use_simple_color = r.u8(); r.align()
    c0 = [r.f32() for _ in range(4)]; c1 = [r.f32() for _ in range(4)]
    cols = [[r.f32() for _ in range(4)] for _ in range(8)]
    ct = [r.i16() & 0xffff for _ in range(8)]; at = [r.i16() & 0xffff for _ in range(8)]
    r.i32()   # 4 byte dem (num keys / mode)
    stretch_size = r.u8(); r.align(); stretch_color = r.u8(); r.align()
    tile = r.f32()
    use_fwd = r.u8(); r.align(); fwd = [r.f32() for _ in range(3)]; fwd_rel = r.u8(); r.align()
    emit = r.u8(); r.align()
    split = [r.i32(), r.i32()]; offset = [r.i32(), r.i32()]
    min_dist = r.f32(); max_pts = r.i32()
    if len(raw) - r.p != 0:
        raise struct.error("Trail: con %d byte" % (len(raw) - r.p))
    # gradient keys: colour keys with a rising time, alpha keys the same; a key after the first with time 0 ends the list
    ckeys = [[ct[0] / 65535.0] + [round(c, 4) for c in cols[0][:3]]]
    akeys = [[at[0] / 65535.0, round(cols[0][3], 4)]]
    for i in range(1, 8):
        if ct[i] > ct[i - 1] or (i == 1 and ct[i] > 0):
            ckeys.append([ct[i] / 65535.0] + [round(c, 4) for c in cols[i][:3]])
        if at[i] > at[i - 1] or (i == 1 and at[i] > 0):
            akeys.append([at[i] / 65535.0, round(cols[i][3], 4)])
    return {"life": life, "size": ([[0.0, ss0], [1.0, ss1]] if use_simple_size else size_keys), "color": ([[0.0] + c0[:3], [1.0] + c1[:3]] if use_simple_color else ckeys),
            "alpha": ([[0.0, c0[3]], [1.0, c1[3]]] if use_simple_color else akeys), "stretch_size": bool(stretch_size), "stretch_color": bool(stretch_color),
            "tile": tile, "fwd": fwd if use_fwd else None, "fwd_rel": bool(fwd_rel), "emit": bool(emit), "split": split, "offset": offset,
            "min_dist": min_dist, "max_pts": max_pts, "material": material(mat[1])}


def read_pc2(raw, text_assets):
    """PC2Anim (hoat anh dinh theo tep POINTCACHE2, 1 prefab tm_zhuixinjian): chi uri_pc2 + fps duoc ghi (88 byte); ParsePCFile
    [TK 0x68d4c0]: WBase.AssetMgr.LoadBytes(uri) -> chu ky 12 byte, fileVersion, numPoints (= so dinh mesh), startFrame, sampleRate,
    numSamples, roi numSamples x numPoints x (x, y, z) float -> Unity (-x, y, z) x 0.01; sau khi doc: curFrame 0, m_bLoop 1, m_bPlay 1.
    Update [TK 0x68cdc0]: fps kep 1..60, khung = increaseTime x fps, noi suy tuyen tinh giua mau khung va khung + 1; toi mau cuoi:
    dinh = mau cuoi, increaseTime = 0, dung neu khong lap.  Toa do ghi ra da qua guong Godot (x, y, z) x 0.01."""
    r = Raw(raw); r.header()
    uri = r.string(); fps = r.i32()
    if len(raw) - r.p != 0:
        raise struct.error("PC2Anim: con %d byte" % (len(raw) - r.p))
    base = uri.replace("\\", "/").split("/")[-1]
    if base.endswith(".bytes"):
        base = base[:-6]
    b = text_assets.get(base)
    if b is None:
        raise IndexError("PC2Anim: khong co TextAsset " + base)
    sig = b[:12]
    if not sig.startswith(b"POINTCACHE2"):
        raise struct.error("PC2Anim: chu ky la " + repr(sig))
    ver, npts = struct.unpack_from("<ii", b, 12); sf, sr = struct.unpack_from("<ff", b, 20); ns = struct.unpack_from("<i", b, 28)[0]
    if len(b) < 32 + ns * npts * 12:
        raise struct.error("PC2Anim: tep ngan (%d < %d)" % (len(b), 32 + ns * npts * 12))
    a = np.array(struct.unpack_from("<%df" % (ns * npts * 3), b, 32), dtype=np.float32).reshape(ns, npts, 3) * 0.01
    frames = [[round(float(v), 4) for v in a[i].reshape(-1)] for i in range(ns)]
    return {"uri": base, "fps": max(1, min(60, fps)), "loop": True, "num_points": npts, "num_samples": ns, "start_frame": sf, "sample_rate": sr,
            "frames": frames}


def read_linemesh(raw):
    """SFXLineMesh (duong noi vat con ve diem sinh, su kien 26; 1 prefab 天际迅雷): color, uvNum_X/Y, uvOffset_X/Y, uvGrow, enhance,
    useRandomGrow, uiCurve, _adjustColor, useWorldPos, manualStart, _startPos, _endPos, _startWidth, _normalLength, _textureMode,
    _uvFrom0, _startTrans, _endTrans, showStartAttachNode, showEndAttachNode, yPriorityOffset (232 byte than)"""
    r = Raw(raw); r.header()
    color = [r.f32() for _ in range(4)]
    nx = r.i32(); ny = r.i32(); ox = r.i32(); oy = r.i32(); grow = r.i32(); enhance = r.f32()
    rnd = r.u8(); r.align()
    n = r.i32(); curve = []
    for _ in range(n):
        kt = r.f32(); kv = r.f32(); r.f32(); r.f32(); r.i32(); r.f32(); r.f32()
        curve.append([round(kt, 4), round(kv, 4)])
    r.i32(); r.i32(); r.i32()
    r.f32(); r.f32(); r.f32(); r.f32()   # _adjustColor
    world = r.u8(); r.align(); manual = r.u8(); r.align()
    sp = [r.f32() for _ in range(3)]; ep = [r.f32() for _ in range(3)]
    width = r.f32(); normal_len = r.f32(); tex_mode = r.i32(); uv0 = r.u8(); r.align()
    r.pptr(); r.pptr()
    sa = r.pptr(); ea = r.pptr()
    ypri = r.f32()
    if len(raw) - r.p != 0:
        raise struct.error("SFXLineMesh: con %d byte" % (len(raw) - r.p))
    return {"color": color, "nx": max(1, nx), "ny": max(1, ny), "ox": ox, "oy": oy, "grow": grow, "enhance": enhance, "random": bool(rnd),
            "curve": curve, "world": bool(world), "manual": bool(manual), "start": [-sp[0], sp[1], sp[2]], "end": [-ep[0], ep[1], ep[2]],
            "width": width, "normal_len": normal_len, "tex_mode": tex_mode, "uv_from0": bool(uv0), "start_attach": sa[1], "end_attach": ea[1]}


def read_xtrail(raw, material_of):
    r = Raw(raw); r.header()
    r.string(); r.u8(); r.u8(); r.align()
    r.pptr(); r.pptr()
    max_frame = r.i32(); gran = r.i32(); fps = r.f32()
    adj = [r.f32() for _ in range(4)]; my = [r.f32() for _ in range(4)]; em = [r.f32() for _ in range(4)]
    mat = r.pptr()
    split = [r.i32(), r.i32()]; off = [r.i32(), r.i32()]
    width = r.f32() if r.p + 4 <= len(raw) else 1.0
    return {"max_frame": max_frame, "granularity": gran, "fps": fps, "adjust": adj, "color": my, "emissive": em,
            "cell": [split[0], split[1], off[0], off[1]], "width": width, "material": material_of(mat[1])}


def read_curve_keys(r):
    n = r.i32()
    keys = []
    for _ in range(n):
        t = r.f32(); v = r.f32(); r.f32(); r.f32(); r.i32(); r.f32(); r.f32()
        keys.append([round(t, 4), round(v, 4)])
    r.i32(); r.i32(); r.i32()
    return keys


def read_mixer(raw):
    r = Raw(raw); r.header()
    color = [r.f32() for _ in range(4)]
    n = r.i32()
    layers = []
    for _ in range(n):
        enable = r.u8(); r.align(); anim = r.u8(); r.align()
        mtype = r.i32()
        art = [r.f32() for _ in range(4)]
        tt = [r.f32() for _ in range(4)]
        pos = [r.f32() for _ in range(3)]
        rot = r.f32()
        rotc = read_curve_keys(r)
        st = r.i32(); ss = r.f32(); sp = r.f32(); sd = r.f32(); sdata = r.f32(); enh = r.f32()
        enhc = read_curve_keys(r)
        if not enable:
            continue
        layers.append({"anim": bool(anim), "type": mtype, "color": art, "cell": tt, "pos": [-pos[0], pos[1], pos[2]], "rot": rot,
                       "rot_curve": rotc, "scale_type": st, "scale_speed": ss, "scale_pos": sp, "scale_dis": sd, "scale_data": sdata,
                       "enhance": enh, "enhance_curve": enhc})
    return {"color": color, "layers": layers}


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
        if not ks:
            # a curve mode without keys: the particle system takes the scalar as the constant (cmn_yanwu frameOverTime 0.937
            # = atlas cell 239, the soft dot; an empty AnimationCurve would give 0 = the black cell)
            return {"min": s, "max": s, "curve": None}
        vals = [k[1] for k in ks] or [s]
        return {"min": min(vals), "max": max(vals), "curve": ks}
    ks = keys(v.get("maxCurve", {})); ks2 = keys(v.get("minCurve", {}))
    if not ks and not ks2:
        return {"min": s, "max": s, "curve": None}
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
        # the .pc2 point caches (TextAsset <ten>.pc2, bytes) PC2Anim loads by uri
        self.text_assets = {}
        for o in self.env.objects:
            if o.type.name == "TextAsset":
                ta = o.read()
                sc = ta.m_Script
                self.text_assets[ta.m_Name] = sc if isinstance(sc, (bytes, bytearray)) else sc.encode("utf-8", "surrogateescape")
        self.log = []

    # ten goc cua container da bam md5: prefab goc (Transform khong cha) + thu muc doan theo hash [TK]: Skill 210, Cmn 49,
    # State 33, Daoguang 29, Halo 12, UI 9 (343 prefab goc trong 558 container - con lai la texture/material)
    FOLDERS = ("Skill", "Cmn", "State", "Daoguang", "Halo", "UI", "Npc", "Boss", "Scene", "Other")

    def all_prefabs(self):
        inv = {pid: k for k, pid in self.cont.items()}
        out = []
        for o in self.env.objects:
            if o.type.name != "Transform":
                continue
            t = o.read_typetree()
            if t["m_Father"]["m_PathID"] != 0:
                continue
            gid = t["m_GameObject"]["m_PathID"]
            if gid not in self.objs or self.objs[gid].type.name != "GameObject":
                continue
            name = self.objs[gid].read().m_Name
            key_ = inv.get(gid)
            rel = None
            for f in self.FOLDERS:
                for ext in (".prefab", ""):
                    if h12("assets/particles/%s/%s%s" % (f, name, ext)) == key_:
                        rel = "%s/%s" % (f, name); break
                if rel:
                    break
            out.append(rel or ("Other/" + name))
        return sorted(set(out))

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
        out = {"name": m.m_Name, "shader": shader, "blend": blend, "tex": tex, "st": st, "tint": tint,
               "enhance": floats.get("_Enhance", 1.0), "cutoff": floats.get("_Cutoff", 0.0)}
        if "rimlight" in shader.lower():
            # blend_dst_zw_ver_rimlight [TK GLSL shaders_apk]: rgb = mix(tex.rgb x enhance, vcol.rgb x _EdgeColor x (vcol.a x 25 + 1),
            # pow(1 - N.V, _RimPower)), alpha = tex.a x _AdjustA (discard < 0.02)
            out["edge"] = colors.get("_EdgeColor", [1, 1, 1, 1])
            out["rim_power"] = floats.get("_RimPower", 2.0)
            out["adjust_a"] = floats.get("_AdjustA", 1.0)
        return out

    # ---------- glTF ghi mesh ----------
    def export(self, rel):
        """rel: vd 'Skill/wd_nuleizhi' -> (json, gltf) hoac None"""
        pid = None
        for cnd in ("assets/particles/%s.prefab" % rel, "assets/particles/%s" % rel):
            pid = self.cont.get(h12(cnd))
            if pid is not None:
                break
        if pid is None and rel.startswith("Other/"):
            # thu muc khong doan duoc: tim prefab goc theo ten
            for o in self.env.objects:
                if o.type.name == "GameObject" and o.read().m_Name == rel[6:]:
                    pid = o.path_id; break
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

        bind_poses = {}   # mesh pid -> [4x4 bind matrices] (a skinned mesh: SkinnedMeshRenderer, the dragon of 飞龙在天)

        def mesh_index(mesh_pid, uvmod=None, skinned=False):
            key = (mesh_pid, json.dumps(uvmod, sort_keys=True) if uvmod else "", skinned)
            if key in mesh_cache:
                return mesh_cache[key]
            mo = self.objs.get(mesh_pid)
            if mo is None or mo.type.name != "Mesh":
                mesh_cache[key] = None; return None
            m = mo.read(); hd = MeshHandler(m); hd.process()
            nv = m.m_VertexData.m_VertexCount
            def arr(a, want, dtype=np.float32):
                if not a:
                    return None
                x = np.array(a, dtype=dtype).reshape(nv, -1)
                if x.shape[1] < want:
                    x = np.concatenate([x, np.zeros((nv, want - x.shape[1]), dtype=dtype)], axis=1)
                return x[:, :want]
            pos = arr(hd.m_Vertices, 3); nor = arr(hd.m_Normals, 3); uv0 = arr(hd.m_UV0, 2); col = arr(getattr(hd, "m_Colors", None), 4)
            P = pos.copy(); P[:, 0] = -P[:, 0]
            attrs = {"POSITION": add_accessor(P, 5126, "VEC3", 34962, True)}
            if skinned:
                # the weights / bone indices the way export_npc.skinned_mesh reads them (the vertex channels, else m_Skin)
                bw = arr(getattr(hd, "m_BoneWeights", None), 4)
                bi = arr(getattr(hd, "m_BoneIndices", None), 4, np.int64)
                if bw is None and m.m_Skin:
                    bw = np.array([[w.weight_0_, w.weight_1_, w.weight_2_, w.weight_3_] for w in m.m_Skin], dtype=np.float32)
                    bi = np.array([[w.boneIndex_0_, w.boneIndex_1_, w.boneIndex_2_, w.boneIndex_3_] for w in m.m_Skin], dtype=np.int64)
                if bw is not None and bi is not None:
                    s_ = bw.sum(axis=1, keepdims=True); s_[s_ == 0] = 1.0
                    attrs["WEIGHTS_0"] = add_accessor((bw / s_).astype(np.float32), 5126, "VEC4", 34962)
                    attrs["JOINTS_0"] = add_accessor(np.clip(bi, 0, 65535).astype(np.uint16), 5123, "VEC4", 34962)
                bind_poses[mesh_pid] = [np.array([[getattr(bp, "e%d%d" % (i, j)) for j in range(4)] for i in range(4)], dtype=np.float64)
                                        for bp in m.m_BindPose]
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
                # UnityPy hands UNorm8 vertex colours as 0..255: glTF wants 0..1 (25 meshes came out white)
                if float(col.max()) > 1.5:
                    col = col / 255.0
                attrs["COLOR_0"] = add_accessor(col.astype(np.float32), 5126, "VEC4", 34962)
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
            if depth == 0:
                # the prefab root's own transform (its editor placement: 236 of 343 roots are off, up to 337 m for the waterfall
                # spray, 90 deg turns for cmn_ma_tui_*) is dropped when the pool instantiates it - GameNodePool.TryInstantiateNode
                # 0x6eb770 parents the instance with localPosition zero / localRotation identity / localScale one [TK]
                gnodes.append({"name": g.m_Name, "translation": [0.0, 0.0, 0.0], "rotation": [0.0, 0.0, 0.0, 1.0], "scale": [1.0, 1.0, 1.0]})
            else:
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
                if cls in TWEEN_KIND:
                    try:
                        tw = read_tween(cls, raw)
                    except (struct.error, IndexError, KeyError):
                        tw = None
                    if tw is not None:
                        jn.setdefault("tweens", []).append(tw)
                if cls == "PC2Anim":
                    try:
                        jn["pc2"] = read_pc2(raw, self.text_assets)
                    except (struct.error, IndexError) as e:
                        self.log.append("PC2Anim khong doc duoc: %s (%s)" % (g.m_Name, e))
                if cls == "SFXLineMesh":
                    try:
                        lm = read_linemesh(raw)
                        # the material of the MeshRenderer beside it (the line builds its own mesh at run time)
                        if comps.get("MeshRenderer"):
                            mr0 = comps["MeshRenderer"][0].read()
                            mats0 = [self.material(m.path_id) for m in mr0.m_Materials]
                            lm["material"] = mats0[0] if mats0 and mats0[0] else None
                        jn["linemesh"] = lm
                        pending_attach.append((gi, lm["start_attach"], lm["end_attach"]))
                    except (struct.error, IndexError):
                        self.log.append("SFXLineMesh khong doc duoc: " + g.m_Name)
                if cls == "Trail":
                    try:
                        jn["ptrail"] = read_ptrail(raw, self.material)
                    except (struct.error, IndexError):
                        self.log.append("Trail khong doc duoc: " + g.m_Name)
                if cls == "XWeaponTrail":
                    try:
                        jn["xtrail"] = read_xtrail(raw, self.material)
                    except (struct.error, IndexError):
                        self.log.append("XWeaponTrail khong doc duoc: " + g.m_Name)
                if cls == "SFXBillboardHelper":
                    # Billboards[] {Trans, Billboard (0 Billboard 1 RotBillboardY 2 NoRotPos 3 Horizontal 4 Vertical 5 RotLocalBillboardZ),
                    # EulerAngle, PosOffset, PosTrans, 4 bools}, GlobalPosOffset, FitOwnSizeBound, mUpdate (128 instances, 0 bytes left)
                    try:
                        r = Raw(raw); r.header()
                        nbb = r.i32()
                        ents = []
                        for _ in range(nbb):
                            t = r.pptr(); mode = r.i32(); e = [r.f32(), r.f32(), r.f32()]; po = [r.f32(), r.f32(), r.f32()]; pt = r.pptr()
                            for _ in range(4):
                                r.u8(); r.align()
                            ents.append((t[1], mode, e, po, pt[1]))
                        gpo = [r.f32(), r.f32(), r.f32()]
                        for t_pid, mode, e, po, pt_pid in ents:
                            pending_bb.append((t_pid, mode, e, [po[0] + gpo[0], po[1] + gpo[1], po[2] + gpo[2]], pt_pid))
                    except (struct.error, IndexError):
                        self.log.append("SFXBillboardHelper khong doc duoc: " + g.m_Name)
                if cls == "SFXMixerMesh":
                    try:
                        jn["mixer"] = read_mixer(raw)
                    except (struct.error, IndexError):
                        self.log.append("SFXMixerMesh khong doc duoc: " + g.m_Name)
                    if comps.get("MeshRenderer"):
                        mr0 = comps["MeshRenderer"][0].read()
                        mats0 = [self.material(m.path_id) for m in mr0.m_Materials]
                        if mats0 and mats0[0]:
                            jn["mixer_material"] = mats0[0]
                if cls == "SFXMeshModify":
                    # fields in metadata order (418 instances check out by the 16 trailing bytes): shareMesh, color, emissive,
                    # uvNum_X/Y, uvOffset_X/Y, uvGrow, enhance, uiCurve (AnimationCurve), useSingleLerpGrow, useVertexColor,
                    # useRandomGrow, useMask, maskNum_X/Y, maskOffset_X/Y (useMask is 0 everywhere: no mask dissolve in use)
                    r = Raw(raw); r.header(); r.pptr()
                    color = [r.f32(), r.f32(), r.f32(), r.f32()]; emissive = [r.f32(), r.f32(), r.f32(), r.f32()]
                    nx = r.u8(); r.align(); ny = r.u8(); r.align(); ox = r.u8(); r.align(); oy = r.u8(); r.align()
                    uvgrow = r.f32(); enhance = r.f32()
                    ncurve = r.i32(); curve = []
                    for _ in range(ncurve):
                        kt = r.f32(); kv = r.f32(); r.f32(); r.f32(); r.i32(); r.f32(); r.f32()
                        curve.append([round(kt, 4), round(kv, 4)])
                    r.i32(); post_wrap = r.i32(); r.i32()
                    lerp = r.u8(); r.u8(); r.u8(); r.u8(); r.align()
                    nx = max(1, nx); ny = max(1, ny)
                    # SFXMeshModify.Update [TK 0x6fdb70]: the cell = frame uvGrow + uvOffset_X counted across the columns then down
                    # the rows (col = f % nx, row = f / nx + uvOffset_Y) - with useSingleLerpGrow the column is ox + uvGrow unwrapped
                    # (a continuous scroll along u); u' = u x 1/nx + col/nx, v' = v x 1/ny + 1 - (row + 1)/ny
                    if lerp:
                        col = ox + uvgrow; row = float(oy)
                    else:
                        f = int(uvgrow) + ox
                        col = float(f % nx); row = float(f // nx + oy)
                    uvmod = [nx, ny, col, row]
                    sfxcol = {"color": color, "emissive": emissive, "uvgrow": uvgrow, "enhance": enhance, "nx": nx, "ny": ny}
                    if len(curve) > 1:
                        # the cell animates: uiCurve(time) gives uvGrow (looping when the post wrap is 2 = Loop), whole frames
                        # unless useSingleLerpGrow; Scn3DSfx shifts the material's uv offset by the cell difference
                        sfxcol["uv_anim"] = {"lerp": int(lerp), "ox": ox, "oy": oy, "curve": curve, "loop": post_wrap == 2, "col0": col, "row0": row}
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
            # skinned mesh (SkinnedMeshRenderer: the dragon model_long_001 of the Cai Bang skills, bones = Transforms of the
            # prefab animated by the Animation clip): the skin is made after the walk, when every bone has its node
            for co in comps.get("SkinnedMeshRenderer", []):
                smr = co.read()
                mpid = smr.m_Mesh.path_id
                if not mpid and sfxcol is not None:
                    # the bundle strips the renderer's mesh: SFXMeshModify.shareMesh (the first PPtr after the header) holds it
                    raw = comps["MonoBehaviour"][0].get_raw_data(); r = Raw(raw); r.header(); mpid = r.pptr()[1]
                mi = mesh_index(mpid, uvmod, True) if mpid else None
                if mi is None:
                    continue
                gnodes[gi]["mesh"] = mi
                mats = [self.material(m.path_id) for m in smr.m_Materials]
                jn["mesh_material"] = mats[0] if mats and mats[0] else None
                jn["skinned"] = True
                if sfxcol:
                    jn["mesh_sfx"] = sfxcol
                bone_go = []
                for b in smr.m_Bones:
                    bt = self.objs.get(b.path_id)
                    bone_go.append(bt.read().m_GameObject.path_id if bt is not None and bt.type.name == "Transform" else None)
                rb = self.objs.get(smr.m_RootBone.path_id) if getattr(smr, "m_RootBone", None) is not None else None
                root_go = rb.read().m_GameObject.path_id if rb is not None and rb.type.name == "Transform" else None
                pending_skins.append((gi, mpid, bone_go, root_go, g.m_Name))
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
        pending_skins = []
        pending_bb = []
        pending_attach = []
        walk(pid, None, 0)
        for gi_, sa_pid, ea_pid in pending_attach:
            # the attach nodes are GameObjects: their glTF node index
            jnodes[gi_]["linemesh"]["start_attach"] = by_pid.get(sa_pid, -1)
            jnodes[gi_]["linemesh"]["end_attach"] = by_pid.get(ea_pid, -1)
        # SFXBillboardHelper.Execute [TK 0x6f5bf0]: the Trans of each entry turns to the camera every frame; written on that node
        for t_pid, mode, e, po, pt_pid in pending_bb:
            tgo = self.objs.get(t_pid)
            pgo = self.objs.get(pt_pid)
            tg = tgo.read().m_GameObject.path_id if tgo is not None and tgo.type.name == "Transform" else None
            pg = pgo.read().m_GameObject.path_id if pgo is not None and pgo.type.name == "Transform" else None
            if tg in by_pid and by_pid[tg] < len(jnodes):
                jnodes[by_pid[tg]]["billboard"] = {"mode": mode, "euler": e, "offset": po, "pos_node": by_pid.get(pg, -1) if pg is not None else -1}
        skins = []
        for gi, mpid, bone_go, root_go, gname in pending_skins:
            joints = []; ibm = []
            binds = bind_poses.get(mpid, [])
            for bi_, bgo in enumerate(bone_go):
                ni = by_pid.get(bgo)
                if ni is None:
                    self.log.append("xuong cua SkinnedMeshRenderer ngoai prefab: %s" % gname)
                    ni = 0
                joints.append(ni)
                M = binds[bi_] if bi_ < len(binds) else np.eye(4)
                Mg = FLIP @ M @ FLIP
                ibm.append(Mg.T.reshape(-1))
            if not joints:
                continue
            skin = {"joints": joints, "inverseBindMatrices": add_accessor(np.array(ibm, dtype=np.float32), 5126, "MAT4"), "name": gname}
            if root_go in by_pid:
                skin["skeleton"] = by_pid[root_go]
            skins.append(skin)
            gnodes[gi]["skin"] = len(skins) - 1

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
        if skins:
            gltf["skins"] = skins
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
        # moi prefab goc cua particles.bdd (ten thu muc doan lai tu hash) + moi duong dan hai bang goi
        paths = sorted(set(ex.all_prefabs() + [v["path"] for v in index["childobj"].values() if v["path"]] + [v["path"] for v in index["sfx"].values() if v["path"]]))
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
