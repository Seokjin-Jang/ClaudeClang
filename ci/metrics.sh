#!/usr/bin/env bash
# 순환복잡도 10 이하, 함수 길이 80라인 이하(Lizard), 주석률(CLOC) 측정.
set -u
RESULT=0
REPORT_DIR="${REPORT_DIR:-reports}"
CCN_LIMIT="${CCN_LIMIT:-10}"
LENGTH_LIMIT="${LENGTH_LIMIT:-80}"
mkdir -p "$REPORT_DIR"

if [ ! -d src ]; then
    echo "[metrics] src/ 가 없다. 건너뜀."
    echo '<?xml version="1.0" encoding="UTF-8"?><checkstyle version="4.3"/>' > "$REPORT_DIR/lizard.xml"
else
    lizard src -C "$CCN_LIMIT" -L "$LENGTH_LIMIT" --warnings_only > "$REPORT_DIR/lizard.txt" || RESULT=1
    cat "$REPORT_DIR/lizard.txt"
    python3 "$(dirname "$0")/lizard_report.py" < "$REPORT_DIR/lizard.txt" > "$REPORT_DIR/lizard.xml"
    cloc --by-file --xml --out="$REPORT_DIR/cloc.xml" src || true
    cloc src || true
fi

exit "$RESULT"
