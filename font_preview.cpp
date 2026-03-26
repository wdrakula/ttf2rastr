#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef FONT_DATA_FILE
#define FONT_DATA_FILE "generated_font.h"
#endif

#ifndef FONT_SYMBOL_PREFIX
#define FONT_SYMBOL_PREFIX demo_font
#endif

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define FONT_GLYPHS CONCAT(FONT_SYMBOL_PREFIX, _glyphs)
#define FONT_GLYPH_COUNT CONCAT(FONT_SYMBOL_PREFIX, _glyph_count)
#define FONT_SIZE CONCAT(FONT_SYMBOL_PREFIX, _font_size)
#define FONT_ASCENT CONCAT(FONT_SYMBOL_PREFIX, _font_ascent)
#define FONT_DESCENT CONCAT(FONT_SYMBOL_PREFIX, _font_descent)
#define FONT_LINE_HEIGHT CONCAT(FONT_SYMBOL_PREFIX, _line_height)

#include FONT_DATA_FILE

typedef struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;

static void print_usage(const char* program_name) {
    fprintf(
        stderr,
        "Usage: %s <text> [fg] [bg] [output.ppm]\n"
        "  fg/bg format: RRGGBB or #RRGGBB\n",
        program_name
    );
}

static int hex_value(char ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'f') {
        return 10 + (ch - 'a');
    }
    if (ch >= 'A' && ch <= 'F') {
        return 10 + (ch - 'A');
    }
    return -1;
}

static int parse_color(const char* text, Color* out) {
    size_t offset = 0;
    int values[6];
    size_t i;

    if (text == NULL || out == NULL) {
        return 0;
    }

    if (text[0] == '#') {
        offset = 1;
    }

    if (strlen(text + offset) != 6) {
        return 0;
    }

    for (i = 0; i < 6; ++i) {
        values[i] = hex_value(text[offset + i]);
        if (values[i] < 0) {
            return 0;
        }
    }

    out->r = (uint8_t)((values[0] << 4) | values[1]);
    out->g = (uint8_t)((values[2] << 4) | values[3]);
    out->b = (uint8_t)((values[4] << 4) | values[5]);
    return 1;
}

static size_t decode_utf8(const char* text, uint32_t* out, size_t max_out) {
    const unsigned char* src = (const unsigned char*)text;
    size_t count = 0;

    while (*src != '\0' && count < max_out) {
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

        out[count++] = codepoint;
    }

    return count;
}

static const RasterGlyph* find_glyph(uint32_t codepoint) {
    size_t i;

    for (i = 0; i < FONT_GLYPH_COUNT; ++i) {
        if (FONT_GLYPHS[i].codepoint == codepoint) {
            return &FONT_GLYPHS[i];
        }
    }

    return NULL;
}

static void set_pixel(
    uint8_t* image,
    size_t image_width,
    size_t image_height,
    size_t x,
    size_t y,
    Color color
) {
    size_t index;

    if (x >= image_width || y >= image_height) {
        return;
    }

    index = (y * image_width + x) * 3u;
    image[index] = color.r;
    image[index + 1u] = color.g;
    image[index + 2u] = color.b;
}

static void fill_image(uint8_t* image, size_t pixel_count, Color color) {
    size_t i;

    for (i = 0; i < pixel_count; ++i) {
        image[i * 3u] = color.r;
        image[i * 3u + 1u] = color.g;
        image[i * 3u + 2u] = color.b;
    }
}

static void draw_missing_glyph(
    uint8_t* image,
    size_t image_width,
    size_t image_height,
    size_t x,
    size_t y,
    size_t width,
    size_t height,
    Color color
) {
    size_t xx;
    size_t yy;

    if (width < 2u || height < 2u) {
        return;
    }

    for (xx = 0; xx < width; ++xx) {
        set_pixel(image, image_width, image_height, x + xx, y, color);
        set_pixel(image, image_width, image_height, x + xx, y + height - 1u, color);
    }

    for (yy = 0; yy < height; ++yy) {
        set_pixel(image, image_width, image_height, x, y + yy, color);
        set_pixel(image, image_width, image_height, x + width - 1u, y + yy, color);
    }
}

static void draw_glyph(
    uint8_t* image,
    size_t image_width,
    size_t image_height,
    size_t x,
    size_t baseline_y,
    const RasterGlyph* glyph,
    Color color
) {
    size_t row;
    size_t col;
    size_t stride;
    int draw_x;
    int draw_y;

    if (glyph == NULL) {
        return;
    }

    draw_x = (int)x + glyph->x_offset;
    draw_y = (int)baseline_y + glyph->y_offset;
    stride = (glyph->width + 7u) / 8u;

    for (row = 0; row < glyph->height; ++row) {
        for (col = 0; col < glyph->width; ++col) {
            size_t byte_index = row * stride + (col / 8u);
            uint8_t mask = (uint8_t)(0x80u >> (col % 8u));
            if (byte_index < glyph->bitmap_size && (glyph->bitmap[byte_index] & mask) != 0u) {
                int px = draw_x + (int)col;
                int py = draw_y + (int)row;
                if (px >= 0 && py >= 0) {
                    set_pixel(image, image_width, image_height, (size_t)px, (size_t)py, color);
                }
            }
        }
    }
}

static int save_ppm(
    const char* path,
    const uint8_t* image,
    size_t width,
    size_t height
) {
    FILE* file = fopen(path, "wb");
    if (file == NULL) {
        perror("fopen");
        return 0;
    }

    if (fprintf(file, "P6\n%zu %zu\n255\n", width, height) < 0) {
        fclose(file);
        return 0;
    }

    if (fwrite(image, 1u, width * height * 3u, file) != width * height * 3u) {
        fclose(file);
        return 0;
    }

    fclose(file);
    return 1;
}

int main(int argc, char** argv) {
    const char* text;
    const char* output_path = "preview.ppm";
    Color fg = {255u, 255u, 255u};
    Color bg = {0u, 0u, 0u};
    uint32_t* codepoints;
    size_t codepoint_capacity;
    size_t codepoint_count;
    size_t width = 8u;
    size_t height = FONT_LINE_HEIGHT + 8u;
    size_t spacing = 1u;
    size_t fallback_width = FONT_SIZE / 2u;
    size_t max_above_baseline = FONT_ASCENT;
    size_t max_below_baseline = FONT_DESCENT;
    uint8_t* image;
    size_t i;
    size_t pen_x = 4u;
    size_t baseline_y = 4u + FONT_ASCENT;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    text = argv[1];

    if (argc >= 3 && !parse_color(argv[2], &fg)) {
        fprintf(stderr, "Invalid foreground color: %s\n", argv[2]);
        return 1;
    }

    if (argc >= 4 && !parse_color(argv[3], &bg)) {
        fprintf(stderr, "Invalid background color: %s\n", argv[3]);
        return 1;
    }

    if (argc >= 5) {
        output_path = argv[4];
    }

    codepoint_capacity = strlen(text);
    if (codepoint_capacity == 0u) {
        fprintf(stderr, "Input text is empty.\n");
        return 1;
    }

    codepoints = (uint32_t*)malloc(codepoint_capacity * sizeof(uint32_t));
    if (codepoints == NULL) {
        fprintf(stderr, "Out of memory.\n");
        return 1;
    }

    codepoint_count = decode_utf8(text, codepoints, codepoint_capacity);
    if (codepoint_count == 0u) {
        fprintf(stderr, "Failed to decode input text.\n");
        free(codepoints);
        return 1;
    }

    for (i = 0; i < codepoint_count; ++i) {
        const RasterGlyph* glyph = find_glyph(codepoints[i]);
        size_t glyph_width = fallback_width;
        size_t above_baseline = FONT_ASCENT;
        size_t below_baseline = FONT_DESCENT;

        if (glyph != NULL) {
            glyph_width = glyph->x_advance;
            if (glyph_width == 0u) {
                glyph_width = glyph->width;
            }

            if (glyph->y_offset < 0) {
                size_t value = (size_t)(-glyph->y_offset);
                if (value > above_baseline) {
                    above_baseline = value;
                }
            } else {
                below_baseline += (size_t)glyph->y_offset;
            }

            if (glyph->height > above_baseline && glyph->y_offset >= 0) {
                below_baseline = (size_t)glyph->y_offset + glyph->height;
            } else if (glyph->height > (size_t)(-glyph->y_offset) && glyph->y_offset < 0) {
                size_t value = glyph->height - (size_t)(-glyph->y_offset);
                if (value > below_baseline) {
                    below_baseline = value;
                }
            }
        }

        width += glyph_width;
        if (i + 1u < codepoint_count) {
            width += spacing;
        }
        if (above_baseline > max_above_baseline) {
            max_above_baseline = above_baseline;
        }
        if (below_baseline > max_below_baseline) {
            max_below_baseline = below_baseline;
        }
    }

    height = max_above_baseline + max_below_baseline + 8u;
    baseline_y = 4u + max_above_baseline;

    image = (uint8_t*)malloc(width * height * 3u);
    if (image == NULL) {
        fprintf(stderr, "Out of memory.\n");
        free(codepoints);
        return 1;
    }

    fill_image(image, width * height, bg);

    for (i = 0; i < codepoint_count; ++i) {
        const RasterGlyph* glyph = find_glyph(codepoints[i]);
        if (glyph != NULL) {
            size_t advance = glyph->x_advance;
            if (advance == 0u) {
                advance = glyph->width;
            }
            draw_glyph(image, width, height, pen_x, baseline_y, glyph, fg);
            pen_x += advance;
        } else {
            draw_missing_glyph(
                image,
                width,
                height,
                pen_x,
                baseline_y - FONT_ASCENT,
                fallback_width,
                FONT_LINE_HEIGHT,
                fg
            );
            pen_x += fallback_width;
        }

        if (i + 1u < codepoint_count) {
            pen_x += spacing;
        }
    }

    if (!save_ppm(output_path, image, width, height)) {
        fprintf(stderr, "Failed to write image: %s\n", output_path);
        free(image);
        free(codepoints);
        return 1;
    }

    printf("Saved %s (%zux%zu)\n", output_path, width, height);

    free(image);
    free(codepoints);
    return 0;
}
