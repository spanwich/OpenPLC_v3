#!/bin/bash
set -euo pipefail

PROJECT_DIR="$(dirname "$(readlink -f "$0")")"
WEB_DIR="$PROJECT_DIR/webserver"
VENV_PY="$PROJECT_DIR/.venv/bin/python3"

if [ ! -x "$VENV_PY" ]; then
    echo "error: expected virtualenv python at $VENV_PY" >&2
    exit 1
fi

if [ -d "/docker_persistent" ]; then
    mkdir -p /docker_persistent/st_files
    cp -n "$WEB_DIR/dnp3_default.cfg" /docker_persistent/dnp3.cfg 2>/dev/null || true
    cp -n "$WEB_DIR/openplc_default.db" /docker_persistent/openplc.db 2>/dev/null || true
    cp -n "$WEB_DIR/active_program_default" /docker_persistent/active_program 2>/dev/null || true
    cp -n "$WEB_DIR/st_files_default"/* /docker_persistent/st_files/ 2>/dev/null || true
    : > /docker_persistent/persistent.file
    : > /docker_persistent/mbconfig.cfg
else
    [ -f "$WEB_DIR/openplc_default.db" ] && [ ! -f "$WEB_DIR/openplc.db" ] && cp "$WEB_DIR/openplc_default.db" "$WEB_DIR/openplc.db"
    [ -f "$WEB_DIR/active_program_default" ] && [ ! -f "$WEB_DIR/active_program" ] && cp "$WEB_DIR/active_program_default" "$WEB_DIR/active_program"
    mkdir -p "$WEB_DIR/st_files"
    if [ -d "$WEB_DIR/st_files_default" ]; then
        cp -n "$WEB_DIR/st_files_default"/* "$WEB_DIR/st_files/" 2>/dev/null || true
    fi
fi

cd "$WEB_DIR"
exec "$VENV_PY" webserver.py "$@"
