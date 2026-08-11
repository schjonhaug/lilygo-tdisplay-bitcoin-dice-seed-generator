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
  // SeedSigner test_mnemonic_generation.py:test_50_dice_rolls
  const char* expected12[] = {"unveil", "nice", "picture", "region", "tragic", "fault", "cream", "strike", "tourist", "control", "recipe", "tourist"};
  assertMnemonic("12345612345612345612345612345612345612345612345612", expected12, 12);

  const char* expected12AllOnes[] = {"diet", "glad", "hat", "rural", "panther", "lawsuit", "act", "drop", "gallery", "urge", "where", "fit"};
  assertMnemonic("11111111111111111111111111111111111111111111111111", expected12AllOnes, 12);

  const char* expected12AllSixes[] = {"senior", "morning", "song", "proud", "recycle", "toy", "search", "apple", "trigger", "lend", "vibrant", "arrest"};
  assertMnemonic("66666666666666666666666666666666666666666666666666", expected12AllSixes, 12);

  // Public 99-roll vector from the article, independently checked on SeedSigner and Coldcard.
  const char* expected24[] = {"wrist", "tired", "novel", "fetch", "woman", "whisper", "jealous", "black", "average", "crawl", "task", "helmet", "negative", "wrong", "foster", "dry", "chronic", "ordinary", "chase", "typical", "recipe", "sunset", "draw", "victory"};
  assertMnemonic("133363436436436415622614221225242212144161454643266122155666664444633643543353132626522332412313253", expected24, 24);

  // SeedSigner test_mnemonic_generation.py:test_known_dice_rolls
  const char* expected24KnownOne[] = {"resource", "timber", "firm", "banner", "horror", "pupil", "frozen", "main", "pear", "direct", "pioneer", "broken", "grid", "core", "insane", "begin", "sister", "pony", "end", "debate", "task", "silk", "empty", "curious"};
  assertMnemonic("522222222222222222222222222222222222222222222555555555555555555555555555555555555555555555555555555", expected24KnownOne, 24);

  const char* expected24KnownTwo[] = {"garden", "uphold", "level", "clog", "sword", "globe", "armor", "issue", "two", "cute", "scorpion", "improve", "verb", "artwork", "blind", "tail", "raw", "butter", "combine", "move", "produce", "foil", "feature", "wave"};
  assertMnemonic("222222222222222222222222222222222222222222222555555555555555555555555555555555555555555555555555555", expected24KnownTwo, 24);

  const char* expected24KnownThree[] = {"lizard", "broken", "love", "tired", "depend", "eyebrow", "excess", "lonely", "advance", "father", "various", "cram", "ignore", "panic", "feed", "plunge", "miss", "regret", "boring", "unique", "galaxy", "fan", "detail", "fly"};
  assertMnemonic("222222222222222222222222222222222222222222222555555555555555555555555555555555555555555555555555556", expected24KnownThree, 24);

  uint16_t indexes[24] = {};
  size_t count = 0;
  assert(!generateMnemonicIndexes("123456", 6, indexes, count));
  assert(!generateMnemonicIndexes("11111111111111111111111111111111111111111111111110", 50, indexes, count));
  return 0;
}
