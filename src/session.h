#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mnemonic.h"

enum class SessionScreen : uint8_t { ChooseLength, PreflightWarning, EnterRolls, ShowWords, VerifyPrompt, Quiz, ConfirmSkipQuiz, ClearWords };

class Session {
 public:
  static constexpr size_t kMaxRolls = 99;
  static constexpr size_t kWordsPerPage = 1;

  Session() { clear(); }
  ~Session() { clear(); }

  void press(char key) {
    switch (screen_) {
      case SessionScreen::ChooseLength:
        if (key == '1' || key == '2') {
          clear();
          requiredRolls_ = key == '1' ? 50 : 99;
          screen_ = SessionScreen::PreflightWarning;
        }
        break;
      case SessionScreen::PreflightWarning:
        if (key == '#') screen_ = SessionScreen::EnterRolls;
        else if (key == '*') clear();
        break;
      case SessionScreen::EnterRolls:
        if (key >= '1' && key <= '6' && rollCount_ < requiredRolls_) {
          rolls_[rollCount_++] = key;
        } else if (key == '*' && rollCount_) {
          rolls_[--rollCount_] = 0;
        } else if (key == '*') {
          clear();
        } else if (key == '#' && rollCount_ == requiredRolls_) {
          generate();
        } else if (key == '#') {
          clear();
        }
        break;
      case SessionScreen::ShowWords:
        if (key == '*' && page_) {
          --page_;
        } else if (key == '*') {
          screen_ = SessionScreen::EnterRolls;
        } else if (key == '#' && (page_ + 1) * kWordsPerPage < wordCount_) {
          ++page_;
        } else if (key == '#') {
          screen_ = SessionScreen::VerifyPrompt;
        }
        break;
      case SessionScreen::VerifyPrompt:
        if (key == '#') {
          prepareQuiz();
          prepareQuizChoices();
          screen_ = SessionScreen::Quiz;
        } else if (key == '*') {
          skipReturnScreen_ = SessionScreen::VerifyPrompt;
          screen_ = SessionScreen::ConfirmSkipQuiz;
        }
        break;
      case SessionScreen::Quiz:
        if (key >= '1' && key <= '3') {
          if (quizChoices_[key - '1'] == wordIndexes_[quizOrder_[quizPosition_]]) {
            if (++quizPosition_ == wordCount_) finish(true);
            else {
              quizIncorrect_ = false;
              quizIncorrectChoice_ = 3;
              prepareQuizChoices();
            }
          } else {
            quizIncorrect_ = true;
            quizIncorrectChoice_ = static_cast<size_t>(key - '1');
          }
        } else if (key == '#') {
          skipReturnScreen_ = SessionScreen::Quiz;
          screen_ = SessionScreen::ConfirmSkipQuiz;
        }
        break;
      case SessionScreen::ConfirmSkipQuiz:
        if (key == '#') finish(false);
        else if (key == '*') screen_ = skipReturnScreen_;
        break;
      case SessionScreen::ClearWords:
        if (key == '#') clear();
        break;
    }
  }

  void reset() { clear(); }
  SessionScreen screen() const { return screen_; }
  size_t requiredRolls() const { return requiredRolls_; }
  size_t rollCount() const { return rollCount_; }
  char rollAt(size_t index) const { return index < rollCount_ ? rolls_[index] : 0; }
  size_t wordCount() const { return wordCount_; }
  uint16_t wordIndexAt(size_t index) const { return index < wordCount_ ? wordIndexes_[index] : 0; }
  size_t page() const { return page_; }
  size_t quizPosition() const { return quizPosition_; }
  size_t quizWordNumber() const { return quizPosition_ < wordCount_ ? quizOrder_[quizPosition_] + 1 : 0; }
  uint16_t quizChoiceAt(size_t index) const { return index < 3 ? quizChoices_[index] : 0; }
  bool quizIncorrect() const { return quizIncorrect_; }
  size_t quizIncorrectChoice() const { return quizIncorrectChoice_; }
  bool verified() const { return verified_; }

 private:
  uint32_t nextQuizValue() {
    quizRandomState_ ^= quizRandomState_ << 13;
    quizRandomState_ ^= quizRandomState_ >> 17;
    quizRandomState_ ^= quizRandomState_ << 5;
    return quizRandomState_;
  }

  void prepareQuiz() {
    quizRandomState_ = 2166136261u;
    for (size_t i = 0; i < wordCount_; ++i) {
      quizRandomState_ = (quizRandomState_ ^ (wordIndexes_[i] & 0xff)) * 16777619u;
      quizRandomState_ = (quizRandomState_ ^ (wordIndexes_[i] >> 8)) * 16777619u;
      quizOrder_[i] = i;
    }
    if (!quizRandomState_) quizRandomState_ = 0x6d2b79f5u;
    for (size_t i = wordCount_ - 1; i > 0; --i) {
      const size_t swapWith = nextQuizValue() % (i + 1);
      const size_t temporary = quizOrder_[i];
      quizOrder_[i] = quizOrder_[swapWith];
      quizOrder_[swapWith] = temporary;
    }
    quizPosition_ = 0;
    quizIncorrect_ = false;
    quizIncorrectChoice_ = 3;
  }

  void prepareQuizChoices() {
    quizChoices_[0] = wordIndexes_[quizOrder_[quizPosition_]];
    for (size_t choice = 1; choice < 3; ++choice) {
      uint16_t candidate;
      bool duplicate;
      do {
        candidate = nextQuizValue() % 2048;
        duplicate = false;
        for (size_t previous = 0; previous < choice; ++previous) {
          if (quizChoices_[previous] == candidate) duplicate = true;
        }
      } while (duplicate);
      quizChoices_[choice] = candidate;
    }
    for (size_t i = 2; i > 0; --i) {
      const size_t swapWith = nextQuizValue() % (i + 1);
      const uint16_t temporary = quizChoices_[i];
      quizChoices_[i] = quizChoices_[swapWith];
      quizChoices_[swapWith] = temporary;
    }
  }

  void generate() {
    if (!generateMnemonicIndexes(rolls_, rollCount_, wordIndexes_, wordCount_)) return;
    page_ = 0;
    screen_ = SessionScreen::ShowWords;
  }

  void finish(bool verified) {
    clear();
    verified_ = verified;
    screen_ = SessionScreen::ClearWords;
  }

  void clear() {
    secureClear(rolls_, sizeof(rolls_));
    secureClear(wordIndexes_, sizeof(wordIndexes_));
    secureClear(quizOrder_, sizeof(quizOrder_));
    secureClear(quizChoices_, sizeof(quizChoices_));
    screen_ = SessionScreen::ChooseLength;
    skipReturnScreen_ = SessionScreen::ShowWords;
    rollCount_ = requiredRolls_ = wordCount_ = page_ = quizPosition_ = 0;
    quizRandomState_ = 0;
    quizIncorrect_ = verified_ = false;
    quizIncorrectChoice_ = 3;
  }

  SessionScreen screen_;
  SessionScreen skipReturnScreen_;
  char rolls_[kMaxRolls];
  size_t rollCount_, requiredRolls_;
  uint16_t wordIndexes_[24];
  size_t wordCount_, page_;
  size_t quizOrder_[24];
  uint16_t quizChoices_[3];
  size_t quizPosition_;
  uint32_t quizRandomState_;
  bool quizIncorrect_, verified_;
  size_t quizIncorrectChoice_;
};
