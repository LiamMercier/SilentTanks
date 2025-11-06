#!/usr/bin/env bash
set -euo pipefail

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

#
# setup options
#

# directories
CERT_DIR="/var/lib/silent-tanks/certs"
CERT_FILE="$CERT_DIR/server.crt"
KEY_FILE="$CERT_DIR/server.key"

# If the directory does not exist, stop.
if [ ! -d "$CERT_DIR" ]; then
    echo "Certificate directory (${CERT_DIR}) does not exist."
    exit 1
fi

# ip, defaults to local
SERVER_IP="${1:-127.0.0.1}"
SERVER_DNS="${2:-localhost}"

# cert stays valid for 10 years by default
DAYS_VALID=3650

# 4096 should be expected with current standards.
KEY_BITS=4096

command -v openssl >/dev/null || { echo "openssl is required"; exit 1; }

# Forcibly regenerate the keyfile if enabled
FORCE=false
if [[ "${1-}" == "-force" || "${1-}" == "--force" ]]; then
    FORCE=true
    SERVER_IP="${2:-127.0.0.1}"
    SERVER_DNS="${3:-localhost}"
fi

if [[ ( -f "$CERT_FILE" || -f "$KEY_FILE" ) && "$FORCE" != "true" ]]; then
  echo "Certificate or key already exist. Use '-force' or '--force' to overwrite."
  exit 0
fi

OPENSSL_CNF=$(mktemp)
trap 'rm -f -- "$OPENSSL_CNF"' EXIT

cat > "$OPENSSL_CNF" <<EOF
[ req ]
distinguished_name = req_distinguished_name
x509_extensions = v3_req
prompt = no

# Empty Metadata
[ req_distinguished_name ]
C = NA
ST = NA
L = NA
O = NA
OU = NA
CN = ${SERVER_DNS}

[ v3_req ]
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[ alt_names ]
DNS.1 = ${SERVER_DNS}
IP.1 = ${SERVER_IP}
EOF

echo "Generating $KEY_BITS-bit private key"
openssl genpkey -algorithm RSA -out "$KEY_FILE" -pkeyopt rsa_keygen_bits:$KEY_BITS

chmod 600 "$KEY_FILE"

echo "Generating self-signed certificate (valid for $DAYS_VALID days) with ip ${SERVER_IP}"

openssl req -new -x509 -days "$DAYS_VALID" \
    -key "$KEY_FILE" \
    -out "$CERT_FILE" \
    -sha256 \
    -config "$OPENSSL_CNF" \
    -extensions v3_req

chmod 644 "$CERT_FILE"

APP_USER="silent-tanks"
APP_GROUP="silent-tanks"

if getent group "$APP_GROUP" >/dev/null && id -u "$APP_USER" >/dev/null 2>&1; then
    chown "${APP_USER}:${APP_GROUP}" "$CERT_FILE"
    chown "${APP_USER}:${APP_GROUP}" "$KEY_FILE"
fi

echo "Created: "
echo " - $KEY_FILE (perm 600)"
echo " - $CERT_FILE (perm 644)"

