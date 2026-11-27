# 소프트웨어 상세 설계 (이터레이션 1)

- 문서 ID: SWD-001, 베이스라인 BL-SWD-0.2
- 입력: `docs/architecture/architecture.md` (BL-SWA-0.1), `docs/requirements/*.md` (BL-SWR-0.6)
- 이 이터레이션은 사용자 지시로 `reviewer` 리뷰를 생략했다. (`docs/reviews/iteration-1-review-record.md`)

## 변경 이력

| 버전 | 날짜 | 변경 내용 | 영향 ID |
|---|---|---|---|
| BL-SWD-0.1 | 2026-11-27 | 초판 작성 | M-VAL-01, M-STA-01, M-ARB-01, M-CTL-01, M-CTL-02 |
| BL-SWD-0.2 | 2026-11-27 | 공유 타입 모듈 ID를 M-TYP-01로 정정 | M-TYP-01 |

## 1. 설계 규칙

- **인터페이스 정의 기준:** 컴포넌트 밖에서 호출되는 모듈(함수)은 헤더에 인터페이스를 정의한다. 이터레이션 1의 모듈은 모두 컴포넌트 인터페이스를 가진다. 모듈의 `static` 함수는 같은 파일 안에서만 호출한다.
- 모든 함수는 반환값을 갖고 `return`은 함수 끝에서 한 번만 쓴다. 반환값이 없는 동작은 `eclp_status_t` 또는 판정 결과를 반환한다. (SWR-109, 115)
- 조건식은 단일 조건 또는 중첩 `if`로 쓰고 `&&`, `||`를 쓰지 않는다. 순환복잡도는 10 이하, 함수 길이는 80라인 이하다.
- 동적 메모리, 전역 가변 변수, 시스템 시계를 쓰지 않는다. (SWR-103, 110)
- 이름은 `eclp_` 접두사로 공개 함수를, `<컴포넌트>_` 접두사로 `static` 함수를 구분한다. 한 함수는 한 가지 일만 한다.

## 2. 모듈

| 모듈 | 컴포넌트 | 파일 | 인터페이스(헤더) | 책임 |
|---|---|---|---|---|
| M-VAL-01 | C-VAL | `src/input_validator.c` | `input_validator.h` | 입력 검증, freshness, 시각 변환 |
| M-STA-01 | C-STA | `src/state_manager.c` | `state_manager.h` | 상태와 경고코드 판정 |
| M-ARB-01 | C-ARB | `src/arbiter.c` | `arbiter.h` | 규칙 평가, 출력 결정, 래치 |
| M-CTL-01 | C-CTL | `src/controller.c` | `controller.h` | 평가 진입점, 이유코드 매핑 |
| M-CTL-02 | C-CTL | `src/result_format.c` | `controller.h` | 이름 문자열, 결과 직렬화 |
| M-TYP-01 | 공통 | `src/eclp_types.h` | (헤더 자체) | 공유 데이터 타입과 상수. 코드는 없다. |

공유 데이터 타입은 `src/eclp_types.h`에 있다. 이 헤더는 값 타입과 상수만 정의하며 코드가 없다.

## 3. 상수

| 상수 | 값 | 근거 |
|---|---|---|
| `ECLP_STALE_LIMIT_US` | 200000 | SWR-013, A-23 |
| `ECLP_SPEED_MAX_KPH` | 300.0 | OEM-IF-001, SWR-101 |
| `ECLP_US_PER_S` | 1000000.0 | A-23 |
| `ECLP_TIME_LIMIT_S` | 9.0e12 | `llround` 결과가 `int64_t`에 들어가는 상한 |

## 4. 모듈별 설계

### 4.1 M-VAL-01 (입력 검증기)

| 함수 | 입력 -> 출력 | 로직 | 요구사항 |
|---|---|---|---|
| `eclp_val_time_us` (공개) | `double t_s` -> `int64_t *out_us`, 상태 | 널 포인터이면 `ECLP_ERR_ARG`. 유한하지 않거나 절댓값이 상한을 넘으면 `ECLP_ERR_RANGE`. 아니면 `llround(t_s × 1e6)`를 저장한다. | SWR-013 |
| `eclp_val_validate` (공개) | 스냅샷 -> `eclp_validated_t` | 널 검사. 출력을 기본값(모든 그룹 `INVALID`, 불리언 FALSE, 요청 없음)으로 채운다. `now_s`를 변환할 수 있으면 그룹별 검증 함수를 호출하고, 유효하지 않은 그룹이 아니면 값을 복사한다. | SWR-101, 013 |
| `val_freshness` | `now_us`, `ts_s` -> `eclp_validity_t` | 시각 변환 실패는 `INVALID`. `age_us = now_us − ts_us`가 0 미만이면 `INVALID`, 200000 초과이면 `STALE`, 아니면 `VALID`. | SWR-013, 101 |
| `val_bool_ok` | `int` -> `bool` | `(unsigned)value <= 1U` | SWR-101 |
| `val_speed_ok` | `double` -> `bool` | 유한이고 0.0 이상이고 300.0 이하이면 참. 중첩 `if`로 판정한다. | SWR-101 |
| `val_vehicle` | 차량 입력, `now_us` -> `eclp_validity_t` | 누락이면 `INVALID`. 속도와 기어가 유효하면 `val_freshness`, 아니면 `INVALID`. | SWR-101 |
| `val_crash` | 충돌 입력, `now_us` -> `eclp_validity_t` | 누락이면 `INVALID`. 충돌 값이 `NONE/PENDING/CONFIRMED`이면 `val_freshness`, 아니면 `INVALID`. | SWR-101, 008 |
| `val_approach` | 접근위험 입력, `now_us` -> `eclp_validity_t` | 누락이면 `INVALID`. 좌, 우 값이 boolean이면 `val_freshness`, 아니면 `INVALID`. | SWR-101 |
| `val_fault` | 고장/시동 입력, `now_us` -> `eclp_validity_t` | 누락이면 `INVALID`. `ignition_on`, `sensor_fault`가 boolean이면 `val_freshness`, 아니면 `INVALID`. | SWR-101 |
| `val_request` | 운전자 요청 -> `eclp_req_state_t` | 누락이면 `NONE`. `side`, `action`, `source`가 등록 집합이면 `VALID`, 아니면 `INVALID`. | SWR-106 |

### 4.2 M-STA-01 (상태 관리기)

| 함수 | 입력 -> 출력 | 로직 | 요구사항 |
|---|---|---|---|
| `eclp_sta_initial` (공개) | -> `eclp_state_t` | 널 검사 후 `NORMAL`을 저장한다. | SWR-105 |
| `eclp_sta_evaluate` (공개) | `eclp_validated_t` -> 상태, 경고 | 널 검사. `sta_fault_active`가 참이면 `FAULT`, 아니면 `sta_count(v, 비정상 그룹)`이 0보다 크면 `DEGRADED`, 아니면 `NORMAL`. 경고는 `FAULT`이면 `W_SENSOR_FAULT`, 아니면 `INVALID` 그룹이 있으면 `W_INPUT_INVALID`, `STALE`이 있으면 `W_INPUT_STALE`, 없으면 `NONE`. | SWR-122, 123, 124, 126 |
| `sta_fault_active` | `eclp_validated_t` -> `bool` | 고장 그룹이 `VALID`이고 `sensor_fault`가 참이면 참. 중첩 `if`. | SWR-124 |
| `sta_count` | `eclp_validated_t`, 대상 유효성 -> `int` | 속도, 충돌, 접근위험, 고장 그룹 중 대상 값과 같은 그룹의 수. 반복문으로 센다. | SWR-122, 126 |

`sta_count`는 `VALID`가 아닌 그룹 수를 구할 때 `INVALID` 수와 `STALE` 수를 더해 쓴다.

### 4.3 M-ARB-01 (판정기)

| 함수 | 입력 -> 출력 | 로직 | 요구사항 |
|---|---|---|---|
| `eclp_arb_init` (공개) | -> `eclp_arb_t` | 널 검사. 직전 출력 `RELEASE`, 래치 해제. | A-22 |
| `eclp_arb_decide` (공개) | `eclp_arb_t`, `eclp_validated_t`, 상태 -> `eclp_decision_t` | 널 검사. `arb_next_latch`로 래치를 갱신한다. 결정 구조체를 직전 출력으로 초기화한다. `arb_run_terminal`이 결정하면 끝. 아니면 `arb_layer_driver`, `arb_layer_approach`를 순서대로 적용하고 `arb_select_priority`로 이긴 규칙을 정한다. 결과를 직전 출력으로 저장한다. | SWR-104, 007, 120 |
| `arb_next_latch` | 현재 래치, `eclp_validated_t` -> `bool` | 고장 그룹이 `VALID`이고 `ignition_on`이 FALSE이면 래치를 푼다. 이어서 충돌 그룹이 `VALID`이고 `CONFIRMED`이면 래치를 건다. | SWR-007, 120 |
| `arb_run_terminal` | 컨텍스트, 결정 -> `int` | 종료 규칙 표(`TERMINAL_RULES`)를 순서대로 호출하고 하나가 1을 반환하면 이후 규칙은 호출하지 않는다. | SWR-104 |
| `arb_rule_crash` | 컨텍스트, 결정 -> `int` | 래치가 걸려 있으면 두 출력을 `RELEASE`, 사유 `CRASH`로 하고 1. | SWR-007, 120 |
| `arb_rule_fault` | 컨텍스트, 결정 -> `int` | 상태가 `FAULT`이면 사유 `SENSOR_FAULT`(출력은 직전 값 유지)로 하고 1. | SWR-021 |
| `arb_crash_pending` | 컨텍스트 -> `bool` | 충돌 그룹이 `VALID`이고 `PENDING`이면 참. | SWR-008 |
| `arb_request_outcome` | 컨텍스트 -> `eclp_req_outcome_t` | 요청 없음이면 `NONE`; 상태 `DEGRADED`이면 `HELD_DEGRADED`; 충돌 `PENDING`이면 `HELD_PENDING`; 요청이 `INVALID`이면 `REJECTED`; 아니면 `APPLIED`. | SWR-106, 108, 125, 008 |
| `arb_side_selected` | 요청 `side`, 대상 도어 -> `bool` | `ALL`이거나 대상과 같으면 참. | SWR-108 |
| `arb_layer_driver` | 컨텍스트, 결정 -> `int` | 결과를 구하고 `APPLIED`이면 선택한 도어에 `LOCK`/`RELEASE`를 적용하고 `driver_action`을 저장한다. | SWR-108 |
| `arb_layer_approach` | 컨텍스트, 결정 -> `int` | 접근위험 그룹이 `VALID`이면 좌, 우 위험을 결정에 담고 도어별로 `arb_apply_risk`를 호출한다. | SWR-005, 009 |
| `arb_apply_risk` | 컨텍스트, 결정, 도어, 위험, 출력 -> `int` | 위험이면: 요청이 `APPLIED`, `UNLOCK`이고 이 도어를 선택했으면 `suppressed`를 켠다. 출력을 `LOCK`으로 한다. | SWR-005, 121 |
| `arb_select_priority` | 컨텍스트, 결정 -> `eclp_priority_t` | `suppressed` -> `APPROACH_RISK_SUPPRESS`; 좌/우 위험 -> `APPROACH_RISK`; `DEGRADED` -> `DEGRADED`; 충돌 `PENDING` -> `CRASH_PENDING`; `REJECTED` -> `REJECTED`; `APPLIED` -> `DRIVER`; 그 외 `NONE`. | SWR-104, 107 |

규칙 확장: 이후 이터레이션의 종료 규칙은 `TERMINAL_RULES`에, 계층 규칙은 `arb_layer_*` 호출 순서에 추가한다. 우선순위 표(A-08)의 순서를 표 순서로 유지한다.

### 4.4 M-CTL-01 (제어기)

| 함수 | 입력 -> 출력 | 로직 | 요구사항 |
|---|---|---|---|
| `eclp_init` (공개) | -> `eclp_t` | 널 검사. `eclp_arb_init`를 호출하고 최근 결과를 초기값(두 출력 `RELEASE`, `NORMAL`, 사유 `INIT`, 이유 `INIT`, 경고 `NONE`, 모든 유효성 `VALID`)으로 채운다. 상태는 `eclp_sta_initial`로 얻는다. | SWR-105 |
| `eclp_evaluate` (공개) | `eclp_t`, 스냅샷 -> `eclp_result_t` | 널 검사. 검증, 상태 판정, 결정, 이유코드 매핑 순으로 호출하고 결과를 조립해 저장한다. 내부 호출 상태는 `ctl_merge`로 합친다. | SWR-102, 103, 111, 114 |
| `eclp_priority_to_reason` (공개) | `eclp_priority_t`, `eclp_action_t` -> `eclp_reason_t` | 범위 밖이면 `ECLP_ERR_ARG`. 표(`REASON_FOR_PRIORITY`)로 매핑하고 `DRIVER`이면서 `UNLOCK`이면 `DRIVER_RELEASE`로 바꾼다. | SWR-107 |
| `ctl_merge` | 상태 둘 -> `eclp_status_t` | 비트 OR로 합친다(분기 없음). | SWR-114 |
| `ctl_assemble` | 검증값, 상태, 경고, 결정, 이유 -> `eclp_result_t` | 필드를 복사해 결과를 만든다. | SWR-102, 114 |

### 4.5 M-CTL-02 (결과 형식)

| 함수 | 입력 -> 출력 | 로직 | 요구사항 |
|---|---|---|---|
| `eclp_name_lock/state/reason/warning/validity` (공개) | enum -> `const char *` | 범위 안이면 이름 표의 문자열, 아니면 `"UNKNOWN"`. | SWR-016 |
| `eclp_serialize` (공개) | 결과, 버퍼, 크기 -> 길이 | 널 검사. 부록 B의 순서로 `snprintf`한다. 쓴 길이가 버퍼 크기 이상이면 `ECLP_ERR_RANGE`. | SWR-016 |

## 5. 추적성

모듈과 요구사항의 연결은 소스 함수 주석의 `@requirement` 태그와 `docs/traceability/trace.csv`의 `modules` 열로 유지한다. 모듈 ID는 이 문서 2장의 표에 정의되어 있다.
