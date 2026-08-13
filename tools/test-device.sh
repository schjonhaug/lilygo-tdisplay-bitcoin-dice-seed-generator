#!/bin/sh
set -eu

test "$#" = 1 || { echo "usage: $0 <serial-port>" >&2; exit 1; }
test -f build-device-test/lilygo-tdisplay-bitcoin-dice-seed-generator.ino.bin || { echo "Run ./build.sh --device-test first" >&2; exit 1; }
binary="build-test-device"
trap 'rm -f "$binary"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror tools/test-device.cpp -o "$binary"
mkdir -p device-screenshots
./"$binary" "$1" device-screenshots
for image in device-screenshots/*.ppm; do magick "$image" "${image%.ppm}.png"; rm "$image"; done
magick device-screenshots/start.png assets/screenshots/start.png
magick device-screenshots/rolls-99-complete.png assets/screenshots/rolls.png
magick device-screenshots/word-24-01.png assets/screenshots/words.png
magick device-screenshots/verify-prompt-24.png assets/screenshots/verify.png
magick device-screenshots/quiz-24.png assets/screenshots/quiz.png
