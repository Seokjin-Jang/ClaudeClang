#!/usr/bin/env bash
# 테스트 스위트 하나를 실행하고 커버리지를 측정한다. 스위트별 결과/커버리지는 서로 분리된다.
#   사용법: ci/test.sh <unit|integration|tdd>
#   unit        : 테스트 성공률 100% + MC/DC 100%
#   integration : 테스트 성공률 100% + 함수 커버리지 100%
#   tdd         : 테스트 성공률 100% (커버리지는 별도로 측정/보고만 하고 기준은 적용하지 않음)
set -u
SUITE="${1:-}"
BUILD_DIR="${BUILD_DIR:-build}"
OUT="${REPORT_DIR:-reports}/${SUITE}"
RESULT=0
METRIC=""
EXE=""

case "$SUITE" in
    unit)        EXE="$BUILD_DIR/tests/unit/unit_tests";                METRIC="mcdc" ;;
    integration) EXE="$BUILD_DIR/tests/integration/integration_tests";  METRIC="functions" ;;
    tdd)         EXE="$BUILD_DIR/tests/tdd/tdd_tests" ;;
    *)           echo "usage: $0 <unit|integration|tdd>" >&2; RESULT=2 ;;
esac

if [ "$RESULT" -eq 0 ]; then
    rm -rf "$OUT" && mkdir -p "$OUT"
    if [ ! -x "$EXE" ]; then
        if [ -n "$(find src -type f \( -name '*.c' -o -name '*.cpp' \) 2>/dev/null | head -n 1)" ]; then
            echo "[$SUITE] 소스가 있는데 테스트 실행 파일($EXE)이 없다. 실패." >&2
            RESULT=1
        else
            echo "[$SUITE] 소스와 테스트가 아직 없다. 건너뜀."
        fi
    else
        LLVM_PROFILE_FILE="$OUT/%p.profraw" "$EXE" --gtest_output="xml:$OUT/results.xml" || RESULT=1

        if ls "$OUT"/*.profraw >/dev/null 2>&1; then
            IGNORE='(^|/)(tests|build|reports)/|^/usr/'
            llvm-profdata merge -sparse "$OUT"/*.profraw -o "$OUT/merged.profdata"
            llvm-cov report "$EXE" -instr-profile="$OUT/merged.profdata" -ignore-filename-regex="$IGNORE"
            llvm-cov show "$EXE" -instr-profile="$OUT/merged.profdata" -ignore-filename-regex="$IGNORE" \
                -format=html -output-dir="$OUT/html" -show-branches=count -show-mcdc
            llvm-cov export "$EXE" -instr-profile="$OUT/merged.profdata" -ignore-filename-regex="$IGNORE" \
                -format=text > "$OUT/export.json"
            llvm-cov export "$EXE" -instr-profile="$OUT/merged.profdata" -ignore-filename-regex="$IGNORE" \
                -format=lcov > "$OUT/coverage.lcov"
            lcov_cobertura "$OUT/coverage.lcov" -b . -o "$OUT/cobertura.xml" > /dev/null 2>&1 || true
            if [ -n "$METRIC" ]; then
                python3 "$(dirname "$0")/check_coverage.py" "$OUT/export.json" "$METRIC" 100 || RESULT=1
            fi
        else
            echo "[$SUITE] 커버리지 데이터가 생성되지 않았다." >&2
            RESULT=1
        fi
    fi
fi

exit "$RESULT"
