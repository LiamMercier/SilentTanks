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
    if [[ -f "$dll" ]]; then
        cp "$dll" "$STAGING/"
        echo "Copying $dll"
    else
        echo "Missing a .dll reported by ldd ($dll)"
    fi
done
