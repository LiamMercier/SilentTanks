#!/usr/bin/env bash

# This script copies over .dll files required for the client on windows.

if [[ $# -ne 2 ]]; then
    echo "Usage: $0 <path-to-binary> <staging-folder>"
    exit 1
fi

BINARY="$1"
STAGING="$2"

mkdir -p "$STAGING"

# Grab every .dll reported by ldd.
ldd "$BINARY" | awk '/=>/ {print $3}' | while read dll; do
    if [[ -f "$dll" ]]; then
        cp "$dll" "$STAGING/"
        echo "Copying $dll"
    else
        echo "Missing a .dll reported by ldd ($dll)"
    fi
done
