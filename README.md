# ttf2rastr

Небольшой инструмент для преобразования TTF/OTF-шрифтов в 1-битные растровые данные на C/C++ без STL.

Проект сейчас ориентирован на два сценария:

- генерация bitmap-глифов из выбранного набора символов
- быстрая desktop-проверка результата через локальный preview в `PPM`
- использование Arduino-библиотеки `ttf2rastr_gfx` с готовым примером для `ESP32 + ILI9341`

Это удобно как для обычной Linux-разработки, так и как подготовка к дальнейшему использованию на микроконтроллерах.


## Что умеет

- читает настройки из CLI или JSON-конфига
- загружает TTF/OTF через Pillow
- рендерит только нужные символы
- упаковывает глифы в 1-битные массивы `uint8_t`
- генерирует C-style структуры и массивы без STL
- сохраняет метрики глифов:
  - `x_offset`
  - `y_offset`
  - `x_advance`
- сохраняет метрики шрифта:
  - `font_ascent`
  - `font_descent`
  - `line_height`
- позволяет быстро проверить результат через `font_preview.cpp`


## Состав проекта

- `main.py` - генератор растрового шрифта
- `font_preview.cpp` - desktop-preview с выводом в `PPM`
- `Makefile` - сборка preview
- `Examples/` - desktop-preview
- `Arduino/ttf2rastr_gfx/` - Arduino-библиотека для отрисовки bitmap-шрифта
- `example_config.json` - пример конфигурации
- `generated_font.h` - пример сгенерированного шрифта
- `USAGE.md` - подробная инструкция
- `SESSION_NOTES.md` - рабочие заметки по развитию проекта


## Быстрый старт

### 1. Установить зависимость Python

```bash
python -m pip install pillow
```

### 2. Сгенерировать шрифт

```bash
python main.py --config example_config.json
```

### 3. Собрать preview

```bash
make
```

### 4. Проверить результат

```bash
./font_preview '0123456789_' FFCC00 102040 Examples/digits_preview.ppm
```

В репозитории также сохранен готовый снимок для быстрой проверки: `Examples/digits_preview.png`.


## Пример прямого запуска

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 72 \
  --chars "0123456789_" \
  --output generated_font.h \
  --symbol-name demo_font
```


## Формат результата

Генератор создает C/C++-файл, в котором есть:

- bitmap-массив для каждого глифа
- массив `RasterGlyph`
- массив кодов символов
- служебные константы:
  - размер шрифта
  - ascent/descent
  - высота строки
  - количество глифов
  - общий объем bitmap-данных

Текущий формат подходит для desktop/Linux-проверки и для Arduino-использования.

`generated_font.h` содержит только данные шрифта и связанные объявления, без отдельной исполняемой логики. Поэтому header-формат здесь естественнее, чем отдельный `.cpp`.


## Пример `RasterGlyph`

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


## Статус

Сейчас проект уже умеет:

- генерировать рабочий output
- корректнее раскладывать текст по baseline в preview
- использовать цифро-ориентированный набор `0123456789_` для демонстрации крупных индикаторов
- поставлять Arduino-библиотеку с примером для `ESP32 + ILI9341`

Ближайшие возможные шаги:

- embedded-режим без Unicode-поиска в runtime
- генерация `.h/.hpp` вместо `.cpp`
- демо для Wokwi / ESP32 / RP2040 / OLED


## Документация

- Подробное использование: [USAGE.md](USAGE.md)
- Рабочие заметки: [SESSION_NOTES.md](SESSION_NOTES.md)
- Отдельная Arduino-библиотека: [Arduino/ttf2rastr_gfx/README.md](Arduino/ttf2rastr_gfx/README.md)


## License

Проект лицензирован под `MIT`. Текст лицензии: [LICENSE](LICENSE).
