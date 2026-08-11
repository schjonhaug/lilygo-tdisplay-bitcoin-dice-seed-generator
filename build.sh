#!/bin/sh
set -eu

command -v arduino-cli >/dev/null 2>&1 || { echo "arduino-cli not found" >&2; exit 1; }
arduino-cli core update-index
arduino-cli core install esp32:esp32@2.0.17
arduino-cli lib install Keypad
arduino-cli compile --fqbn esp32:esp32:ttgo-lora32 --library ../lnpos/libraries/TFT_eSPI --build-path build .
