#include <stdint.h>

#include "../src/mnemonic.h"

extern "C" {
int generate_mnemonic_indexes(const char* rolls, int roll_count, uint16_t* indexes) {
  size_t word_count = 0;
  if (roll_count < 0 || !generateMnemonicIndexes(rolls, static_cast<size_t>(roll_count), indexes, word_count)) {
    return 0;
  }
  return static_cast<int>(word_count);
}

int mnemonic_word(uint16_t index, char* output, int output_size) {
  if (index >= 2048 || output_size < 2) return 0;
  wordAt(index, output, static_cast<size_t>(output_size));
  int length = 0;
  while (output[length]) ++length;
  return length;
}
}
