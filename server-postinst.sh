#!/bin/bash
set -euo pipefail

APP_USER="silent-tanks"
APP_GROUP="silent-tanks"

# find installing user's directory
TARGET_USER="${TARGET_USER:-${SUDO_USER:-$(whoami)}}"
USER_HOME="$(getent passwd "$TARGET_USER" | cut -d: -f6 || true)"

if [ -n "$USER_HOME" ]; then
    ROOT_DIR="$USER_HOME/.local/share/silent-tanks"
else
    echo "Warning: could not determine user home"
    ROOT_DIR="/var/lib/silent-tanks"
fi

CERT_DIR="$ROOT_DIR/certs"
CRED_FILE="$ROOT_DIR/.pgpass"

PKG_SHARE="/usr/share/silent-tanks"
SQL_FILE="$PKG_SHARE/setup/create_tables.sql"

DB_USER="silenttanksoperator"
DB_NAME="silenttanksdb"

if ! getent group "$APP_GROUP" >/dev/null; then
    if command -v addgroup >/dev/null 2>&1; then
        addgroup --system "$APP_GROUP" || true
    elif command -v groupadd >/dev/null 2>&1; then
        groupadd -system "$APP_GROUP" || true
    else
        echo "no groupadd or addgroup available."
    fi
fi

if ! id -u "$APP_USER" >/dev/null 2>&1; then
    if command -v adduser >/dev/null 2>&1; then
        adduser --system --group --no-create-home --quiet "$APP_USER"
    elif command -v useradd >/dev/null 2>&1; then
        useradd --system --no-create-home --shell /usr/sbin/nologin --gid "$APP_GROUP" "$APP_USER" || true
    else
        echo "No useradd/adduser available."
        return 1
    fi
fi

mkdir -p "$ROOT_DIR"
mkdir -p "$CERT_DIR"

chown -R "${APP_USER}:${APP_GROUP}" "$ROOT_DIR"
chown -R "${APP_USER}:${APP_GROUP}" "$CERT_DIR"

chmod 700 "$ROOT_DIR"
chmod 700 "$CERT_DIR"

# Move over the environment files, map file
if [ -d "$PKG_SHARE/envs" ]; then
    DEST_ENVS="$ROOT_DIR/envs"
    if [ ! -d "$DEST_ENVS" ]; then
        cp -r "$PKG_SHARE/envs" "$DEST_ENVS"
        chown -R "${APP_USER}:${APP_GROUP}" "$DEST_ENVS"
        chmod -R u+rwX,go-rwx "$DEST_ENVS"
        echo "Copied environments folder to $DEST_ENVS"
    else
        echo "Environment folder already exists"
    fi
fi

if [ -f "$PKG_SHARE/mapfile.txt" ]; then
    DEST_MAPFILE="$ROOT_DIR/mapfile.txt"
    if [ ! -f "$ROOT_DIR/mapfile.txt" ]; then
        cp "$PKG_SHARE/mapfile.txt" "$DEST_MAPFILE"
        chown "${APP_USER}:${APP_GROUP}" "$DEST_MAPFILE"
        chmod u+rwX,go-rwx "$DEST_MAPFILE"
        echo "Copied mapfile.txt to $DEST_MAPFILE"
    else
        echo "Mapfile already exists"
    fi
fi

if command -v psql >/dev/null 2>&1; then
    if [ ! -f "$CRED_FILE" ]; then
        # Create postgreSQL database, starting with random password.
        if command -v openssl >/dev/null 2>&1; then
            DB_PASS="$(openssl rand -base64 32 | tr -dc 'A-Za-z0-9' | head -c 24)"
        else
            DB_PASS="$(tr -dc A-Za-z0-9 </dev/urandom | head -c 24 || true)"
        fi

        # Create user
        if ! sudo -u postgres psql -tA -c "SELECT 1 FROM pg_roles WHERE rolname = '${DB_USER}'" | grep -q 1; then
            sudo -u postgres psql -c "CREATE USER ${DB_USER} WITH PASSWORD '${DB_PASS}';"
            echo "Created postgres role ${DB_USER}"
        else
            echo "User role ${DB_USER} already exists."
        fi

        # Create database
        if ! sudo -u postgres psql -lqt | cut -d \| -f 1 | awk '{$1=$1};1' | grep -qw "${DB_NAME}"; then
            sudo -u postgres psql -c "CREATE DATABASE ${DB_NAME} OWNER ${DB_USER};"
            echo "Created database ${DB_NAME}"
        else
            echo "Database ${DB_NAME} already exists."
        fi

        # Create tables from sql file
        if [ -f "$SQL_FILE" ]; then
            sudo -u postgres psql -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "$SQL_FILE"
        else
            echo "Sql file at ${SQL_FILE} not found."
        fi

        cat > "$CRED_FILE" << EOF
DB_USER=${DB_USER}
DB_NAME=${DB_NAME}
DB_PASS=${DB_PASS}
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

# Generate key and certificate.
if [ ! -f "${CERT_DIR}/server.key" ] || [ ! -f "${CERT_DIR}/server.crt" ]; then
    echo "Server certificate or key missing. Generate a self signed certificate using create_self_signed_cert.sh"
else
    echo "Certificate and key already exist."
fi

echo "Finished setup."
exit 0
