#!/usr/bin/env bash
# 순환복잡도/함수 길이(Lizard)와 주석률(CLOC, 기준 COMMENT_RATIO_MIN%) 검사.
set -u
RESULT=0
REPORT_DIR="${REPORT_DIR:-reports}"
mkdir -p "$REPORT_DIR"

bash "$(dirname "$0")/lizard.sh" || RESULT=1

if [ -d src ]; then
    cloc --by-file --xml --out="$REPORT_DIR/cloc.xml" src || true
    cloc --json --out="$REPORT_DIR/cloc.json" src || true
    cloc src || true
    python3 "$(dirname "$0")/check_comment_ratio.py" "$REPORT_DIR/cloc.json" "${COMMENT_RATIO_MIN:-30}" || RESULT=1
fi

exit "$RESULT"
