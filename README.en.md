# ttf2rastr

Small tool for converting TTF/OTF fonts into 1-bit raster font data for C/C++ without STL.

The project currently focuses on three practical use cases:

- generating bitmap glyphs for a selected character set
- quickly previewing the result on desktop as a `PPM` image
- using the generated font data together with the `ttf2rastr_gfx` Arduino helper library
- using the generated font data together with ASCII and UTF-8/Unicode Arduino examples

## Features

- reads settings from CLI or JSON config
- loads TTF/OTF fonts through Pillow
- renders only the requested characters
- packs glyphs into 1-bit `uint8_t` bitmap arrays
- generates a `.h` header with plain C-style structures and arrays without STL
- emits `TTF2RASTR_STORAGE`, which maps to `PROGMEM` on Arduino builds
- stores glyph metrics:
  - `x_offset`
  - `y_offset`
  - `x_advance`
- stores font metrics:
  - `font_ascent`
  - `font_descent`
  - `line_height`
- supports quick desktop validation through `font_preview.cpp`

## Project Layout

- `main.py` - raster font generator
- `font_preview.cpp` - desktop preview tool that writes `PPM`
- `Makefile` - preview build helper
- `Examples/` - desktop preview artifacts
- `Arduino/ttf2rastr_gfx/` - Arduino library for drawing generated bitmap fonts
- `example_config.json` - sample generator config
- `generated_font.h` - sample generated font
- `USAGE.md` - detailed usage guide
- `Web/` - browser version; output may differ from `main.py` because it uses a different rendering engine

## Quick Start

### 1. Install Python dependency

```bash
python -m pip install pillow
```

### 2. Generate the font

```bash
python main.py --config example_config.json
```

### 3. Build the desktop preview

```bash
make
```

### 4. Check the result

```bash
./font_preview '0123456789_' FFCC00 102040 Examples/digits_preview.ppm
```

A ready-made preview image is also stored in the repo:

- `Examples/digits_preview.png`

## Direct CLI Example

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 72 \
  --chars "0123456789_" \
  --output generated_font.h \
  --symbol-name demo_font
```

## Output Format

The generator produces a C/C++ include file containing:

- one bitmap array per glyph
- a `RasterGlyph` array
- a codepoint array
- font-wide constants:
  - font size
  - ascent
  - descent
  - line height
  - glyph count
  - total bitmap byte size

`generated_font.h` contains only font data and related declarations, not executable logic, so a header format is more natural than a separate `.cpp` translation unit.
On Arduino, the same generated declarations are automatically marked with `PROGMEM`.

## Example `RasterGlyph`

```cpp
typedef struct RasterGlyph {
    uint32_t codepoint;
    int16_t x_offset;
    int16_t y_offset;
    uint16_t x_advance;
    uint16_t width;
    uint16_t height;
    const uint8_t* bitmap;
    size_t bitmap_size;
} RasterGlyph;
```

## Arduino Library

The repo also contains a reusable Arduino helper library:

- `Arduino/ttf2rastr_gfx/`

It handles:

- glyph lookup
- text measurement
- centered placement in a rectangle
- whole-text drawing
- single-glyph drawing

It also works with UTF-8 text on Arduino, including Cyrillic, by decoding the input string into Unicode codepoints before glyph lookup.

See:

- [Arduino/ttf2rastr_gfx/README.md](/home/wlad/Projects/Python/ttf2rastr/Arduino/ttf2rastr_gfx/README.md)
- [Arduino/ttf2rastr_gfx/README.en.md](/home/wlad/Projects/Python/ttf2rastr/Arduino/ttf2rastr_gfx/README.en.md)
- [Arduino/ttf2rastr_gfx/examples/ESP32_ILI9341_CyrillicDemo/ESP32_ILI9341_CyrillicDemo.ino](/home/wlad/Projects/Python/ttf2rastr/Arduino/ttf2rastr_gfx/examples/ESP32_ILI9341_CyrillicDemo/ESP32_ILI9341_CyrillicDemo.ino)

## Documentation

- Detailed usage: [USAGE.md](/home/wlad/Projects/Python/ttf2rastr/USAGE.md)
- Arduino library docs: [Arduino/ttf2rastr_gfx/README.md](/home/wlad/Projects/Python/ttf2rastr/Arduino/ttf2rastr_gfx/README.md)

## License

This project is licensed under `MIT`.
