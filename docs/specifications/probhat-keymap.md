# Probhat fixed keymap

## Mapping source

The physical scan-code, base, Shift, and CapsLock output table is independently implemented
from the MIT-licensed princebillygk/probhat-layout-windows repository, layout.klc, upstream
revision debc8310c60200143c6c0420433cca8506771e09. Suborno does not copy the upstream driver or installer.

Probhat is a fixed physical keyboard map, not a phonetic transliterator. Vowel signs stay
vowel signs on their mapped keys; they are not rewritten based on a preceding consonant.
For example, base A produces া, Shift+A produces অ, base W produces ূ, and Shift+W
produces ঊ.

## CapsLock semantics

CapsLock follows the KLC CAPLOK/SGCAPS column. Keys marked as CapsLock-sensitive use the
alternate Shift output when CapsLock is active; keys without that flag do not. Holding
Shift and CapsLock together returns the base output for CapsLock-sensitive keys.

## Physical-key policy

Windows fixed-layout input uses scan codes, not the active Windows logical character.
Ctrl/Alt/Windows shortcuts are not consumed. Unmapped numpad digits remain unmapped.
The KLC includes the numpad decimal key but does not define a numpad digit-to-Bengali layer.

The source KLC declares dead keys for certain Latin accent outputs. Suborno currently
uses literal visible output for those layers; full dead-key composition is not claimed.

## Test coverage

Unit tests cover base/Shift pairs, CapsLock flags, literal vowel-sign concatenation,
punctuation/currency, scan-code precedence, unmapped numpad keys, and grapheme-safe backspace.