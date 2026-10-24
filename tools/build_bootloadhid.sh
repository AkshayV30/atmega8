#!/usr/bin/env bash

set -euo pipefail
TOOLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

for command in tar python3 make gcc install; do
    if ! command -v "$command" >/dev/null 2>&1; then
        printf "Required command not found: %s\n" "$command" >&2
        exit 1
    fi
done

ARCHIVE="$TOOLS_DIR/bootloadHID-2012-12-08.tar.gz"
if [[ ! -f "$ARCHIVE" ]]; then
    printf "BootloadHID source archive is missing: %s\n" "$ARCHIVE" >&2
    exit 1
fi
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

tar -xzf "$ARCHIVE" -C "$WORK_DIR"
SOURCE_FILE="$WORK_DIR/bootloadHID.2012-12-08/commandline/main.c"
python3 - "$SOURCE_FILE" <<'PYCODE'
from pathlib import Path
import sys
p = Path(sys.argv[1])
s = p.read_text()
expected = {
    '#define IDENT_VENDOR_STRING     "obdev.at"': '#define IDENT_VENDOR_STRING     NULL',
    '#define IDENT_PRODUCT_STRING    "HIDBoot"': '#define IDENT_PRODUCT_STRING    NULL',
}
for old, new in expected.items():
    if old not in s:
        raise SystemExit(f'Expected BootloadHID identifier not found: {old}')
    s = s.replace(old, new, 1)
p.write_text(s)
PYCODE
cd "$WORK_DIR/bootloadHID.2012-12-08"
make -C commandline
install -m 0755 commandline/bootloadHID "$TOOLS_DIR/bootloadHID"
"$TOOLS_DIR/bootloadHID" --help
