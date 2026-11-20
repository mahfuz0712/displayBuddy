# /// script
# requires-python = ">=3.9"
# dependencies = ["pillow"]
# ///
"""Regenerates resources/icons/*. Run from the project root (uv installs pillow by itself):
    uv run tools/make_icons.py
Without uv:  pip install pillow  and  python tools/make_icons.py"""
import os
from PIL import Image, ImageDraw
import math

os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "resources", "icons"))
S = 1024

def make():
    # gradient background (amber -> orange), rounded square
    bg = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    grad = Image.new("RGBA", (S, S))
    px = grad.load()
    c1, c2 = (255, 196, 64), (240, 100, 20)
    for y in range(S):
        for x in range(S):
            t = (x * 0.35 + y * 0.65) / S
            px[x, y] = tuple(int(c1[i] + (c2[i] - c1[i]) * t) for i in range(3)) + (255,)
    mask = Image.new("L", (S, S), 0)
    ImageDraw.Draw(mask).rounded_rectangle((32, 32, S - 32, S - 32), radius=230, fill=255)
    bg.paste(grad, (0, 0), mask)

    d = ImageDraw.Draw(bg)
    white = (255, 255, 255, 255)
    # monitor body
    d.rounded_rectangle((190, 236, 834, 672), radius=62, outline=white, width=46)
    # stand
    d.rounded_rectangle((470, 700, 554, 790), radius=18, fill=white)
    d.rounded_rectangle((340, 770, 684, 824), radius=27, fill=white)
    # sun
    cx, cy = 512, 454
    d.ellipse((cx - 78, cy - 78, cx + 78, cy + 78), fill=white)
    for k in range(8):
        a = math.radians(k * 45)
        x1, y1 = cx + math.cos(a) * 118, cy + math.sin(a) * 118
        x2, y2 = cx + math.cos(a) * 168, cy + math.sin(a) * 168
        d.line((x1, y1, x2, y2), fill=white, width=30)
        for (x, y) in ((x1, y1), (x2, y2)):
            d.ellipse((x - 15, y - 15, x + 15, y + 15), fill=white)
    return bg

img = make()
sizes = [16, 20, 24, 32, 40, 48, 64, 96, 128, 256]
for s in (16, 24, 32, 48, 64, 128, 256, 512):
    img.resize((s, s), Image.LANCZOS).save(f"displaybuddy-{s}.png")
img.resize((256, 256), Image.LANCZOS).save("displaybuddy.png")
img.resize((256, 256), Image.LANCZOS).save("displaybuddy.ico", sizes=[(s, s) for s in sizes])
print("ok")