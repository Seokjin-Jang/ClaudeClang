#!/usr/bin/env bash
# Doxygen 주석 검사(미문서화/문서 오류 = 실패), Human in the Loop 태그 검사, UML(PlantUML) 문법 검사.
set -u
RESULT=0
REPORT_DIR="${REPORT_DIR:-reports}"
mkdir -p "$REPORT_DIR"

if [ -z "$(find src -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) 2>/dev/null | head -n 1)" ]; then
    echo "[docs] src/ 에 소스가 없다. Doxygen 검사 건너뜀."
else
    rm -rf "$REPORT_DIR/doxygen"
    doxygen Doxyfile || RESULT=1
    python3 "$(dirname "$0")/check_hitl.py" "$REPORT_DIR/doxygen/xml" || RESULT=1
fi

if ls docs/uml/*.puml >/dev/null 2>&1; then
    mkdir -p "$REPORT_DIR/uml"
    plantuml -failonerror -tsvg -o "$PWD/$REPORT_DIR/uml" docs/uml/*.puml || RESULT=1
else
    echo "[docs] docs/uml/*.puml 이 없다. UML 검사 건너뜀."
fi

exit "$RESULT"
