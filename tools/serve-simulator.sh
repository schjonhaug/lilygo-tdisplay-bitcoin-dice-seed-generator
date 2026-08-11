#!/bin/sh
set -eu

test -f simulator/mnemonic_wasm.js && test -f simulator/mnemonic_wasm.wasm || {
  echo "Run tools/build-wasm.sh first" >&2
  exit 1
}
python3 -m http.server --directory simulator 8000
