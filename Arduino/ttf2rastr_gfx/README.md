# ttf2rastr_gfx Arduino Library

`ttf2rastr_gfx` - это небольшой helper-слой для Arduino, который умеет рисовать bitmap-шрифты, сгенерированные `ttf2rastr`, через `Adafruit_GFX`.

Библиотека не зависит от конкретного шрифта и не генерируется вместе с ним. Генерируется только файл шрифта, а сама библиотека остается общей и переиспользуемой.

В текущей версии сами глифы рисуются через `Adafruit_GFX::drawBitmap()`, а библиотека берёт на себя метрики, layout и частичное обновление строки на уровне примера.

## Структура

- `library.properties`
- `src/ttf2rastr_gfx.h`
- `src/ttf2rastr_gfx.cpp`
- `examples/ESP32_ILI9341_FrameDemo/`

## Как установить локально

Самый простой вариант:

- скопировать папку `Arduino/ttf2rastr_gfx` в ваш Arduino `libraries/`

После этого в проекте можно писать:

```cpp
#include <ttf2rastr_gfx.h>
```

## Что еще нужно кроме библиотеки

Нужен отдельный файл шрифта, который генерирует `ttf2rastr`.

Сейчас рекомендуется header-формат:

- `generated_font.h`

Пример подключения:

```cpp
#include "generated_font.h"
#include <ttf2rastr_gfx.h>

static const RasterFont DEMO_FONT = TTF2RASTR_MAKE_FONT(demo_font);
```

## Основные функции

- `ttf2rastrFindGlyph(...)`
- `ttf2rastrGlyphAdvance(...)`
- `ttf2rastrMeasureText(...)`
- `ttf2rastrPlaceTextInBox(...)`
- `ttf2rastrDrawGlyph(...)`
- `ttf2rastrDrawText(...)`
- `ttf2rastrDrawTextInBox(...)`

## Описание API

`ttf2rastrFindGlyph(const RasterFont* font, uint32_t codepoint)`

- ищет глиф по `codepoint`
- возвращает указатель на `RasterGlyph`
- если символа нет, возвращает `NULL`

`ttf2rastrGlyphAdvance(const RasterFont* font, uint32_t codepoint)`

- возвращает шаг по горизонтали для одного символа
- обычно это `x_advance`
- если глиф не найден, возвращается безопасственная fallback-ширина

`ttf2rastrMeasureText(const RasterFont* font, const char* text)`

- измеряет строку UTF-8
- возвращает `RasterTextMetrics`
- удобно, если нужно самому разместить текст

`ttf2rastrTextHeight(const RasterTextMetrics* metrics)`

- превращает метрики строки в итоговую высоту текста

`ttf2rastrPlaceTextInBox(const RasterFont* font, int16_t box_x, int16_t box_y, int16_t box_w, int16_t box_h, const char* text)`

- считает центрированное размещение строки внутри прямоугольника
- возвращает `RasterTextPlacement`
- особенно удобно для индикаторов и виджетов

`ttf2rastrDrawGlyph(Adafruit_GFX& display, const RasterFont* font, int16_t x, int16_t baseline_y, uint32_t codepoint, uint16_t color, uint16_t missing_color)`

- рисует один символ
- использует `drawBitmap()`
- если символ не найден, рисует fallback-прямоугольник

`ttf2rastrDrawText(Adafruit_GFX& display, const RasterFont* font, int16_t x, int16_t baseline_y, const char* text, uint16_t color, uint16_t missing_color)`

- рисует целую строку UTF-8
- возвращает фактическую ширину нарисованного текста
- полезно, если нужно рисовать несколько строк или элементов подряд

`ttf2rastrDrawTextInBox(Adafruit_GFX& display, const RasterFont* font, int16_t box_x, int16_t box_y, int16_t box_w, int16_t box_h, const char* text, uint16_t text_color, uint16_t bg_color, uint16_t missing_color)`

- очищает прямоугольник
- центрирует текст внутри него
- рисует строку одной функцией
- это самый удобный вызов для простых экранных элементов

## Минимальный сценарий

Если нужно просто вывести строку по центру прямоугольника, обычно достаточно:

1. Подключить `generated_font.h`
2. Создать `RasterFont` через `TTF2RASTR_MAKE_FONT(...)`
3. Вызвать `ttf2rastrDrawTextInBox(...)`

## Почему шрифт теперь `.h`

Сгенерированный файл шрифта не содержит исполняемой логики. В нем только:

- bitmap-данные
- таблица глифов
- метрики шрифта
- связанные объявления

Поэтому подключать его как header естественнее, чем компилировать как отдельный `.cpp`.

## Живой пример

- `examples/ESP32_ILI9341_FrameDemo`
