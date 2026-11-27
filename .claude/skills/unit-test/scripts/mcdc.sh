#!/usr/bin/env bash
# LLVM 기반 MC/DC 커버리지 측정 스크립트 (LLVM 18+ 필요)
# 사용법: mcdc.sh <소스.c> <테스트.c> [출력디렉터리]
#   - 소스와 테스트를 함께 빌드해 테스트를 실행하고 MC/DC 보고서를 출력한다.
#   - 환경변수 CC, LLVM_PROFDATA, LLVM_COV 로 도구 경로를 바꿀 수 있다.
set -u

SRC="${1:-}"
TEST="${2:-}"
OUT="${3:-build/mcdc}"
CC="${CC:-clang}"
LLVM_PROFDATA="${LLVM_PROFDATA:-llvm-profdata}"
LLVM_COV="${LLVM_COV:-llvm-cov}"
RESULT=0

if [ -z "$SRC" ] || [ -z "$TEST" ]; then
    echo "usage: $0 <source.c> <test.c> [outdir]" >&2
    RESULT=2
elif ! command -v "$CC" >/dev/null 2>&1; then
    echo "error: $CC not found" >&2
    RESULT=3
else
    mkdir -p "$OUT"
    EXE="$OUT/test_mcdc"
    PROF="$OUT/default.profraw"
    DATA="$OUT/default.profdata"

    if ! "$CC" -O0 -g -fprofile-instr-generate -fcoverage-mapping -fcoverage-mcdc \
            "$SRC" "$TEST" -o "$EXE"; then
        echo "error: build failed (clang 18+ required for -fcoverage-mcdc)" >&2
        RESULT=4
    else
        LLVM_PROFILE_FILE="$PROF" "$EXE"
        TEST_RC=$?
        if [ ! -f "$PROF" ]; then
            echo "error: no profile data produced" >&2
            RESULT=5
        elif ! "$LLVM_PROFDATA" merge -sparse "$PROF" -o "$DATA"; then
            RESULT=6
        else
            "$LLVM_COV" report "$EXE" -instr-profile="$DATA" "$SRC"
            "$LLVM_COV" show "$EXE" -instr-profile="$DATA" "$SRC" \
                -show-branches=count -show-mcdc
            RESULT=$TEST_RC
        fi
    fi
fi

exit "$RESULT"
