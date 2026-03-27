# USAGE

## Назначение

Проект состоит из двух частей:

1. `main.py`  
   Конвертер TrueType/OpenType шрифта в C/C++-файл с растровыми глифами.

2. `font_preview.cpp`  
   Проверочная программа, которая берет сгенерированный растровый шрифт и рисует строку в файл картинки формата `PPM`.


## Требования

### Для конвертера

- Linux
- Python 3
- Pillow

Если `Pillow` еще не установлен:

```bash
python -m pip install pillow
```

### Для проверочной программы

- `g++`
- `make`

На Linux Mint это обычно уже есть или ставится так:

```bash
sudo apt install build-essential
```


## Быстрый старт

### 1. Сгенерировать растровый шрифт

Через JSON-конфиг:

```bash
python main.py --config example_config.json
```

Или напрямую параметрами:

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 16 \
  --chars "ABC123абв" \
  --output generated_font.h \
  --symbol-name demo_font
```

### 2. Собрать проверочную программу

```bash
make
```

### 3. Нарисовать тестовую строку

```bash
./font_preview 'ABC123абв'
```

Результат по умолчанию: `preview.ppm`


## Конвертер `main.py`

### Что делает

Конвертер:

- загружает TTF/OTF-шрифт
- берет только указанные символы
- рендерит каждый символ в монохромный битмап
- упаковывает биты в массив байтов
- вычисляет метрики глифов и шрифта
- создает C/C++-файл без STL

На выходе генерируются:

- массив байтов для каждого символа
- массив структур `RasterGlyph`
- массив кодов символов
- служебные константы размера, ascent/descent, высоты строки и количества

### Что теперь есть в `RasterGlyph`

Каждый глиф теперь содержит не только bitmap, но и метрики:

- `codepoint`
- `x_offset`
- `y_offset`
- `x_advance`
- `width`
- `height`
- `bitmap`
- `bitmap_size`

Это важно для более корректной раскладки текста и как база для будущего embedded-рендера.


### Формат запуска

```bash
python main.py [OPTIONS]
```

### Параметры

#### `--config`

Путь к JSON-файлу конфигурации.

Пример:

```bash
python main.py --config example_config.json
```

#### `--font`

Путь к файлу шрифта `.ttf` или `.otf`.

Пример:

```bash
--font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
```

#### `--size`

Размер шрифта в пикселях.

Пример:

```bash
--size 16
```

#### `--chars`

Строка символов, которые нужно включить в выходной растровый шрифт.

Пример:

```bash
--chars "0123456789ABCDEF"
```

Пример с кириллицей:

```bash
--chars "Привет123"
```

Важно:

- дубликаты символов автоматически убираются
- если символа нет в исходном TTF, ширина или высота могут быть нулевыми
- если нужен пробел в итоговом шрифте, его тоже нужно включить в строку `chars`
- если пробел не сконвертирован, при отрисовке он будет считаться отсутствующим глифом и может отображаться как контур прямоугольника

#### `--output`

Имя выходного `.cpp`-файла.

Пример:

```bash
--output generated_font.h
```

#### `--symbol-name`

Префикс имен, используемых в сгенерированном C/C++-коде.

Пример:

```bash
--symbol-name demo_font
```

Если указан `demo_font`, то в выходном файле будут символы примерно такого вида:

```cpp
static const RasterGlyph demo_font_glyphs[];
static const size_t demo_font_glyph_count;
static const size_t demo_font_font_size;
static const size_t demo_font_font_ascent;
static const size_t demo_font_font_descent;
static const size_t demo_font_line_height;
```

#### `--threshold`

Порог бинаризации пикселей: от `0` до `255`.

По умолчанию:

```bash
--threshold 128
```

Чем меньше значение, тем "толще" может выглядеть символ после перевода в 1-битный вид.


## JSON-конфиг

Пример файла [example_config.json](/home/wlad/Projects/Python/ttf2rastr/example_config.json):

```json
{
  "font": "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
  "size": 16,
  "chars": "ABC123абв",
  "output": "generated_font.h",
  "symbol_name": "demo_font",
  "threshold": 128
}
```

### Приоритет параметров

Если параметр указан и в JSON, и в командной строке, используется значение из командной строки.

Пример:

```bash
python main.py --config example_config.json --size 20
```

В этом случае все будет взято из `example_config.json`, кроме `size`, который станет `20`.


## Примеры запуска конвертера

### Только латиница и цифры

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 14 \
  --chars "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" \
  --output font_latin.cpp \
  --symbol-name latin_font
```

### Кириллица

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 16 \
  --chars "АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ" \
  --output font_cyr.cpp \
  --symbol-name cyr_font
```

### Маленький шрифт для микроконтроллера

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 8 \
  --chars "0123456789:.%C" \
  --output font_small.cpp \
  --symbol-name sensor_font \
  --threshold 100
```


## Что находится в выходном `.cpp`

Типичный результат:

```cpp
typedef struct RasterGlyph {
    uint32_t codepoint;
    uint16_t width;
    uint16_t height;
    const uint8_t* bitmap;
    size_t bitmap_size;
} RasterGlyph;
```

Для каждого символа создается:

- отдельный байтовый массив битмапа
- запись в массиве `..._glyphs`

Также генерируются:

- `..._chars`
- `..._font_size`
- `..._glyph_count`
- `..._total_bitmap_bytes`


## Проверочная программа `font_preview.cpp`

### Что делает

Программа:

- принимает строку UTF-8
- ищет глифы в сгенерированном массиве
- рисует их слева направо
- сохраняет результат в `PPM`

Формат `PPM` выбран специально, чтобы не тянуть внешние графические библиотеки.


### Сборка

По умолчанию:

```bash
make
```

Это использует:

- файл шрифта `generated_font.h`
- префикс `demo_font`

Они задаются в [Makefile](/home/wlad/Projects/Python/ttf2rastr/Makefile):

```make
FONT_CPP ?= generated_font.h
FONT_SYMBOL ?= demo_font
```


### Если у тебя другой выходной файл

Например, конвертер создал:

- файл `font_latin.cpp`
- префикс `latin_font`

Тогда сборка будет такой:

```bash
make FONT_CPP=font_latin.cpp FONT_SYMBOL=latin_font
```


## Запуск проверочной программы

### Формат

```bash
./font_preview <text> [fg] [bg] [output.ppm]
```

### Параметры

#### `<text>`

Обязательный параметр. Строка для рисования.

Пример:

```bash
./font_preview 'ABC123'
```

#### `[fg]`

Необязательный параметр. Цвет текста.

Формат:

- `RRGGBB`
- `#RRGGBB`

Примеры:

```bash
FFCC00
#FFCC00
```

#### `[bg]`

Необязательный параметр. Цвет фона.

Пример:

```bash
102040
```

#### `[output.ppm]`

Необязательный параметр. Имя выходного файла.

По умолчанию:

```bash
preview.ppm
```


## Примеры запуска просмотрщика

### Белый текст на черном фоне

```bash
./font_preview 'ABC123'
```

### Желтый текст на темно-синем фоне

```bash
./font_preview 'ABC123абв' FFCC00 102040 sample.ppm
```

### Зеленый текст на белом фоне

```bash
./font_preview 'HELLO' 00AA00 FFFFFF hello.ppm
```


## Как открыть `PPM`

На Linux Mint файл `PPM` обычно открывается стандартным просмотрщиком изображений.

Например:

```bash
xdg-open sample.ppm
```

Если хочется PNG, можно конвертировать через ImageMagick:

```bash
convert sample.ppm sample.png
```


## Типовой сценарий работы

### Вариант 1. Через конфиг

1. Отредактировать `example_config.json`
2. Запустить:

```bash
python main.py --config example_config.json
```

3. Собрать просмотрщик:

```bash
make
```

4. Проверить рендер:

```bash
./font_preview 'Тест123' FFCC00 202020 test.ppm
```

### Вариант 2. Полностью из CLI

1. Сгенерировать шрифт:

```bash
python main.py \
  --font /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf \
  --size 12 \
  --chars "0123456789.-+ " \
  --output font_digits.cpp \
  --symbol-name digits_font
```

2. Собрать просмотрщик под этот шрифт:

```bash
make FONT_CPP=font_digits.cpp FONT_SYMBOL=digits_font
```

3. Запустить:

```bash
./font_preview '12.5-03' 00FF00 000000 digits.ppm
```


## Полезные замечания

- Генератор делает переменную ширину символов.
- Это удобнее для реальных TTF-шрифтов, чем жесткая моноширинная сетка.
- Сейчас у глифа сохраняются только:
  - код символа
  - ширина
  - высота
  - битмап
- Смещения по базовой линии (`xOffset`, `yOffset`) и `xAdvance` пока не сохраняются.
- Для простых микроконтроллерных задач этого часто хватает, но для более точной типографики потом можно расширить формат.


## Если что-то не работает

### Ошибка загрузки шрифта

Проверь, что путь к `.ttf` правильный:

```bash
ls /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf
```

### Ошибка `No module named PIL`

Установи Pillow:

```bash
python -m pip install pillow
```

### Проверочная программа не находит глифы

Убедись, что:

- строка сборки `make` использует тот же `FONT_SYMBOL`
- префикс совпадает с `--symbol-name` у конвертера
- нужные символы действительно были включены в `--chars`

### Символы выглядят слишком тонкими или толстыми

Попробуй менять `--threshold`, например:

```bash
--threshold 96
--threshold 160
```


## Файлы проекта

- [main.py](/home/wlad/Projects/Python/ttf2rastr/main.py)
- [example_config.json](/home/wlad/Projects/Python/ttf2rastr/example_config.json)
- [generated_font.h](/home/wlad/Projects/Python/ttf2rastr/generated_font.h)
- [font_preview.cpp](/home/wlad/Projects/Python/ttf2rastr/font_preview.cpp)
- [Makefile](/home/wlad/Projects/Python/ttf2rastr/Makefile)
