#!/bin/bash
set -euo pipefail

# Store server variable data in /var/lib/silent-tanks
ROOT_DIR="/var/lib/silent-tanks"

# TODO: determine where to put this, if this is ok or should go to /etc/silent-tanks/
CRED_FILE="$ROOT_DIR/.pgpass"

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
        if ! runuser -u postgres psql -tA -c "SELECT 1 FROM pg_roles WHERE rolname = '${DB_USER}'" | grep -q 1; then
            runuser -u postgres psql -c "CREATE USER ${DB_USER} WITH PASSWORD '${DB_PASS}';"
            echo "Created postgres role ${DB_USER}"
        else
            echo "User role ${DB_USER} already exists."
        fi

        # Create database
        if ! runuser -u postgres psql -lqt | cut -d \| -f 1 | awk '{$1=$1};1' | grep -qw "${DB_NAME}"; then
            runuser -u postgres psql -c "CREATE DATABASE ${DB_NAME} OWNER ${DB_USER};"
            echo "Created database ${DB_NAME}"
        else
            echo "Database ${DB_NAME} already exists."
        fi

        # Create tables from sql file
        if [ -f "$SQL_FILE" ]; then
            runuser -u postgres psql -d "${DB_NAME}" -v ON_ERROR_STOP=1 -f "$SQL_FILE"
        else
            echo "Sql file at ${SQL_FILE} not found."
        fi

        cat > "$CRED_FILE" << EOF
localhost:5432:${DB_NAME}:${DB_USER}:${DB_PASS}
EOF
        # TODO: env variable needed to make this work?
        chmod 600 "$CRED_FILE"
        chown "${APP_USER}:${APP_GROUP}" "$CRED_FILE"
        echo "Wrote DB credentials to ${CRED_FILE}"
    else
        echo "Credentials already exist at $CRED_FILE; skipping creation"
    fi
else
    echo "psql not found, manual setup will be required."
fi
