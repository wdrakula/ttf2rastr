#include "ttf2rastr_gfx.h"

#include <limits.h>

static uint32_t ttf2rastrNextCodepoint(const uint8_t** src_ptr) {
  const uint8_t* src = *src_ptr;
  uint32_t codepoint = 0;
  size_t extra = 0;

  if ((*src & 0x80u) == 0) {
    codepoint = *src;
    extra = 0;
  } else if ((*src & 0xE0u) == 0xC0u) {
    codepoint = *src & 0x1Fu;
    extra = 1;
  } else if ((*src & 0xF0u) == 0xE0u) {
    codepoint = *src & 0x0Fu;
    extra = 2;
  } else if ((*src & 0xF8u) == 0xF0u) {
    codepoint = *src & 0x07u;
    extra = 3;
  } else {
    codepoint = '?';
    extra = 0;
  }

  ++src;

  while (extra > 0 && *src != '\0') {
    if ((*src & 0xC0u) != 0x80u) {
      codepoint = '?';
      break;
    }
    codepoint = (codepoint << 6) | (*src & 0x3Fu);
    ++src;
    --extra;
  }

  *src_ptr = src;
  return codepoint;
}

static uint16_t ttf2rastrGlyphAdvance(const RasterFont* font,
                                      const RasterGlyph* glyph) {
  if (glyph != NULL) {
    if (glyph->x_advance != 0u) {
      return glyph->x_advance;
    }
    return glyph->width;
  }

  return (uint16_t)(font->font_size / 2u);
}

static void ttf2rastrWriteMissingGlyph(Adafruit_GFX& display, int16_t x, int16_t y,
                                       int16_t width, int16_t height,
                                       uint16_t color) {
  if (width < 2 || height < 2) {
    return;
  }

  display.writeFastHLine(x, y, width, color);
  display.writeFastHLine(x, y + height - 1, width, color);
  display.writeFastVLine(x, y, height, color);
  display.writeFastVLine(x + width - 1, y, height, color);
}

static void ttf2rastrWriteGlyph(Adafruit_GFX& display, int16_t origin_x,
                                int16_t baseline_y, const RasterGlyph* glyph,
                                uint16_t color) {
  const int16_t draw_x = origin_x + glyph->x_offset;
  const int16_t draw_y = baseline_y + glyph->y_offset;
  const size_t stride = (glyph->width + 7u) / 8u;

  for (uint16_t row = 0; row < glyph->height; ++row) {
    uint16_t col = 0;

    while (col < glyph->width) {
      const size_t byte_index = row * stride + (col / 8u);
      const uint8_t mask = (uint8_t)(0x80u >> (col % 8u));

      if (byte_index >= glyph->bitmap_size ||
          (glyph->bitmap[byte_index] & mask) == 0u) {
        ++col;
        continue;
      }

      const uint16_t run_start = col;
      while (col < glyph->width) {
        const size_t run_byte_index = row * stride + (col / 8u);
        const uint8_t run_mask = (uint8_t)(0x80u >> (col % 8u));
        if (run_byte_index >= glyph->bitmap_size ||
            (glyph->bitmap[run_byte_index] & run_mask) == 0u) {
          break;
        }
        ++col;
      }

      display.writeFastHLine(draw_x + (int16_t)run_start, draw_y + (int16_t)row,
                             col - run_start, color);
    }
  }
}

const RasterGlyph* ttf2rastrFindGlyph(const RasterFont* font, uint32_t codepoint) {
  if (font == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < font->glyph_count; ++i) {
    if (font->glyphs[i].codepoint == codepoint) {
      return &font->glyphs[i];
    }
  }

  return NULL;
}

uint16_t ttf2rastrGlyphAdvance(const RasterFont* font, uint32_t codepoint) {
  if (font == NULL) {
    return 0;
  }

  return ttf2rastrGlyphAdvance(font, ttf2rastrFindGlyph(font, codepoint));
}

RasterTextMetrics ttf2rastrMeasureText(const RasterFont* font, const char* text) {
  RasterTextMetrics metrics = {0, INT16_MAX, INT16_MIN};
  const uint8_t* src = (const uint8_t*)text;

  if (font == NULL || text == NULL) {
    metrics.min_y = 0;
    metrics.max_y = 0;
    return metrics;
  }

  while (*src != '\0') {
    const uint32_t codepoint = ttf2rastrNextCodepoint(&src);
    const RasterGlyph* glyph = ttf2rastrFindGlyph(font, codepoint);

    metrics.width += (int16_t)ttf2rastrGlyphAdvance(font, glyph);

    if (glyph != NULL) {
      if (glyph->y_offset < metrics.min_y) {
        metrics.min_y = glyph->y_offset;
      }
      if ((int16_t)(glyph->y_offset + (int16_t)glyph->height) > metrics.max_y) {
        metrics.max_y = (int16_t)(glyph->y_offset + (int16_t)glyph->height);
      }
    } else {
      if (0 < metrics.min_y) {
        metrics.min_y = 0;
      }
      if ((int16_t)font->line_height > metrics.max_y) {
        metrics.max_y = (int16_t)font->line_height;
      }
    }
  }

  if (metrics.min_y == INT16_MAX || metrics.max_y == INT16_MIN) {
    metrics.min_y = 0;
    metrics.max_y = (int16_t)font->line_height;
  }

  return metrics;
}

int16_t ttf2rastrTextHeight(const RasterTextMetrics* metrics) {
  if (metrics == NULL) {
    return 0;
  }

  return metrics->max_y - metrics->min_y;
}

RasterTextPlacement ttf2rastrPlaceTextInBox(const RasterFont* font, int16_t box_x,
                                            int16_t box_y, int16_t box_w,
                                            int16_t box_h, const char* text) {
  RasterTextPlacement placement;

  placement.metrics = ttf2rastrMeasureText(font, text);
  placement.x = box_x + (box_w - placement.metrics.width) / 2;
  placement.baseline_y =
      box_y + (box_h - ttf2rastrTextHeight(&placement.metrics)) / 2 -
      placement.metrics.min_y;

  return placement;
}

void ttf2rastrDrawGlyph(Adafruit_GFX& display, const RasterFont* font, int16_t x,
                        int16_t baseline_y, uint32_t codepoint, uint16_t color,
                        uint16_t missing_color) {
  const RasterGlyph* glyph = ttf2rastrFindGlyph(font, codepoint);

  display.startWrite();
  if (glyph != NULL) {
    ttf2rastrWriteGlyph(display, x, baseline_y, glyph, color);
  } else {
    const int16_t box_w = (int16_t)(font->font_size / 2u);
    const int16_t box_h = (int16_t)font->line_height;
    ttf2rastrWriteMissingGlyph(display, x, baseline_y, box_w, box_h,
                               missing_color);
  }
  display.endWrite();
}

int16_t ttf2rastrDrawText(Adafruit_GFX& display, const RasterFont* font, int16_t x,
                          int16_t baseline_y, const char* text, uint16_t color,
                          uint16_t missing_color) {
  const uint8_t* src = (const uint8_t*)text;
  int16_t pen_x = x;

  if (font == NULL || text == NULL) {
    return 0;
  }

  display.startWrite();

  while (*src != '\0') {
    const uint32_t codepoint = ttf2rastrNextCodepoint(&src);
    const RasterGlyph* glyph = ttf2rastrFindGlyph(font, codepoint);

    if (glyph != NULL) {
      ttf2rastrWriteGlyph(display, pen_x, baseline_y, glyph, color);
    } else {
      const int16_t box_w = (int16_t)(font->font_size / 2u);
      const int16_t box_h = (int16_t)font->line_height;
      ttf2rastrWriteMissingGlyph(display, pen_x, baseline_y, box_w, box_h,
                                 missing_color);
    }

    pen_x += (int16_t)ttf2rastrGlyphAdvance(font, glyph);
  }

  display.endWrite();
  return pen_x - x;
}

void ttf2rastrDrawTextInBox(Adafruit_GFX& display, const RasterFont* font,
                            int16_t box_x, int16_t box_y, int16_t box_w,
                            int16_t box_h, const char* text, uint16_t text_color,
                            uint16_t bg_color, uint16_t missing_color) {
  const RasterTextPlacement placement =
      ttf2rastrPlaceTextInBox(font, box_x, box_y, box_w, box_h, text);

  display.fillRect(box_x, box_y, box_w, box_h, bg_color);
  ttf2rastrDrawText(display, font, placement.x, placement.baseline_y, text,
                    text_color, missing_color);
}
