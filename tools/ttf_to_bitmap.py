"""Rasterize printable ASCII from a TrueType font into a kernel header."""

import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw, ImageFont
except ModuleNotFoundError as error:
    raise SystemExit(
        "Custom TTF builds require Pillow. Install it with: python3 -m pip install pillow"
    ) from error


FONT_WIDTH = 12
FONT_HEIGHT = 20
FONT_SIZE = 18
FIRST_CHARACTER = 32
LAST_CHARACTER = 126


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("Usage: ttf_to_bitmap.py INPUT.ttf OUTPUT.h")

    source = Path(sys.argv[1])
    destination = Path(sys.argv[2])
    font = ImageFont.truetype(str(source), FONT_SIZE)
    glyph_rows = []

    for codepoint in range(FIRST_CHARACTER, LAST_CHARACTER + 1):
        character = chr(codepoint)
        image = Image.new("L", (FONT_WIDTH, FONT_HEIGHT), 0)
        drawing = ImageDraw.Draw(image)
        left, top, right, bottom = font.getbbox(character)
        glyph_width = right - left
        glyph_height = bottom - top
        x = (FONT_WIDTH - glyph_width) // 2 - left
        y = (FONT_HEIGHT - glyph_height) // 2 - top
        drawing.text((x, y), character, font=font, fill=255)

        rows = []
        for row in range(FONT_HEIGHT):
            bits = 0
            for column in range(FONT_WIDTH):
                if image.getpixel((column, row)) >= 128:
                    bits |= 1 << (FONT_WIDTH - 1 - column)
            rows.append(f"0x{bits:03X}")
        glyph_rows.append("  {" + ", ".join(rows) + "}")

    header = [
        "#ifndef PEYO_GENERATED_FONT_H",
        "#define PEYO_GENERATED_FONT_H",
        "#define PEYO_FONT_WIDTH 12",
        "#define PEYO_FONT_HEIGHT 20",
        "static const uint16_t peyo_font_bitmap[95][PEYO_FONT_HEIGHT] = {",
        ",\n".join(glyph_rows),
        "};",
        "#endif",
        "",
    ]
    destination.write_text("\n".join(header), encoding="ascii")


if __name__ == "__main__":
    main()
