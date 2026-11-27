#!/usr/bin/env bash
# PostToolUse(Write|Edit) Hook: src/ 의 C/C++ 파일이 바뀌면 Lizard(CCN<=10, 함수 길이<=80)를 검사한다.
# 위반하면 exit 2 로 결과를 Claude 에게 돌려줘 즉시 고치게 한다.
set -u

INPUT="$(cat)"
FILE="$(printf '%s' "$INPUT" | sed -n 's/.*"file_path"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p' | head -n 1)"
FILE="${FILE//\\\\//}"
FILE="${FILE//\\//}"
RESULT=0

case "$FILE" in
    */src/*.c|*/src/*.h|*/src/*.cpp|*/src/*.hpp)
        cd "${CLAUDE_PROJECT_DIR:-.}" || exit 0
        if ! command -v docker >/dev/null 2>&1; then
            echo "[lizard-hook] docker 를 찾을 수 없어 검사를 건너뜀" >&2
        else
            OUTPUT="$(ci/in-docker.sh ci/lizard.sh 2>&1)"
            if [ "$?" -ne 0 ]; then
                echo "Lizard 기준 위반(순환복잡도 10 이하, 함수 길이 80라인 이하). 아래 함수를 분리해서 고쳐라:" >&2
                echo "$OUTPUT" >&2
                RESULT=2
            fi
        fi
        ;;
esac

exit "$RESULT"
