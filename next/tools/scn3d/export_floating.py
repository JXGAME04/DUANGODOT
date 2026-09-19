# -*- coding: utf-8 -*-
"""Xuat bang kieu chu bay (so sat thuong, ten ky nang) cua bo tham khao: MonoBehaviour FloatingText trong prefab TopRoot (bundle UI
NGUI f799b063c668) -> client/assets3d/ui/floating_text.json.

TopRoot.ShowHpChg 0x5a3550 [TK]: chi mang -> kieu 10 (重击掉血), khong thi kieu 1/0 (暴击/普通掉血) theo tham so, mau tu mat ->
kieu 8 (自己掉血); ShowSkillName 0x5a3f80: kieu 3..7 theo ngu hanh (kim moc thuy hoa tho), 13 chung; ShowHitMiss: 2 (闪避).
Moi kieu: co chu, gradient (tren/duoi), hieu ung vien (0 khong 1 bong 2 vien), mau, mau vien, lech ban dau (px NGUI, y len),
4 duong cong theo giay: lech y, lech x, alpha, ti le.
    python tools/scn3d/export_floating.py
"""
import io
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)


def main():
    import UnityPy
    key = io.open(os.environ.get("JX_SCN3D_KEY", r"D:\game3gtQ_mo\khoa_bundle.txt"), encoding="utf-8").read().strip().split()[0]
    UnityPy.set_assetbundle_decrypt_key(key)
    from export_scene import Raw
    src = os.environ.get("JX_SCN3D_SRC", r"D:\game3gtQ_mo\pc\剑网江湖_Data\StreamingAssets")
    env = UnityPy.load(os.path.join(src, "f799b063c668.bdd"))
    objs = {o.path_id: o for o in env.objects}
    out = []
    for o in env.objects:
        if o.type.name != "MonoBehaviour":
            continue
        raw = o.get_raw_data()
        r = Raw(raw)
        go, sc = r.header()
        so = objs.get(sc)
        if so is None or so.type.name != "MonoScript" or so.read().m_ClassName != "FloatingText":
            continue
        n = r.i32()
        for i in range(n):
            name = r.string(); text = r.string(); r.pptr(); grp = r.i32(); size = r.i32()
            grad = r.u8(); r.align()
            top = [r.f32() for _ in range(4)]; bot = [r.f32() for _ in range(4)]
            eff = r.i32()
            col = [r.f32() for _ in range(4)]; ecol = [r.f32() for _ in range(4)]
            off = [r.f32() for _ in range(3)]
            maxt = r.i32()
            curves = []
            for _ in range(4):
                cnt = r.i32(); ks = []
                for _ in range(cnt):
                    ks.append([round(r.f32(), 4), round(r.f32(), 4)]); r.f32(); r.f32(); r.i32(); r.f32(); r.f32()
                r.i32(); r.i32(); r.i32()
                curves.append(ks)
            out.append({"id": i, "name": name, "text": text, "size": size, "gradient": bool(grad), "top": top, "bottom": bot,
                        "effect": eff, "color": col, "effect_color": ecol, "offset": off, "max": maxt,
                        "y": curves[0], "x": curves[1], "alpha": curves[2], "scale": curves[3]})
        break
    dst = os.path.join(NEXT, "client", "assets3d", "ui")
    os.makedirs(dst, exist_ok=True)
    with io.open(os.path.join(dst, "floating_text.json"), "w", encoding="utf-8") as f:
        json.dump({"formats": out, "source": "FloatingText (TopRoot prefab, bundle f799b063c668) [TK]"}, f, ensure_ascii=False, indent=1)
    print("%d kieu chu bay -> %s" % (len(out), os.path.join(dst, "floating_text.json")))


if __name__ == "__main__":
    main()
