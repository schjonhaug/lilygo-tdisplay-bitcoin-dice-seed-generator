#!/bin/sh
set -eu

command -v arduino-cli >/dev/null 2>&1 || { echo "arduino-cli not found" >&2; exit 1; }
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
arduino-cli lib install Keypad@3.1.1 TFT_eSPI@2.5.43
arduino-cli compile \
  --fqbn esp32:esp32:ttgo-lora32 \
  --build-property "compiler.cpp.extra_flags=-DUSER_SETUP_LOADED -DST7789_DRIVER -DTFT_WIDTH=135 -DTFT_HEIGHT=240 -DCGRAM_OFFSET -DTFT_MOSI=19 -DTFT_SCLK=18 -DTFT_CS=5 -DTFT_DC=16 -DTFT_RST=23 -DTFT_BL=4 -DTFT_BACKLIGHT_ON=HIGH -DLOAD_GLCD -DLOAD_FONT2 -DLOAD_FONT4 -DSPI_FREQUENCY=40000000" \
  --build-path build .
