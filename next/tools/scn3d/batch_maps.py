# -*- coding: utf-8 -*-
"""Dung hang loat map 3D tu bo tham khao (MO-NHI-PHAN-3D F7): voi moi scene cua scn_list co bundle, chay
    export_scene.py <scene>  ->  export_npc.py --map <scene>  ->  make_map3d.py <scene> --id <9052 + id scene>
(Ba Lang scene 1 = 9053, Vinh Lac Tran 2 = 9054 ...). Bo qua scene da co client/assets3d/maps/<id>/map3d.json tru khi --force.

    python tools/scn3d/batch_maps.py --list                 # cac scene va id map se dung
    python tools/scn3d/batch_maps.py world_daoxiangcun       # mot vai scene
    python tools/scn3d/batch_maps.py --all [--force]         # tat ca (moi map 10 s .. 2 phut)
Ghi build/batch_maps.log. Spawn = mark BeginPoint01 hoac BeginPoint (cot 9 scn_list); map khong co mark sinh -> bo qua.
"""
import argparse
import io
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
NEXT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
MAP_ID_BASE = 9052


def scenes():
    import UnityPy
    key = io.open(os.environ.get("JX_SCN3D_KEY", r"D:\game3gtQ_mo\khoa_bundle.txt"), encoding="utf-8").read().strip().split()[0]
    UnityPy.set_assetbundle_decrypt_key(key)
    from export_npc import Tables
    from export_scene import bundle_file
    src = os.environ.get("JX_SCN3D_SRC", r"D:\game3gtQ_mo\pc\剑网江湖_Data\StreamingAssets")
    t = Tables(src)
    out = []
    seen = set()
    for r in t.t["scn_list_cmn"][1:]:
        if len(r) < 10 or not r[0].strip().isdigit() or not r[4].strip():
            continue
        name = r[4].strip()
        if name in seen:
            continue
        seen.add(name)
        if not os.path.exists(bundle_file(src, "scenes_" + name)):
            continue
        out.append({"id": int(r[0]), "scene": name, "cn": r[1].strip(), "spawn": r[9].strip() or "BeginPoint01", "map_id": MAP_ID_BASE + int(r[0])})
    return out


def run(cmd, log):
    log.write("$ " + " ".join(cmd) + "\n")
    log.flush()
    env = dict(os.environ, PYTHONIOENCODING="utf-8")
    p = subprocess.run([sys.executable] + cmd, cwd=NEXT, capture_output=True, text=True, encoding="utf-8", errors="replace", env=env)
    log.write(p.stdout[-4000:] + p.stderr[-4000:] + "\n")
    log.flush()
    return p.returncode == 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("names", nargs="*")
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--force", action="store_true")
    a = ap.parse_args()
    lst = scenes()
    if a.list:
        for s in lst:
            print("%4d  %-28s %-14s -> map %d" % (s["id"], s["scene"], s["cn"], s["map_id"]))
        print(len(lst), "scene co bundle")
        return
    want = lst if a.all else [s for s in lst if s["scene"] in a.names]
    os.makedirs(os.path.join(NEXT, "build"), exist_ok=True)
    log = io.open(os.path.join(NEXT, "build", "batch_maps.log"), "a", encoding="utf-8")
    ok = skip = fail = 0
    for s in want:
        out = os.path.join(NEXT, "client", "assets3d", "maps", str(s["map_id"]), "map3d.json")
        if os.path.exists(out) and not a.force:
            skip += 1
            continue
        t0 = time.time()
        steps = [["tools/scn3d/export_scene.py", s["scene"]],
                 ["tools/scn3d/export_npc.py", "--map", s["scene"]],
                 ["tools/scn3d/make_map3d.py", s["scene"], "--id", str(s["map_id"]), "--spawn", s["spawn"]]]
        good = all(run(c, log) for c in steps)
        print("%-28s map %d %s (%.0f s)" % (s["scene"], s["map_id"], "OK" if good else "LOI (xem build/batch_maps.log)", time.time() - t0))
        ok += good
        fail += not good
    print("xong: %d dung, %d bo qua (da co), %d loi" % (ok, skip, fail))


if __name__ == "__main__":
    main()
