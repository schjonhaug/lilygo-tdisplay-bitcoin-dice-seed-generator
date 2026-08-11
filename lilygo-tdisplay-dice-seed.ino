#include <Keypad.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_bt.h>

#include "src/mnemonic.h"

namespace {
constexpr byte kRows = 4;
constexpr byte kColumns = 3;
constexpr size_t kMaxRolls = 99;
constexpr size_t kWordsPerPage = 4;
constexpr size_t kRollsPerLine = 19;
constexpr size_t kVisibleRollLines = 2;

char keyMap[kRows][kColumns] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};
byte rowPins[kRows] = {21, 27, 26, 22};
byte columnPins[kColumns] = {33, 32, 25};
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, columnPins, kRows, kColumns);
TFT_eSPI tft;

enum class Screen { ChooseLength, EnterRolls, ShowWords, VerifyPrompt, Quiz, ConfirmSkipQuiz, ClearWords };
Screen screen = Screen::ChooseLength;
char rolls[kMaxRolls];
size_t rollCount = 0;
size_t requiredRolls = 0;
uint16_t wordIndexes[24];
size_t wordCount = 0;
size_t page = 0;
size_t quizOrder[24];
uint16_t quizChoices[4];
size_t quizPosition = 0;
uint32_t quizRandomState = 0;

void header(const char* title) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(title, 5, 4, 2);
  tft.drawFastHLine(0, 23, 240, TFT_DARKGREY);
}

void drawFooter(const char* left, const char* right, uint16_t y = 113) {
  tft.setTextDatum(TL_DATUM);
  if (left) tft.drawString(left, 5, y, 2);
  tft.setTextDatum(TR_DATUM);
  if (right) tft.drawString(right, 235, y, 2);
  tft.setTextDatum(TL_DATUM);
}

void drawChooseLength() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Dice Seed Generator", 5, 5, 4);
  tft.drawFastHLine(0, 39, 240, TFT_DARKGREY);
  tft.drawString("1: 12 words", 17, 45, 4);
  tft.drawString("50 dice rolls", 22, 73, 1);
  tft.drawString("2: 24 words", 17, 92, 4);
  tft.drawString("99 dice rolls", 22, 120, 1);
}

void drawRollEntry() {
  char line[32];
  header(requiredRolls == 50 ? "12 words: Dice rolls" : "24 words: Dice rolls");
  snprintf(line, sizeof(line), "%u / %u rolls", static_cast<unsigned>(rollCount), static_cast<unsigned>(requiredRolls));
  tft.drawString(line, 5, 31, 4);
  const size_t firstVisibleRoll = rollCount > kRollsPerLine * kVisibleRollLines ? rollCount - kRollsPerLine * kVisibleRollLines : 0;
  for (size_t offset = firstVisibleRoll, row = 0; offset < rollCount; offset += kRollsPerLine, ++row) {
    const size_t length = rollCount - offset < kRollsPerLine ? rollCount - offset : kRollsPerLine;
    memcpy(line, rolls + offset, length);
    line[length] = '\0';
    tft.drawString(line, 5, 66 + row * 21, 2);
    secureClear(line, sizeof(line));
  }
  if (rollCount == requiredRolls) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    drawFooter("*: undo", "#: generate");
  } else {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    drawFooter("*: undo", "#: main menu");
  }
}

void drawWords() {
  char title[24];
  char word[12];
  snprintf(title, sizeof(title), "Write words %u-%u", static_cast<unsigned>(page * kWordsPerPage + 1),
           static_cast<unsigned>((page + 1) * kWordsPerPage > wordCount ? wordCount : (page + 1) * kWordsPerPage));
  header(title);
  for (size_t i = 0; i < kWordsPerPage; ++i) {
    const size_t index = page * kWordsPerPage + i;
    if (index >= wordCount) break;
    wordAt(wordIndexes[index], word, sizeof(word));
    char numbered[20];
    snprintf(numbered, sizeof(numbered), "%2u. %s", static_cast<unsigned>(index + 1), word);
    tft.drawString(numbered, 8, 31 + i * 20, 2);
    secureClear(word, sizeof(word));
  }
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if ((page + 1) * kWordsPerPage < wordCount) {
    drawFooter("*: previous", "#: next");
  } else {
    drawFooter("*: skip", "#: verify");
  }
}

void drawVerifyPrompt() {
  header("Verify backup?");
  tft.drawString("Check every written word", 5, 42, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter("*: skip", "#: start quiz");
}

uint32_t nextQuizValue() {
  quizRandomState ^= quizRandomState << 13;
  quizRandomState ^= quizRandomState >> 17;
  quizRandomState ^= quizRandomState << 5;
  return quizRandomState;
}

void prepareQuiz() {
  quizRandomState = 2166136261u;
  for (size_t i = 0; i < wordCount; ++i) {
    quizRandomState = (quizRandomState ^ (wordIndexes[i] & 0xff)) * 16777619u;
    quizRandomState = (quizRandomState ^ (wordIndexes[i] >> 8)) * 16777619u;
    quizOrder[i] = i;
  }
  if (!quizRandomState) quizRandomState = 0x6d2b79f5u;
  for (size_t i = wordCount - 1; i > 0; --i) {
    const size_t swapWith = nextQuizValue() % (i + 1);
    const size_t temporary = quizOrder[i];
    quizOrder[i] = quizOrder[swapWith];
    quizOrder[swapWith] = temporary;
  }
  quizPosition = 0;
}

void prepareQuizChoices() {
  quizChoices[0] = wordIndexes[quizOrder[quizPosition]];
  for (size_t choice = 1; choice < 4; ++choice) {
    uint16_t candidate;
    bool duplicate;
    do {
      candidate = nextQuizValue() % 2048;
      duplicate = false;
      for (size_t previous = 0; previous < choice; ++previous) {
        if (quizChoices[previous] == candidate) duplicate = true;
      }
    } while (duplicate);
    quizChoices[choice] = candidate;
  }
  for (size_t i = 3; i > 0; --i) {
    const size_t swapWith = nextQuizValue() % (i + 1);
    const uint16_t temporary = quizChoices[i];
    quizChoices[i] = quizChoices[swapWith];
    quizChoices[swapWith] = temporary;
  }
}

void drawQuiz() {
  char title[24];
  char line[18];
  char word[12];
  const size_t wordNumber = quizOrder[quizPosition] + 1;
  snprintf(title, sizeof(title), "Verify %u/%u", static_cast<unsigned>(quizPosition + 1), static_cast<unsigned>(wordCount));
  header(title);
  snprintf(line, sizeof(line), "What is word %u?", static_cast<unsigned>(wordNumber));
  tft.drawString(line, 5, 27, 2);
  for (size_t i = 0; i < 4; ++i) {
    wordAt(quizChoices[i], word, sizeof(word));
    snprintf(line, sizeof(line), "%u. %s", static_cast<unsigned>(i + 1), word);
    tft.drawString(line, 8, 45 + i * 17, 2);
    secureClear(word, sizeof(word));
  }
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("*: skip verification", 5, 114, 2);
}

void drawSkipQuizConfirmation() {
  header("Skip verification?");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Your backup is untested.", 5, 43, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter("*: resume", "#: skip", 114);
}

void drawClearWords(bool verified) {
  header(verified ? "Backup verified" : "Verification skipped");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Seed has been cleared.", 5, 42, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter(nullptr, "#: clear and restart", 114);
}

void clearSession() {
  secureClear(rolls, sizeof(rolls));
  secureClear(wordIndexes, sizeof(wordIndexes));
  rollCount = 0;
  requiredRolls = 0;
  wordCount = 0;
  page = 0;
  quizPosition = 0;
  quizRandomState = 0;
  secureClear(quizOrder, sizeof(quizOrder));
  secureClear(quizChoices, sizeof(quizChoices));
}

void generateWords() {
  if (!generateMnemonicIndexes(rolls, rollCount, wordIndexes, wordCount)) return;
  secureClear(rolls, sizeof(rolls));
  rollCount = 0;
  page = 0;
  screen = Screen::ShowWords;
  drawWords();
}

void finishSeedDisplay(bool verified) {
  clearSession();
  screen = Screen::ClearWords;
  drawClearWords(verified);
}
}  // namespace

void setup() {
  WiFi.mode(WIFI_OFF);
  btStop();
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  drawChooseLength();
}

void loop() {
  const char key = keypad.getKey();
  if (!key) return;

  switch (screen) {
    case Screen::ChooseLength:
      if (key == '1' || key == '2') {
        clearSession();
        requiredRolls = key == '1' ? 50 : 99;
        screen = Screen::EnterRolls;
        drawRollEntry();
      }
      break;
    case Screen::EnterRolls:
      if (key >= '1' && key <= '6' && rollCount < requiredRolls) {
        rolls[rollCount++] = key;
        drawRollEntry();
      } else if (key == '*' && rollCount) {
        rolls[--rollCount] = 0;
        drawRollEntry();
      } else if (key == '*' && !rollCount) {
        clearSession();
        screen = Screen::ChooseLength;
        drawChooseLength();
      } else if (key == '#' && rollCount == requiredRolls) {
        generateWords();
      } else if (key == '#') {
        clearSession();
        screen = Screen::ChooseLength;
        drawChooseLength();
      }
      break;
    case Screen::ShowWords:
      if (key == '*' && page) {
        --page;
        drawWords();
      } else if (key == '#' && (page + 1) * kWordsPerPage < wordCount) {
        ++page;
        drawWords();
      } else if (key == '#') {
        screen = Screen::VerifyPrompt;
        drawVerifyPrompt();
      } else if (key == '*') {
        finishSeedDisplay(false);
      }
      break;
    case Screen::VerifyPrompt:
      if (key == '#') {
        prepareQuiz();
        prepareQuizChoices();
        screen = Screen::Quiz;
        drawQuiz();
      } else if (key == '*') {
        finishSeedDisplay(false);
      }
      break;
    case Screen::Quiz:
      if (key >= '1' && key <= '4') {
        if (quizChoices[key - '1'] == wordIndexes[quizOrder[quizPosition]]) {
          ++quizPosition;
          if (quizPosition == wordCount) {
            finishSeedDisplay(true);
          } else {
            prepareQuizChoices();
            drawQuiz();
          }
        }
      } else if (key == '*') {
        screen = Screen::ConfirmSkipQuiz;
        drawSkipQuizConfirmation();
      }
      break;
    case Screen::ConfirmSkipQuiz:
      if (key == '#') {
        finishSeedDisplay(false);
      } else if (key == '*') {
        screen = Screen::Quiz;
        drawQuiz();
      }
      break;
    case Screen::ClearWords:
      if (key == '#') {
        clearSession();
        screen = Screen::ChooseLength;
        drawChooseLength();
      }
      break;
  }
}
