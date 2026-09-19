"""Ghep anh scn3d_weapon_<id>.png (Scn3D --auto --weapons) thanh bang tong hop de soi tay cam: moi o cat quanh nhan vat.
    python tools/scn3d/weapon_sheet.py [--out build/weapons_sheet.png]"""
import argparse, glob, io, json, os, re
from PIL import Image, ImageDraw

LOGS = os.path.join(os.environ.get("APPDATA", ""), "Godot", "app_userdata", "JX NEXT", "logs")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="build/weapons_sheet.png")
    ap.add_argument("--crop", default="470,120,810,620", help="x0,y0,x1,y1 quanh nhan vat trong anh 1280x720")
    ap.add_argument("--cols", type=int, default=9)
    a = ap.parse_args()
    x0, y0, x1, y1 = [int(v) for v in a.crop.split(",")]
    w, h = x1 - x0, y1 - y0
    names = {}
    wj = os.path.join("client", "assets3d", "weapon", "weapons.json")
    if os.path.exists(wj):
        names = json.load(io.open(wj, encoding="utf-8"))
    files = sorted(glob.glob(os.path.join(LOGS, "scn3d_weapon_*.png")), key=lambda f: int(re.search(r"_(\d+)\.png$", f).group(1)))
    if not files:
        raise SystemExit("khong co anh scn3d_weapon_*.png trong " + LOGS)
    scale = 0.5
    cw, ch = int(w * scale), int(h * scale) + 14
    rows = (len(files) + a.cols - 1) // a.cols
    sheet = Image.new("RGB", (cw * a.cols, ch * rows), (20, 20, 20))
    d = ImageDraw.Draw(sheet)
    for i, f in enumerate(files):
        wid = re.search(r"_(\d+)\.png$", f).group(1)
        im = Image.open(f).crop((x0, y0, x1, y1)).resize((int(w * scale), int(h * scale)))
        cx, cy = (i % a.cols) * cw, (i // a.cols) * ch
        sheet.paste(im, (cx, cy + 14))
        info = names.get(wid, {})
        d.text((cx + 2, cy), "%s %s %s" % (wid, info.get("type_vi", ""), str(info.get("name", ""))[:10]), fill=(255, 255, 0))
    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    sheet.save(a.out)
    print("%d anh -> %s (%dx%d)" % (len(files), a.out, sheet.width, sheet.height))


if __name__ == "__main__":
    main()
