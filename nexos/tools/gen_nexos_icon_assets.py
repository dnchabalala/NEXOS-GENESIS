#!/usr/bin/env python3
"""Rasterize canonical NexOS Unicode design glyphs into alpha-mask PNGs.

This is a standalone asset-preparation tool. It does not wire assets into
the GUI or modify generated kernel sources.
"""
from pathlib import Path
import csv
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets/icons/source/glyphs.tsv"
OUTPUT = ROOT / "assets/icons/generated"
CONTACT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("/tmp/nexos-icon-contact-sheet.png")
SIZES = (16, 24, 32, 48, 64)


def font_for(codepoint):
    result = subprocess.run(
        ["fc-match", "-f", "%{file}", f"Inter:charset={codepoint:04x}"],
        check=True, capture_output=True, text=True,
    )
    path = Path(result.stdout.strip())
    if not path.exists():
        raise SystemExit(f"fontconfig returned missing font for U+{codepoint:04X}: {path}")
    return path


def render(glyph, font_path, size):
    font = ImageFont.truetype(str(font_path), size)
    # Oversized temporary canvas lets ascender/descender bearings be cropped
    # deterministically to the visible glyph bounds.
    canvas = Image.new("L", (size * 8, size * 8), 0)
    draw = ImageDraw.Draw(canvas)
    bbox = draw.textbbox((0, 0), glyph, font=font)
    draw.text((-bbox[0] + size, -bbox[1] + size), glyph, font=font, fill=255)
    box = canvas.getbbox()
    if box is None:
        return Image.new("L", (size, size), 0)
    crop = canvas.crop(box)
    # Keep the visual glyph centered in a size x size alpha slot.
    out = Image.new("L", (size, size), 0)
    x = max(0, (size - crop.width) // 2)
    y = max(0, (size - crop.height) // 2)
    out.paste(crop.resize((min(crop.width, size), min(crop.height, size))), (x, y))
    return out


def read_rows():
    with SOURCE.open(encoding="utf-8") as stream:
        for line in stream:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            # The checked-in manifest is intentionally plain text and may be
            # represented with literal \\t separators by source tooling.
            row = line.split("\t") if "\t" in line else line.split("\\t")
            if len(row) != 7:
                raise SystemExit(f"invalid source row: {row}")
            yield row


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    CONTACT.parent.mkdir(parents=True, exist_ok=True)
    entries = []
    for item_id, stable_id, glyph, cp_text, purpose, slot, filename in read_rows():
        codepoint = ord(glyph[0])
        font_path = font_for(codepoint)
        for size in SIZES:
            image = render(glyph, font_path, size)
            image.save(OUTPUT / f"{filename}-{size}.png", optimize=True)
        entries.append((item_id, stable_id, glyph, cp_text, purpose, slot, filename, font_path))

    # Contact sheet uses the 48px masks and labels each source decision.
    cell_w, cell_h = 260, 112
    columns = 4
    rows = (len(entries) + columns - 1) // columns
    sheet = Image.new("RGB", (columns * cell_w, rows * cell_h), (7, 10, 18))
    draw = ImageDraw.Draw(sheet)
    label_font = ImageFont.truetype(str(font_for(ord("A"))), 13)
    glyph_font = ImageFont.truetype(str(font_for(ord(entries[0][2][0]))), 36)
    for index, (item_id, stable_id, glyph, cp_text, purpose, slot, filename, font_path) in enumerate(entries):
        x = (index % columns) * cell_w
        y = (index // columns) * cell_h
        tile = render(glyph, font_path, 48)
        tile_rgb = Image.new("RGB", tile.size, (25, 31, 50))
        tile_rgb.paste((220, 226, 255), mask=tile)
        sheet.paste(tile_rgb, (x + 12, y + 10))
        draw.text((x + 72, y + 10), f"{stable_id}\n{glyph}  {cp_text}", fill=(235, 238, 248), font=label_font, spacing=3)
        draw.text((x + 72, y + 58), purpose[:28], fill=(162, 171, 198), font=label_font)
        draw.text((x + 72, y + 78), font_path.name[:27], fill=(126, 137, 168), font=label_font)
    sheet.save(CONTACT, optimize=True)

    metadata = OUTPUT / "generated.tsv"
    with metadata.open("w", encoding="utf-8", newline="") as stream:
        stream.write("id\tstable_id\tglyph\tcodepoint\tpurpose\ttarget_slot\tfont\tsource_family\tfont_sizes\tbaseline\tlicense_status\n")
        for item_id, stable_id, glyph, cp_text, purpose, slot, filename, font_path in entries:
            stream.write("\t".join((item_id, stable_id, glyph, cp_text, purpose, slot, str(font_path), "Inter, -apple-system, BlinkMacSystemFont, Segoe UI, sans-serif", ",".join(map(str, SIZES)), "centered visible bounds per square alpha slot", "not determinable from local font metadata")) + "\n")
    print(f"generated {len(entries)} glyphs x {len(SIZES)} sizes in {OUTPUT}")
    print(f"generated contact sheet {CONTACT}")


if __name__ == "__main__":
    main()
