from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageDraw, ImageFont


@dataclass
class GlyphBitmap:
    char: str
    codepoint: int
    x_offset: int
    y_offset: int
    x_advance: int
    width: int
    height: int
    bitmap: list[int]


@dataclass
class Settings:
    font: str
    size: int
    chars: str
    output: Path
    symbol_name: str
    threshold: int


@dataclass
class FontMetrics:
    ascent: int
    descent: int
    line_height: int


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Render selected font glyphs into monochrome bitmaps and export "
            "them as C++ source code."
        )
    )
    parser.add_argument(
        "--config",
        type=Path,
        help="Path to JSON config file. CLI arguments override config values.",
    )
    parser.add_argument("--font", help="Font file path or a font name resolvable by Pillow.")
    parser.add_argument("--size", type=int, help="Font size in pixels.")
    parser.add_argument("--chars", help="String of characters to export.")
    parser.add_argument(
        "--output",
        type=Path,
        help="Destination .h/.hpp/.cpp file for generated font data.",
    )
    parser.add_argument(
        "--symbol-name",
        default=None,
        help="Base name for generated C++ symbols. Default: raster_font.",
    )
    parser.add_argument(
        "--threshold",
        type=int,
        default=None,
        help="Threshold 0..255 for monochrome conversion. Default: 128.",
    )
    return parser.parse_args()


def load_config(path: Path | None) -> dict:
    if path is None:
        return {}

    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise SystemExit(f"Config file not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise SystemExit(f"Invalid JSON config {path}: {exc}") from exc


def build_settings(args: argparse.Namespace) -> Settings:
    config = load_config(args.config)

    font = args.font or config.get("font")
    size = args.size if args.size is not None else config.get("size")
    chars = args.chars or config.get("chars")
    output = args.output or config.get("output")
    symbol_name = args.symbol_name or config.get("symbol_name") or "raster_font"
    threshold = args.threshold if args.threshold is not None else config.get("threshold", 128)

    missing = [
        name
        for name, value in {
            "font": font,
            "size": size,
            "chars": chars,
            "output": output,
        }.items()
        if value in (None, "")
    ]
    if missing:
        raise SystemExit(
            "Missing required settings: "
            + ", ".join(missing)
            + ". Pass them via CLI or JSON config."
        )

    if not isinstance(size, int) or size <= 0:
        raise SystemExit("Font size must be a positive integer.")
    if not isinstance(threshold, int) or not 0 <= threshold <= 255:
        raise SystemExit("Threshold must be an integer in range 0..255.")

    return Settings(
        font=str(font),
        size=size,
        chars=str(chars),
        output=Path(output),
        symbol_name=sanitize_identifier(symbol_name),
        threshold=threshold,
    )


def sanitize_identifier(value: str) -> str:
    cleaned = re.sub(r"\W+", "_", value.strip())
    if not cleaned:
        return "raster_font"
    if cleaned[0].isdigit():
        cleaned = "_" + cleaned
    return cleaned


def load_font(font_name: str, size: int) -> ImageFont.FreeTypeFont:
    try:
        return ImageFont.truetype(font_name, size=size)
    except OSError as exc:
        raise SystemExit(
            f"Unable to load font '{font_name}'. Use a valid font file path or "
            "a font name visible to Pillow."
        ) from exc


def render_glyphs(font: ImageFont.FreeTypeFont, chars: str, threshold: int) -> list[GlyphBitmap]:
    glyphs: list[GlyphBitmap] = []
    seen: set[str] = set()

    for char in chars:
        if char in seen:
            continue
        seen.add(char)
        glyphs.append(render_glyph(font, char, threshold))

    return glyphs


def render_glyph(font: ImageFont.FreeTypeFont, char: str, threshold: int) -> GlyphBitmap:
    bbox = font.getbbox(char)
    x_advance = max(0, round(font.getlength(char)))
    if bbox is None:
        return GlyphBitmap(
            char=char,
            codepoint=ord(char),
            x_offset=0,
            y_offset=0,
            x_advance=x_advance,
            width=0,
            height=0,
            bitmap=[],
        )

    left, top, right, bottom = bbox
    width = max(0, right - left)
    height = max(0, bottom - top)

    if width == 0 or height == 0:
        return GlyphBitmap(
            char=char,
            codepoint=ord(char),
            x_offset=left,
            y_offset=top,
            x_advance=x_advance,
            width=width,
            height=height,
            bitmap=[],
        )

    image = Image.new("L", (width, height), 0)
    draw = ImageDraw.Draw(image)
    draw.text((-left, -top), char, fill=255, font=font)

    pixels = list(image.getdata())
    packed = pack_bits(pixels, width, height, threshold)
    return GlyphBitmap(
        char=char,
        codepoint=ord(char),
        x_offset=left,
        y_offset=top,
        x_advance=x_advance,
        width=width,
        height=height,
        bitmap=packed,
    )


def pack_bits(pixels: Iterable[int], width: int, height: int, threshold: int) -> list[int]:
    rows = list(pixels)
    stride = (width + 7) // 8
    result = [0] * (stride * height)

    for y in range(height):
        for x in range(width):
            if rows[y * width + x] >= threshold:
                index = y * stride + (x // 8)
                bit = 7 - (x % 8)
                result[index] |= 1 << bit

    return result


def get_font_metrics(font: ImageFont.FreeTypeFont) -> FontMetrics:
    ascent, descent = font.getmetrics()
    return FontMetrics(
        ascent=ascent,
        descent=descent,
        line_height=ascent + descent,
    )


def generate_cpp(glyphs: list[GlyphBitmap], settings: Settings, metrics: FontMetrics) -> str:
    symbol = settings.symbol_name
    total_bytes = sum(len(g.bitmap) for g in glyphs)
    codepoint_values = ", ".join(f"0x{glyph.codepoint:08X}u" for glyph in glyphs)
    if not codepoint_values:
        codepoint_values = "0u"

    lines: list[str] = [
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        "#ifndef TTF2RASTR_GLYPH_DEFINED",
        "#define TTF2RASTR_GLYPH_DEFINED",
        "typedef struct RasterGlyph {",
        "    uint32_t codepoint;",
        "    int16_t x_offset;",
        "    int16_t y_offset;",
        "    uint16_t x_advance;",
        "    uint16_t width;",
        "    uint16_t height;",
        "    const uint8_t* bitmap;",
        "    size_t bitmap_size;",
        "} RasterGlyph;",
        "#endif",
        "",
    ]

    for glyph in glyphs:
        glyph_symbol = f"{symbol}_u{glyph.codepoint:04X}"
        bitmap_values = ", ".join(f"0x{byte:02X}" for byte in glyph.bitmap)
        if not bitmap_values:
            bitmap_values = "0x00"
        lines.extend(
            [
                f"static const uint8_t {glyph_symbol}[{max(1, len(glyph.bitmap))}] = {{",
                f"    {bitmap_values}",
                "};",
                "",
            ]
        )

    lines.extend(
        [
            f"static const RasterGlyph {symbol}_glyphs[{len(glyphs)}] = {{",
        ]
    )

    for index, glyph in enumerate(glyphs):
        glyph_symbol = f"{symbol}_u{glyph.codepoint:04X}"
        suffix = "," if index != len(glyphs) - 1 else ""
        lines.append(
            f"    {{0x{glyph.codepoint:08X}u, {glyph.x_offset}, {glyph.y_offset}, "
            f"{glyph.x_advance}, {glyph.width}, {glyph.height}, "
            f"{glyph_symbol}, {len(glyph.bitmap)}}}{suffix}"
        )

    lines.extend(
        [
            "};",
            "",
            f"static const uint32_t {symbol}_chars[{len(glyphs)}] = {{{codepoint_values}}};",
            "",
            f"static const size_t {symbol}_font_size = {settings.size}u;",
            f"static const size_t {symbol}_font_ascent = {metrics.ascent}u;",
            f"static const size_t {symbol}_font_descent = {metrics.descent}u;",
            f"static const size_t {symbol}_line_height = {metrics.line_height}u;",
            f"static const size_t {symbol}_glyph_count = {len(glyphs)}u;",
            f"static const size_t {symbol}_total_bitmap_bytes = {total_bytes}u;",
            "",
        ]
    )

    return "\n".join(lines) + "\n"


def main() -> int:
    settings = build_settings(parse_args())
    font = load_font(settings.font, settings.size)
    glyphs = render_glyphs(font, settings.chars, settings.threshold)
    metrics = get_font_metrics(font)
    cpp_code = generate_cpp(glyphs, settings, metrics)

    settings.output.parent.mkdir(parents=True, exist_ok=True)
    settings.output.write_text(cpp_code, encoding="utf-8")

    print(
        f"Generated {len(glyphs)} glyphs from '{settings.font}' "
        f"into '{settings.output}'."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
