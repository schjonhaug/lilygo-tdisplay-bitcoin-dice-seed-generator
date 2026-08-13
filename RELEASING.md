# Releasing

Only create a release after native tests, WebAssembly tests, a clean firmware build, and on-device testing pass.

```sh
./tests/run.sh
./tools/build-wasm.sh
node tests/test_wasm.mjs
./build.sh
shasum -a 256 build/lilygo-tdisplay-dice-seed.ino.bin
git status --short
git tag -a vX.Y.Z -m "vX.Y.Z"
git push origin main vX.Y.Z
```

Attach `build/lilygo-tdisplay-dice-seed.ino.bin` to the GitHub release and publish its SHA-256 hash in the release notes. Record the source tag and Arduino-ESP32 core version (`3.3.11`).

Test the release binary on a device with public vectors before recommending it for real seed generation.
