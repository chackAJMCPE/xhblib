#!/usr/bin/env python3
"""
resize_and_center.py

Batch-resize PNGs to fit within 960x720 without stretching, then place them centered
inside a 1280x720 PNG canvas. Preserves transparency.

Usage:
    python resize_and_center.py --in input_folder --out output_folder
"""

import argparse
from pathlib import Path
from PIL import Image

# Desired sizes
RESIZE_MAX = (960, 720)    # fit the image within this (no stretching)
CANVAS_SIZE = (1280, 720)  # final canvas size
BACKGROUND = (0, 0, 0, 0)  # transparent RGBA background

def process_image(src_path: Path, dst_path: Path,
                  resize_max=RESIZE_MAX,
                  canvas_size=CANVAS_SIZE,
                  background=BACKGROUND):
    with Image.open(src_path) as im:
        # convert to RGBA to preserve transparency and allow compositing on transparent canvas
        im = im.convert("RGBA")
        w, h = im.size

        # compute scale to fit inside resize_max while preserving aspect ratio
        max_w, max_h = resize_max
        scale = min(max_w / w, max_h / h)

        new_w = max(1, int(round(w * scale)))
        new_h = max(1, int(round(h * scale)))

        # resize with high-quality resampling
        im_resized = im.resize((new_w, new_h), resample=Image.LANCZOS)

        # create canvas (RGBA) with transparent background
        canvas_w, canvas_h = canvas_size
        canvas = Image.new("RGBA", (canvas_w, canvas_h), background)

        # center coordinates
        left = (canvas_w - new_w) // 2
        top  = (canvas_h - new_h) // 2

        # paste using alpha channel as mask so transparency is preserved
        canvas.paste(im_resized, (left, top), im_resized)

        # ensure parent exists
        dst_path.parent.mkdir(parents=True, exist_ok=True)

        # save as PNG (keep transparency). optimize flag helps reduce size.
        canvas.save(dst_path, format="PNG", optimize=True)

def main():
    parser = argparse.ArgumentParser(description="Resize 640x480 PNGs to 960x720 and center them in 1280x720 canvases.")
    parser.add_argument("--in",  dest="input_dir",  required=True, help="Input folder containing PNG files.")
    parser.add_argument("--out", dest="output_dir", required=True, help="Output folder to write processed PNGs.")
    parser.add_argument("--recursive", action="store_true", help="Process PNGs in subdirectories as well.")
    parser.add_argument("--bg-color", default=None,
                        help="Optional background color for the canvas in hex (e.g. '#000000' or '#RRGGBBAA'). Overrides transparency.")
    args = parser.parse_args()

    in_dir = Path(args.input_dir)
    out_dir = Path(args.output_dir)

    if not in_dir.exists() or not in_dir.is_dir():
        raise SystemExit(f"Input folder does not exist or is not a directory: {in_dir}")

    # parse background color if provided
    bg = BACKGROUND
    if args.bg_color:
        s = args.bg_color.strip().lstrip("#")
        if len(s) == 6:
            r, g, b = int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16)
            bg = (r, g, b, 255)
        elif len(s) == 8:
            r, g, b, a = int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16), int(s[6:8], 16)
            bg = (r, g, b, a)
        else:
            raise SystemExit("bg-color must be hex RRGGBB or RRGGBBAA (e.g. #000000 or #000000FF).")

    # choose files
    pattern = "**/*.png" if args.recursive else "*.png"
    files = sorted(in_dir.glob(pattern))

    if not files:
        print("No PNG files found to process.")
        return

    print(f"Processing {len(files)} PNG(s)...")
    for src in files:
        # preserve relative path under output_dir (if recursive)
        rel = src.relative_to(in_dir)
        dst = out_dir.joinpath(rel)
        # ensure output file has .png extension
        dst = dst.with_suffix(".png")

        try:
            process_image(src, dst, resize_max=RESIZE_MAX, canvas_size=CANVAS_SIZE, background=bg)
            print(f"OK: {src} -> {dst}")
        except Exception as e:
            print(f"ERROR processing {src}: {e}")

    print("Done.")

if __name__ == "__main__":
    main()
