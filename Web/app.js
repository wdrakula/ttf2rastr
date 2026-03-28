"use strict";

const controls = {
  fontFile: document.getElementById("fontFile"),
  fontSize: document.getElementById("fontSize"),
  threshold: document.getElementById("threshold"),
  symbolName: document.getElementById("symbolName"),
  outputName: document.getElementById("outputName"),
  chars: document.getElementById("chars"),
  previewText: document.getElementById("previewText"),
  generateButton: document.getElementById("generateButton"),
  downloadButton: document.getElementById("downloadButton"),
  copyButton: document.getElementById("copyButton"),
  output: document.getElementById("output"),
  status: document.getElementById("status"),
  metrics: document.getElementById("metrics"),
  previewCanvas: document.getElementById("previewCanvas"),
};

const previewConfig = {
  background: "#102040",
  foreground: "#ffcc00",
  missing: "#ff6b6b",
  paddingX: 24,
  paddingY: 24,
};

let generatedState = null;
let fontFaceHandle = null;

controls.generateButton.addEventListener("click", () => {
  void generateFromForm();
});

controls.previewText.addEventListener("input", () => {
  if (generatedState) {
    drawBitmapPreview();
  }
});

controls.downloadButton.addEventListener("click", () => {
  if (!generatedState) {
    return;
  }

  const blob = new Blob([generatedState.headerText], { type: "text/plain;charset=utf-8" });
  const link = document.createElement("a");
  link.href = URL.createObjectURL(blob);
  link.download = sanitizeOutputName(controls.outputName.value.trim()) || "generated_font.h";
  link.click();
  URL.revokeObjectURL(link.href);
});

controls.copyButton.addEventListener("click", () => {
  if (!generatedState) {
    return;
  }

  void navigator.clipboard.writeText(generatedState.headerText)
    .then(() => setStatus("Header скопирован в буфер обмена."))
    .catch(() => setStatus("Не удалось скопировать header. Браузер запретил clipboard API."));
});

async function generateFromForm() {
  try {
    const settings = await collectSettings();
    setStatus("Растрирую глифы и собираю header...");

    const fontData = await loadFont(settings.file, settings.familyName);
    const glyphs = renderGlyphs(fontData.familyName, settings);
    const metrics = fontData.metrics;
    const headerText = generateHeader(glyphs, settings, metrics);

    generatedState = { settings, glyphs, metrics, headerText };
    controls.output.value = headerText;
    controls.downloadButton.disabled = false;
    controls.copyButton.disabled = false;

    drawBitmapPreview();
    setStatus(`Готово: ${glyphs.length} глифов из "${settings.file.name}".`);
  } catch (error) {
    console.error(error);
    generatedState = null;
    controls.downloadButton.disabled = true;
    controls.copyButton.disabled = true;
    controls.output.value = "";
    clearPreview();
    setStatus(error instanceof Error ? error.message : String(error));
  }
}

async function collectSettings() {
  const file = controls.fontFile.files?.[0];
  if (!file) {
    throw new Error("Нужно выбрать файл шрифта .ttf или .otf.");
  }

  const size = Number.parseInt(controls.fontSize.value, 10);
  if (!Number.isInteger(size) || size <= 0) {
    throw new Error("Размер шрифта должен быть положительным целым числом.");
  }

  const threshold = Number.parseInt(controls.threshold.value, 10);
  if (!Number.isInteger(threshold) || threshold < 0 || threshold > 255) {
    throw new Error("Threshold должен быть целым числом в диапазоне 0..255.");
  }

  const chars = controls.chars.value;
  if (!chars) {
    throw new Error("Нужно указать хотя бы один символ для генерации.");
  }

  const symbolName = sanitizeIdentifier(controls.symbolName.value);
  const familyName = `ttf2rastr_${makeUniqueId()}`;

  return {
    file,
    size,
    threshold,
    chars,
    symbolName,
    familyName,
  };
}

async function loadFont(file, familyName) {
  if (fontFaceHandle) {
    document.fonts.delete(fontFaceHandle);
    fontFaceHandle = null;
  }

  const buffer = await file.arrayBuffer();
  const fontFace = new FontFace(familyName, buffer);
  await fontFace.load();
  document.fonts.add(fontFace);
  await document.fonts.ready;
  fontFaceHandle = fontFace;

  const metricsCanvas = document.createElement("canvas");
  const metricsContext = metricsCanvas.getContext("2d", { willReadFrequently: true });
  metricsContext.font = `${baseFontCss(Number.parseInt(controls.fontSize.value, 10), familyName)}`;
  metricsContext.textBaseline = "alphabetic";

  const sample = metricsContext.measureText("Hg");
  const ascent = Math.max(0, Math.ceil(sample.actualBoundingBoxAscent || 0));
  const descent = Math.max(0, Math.ceil(sample.actualBoundingBoxDescent || 0));

  return {
    familyName,
    metrics: {
      ascent,
      descent,
      lineHeight: ascent + descent,
    },
  };
}

function renderGlyphs(fontFamily, settings) {
  const uniqueChars = [];
  const seen = new Set();

  for (const char of settings.chars) {
    if (!seen.has(char)) {
      seen.add(char);
      uniqueChars.push(char);
    }
  }

  return uniqueChars.map((char) => renderGlyph(fontFamily, char, settings));
}

function renderGlyph(fontFamily, char, settings) {
  const oversample = 4;
  const padding = Math.max(12, Math.ceil(settings.size * 0.75));

  const canvas = document.createElement("canvas");
  const context = canvas.getContext("2d", { willReadFrequently: true });
  context.font = baseFontCss(settings.size, fontFamily);
  context.textBaseline = "alphabetic";
  context.textAlign = "left";

  const metrics = context.measureText(char);
  const xAdvance = Math.max(0, Math.round(metrics.width));
  const bboxLeft = metrics.actualBoundingBoxLeft || 0;
  const bboxRight = metrics.actualBoundingBoxRight || 0;
  const bboxAscent = metrics.actualBoundingBoxAscent || 0;
  const bboxDescent = metrics.actualBoundingBoxDescent || 0;
  const fallbackLeft = Math.floor(-bboxLeft);
  const fallbackTop = Math.floor(-bboxAscent);
  const fallbackWidth = Math.max(0, Math.ceil(bboxLeft + bboxRight));
  const fallbackHeight = Math.max(0, Math.ceil(bboxAscent + bboxDescent));

  if (fallbackWidth === 0 || fallbackHeight === 0) {
    return {
      char,
      codepoint: char.codePointAt(0),
      xOffset: fallbackLeft,
      yOffset: fallbackTop,
      xAdvance,
      width: fallbackWidth,
      height: fallbackHeight,
      bitmap: [],
    };
  }

  canvas.width = (fallbackWidth + padding * 2) * oversample;
  canvas.height = (fallbackHeight + padding * 2) * oversample;

  const hi = canvas.getContext("2d", { willReadFrequently: true });
  hi.scale(oversample, oversample);
  hi.fillStyle = "#000000";
  hi.fillRect(0, 0, canvas.width, canvas.height);
  hi.fillStyle = "#ffffff";
  hi.font = baseFontCss(settings.size, fontFamily);
  hi.textBaseline = "alphabetic";

  const baselineX = padding + fallbackLeft;
  const baselineY = padding + bboxAscent;
  hi.fillText(char, baselineX, baselineY);

  const hiImage = hi.getImageData(0, 0, canvas.width, canvas.height);
  const bounds = findInkBounds(hiImage.data, canvas.width, canvas.height, 1);

  if (!bounds) {
    return {
      char,
      codepoint: char.codePointAt(0),
      xOffset: fallbackLeft,
      yOffset: fallbackTop,
      xAdvance,
      width: fallbackWidth,
      height: fallbackHeight,
      bitmap: [],
    };
  }

  const downsampled = document.createElement("canvas");
  downsampled.width = Math.ceil(bounds.width / oversample);
  downsampled.height = Math.ceil(bounds.height / oversample);
  const down = downsampled.getContext("2d", { willReadFrequently: true });
  down.drawImage(
    canvas,
    bounds.left,
    bounds.top,
    bounds.width,
    bounds.height,
    0,
    0,
    downsampled.width,
    downsampled.height
  );

  const image = down.getImageData(0, 0, downsampled.width, downsampled.height);
  const packed = packBitsFromImage(image.data, downsampled.width, downsampled.height, settings.threshold);

  const xOffset = Math.floor(bounds.left / oversample - baselineX);
  const yOffset = Math.floor(bounds.top / oversample - baselineY);

  return {
    char,
    codepoint: char.codePointAt(0),
    xOffset,
    yOffset,
    xAdvance,
    width: downsampled.width,
    height: downsampled.height,
    bitmap: packed,
  };
}

function packBitsFromImage(rgba, width, height, threshold) {
  const stride = Math.ceil(width / 8);
  const result = new Array(stride * height).fill(0);

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = (y * width + x) * 4;
      const value = rgba[pixelIndex];
      if (value >= threshold) {
        const index = y * stride + Math.floor(x / 8);
        const bit = 7 - (x % 8);
        result[index] |= 1 << bit;
      }
    }
  }

  return result;
}

function findInkBounds(rgba, width, height, minValue) {
  let left = width;
  let top = height;
  let right = -1;
  let bottom = -1;

  for (let y = 0; y < height; y += 1) {
    for (let x = 0; x < width; x += 1) {
      const pixelIndex = (y * width + x) * 4;
      const value = Math.max(
        rgba[pixelIndex],
        rgba[pixelIndex + 1],
        rgba[pixelIndex + 2]
      );
      if (value >= minValue) {
        if (x < left) {
          left = x;
        }
        if (y < top) {
          top = y;
        }
        if (x > right) {
          right = x;
        }
        if (y > bottom) {
          bottom = y;
        }
      }
    }
  }

  if (right < left || bottom < top) {
    return null;
  }

  return {
    left,
    top,
    width: right - left + 1,
    height: bottom - top + 1,
  };
}

function generateHeader(glyphs, settings, metrics) {
  const symbol = settings.symbolName;
  const totalBytes = glyphs.reduce((sum, glyph) => sum + glyph.bitmap.length, 0);
  let codepointValues = glyphs.map((glyph) => `0x${hex(glyph.codepoint, 8)}u`).join(", ");
  if (!codepointValues) {
    codepointValues = "0u";
  }

  const lines = [
    "#include <stddef.h>",
    "#include <stdint.h>",
    "",
    "#if defined(ARDUINO)",
    "#include <Arduino.h>",
    "#endif",
    "",
    "#ifndef TTF2RASTR_STORAGE",
    "#if defined(ARDUINO)",
    "#define TTF2RASTR_STORAGE PROGMEM",
    "#else",
    "#define TTF2RASTR_STORAGE",
    "#endif",
    "#endif",
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
  ];

  for (const glyph of glyphs) {
    const glyphSymbol = `${symbol}_u${hex(glyph.codepoint, 4)}`;
    const bitmapValues = glyph.bitmap.length > 0
      ? glyph.bitmap.map((byte) => `0x${hex(byte, 2)}`).join(", ")
      : "0x00";

    lines.push(
      `static const uint8_t ${glyphSymbol}[${Math.max(1, glyph.bitmap.length)}] TTF2RASTR_STORAGE = {`,
      `    ${bitmapValues}`,
      "};",
      ""
    );
  }

  lines.push(`static const RasterGlyph ${symbol}_glyphs[${glyphs.length}] TTF2RASTR_STORAGE = {`);
  glyphs.forEach((glyph, index) => {
    const suffix = index === glyphs.length - 1 ? "" : ",";
    const glyphSymbol = `${symbol}_u${hex(glyph.codepoint, 4)}`;
    lines.push(
      `    {0x${hex(glyph.codepoint, 8)}u, ${glyph.xOffset}, ${glyph.yOffset}, ${glyph.xAdvance}, ${glyph.width}, ${glyph.height}, ${glyphSymbol}, ${glyph.bitmap.length}}${suffix}`
    );
  });
  lines.push("};", "");

  lines.push(
    `static const uint32_t ${symbol}_chars[${glyphs.length}] TTF2RASTR_STORAGE = {${codepointValues}};`,
    "",
    `static const size_t ${symbol}_font_size TTF2RASTR_STORAGE = ${settings.size}u;`,
    `static const size_t ${symbol}_font_ascent TTF2RASTR_STORAGE = ${metrics.ascent}u;`,
    `static const size_t ${symbol}_font_descent TTF2RASTR_STORAGE = ${metrics.descent}u;`,
    `static const size_t ${symbol}_line_height TTF2RASTR_STORAGE = ${metrics.lineHeight}u;`,
    `static const size_t ${symbol}_glyph_count TTF2RASTR_STORAGE = ${glyphs.length}u;`,
    `static const size_t ${symbol}_total_bitmap_bytes TTF2RASTR_STORAGE = ${totalBytes}u;`,
    ""
  );

  return `${lines.join("\n")}\n`;
}

function drawBitmapPreview() {
  if (!generatedState) {
    clearPreview();
    return;
  }

  const { glyphs, metrics } = generatedState;
  const text = controls.previewText.value || generatedState.settings.chars;
  const glyphMap = new Map(glyphs.map((glyph) => [glyph.codepoint, glyph]));
  const canvas = controls.previewCanvas;
  const context = canvas.getContext("2d");

  context.fillStyle = previewConfig.background;
  context.fillRect(0, 0, canvas.width, canvas.height);

  const baseline = previewConfig.paddingY + metrics.ascent;
  let cursorX = previewConfig.paddingX;
  const missingAdvance = Math.max(4, Math.round(generatedState.settings.size * 0.6));
  let previewBitmapBytes = 0;

  for (const char of text) {
    const codepoint = char.codePointAt(0);
    const glyph = glyphMap.get(codepoint);

    if (glyph) {
      drawGlyphBitmap(context, cursorX, baseline, glyph, previewConfig.foreground);
      cursorX += glyph.xAdvance;
      previewBitmapBytes += glyph.bitmap.length;
    } else {
      drawMissingGlyph(context, cursorX, baseline - metrics.ascent, missingAdvance, metrics.lineHeight);
      cursorX += missingAdvance;
    }
  }

  const maxGlyphWidth = glyphs.reduce((max, glyph) => Math.max(max, glyph.width), 0);
  const maxGlyphHeight = glyphs.reduce((max, glyph) => Math.max(max, glyph.height), 0);
  const maxGlyphBytes = glyphs.reduce((max, glyph) => Math.max(max, glyph.bitmap.length), 0);

  context.strokeStyle = "rgba(255,255,255,0.25)";
  context.beginPath();
  context.moveTo(0, baseline + 0.5);
  context.lineTo(canvas.width, baseline + 0.5);
  context.stroke();

  controls.metrics.textContent = [
    `glyphs: ${glyphs.length}`,
    `ascent: ${metrics.ascent}`,
    `descent: ${metrics.descent}`,
    `line height: ${metrics.lineHeight}`,
    `max glyph: ${maxGlyphWidth}x${maxGlyphHeight}`,
    `max glyph bytes: ${maxGlyphBytes}`,
    `font bitmap bytes: ${glyphs.reduce((sum, glyph) => sum + glyph.bitmap.length, 0)}`,
    `preview bitmap bytes: ${previewBitmapBytes}`,
    `preview text width: ${cursorX - previewConfig.paddingX}px`,
  ].join(" | ");
}

function drawGlyphBitmap(context, x, baselineY, glyph, fillStyle) {
  const stride = Math.ceil(glyph.width / 8);
  context.fillStyle = fillStyle;

  for (let row = 0; row < glyph.height; row += 1) {
    for (let col = 0; col < glyph.width; col += 1) {
      const byteIndex = row * stride + Math.floor(col / 8);
      const mask = 0x80 >> (col % 8);
      if (byteIndex < glyph.bitmap.length && (glyph.bitmap[byteIndex] & mask) !== 0) {
        const px = x + glyph.xOffset + col;
        const py = baselineY + glyph.yOffset + row;
        context.fillRect(px, py, 1, 1);
      }
    }
  }
}

function drawMissingGlyph(context, x, y, width, height) {
  context.strokeStyle = previewConfig.missing;
  context.strokeRect(x + 0.5, y + 0.5, Math.max(2, width - 1), Math.max(2, height - 1));
}

function clearPreview() {
  const context = controls.previewCanvas.getContext("2d");
  context.fillStyle = previewConfig.background;
  context.fillRect(0, 0, controls.previewCanvas.width, controls.previewCanvas.height);
  controls.metrics.textContent = "";
}

function sanitizeIdentifier(value) {
  const cleaned = value.trim().replace(/\W+/g, "_");
  if (!cleaned) {
    return "raster_font";
  }
  return /^\d/.test(cleaned) ? `_${cleaned}` : cleaned;
}

function sanitizeOutputName(value) {
  return value.replace(/[\\/:*?"<>|]+/g, "_");
}

function setStatus(message) {
  controls.status.textContent = message;
}

function baseFontCss(size, familyName) {
  return `${size}px "${familyName}"`;
}

function hex(value, width) {
  return value.toString(16).toUpperCase().padStart(width, "0");
}

function makeUniqueId() {
  if (globalThis.crypto && typeof globalThis.crypto.randomUUID === "function") {
    return globalThis.crypto.randomUUID().replace(/-/g, "");
  }
  return `${Date.now().toString(16)}${Math.random().toString(16).slice(2)}`;
}

clearPreview();
