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
constexpr uint16_t kScreenWidth = 240;
constexpr uint16_t kScreenHeight = 135;

char keyMap[kRows][kColumns] = {{'1', '2', '3'}, {'4', '5', '6'}, {'7', '8', '9'}, {'*', '0', '#'}};
byte rowPins[kRows] = {21, 27, 26, 22};
byte columnPins[kColumns] = {33, 32, 25};
Keypad keypad = Keypad(makeKeymap(keyMap), rowPins, columnPins, kRows, kColumns);
TFT_eSPI tft;
#ifdef DEVICE_TEST_MODE
TFT_eSprite canvas = TFT_eSprite(&tft);
#else
TFT_eSPI& canvas = tft;
#endif

Session session;

#ifdef DEVICE_TEST_MODE
const char* screenName(SessionScreen screen) {
  static const char* const names[] = {"choose-length", "preflight", "rolls", "words", "verify-prompt", "quiz", "confirm-skip", "clear-words"};
  return names[static_cast<size_t>(screen)];
}

void reportTestState() {
  size_t correctChoice = 0;
  if (session.screen() == SessionScreen::Quiz) {
    const uint16_t answer = session.wordIndexAt(session.quizWordNumber() - 1);
    for (; correctChoice < 3; ++correctChoice) {
      if (session.quizChoiceAt(correctChoice) == answer) break;
    }
  }
  Serial.printf("STATE screen=%s rolls=%u words=%u page=%u quiz=%u correct=%u verified=%u\n", screenName(session.screen()),
                static_cast<unsigned>(session.rollCount()), static_cast<unsigned>(session.wordCount()), static_cast<unsigned>(session.page()),
                static_cast<unsigned>(session.quizPosition()), static_cast<unsigned>(correctChoice + 1), session.verified());
}
#endif

void clearCanvas() {
#ifdef DEVICE_TEST_MODE
  canvas.fillSprite(TFT_BLACK);
#else
  canvas.fillScreen(TFT_BLACK);
#endif
}

void header(const char* title) {
  clearCanvas();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(title, 5, 4, 2);
  canvas.drawFastHLine(0, 23, 240, TFT_DARKGREY);
}

void drawFooter(const char* left, const char* right, uint16_t y = 113) {
  canvas.setTextDatum(TL_DATUM);
  if (left) canvas.drawString(left, 5, y, 2);
  canvas.setTextDatum(TR_DATUM);
  if (right) canvas.drawString(right, 235, y, 2);
  canvas.setTextDatum(TL_DATUM);
}

void drawChooseLength() {
  clearCanvas();
  canvas.setTextDatum(TL_DATUM);
  canvas.setTextColor(TFT_CYAN, TFT_BLACK);
  canvas.drawString("Dice Seed Gen", 5, 5, 4);
  canvas.drawFastHLine(0, 39, 240, TFT_DARKGREY);
  canvas.setTextColor(TFT_GREEN, TFT_BLACK);
  canvas.drawString("1: 12 words", 5, 43, 4);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.drawString("50 dice rolls", 10, 70, 2);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString("2: 24 words", 5, 87, 4);
  canvas.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  canvas.drawString("99 dice rolls", 10, 114, 2);
}

void drawPreflightWarning() {
  header("Before you roll");
  canvas.drawString("Use fair, private dice.", 5, 31, 2);
  canvas.drawString("Power loss clears this", 5, 51, 2);
  canvas.drawString("session. Keep roll notes", 5, 71, 2);
  canvas.drawString("until backup is verified.", 5, 91, 2);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter("*: cancel", "#: continue");
}

void drawRollEntry() {
  clearCanvas();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawString(session.requiredRolls() == 50 ? "12 words" : "24 words", 5, 3, 2);
  canvas.setTextDatum(TR_DATUM);
  char line[20];
  snprintf(line, sizeof(line), "%u / %u rolls", static_cast<unsigned>(session.rollCount()), static_cast<unsigned>(session.requiredRolls()));
  canvas.drawString(line, 235, 3, 2);
  canvas.setTextDatum(TL_DATUM);
  canvas.drawFastHLine(0, 21, 240, TFT_DARKGREY);
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
        canvas.setTextColor(TFT_WHITE, TFT_BLACK);
        canvas.drawString(line, x, y, 2);
      } else {
        line[0] = '.';
        line[1] = '\0';
        canvas.setTextColor(TFT_DARKGREY, TFT_BLACK);
        canvas.drawString(line, x + 3, y, 2);
      }
    }
  }
  secureClear(line, sizeof(line));
  if (session.rollCount() == session.requiredRolls()) {
    canvas.setTextColor(TFT_GREEN, TFT_BLACK);
    drawFooter("*: undo", "#: generate");
  } else {
    canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
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
  canvas.setTextDatum(TC_DATUM);
  canvas.drawString(word, 120, 59, 4);
  canvas.setTextDatum(TL_DATUM);
  secureClear(word, sizeof(word));
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter("*: previous", "#: next");
}

void drawVerifyPrompt() {
  header("Verify backup?");
  canvas.drawString("Check every written word", 5, 42, 2);
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
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
  canvas.drawString(line, 5, 27, 2);
  for (size_t i = 0; i < 3; ++i) {
    wordAt(session.quizChoiceAt(i), word, sizeof(word));
    snprintf(line, sizeof(line), "%u. %s", static_cast<unsigned>(i + 1), word);
    canvas.setTextColor(session.quizIncorrect() && i == session.quizIncorrectChoice() ? TFT_RED : TFT_WHITE, TFT_BLACK);
    canvas.drawString(line, 8, 45 + i * 20, 2);
    secureClear(word, sizeof(word));
  }
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  drawFooter(nullptr, "#: skip verification", 114);
  secureClear(line, sizeof(line));
  secureClear(word, sizeof(word));
}

void drawSkipQuizConfirmation() {
  header("Skip verification?");
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString("Your backup is untested.", 5, 43, 2);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  drawFooter("*: resume", "#: skip", 114);
}

void drawClearWords() {
  header(session.verified() ? "Backup verified" : "Verification skipped");
  canvas.setTextColor(TFT_YELLOW, TFT_BLACK);
  canvas.drawString("Seed has been cleared.", 5, 42, 2);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
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
#ifdef DEVICE_TEST_MODE
  canvas.pushSprite(0, 0);
#endif
}

void handleKey(char key) {
  if (!key) return;
  session.press(key);
  drawCurrentScreen();
#ifdef DEVICE_TEST_MODE
  reportTestState();
#endif
}
}  // namespace

void setup() {
  WiFi.mode(WIFI_OFF);
  btStop();
  tft.init();
  tft.setRotation(1);
  tft.invertDisplay(true);
#ifdef DEVICE_TEST_MODE
  canvas.setColorDepth(16);
  if (!canvas.createSprite(kScreenWidth, kScreenHeight)) while (true) delay(1000);
#endif
  drawCurrentScreen();
#ifdef DEVICE_TEST_MODE
  Serial.begin(115200);
  reportTestState();
#endif
}

void loop() {
  handleKey(keypad.getKey());
#ifdef DEVICE_TEST_MODE
  while (Serial.available()) {
    const char key = static_cast<char>(Serial.read());
    if (key == 'D') {
      const size_t length = static_cast<size_t>(kScreenWidth) * kScreenHeight * sizeof(uint16_t);
      Serial.printf("FRAME %u\n", static_cast<unsigned>(length));
      Serial.write(static_cast<const uint8_t*>(canvas.getPointer()), length);
    } else if ((key >= '0' && key <= '9') || key == '*' || key == '#') {
      handleKey(key);
    }
  }
#endif
}
