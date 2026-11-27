#!/usr/bin/env bash
# CMake + Ninja 빌드 (커버리지 계측 포함). 빌드 이미지 안에서 실행한다.
set -u
RESULT=0
BUILD_DIR="${BUILD_DIR:-build}"

if ! cmake -S . -B "$BUILD_DIR" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON; then
    RESULT=1
elif ! cmake --build "$BUILD_DIR"; then
    RESULT=1
fi

exit "$RESULT"
