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

PKG_SHARE="/usr/share/silent-tanks"

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
chown -R "${APP_USER}:${APP_GROUP}" "$ROOT_DIR"
chmod 700 "$ROOT_DIR"

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

# Create a server-list.txt file for the client.
if [ -f "$PKG_SHARE/server-list.txt" ]; then
    DEST_SERVERLIST="$ROOT_DIR/server-list.txt"
    if [ ! -f "$ROOT_DIR/server-list.txt" ]; then
        cp "$PKG_SHARE/server-list.txt" "$DEST_SERVERLIST"
        chown "${APP_USER}:${APP_GROUP}" "$DEST_SERVERLIST"
        chmod u+rwX,go-rwx "$DEST_SERVERLIST"
        echo "Copied server-list.txt to $DEST_SERVERLIST"
    else
        echo "Server list file already exists"
    fi
fi

