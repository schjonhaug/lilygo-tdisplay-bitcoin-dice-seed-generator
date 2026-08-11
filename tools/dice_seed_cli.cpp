#include <stdio.h>
#include <string.h>

#include "../src/mnemonic.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <50-or-99-dice-rolls>\n", argv[0]);
    return 2;
  }

  const size_t rollCount = strlen(argv[1]);
  uint16_t indexes[24] = {};
  size_t wordCount = 0;
  if (!generateMnemonicIndexes(argv[1], rollCount, indexes, wordCount)) {
    fprintf(stderr, "Enter exactly 50 or 99 rolls, using only digits 1 through 6.\n");
    return 2;
  }

  char word[12];
  printf("%u rolls -> %u BIP39 English words\n", static_cast<unsigned>(rollCount), static_cast<unsigned>(wordCount));
  for (size_t i = 0; i < wordCount; ++i) {
    wordAt(indexes[i], word, sizeof(word));
    printf("%2u. %s\n", static_cast<unsigned>(i + 1), word);
  }
  secureClear(word, sizeof(word));
  secureClear(indexes, sizeof(indexes));
  return 0;
}
