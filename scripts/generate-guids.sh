#!/usr/bin/env bash

# Generate two unique GUIDs for Suborno x64 and x86 installers.

echo "Suborno x64 GUID:"
uuidgen | tr '[:lower:]' '[:upper:]'

echo
echo "Suborno x86 GUID:"
uuidgen | tr '[:lower:]' '[:upper:]'
