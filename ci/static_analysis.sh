#!/usr/bin/env bash
# Cppcheck 전체 룰(--enable=all)로 분석한다. 예외(suppress)는 허용하지 않는다.
# src/ 에서 발생한 지적이 1건이라도 있으면 실패한다.
# 테스트 코드도 함께 분석하는 이유: 테스트가 호출하는 함수를 unusedFunction 으로 오탐하지 않기 위해서다.
set -u
RESULT=0
REPORT_DIR="${REPORT_DIR:-reports}"
mkdir -p "$REPORT_DIR"

if [ -z "$(find src -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) 2>/dev/null | head -n 1)" ]; then
    echo "[static_analysis] src/ 에 분석할 소스가 없다. 건너뜀."
    echo '<?xml version="1.0" encoding="UTF-8"?><results version="2"><cppcheck version="n/a"/><errors/></results>' \
        > "$REPORT_DIR/cppcheck.xml"
else
    INPUTS=(src)
    [ -d tests ] && INPUTS+=(tests)
    cppcheck --enable=all --inconclusive --check-level=exhaustive \
        --library=googletest -I src \
        --xml --xml-version=2 "${INPUTS[@]}" 2> "$REPORT_DIR/cppcheck.xml"
    python3 "$(dirname "$0")/check_cppcheck.py" "$REPORT_DIR/cppcheck.xml" || RESULT=1
fi

exit "$RESULT"
