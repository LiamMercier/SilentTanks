#!/bin/bash
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

# Store server variable data in /var/lib/silent-tanks
ROOT_DIR="/var/lib/silent-tanks"

CRED_FILE="$ROOT_DIR/.pgpass"

PKG_SHARE="/usr/share/silent-tanks"
SQL_FILE="$PKG_SHARE/setup/create-tables.sql"

DB_USER="silenttanksoperator"
DB_NAME="silenttanksdb"

APP_USER="silent-tanks"
APP_GROUP="silent-tanks"

if ! getent group "$APP_GROUP" >/dev/null; then
    if command -v addgroup >/dev/null 2>&1; then
        addgroup --system "$APP_GROUP" || true
    elif command -v groupadd >/dev/null 2>&1; then
        groupadd --system "$APP_GROUP" || true
    else
        echo "no groupadd or addgroup available to create server group."
        exit 1
    fi
fi

if ! id -u "$APP_USER" >/dev/null 2>&1; then
    if command -v adduser >/dev/null 2>&1; then
        adduser --system --group --no-create-home --quiet "$APP_USER"
    elif command -v useradd >/dev/null 2>&1; then
        useradd --system --no-create-home --shell /usr/sbin/nologin --gid "$APP_GROUP" "$APP_USER" || true
    else
        echo "No useradd/adduser available to create server user."
        exit 1
    fi
fi

# Try to setup postgres
if command -v psql >/dev/null 2>&1; then
    if [ ! -f "$CRED_FILE" ]; then
        # Create postgreSQL database, starting with random password.
        if command -v openssl >/dev/null 2>&1; then
            DB_PASS="$(openssl rand -base64 32 | tr -dc 'A-Za-z0-9' | head -c 24)"
        else
            DB_PASS="$(tr -dc A-Za-z0-9 </dev/urandom | head -c 24 || true)"
        fi

        # Create user
        if ! runuser -u postgres -- psql -tA -c "SELECT 1 FROM pg_roles WHERE rolname = '${DB_USER}'" | grep -q 1; then
            runuser -u postgres -- psql -c "CREATE USER ${DB_USER} WITH PASSWORD '${DB_PASS}';"
            echo "Created postgres role ${DB_USER}"
        else
            echo "User role ${DB_USER} already exists."
        fi

        # Create database
        DB_EXISTS=$(runuser -u postgres -- psql -tA -c "SELECT 1 FROM pg_database WHERE datname='${DB_NAME}'")

        if [ -z "$DB_EXISTS" ]; then
            runuser -u postgres -- psql -c "CREATE DATABASE ${DB_NAME} OWNER ${DB_USER};"
            echo "Created database ${DB_NAME}"
        else
            echo "Database ${DB_NAME} already exists."
        fi

        # Create tables from sql file
        if [ -f "$SQL_FILE" ]; then
            runuser -u postgres -- psql -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "$SQL_FILE"
        else
            echo "Sql file at ${SQL_FILE} not found."
        fi

        cat > "$CRED_FILE" << EOF
127.0.0.1:5432:${DB_NAME}:${DB_USER}:${DB_PASS}
EOF

        chmod 600 "$CRED_FILE"
        chown "${APP_USER}:${APP_GROUP}" "$CRED_FILE"
        echo "Wrote DB credentials to ${CRED_FILE}"
    else
        echo "Credentials already exist at $CRED_FILE; skipping creation"
    fi
else
    echo "psql not found, manual setup will be required."
fi
