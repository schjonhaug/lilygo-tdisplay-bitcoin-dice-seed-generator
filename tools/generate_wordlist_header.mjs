import {readFileSync, writeFileSync} from "node:fs";

const source = new URL("../../bitcoin-hww/iancoleman-bip39/src/js/wordlist_english.js", import.meta.url);
const sourceText = readFileSync(source, "utf8");
const assignment = sourceText.indexOf("=", sourceText.indexOf('WORDLISTS["english"]'));
const listStart = sourceText.indexOf("[", assignment);
const listEnd = sourceText.indexOf("];", listStart);
const words = [...sourceText.slice(listStart, listEnd).matchAll(/"([a-z]+)"/g)].map((match) => match[1]);
if (words.length !== 2048 || words[0] !== "abandon" || words.at(-1) !== "zoo") {
  throw new Error("Unexpected BIP39 English wordlist");
}

const lines = [];
for (let index = 0; index < words.length; index += 12) {
  lines.push(`    ${words.slice(index, index + 12).map((word) => `"${word}\\0"`).join(" ")}`);
}
const output = `// Generated from iancoleman-bip39/src/js/wordlist_english.js.\n#pragma once\n\nstatic const char kEnglishWords[] =\n${lines.join("\n")}\n    ;\n`;
writeFileSync(new URL("../src/wordlist_english.h", import.meta.url), output);
writeFileSync(new URL("../simulator/wordlist.js", import.meta.url), `// Generated from iancoleman-bip39/src/js/wordlist_english.js.\nwindow.ENGLISH_WORDS = ${JSON.stringify(words)};\n`);
