#include <Keypad.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_bt.h>

#include "src/session.h"

namespace {
constexpr byte kRows = 4;
constexpr byte kColumns = 3;
constexpr size_t kRollsPerLine = 25;
constexpr size_t kVisibleRollLines = 2;

char keyMap[kRows][kColumns] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};
byte rowPins[kRows] = {21, 27, 26, 22};
byte columnPins[kColumns] = {33, 32, 25};
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, columnPins, kRows, kColumns);
TFT_eSPI tft;

Session session;

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
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("Dice Seed Gen", 5, 5, 4);
  tft.drawFastHLine(0, 39, 240, TFT_DARKGREY);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("1: 12 words", 17, 45, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("50 dice rolls", 22, 73, 1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("2: 24 words", 17, 92, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("99 dice rolls", 22, 120, 1);
}

void drawRollEntry() {
  char line[32];
  header(session.requiredRolls() == 50 ? "12 words: Dice rolls" : "24 words: Dice rolls");
  snprintf(line, sizeof(line), "%u / %u rolls", static_cast<unsigned>(session.rollCount()), static_cast<unsigned>(session.requiredRolls()));
  tft.drawString(line, 5, 31, 4);
  const size_t firstVisibleRoll = session.rollCount() > kRollsPerLine * kVisibleRollLines ? session.rollCount() - kRollsPerLine * kVisibleRollLines : 0;
  for (size_t offset = firstVisibleRoll, row = 0; offset < session.rollCount(); offset += kRollsPerLine, ++row) {
    const size_t length = session.rollCount() - offset < kRollsPerLine ? session.rollCount() - offset : kRollsPerLine;
    for (size_t i = 0; i < length; ++i) line[i] = session.rollAt(offset + i);
    line[length] = '\0';
    tft.drawString(line, 5, 66 + row * 21, 2);
    secureClear(line, sizeof(line));
  }
  if (session.rollCount() == session.requiredRolls()) {
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
  snprintf(title, sizeof(title), "Write words %u-%u", static_cast<unsigned>(session.page() * Session::kWordsPerPage + 1),
           static_cast<unsigned>((session.page() + 1) * Session::kWordsPerPage > session.wordCount() ? session.wordCount() : (session.page() + 1) * Session::kWordsPerPage));
  header(title);
  for (size_t i = 0; i < Session::kWordsPerPage; ++i) {
    const size_t index = session.page() * Session::kWordsPerPage + i;
    if (index >= session.wordCount()) break;
    wordAt(session.wordIndexAt(index), word, sizeof(word));
    char numbered[20];
    snprintf(numbered, sizeof(numbered), "%2u. %s", static_cast<unsigned>(index + 1), word);
    tft.drawString(numbered, 8, 31 + i * 20, 2);
    secureClear(word, sizeof(word));
  }
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter("*: previous", "#: next");
}

void drawVerifyPrompt() {
  header("Verify backup?");
  tft.drawString("Check every written word", 5, 42, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter("*: skip", "#: start quiz");
}

void drawQuiz() {
  char title[24];
  char line[18];
  char word[12];
  const size_t wordNumber = session.quizWordNumber();
  snprintf(title, sizeof(title), "Verify %u/%u", static_cast<unsigned>(session.quizPosition() + 1), static_cast<unsigned>(session.wordCount()));
  header(title);
  snprintf(line, sizeof(line), "What is word %u?", static_cast<unsigned>(wordNumber));
  tft.drawString(line, 5, 27, 2);
  for (size_t i = 0; i < 4; ++i) {
    wordAt(session.quizChoiceAt(i), word, sizeof(word));
    snprintf(line, sizeof(line), "%u. %s", static_cast<unsigned>(i + 1), word);
    tft.drawString(line, 8, 45 + i * 17, 2);
    secureClear(word, sizeof(word));
  }
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if (session.quizIncorrect()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("Incorrect. Try again.", 5, 114, 1);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    drawFooter(nullptr, "*: skip", 112);
  } else {
    tft.drawString("*: skip verification", 5, 114, 2);
  }
}

void drawSkipQuizConfirmation() {
  header("Skip verification?");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Your backup is untested.", 5, 43, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter("*: resume", "#: skip", 114);
}

void drawClearWords() {
  header(session.verified() ? "Backup verified" : "Verification skipped");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("Seed has been cleared.", 5, 42, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter(nullptr, "#: clear and restart", 114);
}

void drawCurrentScreen() {
  switch (session.screen()) {
    case SessionScreen::ChooseLength: drawChooseLength(); break;
    case SessionScreen::EnterRolls: drawRollEntry(); break;
    case SessionScreen::ShowWords: drawWords(); break;
    case SessionScreen::VerifyPrompt: drawVerifyPrompt(); break;
    case SessionScreen::Quiz: drawQuiz(); break;
    case SessionScreen::ConfirmSkipQuiz: drawSkipQuizConfirmation(); break;
    case SessionScreen::ClearWords: drawClearWords(); break;
  }
}
}  // namespace

void setup() {
  WiFi.mode(WIFI_OFF);
  btStop();
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
  drawCurrentScreen();
}

void loop() {
  const char key = keypad.getKey();
  if (!key) return;

  session.press(key);
  drawCurrentScreen();
}
