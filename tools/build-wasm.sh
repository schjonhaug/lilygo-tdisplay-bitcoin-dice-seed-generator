#!/bin/sh
set -eu

command -v emcc >/dev/null 2>&1 || { echo "Emscripten (emcc) not found" >&2; exit 1; }
emcc simulator/mnemonic_wasm.cpp \
  -O3 \
  -std=c++17 \
  -sMODULARIZE=1 \
  -sEXPORT_ES6=1 \
  -sENVIRONMENT=web,node \
  -sFILESYSTEM=0 \
  -sEXPORTED_FUNCTIONS='["_generate_mnemonic_indexes","_mnemonic_word","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["HEAPU8","HEAPU16"]' \
  -o simulator/mnemonic_wasm.js
