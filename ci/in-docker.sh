#!/usr/bin/env bash
# 빌드 이미지(claudeclangbuild) 안에서 명령을 실행한다.
# 개발 빌드와 CI 빌드가 같은 이미지를 쓰도록 이 스크립트 하나로 통일한다.
#   사용법: ci/in-docker.sh <명령...>      예) ci/in-docker.sh ci/build.sh
set -u

IMAGE="${BUILD_IMAGE:-claudeclangbuild}"
RESULT=0

if [ "$#" -eq 0 ]; then
    echo "usage: $0 <command...>" >&2
    RESULT=2
else
    if [ -n "${JENKINS_HOME:-}" ]; then
        # Jenkins 컨테이너(이름: jenkins)의 볼륨을 공유해 워크스페이스 경로를 그대로 맞춘다.
        MOUNT=(--volumes-from jenkins --user "$(id -u):$(id -g)" -w "$PWD")
    else
        HOST_DIR="$(pwd -W 2>/dev/null || pwd)"   # Git Bash 에서는 Windows 경로 사용
        MOUNT=(-v "${HOST_DIR}:/workspace" -w /workspace)
        if [ "$(uname -s)" = "Linux" ] && [ "$(id -u)" != "0" ]; then
            MOUNT+=(--user "$(id -u):$(id -g)")
        fi
    fi
    MSYS_NO_PATHCONV=1 docker run --rm "${MOUNT[@]}" "$IMAGE" "$@"
    RESULT=$?
fi

exit "$RESULT"
