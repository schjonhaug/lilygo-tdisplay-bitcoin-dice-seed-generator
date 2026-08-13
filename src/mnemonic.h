#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "wordlist_english.h"

inline void secureClear(void* value, size_t length) {
  volatile uint8_t* bytes = static_cast<volatile uint8_t*>(value);
  while (length--) {
    *bytes++ = 0;
  }
}

class Sha256 {
 public:
  Sha256() { reset(); }

  ~Sha256() {
    secureClear(state_, sizeof(state_));
    secureClear(buffer_, sizeof(buffer_));
    length_ = 0;
    used_ = 0;
  }

  void reset() {
    state_[0] = 0x6a09e667;
    state_[1] = 0xbb67ae85;
    state_[2] = 0x3c6ef372;
    state_[3] = 0xa54ff53a;
    state_[4] = 0x510e527f;
    state_[5] = 0x9b05688c;
    state_[6] = 0x1f83d9ab;
    state_[7] = 0x5be0cd19;
    length_ = 0;
    used_ = 0;
  }

  void update(const uint8_t* data, size_t length) {
    length_ += static_cast<uint64_t>(length) * 8;
    while (length) {
      const size_t available = 64 - used_;
      const size_t take = length < available ? length : available;
      memcpy(buffer_ + used_, data, take);
      used_ += take;
      data += take;
      length -= take;
      if (used_ == 64) {
        transform(buffer_);
        used_ = 0;
      }
    }
  }

  void final(uint8_t digest[32]) {
    const uint64_t messageLength = length_;
    buffer_[used_++] = 0x80;
    if (used_ > 56) {
      while (used_ < 64) buffer_[used_++] = 0;
      transform(buffer_);
      used_ = 0;
    }
    while (used_ < 56) buffer_[used_++] = 0;
    for (int i = 7; i >= 0; --i) {
      buffer_[used_++] = static_cast<uint8_t>(messageLength >> (i * 8));
    }
    transform(buffer_);
    for (size_t i = 0; i < 8; ++i) {
      digest[i * 4] = static_cast<uint8_t>(state_[i] >> 24);
      digest[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
      digest[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
      digest[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
    }
    secureClear(buffer_, sizeof(buffer_));
  }

 private:
  static uint32_t rotateRight(uint32_t value, uint8_t bits) {
    return (value >> bits) | (value << (32 - bits));
  }

  static uint32_t choose(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
  }

  static uint32_t majority(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
  }

  void transform(const uint8_t block[64]) {
    static const uint32_t constants[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t words[64];
    for (size_t i = 0; i < 16; ++i) {
      words[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
                 (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
                 (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
                 static_cast<uint32_t>(block[i * 4 + 3]);
    }
    for (size_t i = 16; i < 64; ++i) {
      const uint32_t s0 = rotateRight(words[i - 15], 7) ^ rotateRight(words[i - 15], 18) ^ (words[i - 15] >> 3);
      const uint32_t s1 = rotateRight(words[i - 2], 17) ^ rotateRight(words[i - 2], 19) ^ (words[i - 2] >> 10);
      words[i] = words[i - 16] + s0 + words[i - 7] + s1;
    }
    uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
    uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
    for (size_t i = 0; i < 64; ++i) {
      const uint32_t s1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
      const uint32_t temp1 = h + s1 + choose(e, f, g) + constants[i] + words[i];
      const uint32_t s0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
      const uint32_t temp2 = s0 + majority(a, b, c);
      h = g; g = f; f = e; e = d + temp1;
      d = c; c = b; b = a; a = temp1 + temp2;
    }
    state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
    state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    secureClear(words, sizeof(words));
  }

  uint32_t state_[8];
  uint8_t buffer_[64];
  uint64_t length_;
  size_t used_;
};

inline bool generateMnemonicIndexes(const char* rolls, size_t rollCount, uint16_t indexes[24], size_t& wordCount) {
  if (rollCount != 50 && rollCount != 99) return false;
  for (size_t i = 0; i < rollCount; ++i) {
    if (rolls[i] < '1' || rolls[i] > '6') return false;
  }

  uint8_t digest[32];
  Sha256 hash;
  hash.update(reinterpret_cast<const uint8_t*>(rolls), rollCount);
  hash.final(digest);
  const size_t entropyBytes = rollCount == 50 ? 16 : 32;
  const size_t entropyBits = entropyBytes * 8;
  uint8_t checksum[32];
  Sha256 checksumHash;
  checksumHash.update(digest, entropyBytes);
  checksumHash.final(checksum);
  wordCount = entropyBytes == 16 ? 12 : 24;
  for (size_t word = 0; word < wordCount; ++word) {
    uint16_t index = 0;
    for (size_t bit = 0; bit < 11; ++bit) {
      const size_t position = word * 11 + bit;
      uint8_t value;
      if (position < entropyBits) {
        value = (digest[position / 8] >> (7 - (position % 8))) & 1;
      } else {
        value = (checksum[(position - entropyBits) / 8] >> (7 - ((position - entropyBits) % 8))) & 1;
      }
      index = static_cast<uint16_t>((index << 1) | value);
    }
    indexes[word] = index;
  }
  secureClear(digest, sizeof(digest));
  secureClear(checksum, sizeof(checksum));
  return true;
}

inline void wordAt(uint16_t index, char* output, size_t outputSize) {
  if (!outputSize) return;
  if (index >= 2048) {
    output[0] = '\0';
    return;
  }
  const char* word = kEnglishWords;
  while (index--) {
    while (*word++) {}
  }
  size_t position = 0;
  while (*word && position + 1 < outputSize) output[position++] = *word++;
  output[position] = '\0';
}
