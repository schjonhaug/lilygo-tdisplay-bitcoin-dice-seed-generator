#!/bin/sh
set -eu

g++ -std=c++17 -Wall -Wextra -Werror tests/test_mnemonic.cpp -o build-test-mnemonic
./build-test-mnemonic
rm build-test-mnemonic
