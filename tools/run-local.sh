#!/bin/sh
set -eu

binary="build-dice-seed-cli"
trap 'rm -f "$binary"' EXIT
g++ -std=c++17 -Wall -Wextra -Werror tools/dice_seed_cli.cpp -o "$binary"
./"$binary" "$@"
