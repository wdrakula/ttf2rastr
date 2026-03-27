#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <stdio.h>

#include <ttf2rastr_gfx.h>
#include "generated_cyrillic_font.h"

constexpr uint8_t TFT_CS = 15;
constexpr uint8_t TFT_DC = 2;
constexpr uint8_t TFT_RST = 4;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

static const RasterFont DEMO_FONT = TTF2RASTR_MAKE_FONT(cyrillic_demo_font);

static const uint16_t COLOR_BG = ILI9341_NAVY;
static const uint16_t COLOR_TEXT = ILI9341_YELLOW;
static const uint16_t COLOR_FRAME = ILI9341_DARKCYAN;
static const uint16_t COLOR_LABEL = ILI9341_WHITE;
static const uint16_t COLOR_MISSING = ILI9341_RED;

static const int16_t VALUE_BOX_X = 12;
static const int16_t VALUE_BOX_Y = 82;
static const int16_t VALUE_BOX_W = 216;
static const int16_t VALUE_BOX_H = 128;

static const char* LETTER_SEQUENCE[] = {
    "А",
    "Б",
    "В",
    "Г",
    "Д",
    "Е",
    "Ж",
};

static const size_t LETTER_COUNT =
    sizeof(LETTER_SEQUENCE) / sizeof(LETTER_SEQUENCE[0]);

static size_t currentLetterIndex = 0;

// Minimal UTF-8 decoder for the first character in a C string.
// This is only for demonstration and debug output.
// Normal rendering code can simply pass UTF-8 strings to ttf2rastrDrawText().
static uint32_t firstCodepointFromUtf8(const char* text) {
  const uint8_t* src = (const uint8_t*)text;
  uint32_t codepoint = 0;
  size_t extra = 0;

  if (src == NULL || *src == '\0') {
    return 0;
  }

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
    return '?';
  }

  ++src;

  while (extra > 0 && *src != '\0') {
    if ((*src & 0xC0u) != 0x80u) {
      return '?';
    }
    codepoint = (codepoint << 6) | (*src & 0x3Fu);
    ++src;
    --extra;
  }

  return codepoint;
}

static void drawStaticScreen() {
  tft.fillScreen(COLOR_BG);

  tft.drawRect(8, 8, 224, 304, COLOR_FRAME);
  tft.drawRect(12, 12, 216, 296, COLOR_FRAME);

  tft.setTextColor(COLOR_LABEL);
  tft.setTextSize(1);
  tft.setCursor(18, 20);
  tft.print("ESP32 / ILI9341 / ttf2rastr");

  tft.setTextSize(2);
  tft.setCursor(18, 42);
  tft.print("UTF-8 Cyrillic demo");

  tft.drawRoundRect(VALUE_BOX_X, VALUE_BOX_Y, VALUE_BOX_W, VALUE_BOX_H, 8,
                    COLOR_FRAME);

  tft.setTextSize(1);
  tft.setCursor(20, 226);
  tft.print("Pass UTF-8 text like ");
  tft.print("\"");
  tft.print("А");
  tft.print("\"");
  tft.setCursor(20, 242);
  tft.print("Library decodes it to Unicode");
  tft.setCursor(20, 258);
  tft.print("Lookup uses uint32_t codepoint");
}

static void drawLetter(size_t index) {
  char info[32];
  const char* letter = LETTER_SEQUENCE[index];
  const uint32_t codepoint = firstCodepointFromUtf8(letter);
  const RasterTextPlacement placement =
      ttf2rastrPlaceTextInBox(&DEMO_FONT, VALUE_BOX_X + 2, VALUE_BOX_Y + 2,
                              VALUE_BOX_W - 4, VALUE_BOX_H - 4, letter);

  tft.fillRect(VALUE_BOX_X + 2, VALUE_BOX_Y + 2, VALUE_BOX_W - 4,
               VALUE_BOX_H - 4, COLOR_BG);
  ttf2rastrDrawText(tft, &DEMO_FONT, placement.x, placement.baseline_y, letter,
                    COLOR_TEXT, COLOR_MISSING);

  snprintf(info, sizeof(info), "UTF-8 -> U+%04lX", (unsigned long)codepoint);

  tft.fillRect(20, 276, 200, 14, COLOR_BG);
  tft.setTextSize(1);
  tft.setTextColor(COLOR_LABEL);
  tft.setCursor(20, 276);
  tft.print(info);
}

void setup() {
  tft.begin();
  tft.setRotation(0);
  drawStaticScreen();
  drawLetter(currentLetterIndex);
}

void loop() {
  currentLetterIndex = (currentLetterIndex + 1u) % LETTER_COUNT;
  drawLetter(currentLetterIndex);
  delay(700);
}
