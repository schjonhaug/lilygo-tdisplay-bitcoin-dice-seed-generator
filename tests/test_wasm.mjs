import createMnemonicModule from "../simulator/mnemonic_wasm.js";

const module = await createMnemonicModule();
function assertMnemonic(rolls, expected) {
  const rollPointer = module._malloc(rolls.length);
  const indexPointer = module._malloc(24 * Uint16Array.BYTES_PER_ELEMENT);
  module.HEAPU8.set(new TextEncoder().encode(rolls), rollPointer);
  const count = module._generate_mnemonic_indexes(rollPointer, rolls.length, indexPointer);
  const indexes = module.HEAPU16.slice(indexPointer / 2, indexPointer / 2 + count);
  for (let i = 0; i < expected.length; ++i) {
    const wordPointer = module._malloc(12);
    const length = module._mnemonic_word(indexes[i], wordPointer, 12);
    const word = String.fromCharCode(...module.HEAPU8.subarray(wordPointer, wordPointer + length));
    module._free(wordPointer);
    if (word !== expected[i]) throw new Error(`Word ${i + 1}: expected ${expected[i]}, got ${word}`);
  }
  module._free(indexPointer);
  module._free(rollPointer);
}

assertMnemonic(
  "12345612345612345612345612345612345612345612345612",
  "unveil nice picture region tragic fault cream strike tourist control recipe tourist".split(" "),
);

module._session_press("1".charCodeAt(0));
if (module._session_screen() !== 1) throw new Error("Session did not show preflight warning");
module._session_press("#".charCodeAt(0));
for (const roll of "12345612345612345612345612345612345612345612345612") module._session_press(roll.charCodeAt(0));
if (module._session_roll_count() !== 50) throw new Error("Session did not record rolls");
module._session_press("#".charCodeAt(0));
if (module._session_screen() !== 3 || module._session_word_count() !== 12) throw new Error("Session did not generate words");
for (let i = 0; i < module._session_word_count(); ++i) module._session_press("#".charCodeAt(0));
if (module._session_screen() !== 4) throw new Error("Session did not reach verification prompt");
module._session_press("#".charCodeAt(0));
if (module._session_screen() !== 5) throw new Error("Session did not start quiz");
const choices = Array.from({length: 3}, (_, i) => module._session_quiz_choice_at(i));
if (new Set(choices).size !== 3) throw new Error("Quiz choices are not unique");
assertMnemonic(
  "133363436436436415622614221225242212144161454643266122155666664444633643543353132626522332412313253",
  "wrist tired novel fetch woman whisper jealous black average crawl task helmet negative wrong foster dry chronic ordinary chase typical recipe sunset draw victory".split(" "),
);
