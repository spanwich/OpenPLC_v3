#!/bin/bash
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
exec "$SCRIPT_DIR/start_openplc_web.sh" "$@"
