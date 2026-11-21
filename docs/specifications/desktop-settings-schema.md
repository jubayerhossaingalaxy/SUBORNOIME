# Desktop Settings Schema

## Format and compatibility

Settings are stored as UTF-8, newline-delimited key/value pairs and written
atomically. The current schema version is **3**.

- Version 1 contained engine settings: active layout, candidate limit, and
  learning enablement.
- Version 2 adds General, Interface, Topbar, and Input groups.
- Version 3 changes personal learning to an explicit opt-in: it is disabled by
  default, and version 1/2 files migrate with learning disabled.
- The next successful save writes the complete version 3 representation.
- Unknown keys, duplicate keys, malformed values, unsupported enum values,
  integer overflow, and values outside supported ranges are rejected.
- A missing file returns default settings. An unreadable existing file is an
  I/O error.

## Engine

| Key | Type | Default | Allowed values |
|---|---|---|---|
| active_layout | string | suborno-phonetic | registered layout ID |
| candidate_limit | integer | 16 | 1–100 |
| learning_enabled | boolean | false | true / false; false until explicitly enabled |

Personal learning records committed words and typed aliases in the local user
dictionary. It must remain disabled unless the user explicitly opts in. Password
and other sensitive-field suppression is a separate release requirement and
must be verified before enabling learning in production.

## General

| Key | Type | Default | Allowed values |
|---|---|---|---|
| remember_last_input_layout | boolean | true | true / false |
| start_with_windows | boolean | false | true / false |
| start_mode | enum | last-state | last-state, topbar, system-tray, main-window |
| switching_mode | enum | per-application-window | per-application-window, global |

The Windows Run registry value is the effective OS startup mechanism. The
desktop application mirrors the observed registry state into
start_with_windows and uses rollback if settings persistence fails.

## Interface

| Key | Type | Default | Validation |
|---|---|---|---|
| theme_id | string | suborno-dark | lowercase ASCII letters, digits, hyphen, underscore |
| show_candidate_window | boolean | true | true / false |
| next_word_prediction_enabled | boolean | false | true / false |
| candidate_orientation | enum | vertical | vertical, horizontal |
| candidate_fixed_height | boolean | true | true / false |
| use_os_default_bangla_font | boolean | true | true / false |
| enable_sliding_effect | boolean | true | true / false |
| follow_caret | boolean | true | true / false |
| follow_mouse_pointer_if_caret_tracking_fails | boolean | true | true / false |

## Topbar

| Key | Type | Default | Range |
|---|---|---|---|
| idle_dimming_enabled | boolean | true | true / false |
| idle_opacity_percent | integer | 50 | 0–100 |
| idle_time_seconds | integer | 10 | 1–3600 |
| remember_topbar_position | boolean | true | true / false |

## Input

| Key | Type | Default | Allowed values |
|---|---|---|---|
| layout_switch_shortcut | string | F10 | F1, F2, F3, F4, F10, F12, Ctrl+Space, Alt+Shift, Win+Space |
| disable_encoding_shortcut | boolean | false | true / false |
| map_numpad_to_bangla_numbers | boolean | true | true / false |
| change_locale_to_bn_bd_when_layout_not_english | boolean | true | true / false |
| ignore_caps_lock_state_in_bangla | boolean | false | true / false |
| select_candidate_using_number_keys | boolean | true | true / false |
| preferred_encoding | enum | unicode | unicode, ansi |

## Implementation boundary

The storage schema and validation are implemented. Not every option is wired to
a runtime UI or TSF behavior yet. The feature-parity matrix identifies which
consumer/acceptance test is still required for each option. A persisted option
must not be represented in the UI as operational until its runtime consumer is
implemented and tested.
