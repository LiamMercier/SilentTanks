#!/usr/bin/env bash

# Copyright (c) 2025 Liam Mercier
#
# This file is part of SilentTanks.
#
# SilentTanks is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License Version 3.0
# as published by the Free Software Foundation.
#
# SilentTanks is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License v3.0
# for more details.
#
# You should have received a copy of the GNU Affero General Public License v3.0
# along with SilentTanks. If not, see <https://www.gnu.org/licenses/agpl-3.0.txt>

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
    if [[ "$dll" == /c/WINDOWS/* ]]; then
        echo "Skipping windows system dll ($dll)"
        continue
    fi

    if [[ "$dll" != /ucrt64/bin/* ]]; then
        echo "Skipped dll ($dll) not from /ucrt64/bin/"
        continue
    fi

    if [[ -f "$dll" ]]; then
        cp "$dll" "$STAGING/"
        echo "Copying $dll"

        # also copy license file
        pkg="$(pacman -Qo "$dll" 2>/dev/null | awk '{print $5}' || true)"

        if [[ -n "$pkg" ]]; then
            pkg_root="$(echo "$pkg" | sed -E 's/^(mingw-w64-(ucrt-)?(x86_64)-|ucrt64-)//')"
            licenses="/ucrt64/share/licenses/$pkg_root"
            if [[ -d "$licenses" ]]; then
                mkdir -p "$STAGING/licenses/$pkg_root"
                echo "Copying licenses for $pkg"
                cp -a "$licenses" "$STAGING/licenses/$pkg_root/"
            else
                echo "WARNING: No licenses found for $pkg"
            fi
        else
            echo "WARNING: pacman lookup failed for $dll"
        fi
    else
        echo "Missing a .dll reported by ldd ($dll)"
    fi
done
