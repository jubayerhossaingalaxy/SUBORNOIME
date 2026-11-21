# Dictionary

The portable dictionary stores normalized word entries in SQLite and exposes
exact-match, prefix-search, commit-frequency, and personal-word operations.
Queries use bound parameters; prefix search treats SQL `LIKE` metacharacters in
the requested prefix literally. SQLite step failures are reported as
`DictionaryError`.

The repository does not bundle a Bengali word list. Production dictionary data
must be added only with verified provenance, redistribution terms, and
attribution.
