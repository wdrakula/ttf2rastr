#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <stdio.h>

#include <ttf2rastr_gfx.h>
#include "generated_font.h"

constexpr uint8_t TFT_CS = 15;
constexpr uint8_t TFT_DC = 2;
constexpr uint8_t TFT_RST = 4;

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

static const RasterFont DEMO_FONT = TTF2RASTR_MAKE_FONT(demo_font);

static const uint16_t COLOR_BG = ILI9341_NAVY;
static const uint16_t COLOR_TEXT = ILI9341_YELLOW;
static const uint16_t COLOR_FRAME = ILI9341_DARKCYAN;
static const uint16_t COLOR_LABEL = ILI9341_WHITE;
static const uint16_t COLOR_MISSING = ILI9341_RED;

static const int16_t VALUE_BOX_X = 12;
static const int16_t VALUE_BOX_Y = 82;
static const int16_t VALUE_BOX_W = 216;
static const int16_t VALUE_BOX_H = 112;
static const char FRAME_TEMPLATE[] = "88_88";

static RasterTextPlacement framePlacement;
static int16_t frameCharX[sizeof(FRAME_TEMPLATE) - 1];
static int16_t frameCharAdvance[sizeof(FRAME_TEMPLATE) - 1];
static char currentFrameText[sizeof(FRAME_TEMPLATE)] = "00_00";

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
  tft.print("Library frame demo");

  tft.drawRoundRect(VALUE_BOX_X, VALUE_BOX_Y, VALUE_BOX_W, VALUE_BOX_H, 8,
                    COLOR_FRAME);

  tft.setTextSize(1);
  tft.setCursor(20, 212);
  tft.print("Sequence: MM_SS");
  tft.setCursor(20, 228);
  tft.print("Only changed symbols are redrawn");
}

static void drawFrameValue(uint16_t frame_value) {
  char buffer[8];
  const uint16_t minutes = (frame_value / 60u) % 100u;
  const uint16_t seconds = frame_value % 60u;

  snprintf(buffer, sizeof(buffer), "%02u_%02u", minutes, seconds);

  for (size_t i = 0; i < sizeof(currentFrameText) - 1; ++i) {
    if (buffer[i] == currentFrameText[i]) {
      continue;
    }

    tft.fillRect(frameCharX[i], VALUE_BOX_Y + 2, frameCharAdvance[i],
                 VALUE_BOX_H - 4, COLOR_BG);
    ttf2rastrDrawGlyph(tft, &DEMO_FONT, frameCharX[i], framePlacement.baseline_y,
                       (uint8_t)buffer[i], COLOR_TEXT, COLOR_MISSING);
    currentFrameText[i] = buffer[i];
  }
}

static void initializeFrameLayout() {
  int16_t pen_x = 0;

  framePlacement = ttf2rastrPlaceTextInBox(&DEMO_FONT, VALUE_BOX_X + 2,
                                           VALUE_BOX_Y + 2, VALUE_BOX_W - 4,
                                           VALUE_BOX_H - 4, FRAME_TEMPLATE);

  for (size_t i = 0; i < sizeof(FRAME_TEMPLATE) - 1; ++i) {
    frameCharX[i] = framePlacement.x + pen_x;
    frameCharAdvance[i] =
        (int16_t)ttf2rastrGlyphAdvance(&DEMO_FONT, (uint8_t)FRAME_TEMPLATE[i]);
    pen_x += frameCharAdvance[i];
  }

  tft.fillRect(VALUE_BOX_X + 2, VALUE_BOX_Y + 2, VALUE_BOX_W - 4, VALUE_BOX_H - 4,
               COLOR_BG);
  ttf2rastrDrawText(tft, &DEMO_FONT, framePlacement.x, framePlacement.baseline_y,
                    currentFrameText, COLOR_TEXT, COLOR_MISSING);
}

void setup() {
  tft.begin();
  tft.setRotation(0);
  drawStaticScreen();
  initializeFrameLayout();
}

void loop() {
  static uint16_t frame_value = 0;

  drawFrameValue(frame_value);
  frame_value = (uint16_t)((frame_value + 1u) % 6000u);
  delay(400);
}
