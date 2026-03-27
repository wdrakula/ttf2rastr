# ESP32 ILI9341 Cyrillic Demo

Пример для `ESP32 + ILI9341`, который показывает вывод нескольких кириллических букв через `ttf2rastr_gfx`.

## Что демонстрирует

- обычные UTF-8 строковые литералы в скетче: `"А"`, `"Б"`, `"В"`
- автоматическое декодирование UTF-8 в Unicode `codepoint`
- поиск глифа по `uint32_t codepoint`
- работу с bitmap-шрифтом на `Adafruit_GFX` и `ILI9341`

## Главное для пользователя

В типовом скетче не нужно вручную хранить `0x0410`, `0x0411` и так далее.

Обычно достаточно:

```cpp
ttf2rastrDrawText(tft, &font, x, baseline, "АБВ", ILI9341_YELLOW, ILI9341_RED);
```

Библиотека сама:

- читает UTF-8 строку
- декодирует следующий символ
- получает Unicode codepoint
- ищет соответствующий глиф в шрифте

## Файлы

- `ESP32_ILI9341_CyrillicDemo.ino` - основной скетч
- `generated_cyrillic_font.h` - готовый bitmap-шрифт для букв `АБВГДЕЖ`
- `cyrillic_demo_config.json` - конфиг генератора для пересборки шрифта
- `regenerate_font.sh` - локальная пересборка шрифта
- `diagram.json` и `wokwi.toml` - Wokwi-конфигурация

## Пересборка шрифта

Из корня репозитория:

```bash
bash Arduino/ttf2rastr_gfx/examples/ESP32_ILI9341_CyrillicDemo/regenerate_font.sh
```
