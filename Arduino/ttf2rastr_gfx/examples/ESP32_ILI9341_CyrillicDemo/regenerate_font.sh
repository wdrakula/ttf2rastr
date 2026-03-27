#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/../../../.."
python main.py --config Arduino/ttf2rastr_gfx/examples/ESP32_ILI9341_CyrillicDemo/cyrillic_demo_config.json
