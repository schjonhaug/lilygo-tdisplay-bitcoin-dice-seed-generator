#!/bin/sh
set -eu

test "$#" = 1 || { echo "usage: $0 <serial-port>" >&2; exit 1; }
binary="build-test-device"
trap 'rm -f "$binary"' EXIT
c++ -std=c++17 -Wall -Wextra -Werror tools/test-device.cpp -o "$binary"
mkdir -p device-screenshots
./"$binary" "$1" device-screenshots
for image in device-screenshots/*.ppm; do magick "$image" "${image%.ppm}.png"; rm "$image"; done
