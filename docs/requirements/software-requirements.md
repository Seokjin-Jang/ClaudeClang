# 소프트웨어 요구사항 명세서 (이터레이션 1 범위)

- 문서 ID: SRS-001, 베이스라인 BL-SWR-0.7
- 입력: OEM-SWR-001 (BL-OEM-1.0), `assumptions.md`
- 이터레이션 1 범위: OEM-SR-001~004, OEM-FR-007(초기 출력), OEM-NFR-001, OEM-IF-001/002/003/005/006/009, 그리고 OEM-SR-002 검증에 필요한 최소 운전자 요청(OEM-FR-001, OEM-IF-004의 값 집합 검증과 적용, A-24). 그 외 FR-001 상세, FR-002~006, NFR-002 등은 이후 이터레이션이다.
- 기능 안전 요구사항과 ASIL B 요구(SWR-005, 007, 008, 009, 013, 021, 101, 102, 104, 114, 120~126)는 `safety-requirements.md`에서 관리하며 이 문서에는 쓰지 않는다.
- 이 문서의 QM 요구사항도 같은 SW 요소 안에서 ASIL B 수준의 개발 기준(MC/DC 100%, 정적 분석 예외 없음)으로 개발한다. (A-17)
- 번호 규칙: SWR-001~021은 OEM 사양서 추적표가 배정한 번호를 따르고, 공급자가 도출한 요구사항은 SWR-1xx를 쓴다. OEM 번호 SWR-020(OEM-FR-007)은 이터레이션 2에서 구현한다. SWR-019는 예약(A-14)이다.
- 우선순위: 이터레이션 1의 모든 요구사항은 OEM 입력 또는 ASIL B 안전 요구의 구현과 검증에 필요하므로 `필수`로 둔다. `권장`은 필수가 아닌 요구가 생길 때 쓴다.

## 변경 이력

| 버전 | 날짜 | 변경 내용 | 영향 ID |
|---|---|---|---|
| BL-SWR-0.1 | 2026-11-27 | 초판 작성 | SWR-016, 101~106, 110~114 |
| BL-SWR-0.2 | 2026-11-27 | 리뷰 반려 조치: 복합 요구 분할(SWR-107, 108, 109 신설), SWR-103/110을 제약사항으로 이동, SWR-113 삭제, SWR-114를 안전 문서로 이동, 우선순위·ASIL 열 추가, SWR-020 추가 | SWR-016, 020, 101~112 |
| BL-SWR-0.3 | 2026-11-27 | 재리뷰 반려 조치: SWR-101, 102, 104를 안전 문서로 이동, SWR-109/112를 분할(SWR-115, 116 신설), SWR-106/108 우선순위를 필수로 변경, SWR-020 수용기준과 부록 A 이유코드 전체 집합 복원, 범위 문구(A-24) 명시 | SWR-020, 101, 102, 104, 106, 108, 109, 112, 115, 116 |
| BL-SWR-0.4 | 2026-11-27 | 3차 리뷰 반려 조치: SWR-112를 112(Clang 18+)와 117(C17)로 분할, 우선순위 근거 명시 | SWR-112, 117 |
| BL-SWR-0.5 | 2026-11-27 | 5차 리뷰 반려 조치: 부록 A에 priority_reason 집합 행, 부록 C(입력·필드 값 정의) 추가 | 부록 A, C |
| BL-SWR-0.6 | 2026-11-27 | 6차 리뷰 지적 조치: 부록 C에 OEM-IF-001, 003의 필드 3개(vehicle_speed_kph, rear_left_approach_risk, rear_right_approach_risk) 추가 | 부록 C |
| BL-SWR-0.7 | 2026-11-27 | 구현과 시험 연결 후 검증 열 정정: SWR-016에 단위, SWR-103에 통합 추가 | SWR-016, 103 |

## 1. 기능 요구사항

| ID | 출처 | ASIL | 우선순위 | 요구사항 | 수용기준 | 검증 |
|---|---|---|---|---|---|---|
| SWR-107 | OEM-IF-006 | QM | 필수 | SW는 `priority_reason`을 부록 A의 표에 따라 `reason_code`로도 매핑해 제공해야 한다. | 모든 `priority_reason` 값에 대해 부록 A의 `reason_code`가 제공된다. | 단위, 통합, 시스템 |
| SWR-105 | OEM-FR-007, A-09 | QM | 필수 | SW는 기동 직후 두 출력을 `RELEASE`, 상태를 `NORMAL`, 이유코드를 `INIT`으로 시작해야 한다. | 초기화 직후 입력을 주입하기 전의 결과와, 유효한 중립 입력(충돌 `NONE`, 접근위험 FALSE, `sensor_fault` FALSE, 운전자 요청 없음)으로 수행한 첫 평가 결과에서 두 출력은 `RELEASE`, 상태는 `NORMAL`이다. | 단위, 통합, 시스템 |
| SWR-106 | OEM-FR-001, OEM-IF-004, A-13, A-24 | QM | 필수 | SW는 운전자 요청(`side`, `action`, `source`)의 값이 등록된 집합이 아니면 요청을 거절해야 한다. | `side`는 `left/right/all`, `action`은 `lock/unlock`, `source`는 `physical_button/avn/voice/mobile_app`만 수락한다. 그 외 값이나 누락은 요청이 거절되어 출력이 변하지 않고 이유코드는 `REQUEST_REJECTED`다. | 단위, 통합, 시스템 |
| SWR-108 | OEM-FR-001, A-13, A-24 | QM | 필수 | SW는 유효한 운전자 요청을 상위 규칙이 막지 않을 때 선택한 도어에 적용해야 한다. | `lock/unlock`이 `left/right/all`로 선택한 도어에만 적용된다. 이유코드는 `DRIVER_LOCK` 또는 `DRIVER_RELEASE`다. 상위 규칙(SWR-104)이 막는 경우 적용되지 않는다. 4개 경로별 처리와 주행 중 해제 거절은 이터레이션 2다. | 단위, 통합, 시스템 |
| SWR-020 | OEM-FR-007 | QM | 필수 | SW는 `ignition_on`이 FALSE이면 두 출력을 `RELEASE`로, 상태를 `OFF`로 전환하고 이유코드 `IGNITION_OFF`를 제공해야 한다. (이터레이션 2에서 구현) | `ignition_on=FALSE`인 첫 평가주기에 두 출력이 `RELEASE`, 상태가 `OFF`, 이유코드가 `IGNITION_OFF`다. | 단위, 통합, 시스템 |

## 2. 비기능 요구사항

### 2.1 품질 요구사항 (ISO 25010)

| ID | 출처 | ASIL | ISO 25010 특성 | 우선순위 | 요구사항 | 수용기준 | 검증 |
|---|---|---|---|---|---|---|---|
| SWR-016 | OEM-NFR-001 | QM | 신뢰성 | 필수 | 동일한 입력순서와 초기상태에는 동일한 제어결과가 생성되어야 한다. | 고정 시계로 같은 입력 벡터를 1,000회 재생했을 때 공개 결과(부록 B 직렬화)의 SHA-256이 모두 같다. | 단위, 시스템 |

### 2.2 제한 (Limitation)

| ID | 출처 | ASIL | 우선순위 | 요구사항 | 수용기준 | 검증 |
|---|---|---|---|---|---|---|
| SWR-111 | A-01 | QM | 필수 | 평가 주기는 10 ms이며 한 번의 평가는 입력 스냅샷 1건을 처리해야 한다. | 평가 진입점은 호출당 스냅샷 1건만 받는다. 시간 기준 요구사항은 10 ms 단위로 검증한다. | 단위, 통합 |

### 2.3 제약사항 (Constraint)

| ID | 출처 | ASIL | 우선순위 | 요구사항 | 수용기준 | 검증 |
|---|---|---|---|---|---|---|
| SWR-103 | OEM-NFR-001, A-01 | QM | 필수 | SW는 시스템 시계를 읽지 않고 호출자가 주입한 시각만 사용해야 한다. | 평가 진입점이 `now_s`를 입력으로 받고, 같은 입력 열과 시각 열이면 같은 결과가 나온다. 소스에 시스템 시계 함수 호출이 없다. | 단위, 통합, 정적 분석 |
| SWR-110 | A-20 | QM | 필수 | SW는 동적 메모리를 사용하지 않아야 한다. | 정적 분석(또는 리뷰) 결과 `malloc`, `calloc`, `realloc`, `free` 호출이 없다. | 정적 분석, 리뷰 |
| SWR-112 | A-19 | QM | 필수 | 제품 코드는 Clang 18 이상으로 빌드되어야 한다. | 빌드 로그의 컴파일러가 Clang 18 이상이다. | 빌드 |
| SWR-117 | A-19 | QM | 필수 | 제품 코드는 C17 표준으로 빌드되어야 한다. | `-std=c17`(CMake `C_STANDARD 17`)로 빌드된다. | 빌드 |
| SWR-116 | A-19 | QM | 필수 | 제품 코드는 빌드 경고 없이 빌드되어야 한다. | 빌드에 `-Wall -Wextra -Wpedantic -Werror`를 적용해 통과한다. | 빌드 |
| SWR-109 | A-19 | QM | 필수 | 제품 코드의 모든 함수는 반환값을 가져야 한다. | `void` 함수가 없다. | 리뷰, 정적 분석 |
| SWR-115 | A-19 | QM | 필수 | 제품 코드의 모든 함수는 `return`을 함수 끝에서 한 번만 호출해야 한다. | 함수당 `return`이 1개이고 함수 끝에 있다. | 리뷰, 정적 분석 |

## 부록 A. 상태, 이유코드, 경고코드

| 구분 | 값 |
|---|---|
| 상태 `state` | `NORMAL`, `DEGRADED`, `FAULT`, `OFF`(SWR-020, 이터레이션 2) |
| 입력 유효성 `input_validity` | `VALID`, `STALE`, `INVALID` (그룹별) |
| 우선순위 사유 `priority_reason` 집합 | `INIT`, `CRASH`, `CRASH_PENDING`, `SENSOR_FAULT`, `DEGRADED`, `APPROACH_RISK`, `APPROACH_RISK_SUPPRESS`, `DRIVER`, `REJECTED`, `IGNITION_OFF`(SWR-020, 이터레이션 2), `NONE` |
| 이유코드 `reason_code` 전체 집합 | `NONE`, `INIT`, `CRASH_RELEASE`, `CRASH_PENDING_HOLD`, `SENSOR_FAULT_HOLD`, `INPUT_DEGRADED_HOLD`, `APPROACH_RISK_LOCK`, `RELEASE_SUPPRESSED`, `DRIVER_LOCK`, `DRIVER_RELEASE`, `REQUEST_REJECTED`, `IGNITION_OFF`(SWR-020, 이터레이션 2) |
| 경고코드 `warning_code` | `NONE`, `W_INPUT_STALE`, `W_INPUT_INVALID`, `W_SENSOR_FAULT` (SWR-126) |

`priority_reason`(판정에서 이긴 규칙)과 `reason_code`의 매핑 (SWR-107). 근거 요구사항을 함께 둔다.

| priority_reason | reason_code | 근거 요구사항 |
|---|---|---|
| `INIT` | `INIT` | SWR-105 |
| `CRASH` | `CRASH_RELEASE` | SWR-007, SWR-120 |
| `CRASH_PENDING` | `CRASH_PENDING_HOLD` | SWR-008 |
| `SENSOR_FAULT` | `SENSOR_FAULT_HOLD` | SWR-021 |
| `DEGRADED` | `INPUT_DEGRADED_HOLD` | SWR-125 |
| `APPROACH_RISK` | `APPROACH_RISK_LOCK` | SWR-005 |
| `APPROACH_RISK_SUPPRESS` | `RELEASE_SUPPRESSED` | SWR-121 |
| `DRIVER` | `DRIVER_LOCK` 또는 `DRIVER_RELEASE`(요청의 action에 따름) | SWR-108 |
| `REJECTED` | `REQUEST_REJECTED` | SWR-106 |
| `IGNITION_OFF` | `IGNITION_OFF` | SWR-020 (이터레이션 2) |
| `NONE` | `NONE` | 어떤 규칙도 출력을 바꾸지 않은 주기 |

## 부록 B. 공개 결과 직렬화 (SWR-016)
한 평가주기의 결과를 `lock_left,lock_right,state,reason_code,warning_code,validity_speed,validity_crash,validity_approach,validity_fault`의 순서로 `,`로 이어 붙이고 끝에 `\n`을 붙인 문자열(UTF-8)을 한 줄로 만든다. 입력 벡터의 모든 평가주기의 줄을 순서대로 이어 붙인 전체 문자열을 SHA-256으로 해시해 비교한다.

## 부록 C. 입력과 필드의 값 정의
요구사항과 가정 문서에서 백틱으로 쓰는 식별자와 열거값은 이 표에 있어야 한다. (`ci/check_traceability.py`가 검사한다.)

| 식별자 | 값 집합 / 형식 | 출처 |
|---|---|---|
| `vehicle_speed_kph` | 0.0~300.0 km/h (double) | OEM-IF-001 |
| `gear` | `P/N/D/R` | OEM-IF-001 |
| `crash_status` | `NONE/PENDING/CONFIRMED` | OEM-IF-002 |
| `rear_left_approach_risk` | boolean | OEM-IF-003 |
| `rear_right_approach_risk` | boolean | OEM-IF-003 |
| `side` | `left/right/all` | OEM-IF-004 |
| `action` | `lock/unlock` | OEM-IF-004 |
| `source` | `physical_button/avn/voice/mobile_app` | OEM-IF-004 |
| `ignition_on` | boolean | OEM-IF-009 |
| `sensor_fault` | boolean | OEM-IF-009 |
| `source_timestamp_s` | 초(double) | A-02 |
| `now_s` | 초(double) | A-01 |
| `VehicleSnapshot.timestamp_s` | 초(double), `now_s`와 같은 값 | A-01 |
| `age_us` | 마이크로초 정수 | A-23 |
| `lock_left` | `LOCK/RELEASE` | OEM-IF-005 |
| `lock_right` | `LOCK/RELEASE` | OEM-IF-005 |
| `state` | 부록 A 상태 집합 | OEM-IF-006 |
| `priority_reason` | 부록 A 우선순위 사유 집합 | OEM-IF-006 |
| `reason_code` | 부록 A 이유코드 집합 | OEM-IF-006 |
| `warning_code` | 부록 A 경고코드 집합 | SWR-126 |
| `input_validity` | 부록 A 입력 유효성 집합 | OEM-IF-006 |
