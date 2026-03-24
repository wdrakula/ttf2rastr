# ttf2rastr

Небольшой инструмент для преобразования TTF/OTF-шрифтов в 1-битные растровые данные на C/C++ без STL.

Проект сейчас ориентирован на два сценария:

- генерация bitmap-глифов из выбранного набора символов
- быстрая desktop-проверка результата через локальный preview в `PPM`

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
- `example_config.json` - пример конфигурации
- `generated_font.cpp` - пример сгенерированного шрифта
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
./font_preview 'ABC123абв' FFCC00 102040 sample.ppm
```


## Пример прямого запуска

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 16 \
  --chars "ABC123абв" \
  --output generated_font.cpp \
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

Текущий формат подходит для desktop/Linux-проверки. Для embedded-сценариев планируется отдельный более дружественный режим адресации глифов.


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
- использоваться как база для следующего этапа с embedded-friendly форматом

Ближайшие возможные шаги:

- embedded-режим без Unicode-поиска в runtime
- генерация `.h/.hpp` вместо `.cpp`
- демо для Wokwi / ESP32 / RP2040 / OLED


## Документация

- Подробное использование: [USAGE.md](USAGE.md)
- Рабочие заметки: [SESSION_NOTES.md](SESSION_NOTES.md)


## License

Проект лицензирован под `MIT`. Текст лицензии: [LICENSE](LICENSE).
