# ttf2rastr_gfx Arduino Library

`ttf2rastr_gfx` - это небольшой helper-слой для Arduino, который умеет рисовать bitmap-шрифты, сгенерированные `ttf2rastr`, через `Adafruit_GFX`.

Библиотека не зависит от конкретного шрифта и не генерируется вместе с ним. Генерируется только файл шрифта, а сама библиотека остается общей и переиспользуемой.

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

## Почему шрифт теперь `.h`

Сгенерированный файл шрифта не содержит исполняемой логики. В нем только:

- bitmap-данные
- таблица глифов
- метрики шрифта
- связанные объявления

Поэтому подключать его как header естественнее, чем компилировать как отдельный `.cpp`.

## Живой пример

- `examples/ESP32_ILI9341_FrameDemo`
