#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
RUNTIME_DIR="$SCRIPT_DIR/webserver/core"

cd "$RUNTIME_DIR"
exec ./openplc "$@"
