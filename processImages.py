import math
from PIL import Image
from pathlib import Path
import os

source = Path("resources") / "images"
dest = Path("binres") / "images"

os.makedirs(dest, exist_ok=True)

for f in source.iterdir():
    if f.suffix.lower() in [".png"]:
        with open(dest / (f.stem + ".bin"), "wb") as wf:
            pixels = bytes(Image.open(f).convert("L").getdata())
            out = bytearray(math.ceil(len(pixels)/2))
            for i in range(0, len(pixels), 2):
                if i < len(pixels)-1:
                    out[i//2] = 0xFF ^ ((pixels[i]>>4) << 4 | (pixels[i+1]>>4))
                else:
                    out[i//2] = 0xFF ^ ((pixels[i]>>4) << 4)

            wf.write(out)
