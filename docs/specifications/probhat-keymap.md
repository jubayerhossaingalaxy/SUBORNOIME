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

The KLC defines base, Shift, Ctrl+Alt, and Shift+Ctrl+Alt columns. Suborno maps the
Ctrl+Alt columns through the Windows Right-Alt (AltGr) key state. Ordinary Ctrl+Alt chords
remain shortcuts and are not consumed unless Right Alt is active. The KLC CAP column is a
bit field: CAPLOK (0x01) controls the base/Shift layers and CAPLOKALTGR (0x04) controls
the AltGr/Shift+AltGr layers. Values 1 and 5 enable ordinary CapsLock-to-Shift behavior;
value 4 changes output only in AltGr mode and must not affect base/Shift output. Holding
Shift and CapsLock together returns the unshifted output within the applicable layer.
Dead-key composition remains incomplete.

## Physical-key policy

Windows fixed-layout input uses scan codes, not the active Windows logical character.
Ctrl/Alt/Windows shortcuts are not consumed. Unmapped numpad digits remain unmapped.
The KLC OEM_102 key (scan code 0x56) and numpad decimal key (scan code 0x53) are
recognized by the TSF virtual-key translation boundary so their KLC entries can be
reached. The KLC includes the numpad decimal key but does not define a numpad
digit-to-Bengali layer.

The source KLC declares dead keys for certain Latin accent outputs. Suborno currently
uses literal visible output for those layers; full dead-key composition is not claimed.

## Test coverage

Unit tests cover base/Shift pairs, both AltGr layers, ordinary CAPLOK behavior,
CAPLOKALTGR behavior within AltGr only, shortcut pass-through, literal vowel-sign
concatenation, punctuation/currency, scan-code precedence, unmapped numpad keys, and
grapheme-safe backspace, OEM_102 translation, and numpad-decimal translation.
Dead-key composition remains incomplete pending its own stateful implementation and tests.