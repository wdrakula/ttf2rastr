#ifndef TTF2RASTR_GFX_H
#define TTF2RASTR_GFX_H

#include <Adafruit_GFX.h>
#include <stddef.h>
#include <stdint.h>

#ifndef TTF2RASTR_GLYPH_DEFINED
#define TTF2RASTR_GLYPH_DEFINED
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
#endif

typedef struct RasterFont {
  const RasterGlyph* glyphs;
  size_t glyph_count;
  size_t font_size;
  size_t font_ascent;
  size_t font_descent;
  size_t line_height;
} RasterFont;

typedef struct RasterTextMetrics {
  int16_t width;
  int16_t min_y;
  int16_t max_y;
} RasterTextMetrics;

typedef struct RasterTextPlacement {
  int16_t x;
  int16_t baseline_y;
  RasterTextMetrics metrics;
} RasterTextPlacement;

#define TTF2RASTR_MAKE_FONT(symbol_prefix)                                    \
  {                                                                           \
    symbol_prefix##_glyphs, symbol_prefix##_glyph_count,                      \
        symbol_prefix##_font_size, symbol_prefix##_font_ascent,               \
        symbol_prefix##_font_descent, symbol_prefix##_line_height             \
  }

// Returns a pointer to the glyph for the given codepoint, or NULL if absent.
const RasterGlyph* ttf2rastrFindGlyph(const RasterFont* font, uint32_t codepoint);

// Returns xAdvance for a codepoint, or a fallback width if the glyph is missing.
uint16_t ttf2rastrGlyphAdvance(const RasterFont* font, uint32_t codepoint);

// Measures text width and vertical bounds relative to the baseline.
RasterTextMetrics ttf2rastrMeasureText(const RasterFont* font, const char* text);

// Converts measured vertical bounds into total text height.
int16_t ttf2rastrTextHeight(const RasterTextMetrics* metrics);

// Computes centered text placement inside a rectangle.
RasterTextPlacement ttf2rastrPlaceTextInBox(const RasterFont* font, int16_t box_x,
                                            int16_t box_y, int16_t box_w,
                                            int16_t box_h, const char* text);

// Draws a single glyph at the specified pen position and baseline.
void ttf2rastrDrawGlyph(Adafruit_GFX& display, const RasterFont* font, int16_t x,
                        int16_t baseline_y, uint32_t codepoint, uint16_t color,
                        uint16_t missing_color);

// Draws a whole UTF-8 string starting from x/baseline_y and returns drawn width.
int16_t ttf2rastrDrawText(Adafruit_GFX& display, const RasterFont* font, int16_t x,
                          int16_t baseline_y, const char* text, uint16_t color,
                          uint16_t missing_color);

// Clears a rectangle and draws centered text inside it.
void ttf2rastrDrawTextInBox(Adafruit_GFX& display, const RasterFont* font,
                            int16_t box_x, int16_t box_y, int16_t box_w,
                            int16_t box_h, const char* text, uint16_t text_color,
                            uint16_t bg_color, uint16_t missing_color);

#endif
