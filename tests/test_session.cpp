#include <assert.h>
#include <string.h>

#include "../src/session.h"

void enter(Session& session, const char* rolls) {
  session.press(strlen(rolls) == 50 ? '1' : '2');
  session.press('#');
  for (const char* roll = rolls; *roll; ++roll) session.press(*roll);
}

int main() {
  const char* rolls = "12345612345612345612345612345612345612345612345612";
  Session session;
  assert(session.screen() == SessionScreen::ChooseLength);
  session.press('1');
  assert(session.screen() == SessionScreen::PreflightWarning);
  session.press('#');
  assert(session.screen() == SessionScreen::EnterRolls && session.requiredRolls() == 50);
  session.press('7');
  assert(session.rollCount() == 0);
  session.press('1');
  session.press('*');
  assert(session.rollCount() == 0);
  session.press('*');
  assert(session.screen() == SessionScreen::ChooseLength);

  enter(session, rolls);
  assert(session.rollCount() == 50);
  session.press('#');
  assert(session.screen() == SessionScreen::ShowWords && session.wordCount() == 12);
  assert(session.rollCount() == 50);
  char word[12];
  wordAt(session.wordIndexAt(0), word, sizeof(word));
  assert(session.page() == 0 && strcmp(word, "unveil") == 0);
  session.press('*');
  assert(session.screen() == SessionScreen::EnterRolls && session.rollCount() == 50);
  session.press('#');
  assert(session.screen() == SessionScreen::ShowWords && session.page() == 0);
  session.press('#');
  session.press('#');
  session.press('#');
  assert(session.screen() == SessionScreen::VerifyPrompt);
  session.press('*');
  assert(session.screen() == SessionScreen::ConfirmSkipQuiz);
  session.press('*');
  assert(session.screen() == SessionScreen::VerifyPrompt);
  session.press('#');
  assert(session.screen() == SessionScreen::Quiz && session.quizPosition() == 0);

  const uint16_t answer = session.wordIndexAt(session.quizWordNumber() - 1);
  size_t answerChoices = 0;
  for (size_t i = 0; i < 4; ++i) answerChoices += session.quizChoiceAt(i) == answer;
  assert(answerChoices == 1);
  session.press('9');
  assert(!session.quizIncorrect());
  for (size_t i = 0; i < 4; ++i) {
    if (session.quizChoiceAt(i) != answer) {
      session.press(static_cast<char>('1' + i));
      break;
    }
  }
  assert(session.quizIncorrect());
  session.press('*');
  session.press('#');
  assert(session.screen() == SessionScreen::ClearWords && !session.verified());
  session.press('#');
  assert(session.screen() == SessionScreen::ChooseLength);

  enter(session, rolls);
  session.press('#');
  session.press('#'); session.press('#'); session.press('#');
  session.press('#');
  for (size_t question = 0; question < 12; ++question) {
    const uint16_t correct = session.wordIndexAt(session.quizWordNumber() - 1);
    for (size_t i = 0; i < 4; ++i) {
      if (session.quizChoiceAt(i) == correct) {
        session.press(static_cast<char>('1' + i));
        break;
      }
    }
  }
  assert(session.screen() == SessionScreen::ClearWords && session.verified());
}
