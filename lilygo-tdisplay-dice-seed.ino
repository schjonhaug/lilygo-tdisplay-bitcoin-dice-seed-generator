#include <Keypad.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <esp_bt.h>

#include "src/session.h"

namespace {
constexpr byte kRows = 4;
constexpr byte kColumns = 3;
constexpr size_t kRollsPerLine = 25;
constexpr size_t kRollLines = 4;
constexpr int kRollCellWidth = 9;

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
  tft.drawString("1: 12 words", 17, 43, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("50 dice rolls", 22, 72, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("2: 24 words", 17, 84, 4);
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("99 dice rolls", 22, 113, 2);
}

void drawPreflightWarning() {
  header("Before you roll");
  tft.drawString("Use fair, private dice.", 5, 31, 2);
  tft.drawString("Power loss clears this", 5, 51, 2);
  tft.drawString("session. Keep roll notes", 5, 71, 2);
  tft.drawString("until backup is verified.", 5, 91, 2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter("*: cancel", "#: continue");
}

void drawRollEntry() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString(session.requiredRolls() == 50 ? "12 words" : "24 words", 5, 3, 2);
  tft.setTextDatum(TR_DATUM);
  char line[20];
  snprintf(line, sizeof(line), "%u / %u rolls", static_cast<unsigned>(session.rollCount()), static_cast<unsigned>(session.requiredRolls()));
  tft.drawString(line, 235, 3, 2);
  tft.setTextDatum(TL_DATUM);
  tft.drawFastHLine(0, 21, 240, TFT_DARKGREY);
  const size_t rollLines = session.requiredRolls() == 50 ? 2 : kRollLines;
  for (size_t row = 0; row < rollLines; ++row) {
    const size_t offset = row * kRollsPerLine;
    const size_t columns = session.requiredRolls() - offset < kRollsPerLine ? session.requiredRolls() - offset : kRollsPerLine;
    const int y = 26 + row * 21;
    for (size_t column = 0; column < columns; ++column) {
      const size_t index = offset + column;
      const int x = 5 + column * kRollCellWidth;
      if (index < session.rollCount()) {
        line[0] = session.rollAt(index);
        line[1] = '\0';
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(line, x, y, 2);
      } else {
        line[0] = '.';
        line[1] = '\0';
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString(line, x + 3, y, 2);
      }
    }
  }
  secureClear(line, sizeof(line));
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
  const size_t index = session.page();
  snprintf(title, sizeof(title), "Word %u / %u", static_cast<unsigned>(index + 1), static_cast<unsigned>(session.wordCount()));
  header(title);
  wordAt(session.wordIndexAt(index), word, sizeof(word));
  tft.setTextDatum(TC_DATUM);
  tft.drawString(word, 120, 59, 4);
  tft.setTextDatum(TL_DATUM);
  secureClear(word, sizeof(word));
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
    tft.drawString(line, 8, 45 + i * 20, 2);
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
  secureClear(line, sizeof(line));
  secureClear(word, sizeof(word));
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
    case SessionScreen::PreflightWarning: drawPreflightWarning(); break;
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
