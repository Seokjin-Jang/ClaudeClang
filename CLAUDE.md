# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 저장소 현황

이터레이션 1(안전 코어: OEM-SR-001~004 등)의 분석, 설계, 구현, 테스트가 `feature/iteration-1-safety-core`에 있다. 산출물은 `docs/`(요구사항, 아키텍처, 상세 설계, UML, 추적성, 리뷰 기록), 제품 코드는 `src/`(C17, 컴포넌트 4개), 테스트는 `tests/`에 있다. 빌드 이미지, CI 스크립트, Jenkins 구성은 4~5장을 참고한다. `src/`에 소스가 있는데 테스트 실행 파일이 없으면 CI 스크립트가 실패한다.

문서의 정본: 요구사항 `docs/requirements/`(안전 요구는 `safety-requirements.md`, 가정은 `assumptions.md`), 추적성 `docs/traceability/trace.csv`, 설계 `docs/architecture/architecture.md`, `docs/design/detailed-design.md`. 이터레이션 1은 사용자 지시로 `reviewer` 리뷰를 생략했다. (`docs/reviews/iteration-1-review-record.md`)

- 원격: https://github.com/Seokjin-Jang/ClaudeClang.git
- 개발은 Windows(`D:\Repo\ClaudeClang`)에서 진행한다.
- 프로젝트 스킬은 `.claude/skills/`, 서브에이전트는 `.claude/agents/`에 있다.
- 개발 대상은 전자식 차일드락 제어 SW다. 입력 사양은 `OEM/OEM-SWR-001_OEM SW 요구사항 사양서.docx`(OEM-A, BL-OEM-1.0)다. 저장소는 public으로 유지하며, 이 문서의 배포는 승인받았다.
- **언어는 Clang(C/C++)으로 확정**되었다. 사양서 3장의 "Python 3.12 PC/SIL" 실행 환경 기술은 이 결정으로 대체된다. (요구사항 변경)

## 1. 개발 절차

- 개발은 **A-SPICE**를 기준으로 한다.
- 생명주기는 애자일이며, **분석 -> 아키텍처 설계 -> 상세 설계 -> 구현 -> 단위/통합/시스템 테스트를 하나의 이터레이션**으로 한다.
- 개발은 기본적으로 메인 에이전트가 수행하고, 단계별로 해당 스킬을 사용한다.

  | 단계 | 스킬 |
  |---|---|
  | 분석 | `requirements-analysis` |
  | 아키텍처 설계 | `architecture-design` |
  | 상세 설계 | `detailed-design` |
  | 구현 | `implementation` + `test-driven-development` |
  | 단위 테스트 | `unit-test` |
  | 통합 테스트 | `integration-test` |
  | 시스템 테스트 | `sw-system-test-case` |
  | 리뷰 | `review` (reviewer 서브에이전트가 사용) |

- **단계 게이트:** 각 단계가 끝나면 `reviewer` 서브에이전트가 해당 산출물을 리뷰한다. **부적합 사항을 모두 조치하고 재리뷰에서 승인받아야 다음 단계로 진행**할 수 있다.
- **이터레이션 게이트:** 이터레이션이 끝나면 **사용자에게 확인을 받은 뒤** PR 절차를 진행한다. 확인 없이 PR을 만들지 않는다.
- **TDD:** 구현은 `test-driven-development` 스킬로 TDD 방식으로 한다. 단, TDD로 만든 테스트 코드는 **단위 테스트 코드와 분리**한다.
  - 디렉터리를 분리한다. (예: `tests/tdd/`, `tests/unit/`)
  - 테스트 결과 리포트를 별도로 낸다.
  - 커버리지도 별도로 측정한다. 품질 기준(2장)의 단위 테스트 커버리지는 `tests/unit/`만으로 달성해야 하며 TDD 테스트로 대체할 수 없다.

## 2. 소스코드 품질 기준

모두 예외 없이 충족해야 한다.

### 테스트 커버리지
| 테스트 | 기준 |
|---|---|
| 시스템 테스트 | 요구사항 커버리지 100% |
| 통합 테스트 | 함수 커버리지 100%, 테스트 성공률 100% |
| 단위 테스트 | MC/DC 커버리지 100%, 테스트 성공률 100% |

### 정적 분석
- **Cppcheck** 룰 대상 100% 만족. 예외 처리는 없다. (`--suppress`, `// cppcheck-suppress` 사용 금지)

### 소스코드 메트릭
- 순환복잡도 **10 이하**, 함수 길이 **80라인 이하**.
- **Lizard**로 검사하며, 구현이 끝났을 때 **Hook으로 자동 검사**한다. (Hook은 `.claude/settings.json`에 구성한다. 아직 구성되지 않음)

### 주석 (Doxygen, Human in the Loop 정보 필수)
- 주석은 **Doxygen** 형식으로 작성한다.
- 모든 소스 파일과 함수의 Doxygen 주석에 아래를 **반드시** 포함한다. 이 항목도 Doxygen 형식이다.
  - AI가 작성한 경우: 어느 AI 모델이, 언제, 누가 요청해서 작성했는지
  - 사용자 리뷰 완료 여부: 완료했다면 언제, 누가 했는지

```c
/**
 * @brief   속도 설정값을 갱신한다.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  <요청자>
 * @review_status Pending            (Pending | Done)
 * @reviewer      -                  (Done이면 리뷰어 이름)
 * @review_date   -                  (Done이면 리뷰 날짜)
 */
```

- `@ai_*`, `@review_*`는 Doxygen 커스텀 태그이므로 `Doxyfile`의 `ALIASES`에 등록한다.
- **`@review_status`를 `Done`으로 바꾸는 것은 사용자의 리뷰를 받은 뒤에만 한다.** AI가 임의로 `Done`으로 표시하지 않는다.
- 주석률은 CLOC으로 측정하며 `src/` 기준 **30% 이상**(주석 / (주석 + 코드))이어야 한다.
- `Doxyfile`이 `src/`의 미문서화 함수와 문서 오류를 경고로 잡고(`WARN_AS_ERROR`), `ci/check_hitl.py`가 모든 파일·함수에 `@ai_model`, `@ai_date`, `@ai_requester`, `@review_status`가 있는지 검사한다. (`Done`이면 `@reviewer`, `@review_date`도 필수) 테스트 코드의 주석은 `unit-test`/`integration-test` 스킬 규칙을 따른다.

## 3. Git 브랜치 및 PR 전략

- 브랜치는 **Gitflow**를 따른다: `main`(릴리스), `develop`(통합), `feature/*`, `release/*`, `hotfix/*`.
  - 작업은 `develop`에서 딴 `feature/*` 브랜치에서 하고, `main`/`develop`에 직접 푸시하지 않는다.
- PR의 CI는 **GitHub Actions**(`.github/workflows/ci.yml`, 잡 이름 `quality`)이며, **CI를 통과해야만 PR을 완료**한다.
- CI 통과 조건은 빌드 성공과 함께 **2장의 소스코드 품질 기준 전부**를 만족하는 것이다.
- 내부 Jenkins는 PR 게이트가 아니라 `main`/`develop`을 **주기적으로 실행**하는 용도다. (5장)

## 4. 개발 환경

| 용도 | 도구 |
|---|---|
| 언어 | **Clang 18 이상**(현재 이미지는 22). 제품 코드 `src/`는 **C17**(안전 코어), 테스트는 C++20 + GoogleTest |
| 빌드 | **CMake + Ninja** (추천) |
| 정적 분석 | Cppcheck |
| 테스트 프레임워크 | Google Test / Google Mock |
| 커버리지 | LLVM (`llvm-cov`, `llvm-profdata`, MC/DC 포함) |
| 순환복잡도/함수 길이 | Lizard |
| 주석률 | CLOC |
| 문서/주석 검증 | Doxygen (`Doxyfile`, HITL 태그 `ALIASES`) |
| UML | PlantUML (`docs/uml/*.puml`) |
| SBOM | Syft (CycloneDX JSON) |
| 포맷 (추천) | clang-format |
| 조합 테스트 (스킬에서 사용) | Microsoft PICT |

- 추천 사유: CMake는 Clang, GoogleTest, 커버리지 플래그, Cppcheck, `compile_commands.json` 생성을 모두 표준적으로 지원하고, Ninja는 증분 빌드가 빠르다.
- **개발 빌드와 CI 빌드는 동일한 Docker 이미지**를 사용한다. 이미지명은 `ClaudeCLangBuild`다. (Docker는 저장소 이름에 소문자만 허용하므로 실제 태그는 `claudeclangbuild`)
- 위 도구는 모두 이 이미지에 포함한다. 호스트에 직접 설치해서 빌드하지 않는다. (Ubuntu 24.04, Clang 22)

### 명령어

모든 명령은 저장소 루트에서 Git Bash로 실행하며, `ci/in-docker.sh`가 `claudeclangbuild` 이미지 안에서 실행해 준다.

```bash
docker build -t claudeclangbuild docker/build          # 이미지 빌드 (최초 1회, Dockerfile 변경 시)
ci/in-docker.sh ci/build.sh                            # CMake+Ninja 빌드 (커버리지 계측 포함, build/)
ci/in-docker.sh ci/test.sh unit                        # 단위 테스트 + MC/DC 100% 검사
ci/in-docker.sh ci/test.sh integration                 # 통합 테스트 + 함수 커버리지 100% 검사
ci/in-docker.sh ci/test.sh system                      # 시스템 테스트 (공개 인터페이스만 사용)
ci/in-docker.sh ci/test.sh tdd                         # TDD 테스트 (별도 결과/커버리지)
ci/in-docker.sh python3 ci/check_traceability.py       # 추적성 + 시스템 테스트 요구사항 커버리지 100%
ci/in-docker.sh python3 ci/sync_trace.py --status verified   # 소스/테스트 태그에서 trace.csv의 모듈·시험 열 갱신
ci/in-docker.sh ci/static_analysis.sh                  # Cppcheck (src/ 지적 0건)
ci/in-docker.sh ci/metrics.sh                          # Lizard (CCN<=10, 길이<=80) + CLOC 주석률(>=30%)
ci/in-docker.sh ci/lizard.sh                           # Lizard만 (Hook이 사용)
ci/in-docker.sh ci/docs.sh                             # Doxygen 경고, HITL 태그, UML 문법 검사
ci/in-docker.sh ci/sbom.sh                             # SBOM(reports/sbom.cdx.json)
ci/in-docker.sh build/tests/unit/unit_tests --gtest_filter='Suite.Case'   # 단일 테스트 실행
```

- 결과는 `reports/`에 스위트별로 분리되어 저장된다. (`reports/unit/`, `reports/integration/`, `reports/tdd/`: `results.xml`, `cobertura.xml`, `html/` LLVM 리포트 포함)
- 디렉터리 규약(루트 `CMakeLists.txt`가 자동 포함): `src/`, `tests/unit/`(실행 파일 `unit_tests`), `tests/integration/`(`integration_tests`), `tests/system/`(`system_tests`), `tests/tdd/`(`tdd_tests`). 각 디렉터리에 `CMakeLists.txt`가 있어야 한다. 공용 도우미는 `tests/support/`다.
- 추적성 태그 규약: 소스 파일 머리말에 `@module M-...`, 함수 주석에 `@requirement SWR-...`를 쓴다. 테스트 주석에는 `@test <ID>`와 `@requirement SWR-...`를 쓰며 ID는 `UT-`/`IT-`/`ST-`로 시작한다. TDD 테스트는 `@tdd <ID>`를 쓰고 추적 대상이 아니다. 새 요구사항이나 테스트를 추가하면 `sync_trace.py`로 `trace.csv`를 갱신하고 `check_traceability.py`가 통과해야 한다.
- 단위 테스트는 MC/DC와 분기 커버리지를 모두 100%로 요구한다. 판정은 단일 조건 또는 중첩 `if`로 써서 MC/DC 대상이 적으므로 분기 100%를 함께 강제한다.
- 커버리지 측정은 `src/` 코드만 대상으로 한다. (`tests/`, `build/`, `/usr/` 제외)
- Cppcheck는 `--enable=all`로 `src/`와 `tests/`를 함께 분석하되(테스트가 호출하는 함수를 미사용으로 오탐하지 않도록), 판정은 `src/` 지적만 센다. severity가 `information`인 도구 진단 메시지(`missingIncludeSystem` 등)는 세지 않으며, 특정 룰을 제외하는 예외는 없다.

### Hook
- `.claude/settings.json`의 `PostToolUse` Hook(`.claude/hooks/lizard-check.sh`)이 `src/`의 C/C++ 파일을 Write/Edit할 때마다 Lizard를 실행한다. 기준(CCN 10, 함수 80라인)을 넘으면 결과를 돌려주므로 즉시 함수를 분리해서 고친다.

### 아직 자동화되지 않은 기준
- 테스트 코드의 Doxygen 주석 항목(목적, 입력값, 절차, 예상 출력값)과 HITL 태그 검사 (`check_traceability.py`는 `@test`, `@requirement`만 검사한다)
- 시스템 테스트 요구사항 커버리지(`check_traceability.py`)는 `trace.csv`의 `status`가 `verified`인 기능·품질 요구사항을 대상으로 한다. 새 요구사항은 `planned`로 두고, 구현과 시험을 마친 뒤 `verified`로 올린다.

## 5. CI 환경

### GitHub Actions (PR 필수 CI)

- `.github/workflows/ci.yml`: `main`/`develop` 대상 PR, 두 브랜치로의 push, 수동 실행(`workflow_dispatch`)에서 동작한다.
- `docker/build`로 `claudeclangbuild` 이미지를 빌드해(캐시 사용) 그 안에서 `ci/*.sh`를 실행한다. 개발 환경, Jenkins와 같은 스크립트와 이미지다.
- 순서: Build -> Cppcheck -> Lizard/CLOC -> 단위 -> 통합 -> TDD 테스트. 한 검사가 실패해도 나머지를 계속 실행하고, 마지막 `Verdict` 스텝에서 하나라도 실패하면 잡을 실패 처리한다.
- 결과는 Job Summary(`ci/summary.py`)와 `ci-reports` 아티팩트(`reports/`)로 확인한다.
- PR을 막으려면 GitHub 브랜치 보호에서 `main`, `develop`에 **필수 상태 체크 `quality`** 를 설정해야 한다. (아직 설정하지 않음)

### 내부 Jenkins (주기 실행)

- Jenkins는 `main`, `develop`만 대상으로 **매일 02시대(`H 2 * * *`)에 주기 실행**한다. (`Jenkinsfile`의 `cron` 트리거. 다른 브랜치는 주기 실행 없음)
- **Jenkins 안정 버전(LTS, `jenkins/jenkins:lts-jdk21`)** 을 현재 PC의 Docker 위에서 실행하며, 포트는 **8080**을 쓴다. (`http://localhost:8080`)
- 구성 파일: `docker/jenkins/`(Dockerfile, `plugins.txt`, `casc.yaml`), `ci/docker-compose.yml`, 루트 `Jenkinsfile`.
- 관리자 계정은 `admin`, 비밀번호는 `ci/.env`(git 제외)의 `JENKINS_ADMIN_PASSWORD`다. 없으면 만든다: `JENKINS_ADMIN_PASSWORD=<임의 문자열>`

```bash
docker compose -f ci/docker-compose.yml up -d --build   # 기동
docker compose -f ci/docker-compose.yml down            # 중지 (데이터는 jenkins_home 볼륨에 유지)
```

- 설치된 주요 플러그인: Pipeline, Git, JUnit, Warnings Next Generation(Cppcheck, Lizard 결과), Coverage(Cobertura), HTML Publisher(LLVM HTML 리포트), Job DSL, Configuration as Code 등. (`docker/jenkins/plugins.txt`)
- `ClaudeClang` 멀티브랜치 파이프라인 잡이 GitHub 저장소를 하루에 한 번 스캔하며, 브랜치는 `main`, `develop`만 대상으로 한다. 이 두 브랜치에 `Jenkinsfile`이 들어가야 빌드된다.
- 파이프라인은 호스트 Docker 데몬(`docker.sock`)으로 `claudeclangbuild` 컨테이너를 띄워 개발 환경과 같은 방식으로 빌드/검사한다. 한 검사가 실패해도 나머지를 계속 실행하고, 마지막 `Verdict` 단계에서 하나라도 실패했으면 빌드를 실패 처리한다.
- 이 Jenkins는 로컬 PC 전용이다. (root 실행, CSP 해제, `docker.sock` 마운트) 외부에 노출하지 않는다.
- 로컬 Jenkins는 GitHub webhook을 받을 수 없고 GitHub에 상태를 보고하지 않는다. 그래서 PR 게이트는 GitHub Actions가 맡는다.
