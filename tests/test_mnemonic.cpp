#include <assert.h>
#include <string.h>

#include "../src/mnemonic.h"

void assertMnemonic(const char* rolls, const char* const expected[], size_t expectedCount) {
  uint16_t indexes[24] = {};
  size_t count = 0;
  assert(generateMnemonicIndexes(rolls, strlen(rolls), indexes, count));
  assert(count == expectedCount);
  char actual[12];
  for (size_t i = 0; i < count; ++i) {
    wordAt(indexes[i], actual, sizeof(actual));
    assert(strcmp(actual, expected[i]) == 0);
  }
  secureClear(actual, sizeof(actual));
  secureClear(indexes, sizeof(indexes));
}

int main() {
  const char* expected12[] = {"unveil", "nice", "picture", "region", "tragic", "fault", "cream", "strike", "tourist", "control", "recipe", "tourist"};
  assertMnemonic("12345612345612345612345612345612345612345612345612", expected12, 12);

  const char* expected24[] = {"wrist", "tired", "novel", "fetch", "woman", "whisper", "jealous", "black", "average", "crawl", "task", "helmet", "negative", "wrong", "foster", "dry", "chronic", "ordinary", "chase", "typical", "recipe", "sunset", "draw", "victory"};
  assertMnemonic("133363436436436415622614221225242212144161454643266122155666664444633643543353132626522332412313253", expected24, 24);

  uint16_t indexes[24] = {};
  size_t count = 0;
  assert(!generateMnemonicIndexes("123456", 6, indexes, count));
  assert(!generateMnemonicIndexes("11111111111111111111111111111111111111111111111110", 50, indexes, count));
  return 0;
}
