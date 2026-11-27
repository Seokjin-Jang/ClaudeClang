# ClaudeCLangBuild: 개발 빌드와 CI 빌드가 공통으로 사용하는 이미지.
# Docker 저장소 이름은 소문자만 허용하므로 태그는 claudeclangbuild 이다.
FROM ubuntu:24.04

ARG LLVM_VERSION=22
ARG PICT_VERSION=v3.7.4
ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    HOME=/tmp \
    VIRTUAL_ENV=/opt/venv \
    PATH=/opt/venv/bin:$PATH

# 기본 도구: 빌드(CMake/Ninja), 정적 분석(Cppcheck), 주석률(CLOC),
# 문서(Doxygen), 테스트 프레임워크(GoogleTest/GoogleMock)
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl gnupg git make cmake ninja-build \
        cppcheck cloc doxygen graphviz \
        libgtest-dev libgmock-dev \
        python3 python3-venv python3-pip \
    && rm -rf /var/lib/apt/lists/*

# LLVM/Clang (apt.llvm.org): 컴파일러, llvm-cov/llvm-profdata(MC/DC 커버리지),
# clang-format, clang-tidy, 커버리지 런타임
RUN curl -fsSL https://apt.llvm.org/llvm-snapshot.gpg.key \
        | gpg --dearmor -o /usr/share/keyrings/llvm.gpg \
    && echo "deb [signed-by=/usr/share/keyrings/llvm.gpg] https://apt.llvm.org/noble/ llvm-toolchain-noble-${LLVM_VERSION} main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y --no-install-recommends \
        clang-${LLVM_VERSION} llvm-${LLVM_VERSION} lld-${LLVM_VERSION} \
        clang-format-${LLVM_VERSION} clang-tidy-${LLVM_VERSION} \
        libclang-rt-${LLVM_VERSION}-dev libc++-${LLVM_VERSION}-dev \
    && rm -rf /var/lib/apt/lists/* \
    && for t in clang clang++ clang-format clang-tidy llvm-cov llvm-profdata lld; do \
         ln -sf /usr/bin/${t}-${LLVM_VERSION} /usr/local/bin/${t}; \
       done

ENV CC=clang CXX=clang++

# Python 도구: Lizard(순환복잡도/함수 길이), lcov -> Cobertura 변환
RUN python3 -m venv $VIRTUAL_ENV \
    && pip install --no-cache-dir lizard lcov_cobertura

# Microsoft PICT (조합 테스트 케이스 생성, unit-test 스킬에서 사용)
RUN git clone --depth 1 --branch ${PICT_VERSION} https://github.com/microsoft/pict.git /tmp/pict \
    && make -C /tmp/pict -j"$(nproc)" \
    && install -m 0755 /tmp/pict/pict /usr/local/bin/pict \
    && rm -rf /tmp/pict

WORKDIR /workspace
CMD ["bash"]
