#include <stdint.h>

#include "../src/session.h"

namespace { Session session; }

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

void session_press(int key) { session.press(static_cast<char>(key)); }
void session_reset() { session.reset(); }
int session_screen() { return static_cast<int>(session.screen()); }
int session_required_rolls() { return static_cast<int>(session.requiredRolls()); }
int session_roll_count() { return static_cast<int>(session.rollCount()); }
int session_roll_at(int index) { return index >= 0 ? session.rollAt(static_cast<size_t>(index)) : 0; }
int session_word_count() { return static_cast<int>(session.wordCount()); }
int session_word_index_at(int index) { return index >= 0 ? session.wordIndexAt(static_cast<size_t>(index)) : 0; }
int session_page() { return static_cast<int>(session.page()); }
int session_quiz_position() { return static_cast<int>(session.quizPosition()); }
int session_quiz_word_number() { return static_cast<int>(session.quizWordNumber()); }
int session_quiz_choice_at(int index) { return index >= 0 ? session.quizChoiceAt(static_cast<size_t>(index)) : 0; }
int session_quiz_incorrect() { return session.quizIncorrect(); }
int session_verified() { return session.verified(); }
}
