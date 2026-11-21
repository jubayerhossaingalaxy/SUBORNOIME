# ADR 0002: TSF and COM on Windows

Status: Accepted

## Context

Modern Windows IMEs must integrate with the Text Services Framework and COM infrastructure. Also, the project follows a clean-room implementation strategy and must avoid the IMM32 architecture.

## Decision

The project uses the TSF/COM design for the Windows adapter layer. The shared engine remains portable, while the native Windows adapter implements registration, composition lifecycle, and language-mode handling through TSF and COM interfaces.

## Consequences

- Real TSF validation requires a Windows environment or hosted GitHub Actions runner.
- The portable engine and the native adapter remain clearly separated.
- The architecture is intentionally not a fake keyboard or global keyboard-hook solution.
