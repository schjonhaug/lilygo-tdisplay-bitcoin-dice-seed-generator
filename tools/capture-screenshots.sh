#!/bin/sh
set -eu

test -f simulator/mnemonic_wasm.js && test -f simulator/mnemonic_wasm.wasm || {
  echo "Run tools/build-wasm.sh first" >&2
  exit 1
}

mkdir -p assets/screenshots
tools/serve-simulator.sh >/tmp/lilygo-tdisplay-simulator.log 2>&1 &
server_pid=$!
trap 'kill "$server_pid" 2>/dev/null || true' EXIT
until curl --silent --fail http://localhost:8000/ >/dev/null; do sleep 1; done

chrome="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"
for screen in start rolls words verify quiz; do
  "$chrome" --headless --disable-gpu --hide-scrollbars --window-size=240,135 \
    --screenshot="assets/screenshots/${screen}.png" \
    "http://localhost:8000/?screenshot=${screen}&capture=1" >/dev/null 2>&1
done
