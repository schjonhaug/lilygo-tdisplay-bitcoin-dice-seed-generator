#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

namespace {
constexpr const char* kRolls = "12345612345612345612345612345612345612345612345612";
constexpr const char* kRolls99 = "133363436436436415622614221225242212144161454643266122155666664444633643543353132626522332412313253";
int port = -1;
const char* outputDirectory = nullptr;

[[noreturn]] void fail(const char* message) {
  fprintf(stderr, "device test failed: %s\n", message);
  exit(1);
}

void sendKeys(const char* keys) {
  const size_t length = strlen(keys);
  if (write(port, keys, length) != static_cast<ssize_t>(length)) fail("could not write serial input");
}

void expect(const char* screen, unsigned rolls, unsigned words, unsigned page, unsigned quiz, unsigned verified, unsigned* correct = nullptr) {
  char line[256];
  size_t used = 0;
  for (;;) {
    pollfd descriptor = {port, POLLIN, 0};
    if (poll(&descriptor, 1, 3000) <= 0) fail("timed out waiting for device state");
    char byte;
    if (read(port, &byte, 1) != 1) continue;
    if (byte == '\r') continue;
    if (byte == '\n') {
      line[used] = '\0';
      used = 0;
      char actual[32];
      unsigned actualRolls, actualWords, actualPage, actualQuiz, actualCorrect, actualVerified;
      if (sscanf(line, "STATE screen=%31s rolls=%u words=%u page=%u quiz=%u correct=%u verified=%u", actual, &actualRolls, &actualWords,
                 &actualPage, &actualQuiz, &actualCorrect, &actualVerified) == 7) {
        if (strcmp(actual, screen) || actualRolls != rolls || actualWords != words || actualPage != page || actualQuiz != quiz || actualVerified != verified) {
          fprintf(stderr, "unexpected %s\n", line);
          fail("unexpected device state");
        }
        if (correct) *correct = actualCorrect;
        return;
      }
    } else if (used + 1 < sizeof(line)) {
      line[used++] = byte;
    }
  }
}

void capture(const char* name) {
  constexpr size_t width = 240;
  constexpr size_t height = 135;
  constexpr size_t byteCount = width * height * sizeof(uint16_t);
  char line[64];
  size_t used = 0;
  sendKeys("D");
  for (;;) {
    pollfd descriptor = {port, POLLIN, 0};
    if (poll(&descriptor, 1, 3000) <= 0) fail("timed out waiting for frame header");
    char byte;
    if (read(port, &byte, 1) != 1 || byte == '\r') continue;
    if (byte == '\n') {
      line[used] = '\0';
      if (!strcmp(line, "FRAME 64800")) break;
      used = 0;
      continue;
    }
    if (used + 1 >= sizeof(line)) used = 0;
    line[used++] = byte;
  }
  uint16_t pixels[width * height];
  size_t received = 0;
  while (received < byteCount) {
    pollfd descriptor = {port, POLLIN, 0};
    if (poll(&descriptor, 1, 3000) <= 0) fail("timed out reading frame");
    const ssize_t count = read(port, reinterpret_cast<uint8_t*>(pixels) + received, byteCount - received);
    if (count > 0) received += static_cast<size_t>(count);
  }
  char path[256];
  snprintf(path, sizeof(path), "%s/%s.ppm", outputDirectory, name);
  FILE* image = fopen(path, "wb");
  if (!image) fail("could not create screenshot");
  fprintf(image, "P6\n%zu %zu\n255\n", width, height);
  for (size_t i = 0; i < width * height; ++i) {
    const uint16_t pixel = pixels[i];
    const uint8_t rgb[] = {static_cast<uint8_t>(((pixel >> 11) & 0x1f) * 255 / 31), static_cast<uint8_t>(((pixel >> 5) & 0x3f) * 255 / 63), static_cast<uint8_t>((pixel & 0x1f) * 255 / 31)};
    if (fwrite(rgb, sizeof(rgb), 1, image) != 1) fail("could not write screenshot");
  }
  fclose(image);
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) fail("usage: tools/test-device <serial-port> <output-directory>");
  outputDirectory = argv[2];
  if (mkdir(outputDirectory, 0755) && errno != EEXIST) fail("could not create screenshot directory");
  port = open(argv[1], O_RDWR | O_NOCTTY);
  if (port < 0) fail("could not open serial port");
  termios settings;
  if (tcgetattr(port, &settings) || cfsetspeed(&settings, B115200)) fail("could not configure serial port");
  settings.c_cflag = CS8 | CLOCAL | CREAD;
  settings.c_iflag = settings.c_oflag = settings.c_lflag = 0;
  settings.c_cc[VMIN] = 0;
  settings.c_cc[VTIME] = 0;
  if (tcsetattr(port, TCSANOW, &settings)) fail("could not configure serial port");
  tcflush(port, TCIOFLUSH);

  capture("start");
  sendKeys("1");
  expect("preflight", 0, 0, 0, 0, 0);
  capture("preflight");
  sendKeys("#");
  expect("rolls", 0, 0, 0, 0, 0);
  capture("rolls-empty-12");
  sendKeys(kRolls);
  for (unsigned count = 1; count <= 50; ++count) expect("rolls", count, 0, 0, 0, 0);
  sendKeys("#");
  expect("words", 50, 12, 0, 0, 0);
  capture("word-12-01");
  for (unsigned page = 1; page < 12; ++page) {
    sendKeys("#");
    expect("words", 50, 12, page, 0, 0);
  }
  sendKeys("#");
  expect("verify-prompt", 50, 12, 11, 0, 0);
  capture("verify-prompt-12");
  sendKeys("#");
  unsigned correct;
  expect("quiz", 50, 12, 11, 0, 0, &correct);
  capture("quiz-12");
  sendKeys(correct == 1 ? "2" : "1");
  expect("quiz", 50, 12, 11, 0, 0);
  capture("quiz-12-incorrect");
  sendKeys("#");
  expect("confirm-skip", 50, 12, 11, 0, 0);
  capture("confirm-skip-12");
  sendKeys("#");
  expect("clear-words", 0, 0, 0, 0, 0);
  sendKeys("#");
  expect("choose-length", 0, 0, 0, 0, 0);
  sendKeys("1");
  expect("preflight", 0, 0, 0, 0, 0);
  sendKeys("#");
  expect("rolls", 0, 0, 0, 0, 0);
  sendKeys(kRolls);
  for (unsigned count = 1; count <= 50; ++count) expect("rolls", count, 0, 0, 0, 0);
  sendKeys("#");
  expect("words", 50, 12, 0, 0, 0);
  for (unsigned page = 1; page < 12; ++page) {
    sendKeys("#");
    expect("words", 50, 12, page, 0, 0);
  }
  sendKeys("#");
  expect("verify-prompt", 50, 12, 11, 0, 0);
  sendKeys("#");
  for (unsigned quiz = 0; quiz < 12; ++quiz) {
    expect("quiz", 50, 12, 11, quiz, 0, &correct);
    char answer[] = {static_cast<char>('0' + correct), '\0'};
    sendKeys(answer);
  }
  expect("clear-words", 0, 0, 0, 0, 1);
  capture("verified-12");

  sendKeys("#");
  expect("choose-length", 0, 0, 0, 0, 0);
  sendKeys("2");
  expect("preflight", 0, 0, 0, 0, 0);
  sendKeys("#");
  expect("rolls", 0, 0, 0, 0, 0);
  capture("rolls-empty-24");
  sendKeys(kRolls99);
  for (unsigned count = 1; count <= 99; ++count) expect("rolls", count, 0, 0, 0, 0);
  sendKeys("#");
  expect("words", 99, 24, 0, 0, 0);
  capture("word-24-01");
  for (unsigned page = 1; page < 24; ++page) {
    sendKeys("#");
    expect("words", 99, 24, page, 0, 0);
  }
  sendKeys("#");
  expect("verify-prompt", 99, 24, 23, 0, 0);
  capture("verify-prompt-24");
  sendKeys("#");
  for (unsigned quiz = 0; quiz < 24; ++quiz) {
    expect("quiz", 99, 24, 23, quiz, 0, &correct);
    char answer[] = {static_cast<char>('0' + correct), '\0'};
    sendKeys(answer);
  }
  expect("clear-words", 0, 0, 0, 0, 1);
  capture("verified-24");
  puts("device test passed");
}
