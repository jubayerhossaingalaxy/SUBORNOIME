# Clean-room policy for SUBORNO

SUBORNO is designed as an independently authored Bangla input system. The product must never copy or derive source code, mapping tables, dictionaries, tests, UI assets or documentation from third-party IME projects.

The implementation follows these rules:

1. Public reference projects are used only for product research and user expectation analysis.
2. No source code from Avro, Borno, or related projects is incorporated into the repository.
3. No mapping tables are copied verbatim.
4. No GPL/MPL-licensed code is imported into the project.
5. No dictionaries are bundled without provenance and licensing review.
6. All transliteration behavior is specified and verified in SUBORNO's own documentation and tests.

Where a feature matches a common user expectation (for example, Bangla phonetic output), the implementation is written from first principles and validated by independent test cases in the project repository.
