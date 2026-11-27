cmake_minimum_required(VERSION 3.28)
project(ClaudeClang LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# 소스 기반 커버리지(MC/DC 포함). 스위트별 측정은 ci/test.sh 가 수행한다.
option(ENABLE_COVERAGE "Instrument for LLVM source-based coverage (incl. MC/DC)" OFF)
if(ENABLE_COVERAGE)
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping -fcoverage-mcdc)
    add_link_options(-fprofile-instr-generate)
endif()

enable_testing()

# 디렉터리 규약 (각 디렉터리에 CMakeLists.txt 가 있으면 자동으로 포함된다)
#   src/               제품 코드
#   tests/unit/        단위 테스트  -> 실행 파일 unit_tests
#   tests/integration/ 통합 테스트  -> 실행 파일 integration_tests
#   tests/tdd/         TDD 테스트   -> 실행 파일 tdd_tests (단위 테스트와 분리)
foreach(dir src tests/unit tests/integration tests/tdd)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt")
        add_subdirectory(${dir})
    endif()
endforeach()
