# T-Display Dice Seed

Offline BIP39 English mnemonic generator for the LilyGO TTGO T-Display with the LilyGO 4x3 keyboard module used by LNPoS.

The firmware uses the same deterministic dice construction as SeedSigner and Coldcard's dedicated dice-only flow:

- 12 words: exactly 50 rolls, `SHA256(ASCII rolls)` truncated to 16 bytes.
- 24 words: exactly 99 rolls, full `SHA256(ASCII rolls)` digest.

It does not use the ESP32 RNG, convert dice to base 6, store seed material, enable networking, or log seed material to serial output.

## Hardware

- LilyGO TTGO T-Display ESP32 with ST7789 display.
- LilyGO 4x3 keyboard module.
- Keypad rows: GPIO `21`, `27`, `26`, `22`.
- Keypad columns: GPIO `33`, `32`, `25`.

## Controls

- Start screen: `1` selects 12 words; `2` selects 24 words.
- Dice entry: `1` through `6` enter rolls; `*` removes the latest roll.
- Before the exact roll count, `#` cancels to the main menu. At exactly 50 or 99 rolls, `#` generates the mnemonic immediately. After the final word page, `#` starts a shuffled all-word backup quiz; `*` skips it. During the quiz, keys `1` through `4` select the displayed answer and `*` offers a confirmed skip.

## Build

The build uses the known-good T-Display `TFT_eSPI` setup vendored by the adjacent `~/Developer/lnpos` checkout. It is a build-time display-driver dependency only; LNPoS firmware and configuration are not included in the output.

```sh
cd ~/Developer/tdisplay-dice-seed
./tests/run.sh
./build.sh
arduino-cli upload --fqbn esp32:esp32:ttgo-lora32 --input-dir build --port /dev/tty.usbserial-XXXX --upload-property upload.speed=115200
```

`build.sh` installs Arduino ESP32 core `2.0.17` and `Keypad`. Install `arduino-cli` before running it.

## Verification

The test suite checks these public compatibility vectors:

- SeedSigner 50-roll vector: `12345612345612345612345612345612345612345612345612`.
- The public 99-roll test sequence and mnemonic from [Why Dice-Only Coldcard Seeds Were Unaffected by the RNG Bug](https://schjonhaug.dev/articles/why-dice-only-coldcard-seeds-were-unaffected-by-the-rng-bug/).

For an independent check, run SeedSigner locally or use an offline Ian Coleman copy with entropy interpreted as `Base 10 [0-9]` or `Hex [0-9A-F]`, not `Dice [1-6]`.

## Local Simulator

Use the terminal simulator to verify public test data against the exact firmware mnemonic logic before flashing the device:

```sh
./tools/run-local.sh 133363436436436415622614221225242212144161454643266122155666664444633643543353132626522332412313253
```

It prints the 24 public words from the article. Do not pass real dice rolls to this command: command-line arguments can be saved in shell history and visible to other local processes.

For a visual simulation of the display and keypad, regenerate its wordlist if needed and open `simulator/index.html` in a browser:

```sh
node tools/generate_wordlist_header.mjs
open simulator/index.html
```

Click `Load public 99-roll article vector`, then press `#` twice to view the words. The browser simulator is only for public vectors and UI review.
