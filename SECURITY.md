# Security Policy

## Threat Model

This project is a dice-to-BIP39 mnemonic generator for a LilyGO T-Display Keyboard. It is not a hardware wallet, secure element, or tamper-resistant device.

- Build and flash firmware from a trusted host, then disconnect USB before entering real rolls.
- The ESP32 cannot protect against compromised firmware, a compromised flashing host, or physical access to the device.
- Wi-Fi and Bluetooth are disabled by the application, but the ESP32 contains both radios.
- Do not photograph, share, reuse, or enter real dice rolls into the browser simulator or a normal desktop command line.

## Reporting Vulnerabilities

Do not open a public issue for a suspected vulnerability. Report it privately to [andreas@schjonhaug.com](mailto:andreas@schjonhaug.com) with enough detail to reproduce it.
