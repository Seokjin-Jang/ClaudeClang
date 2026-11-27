# 소프트웨어 아키텍처 설계 (이터레이션 1)

- 문서 ID: SWA-001, 베이스라인 BL-SWA-0.1
- 입력: `docs/requirements/*.md` (BL-SWR-0.6), `docs/traceability/trace.csv`
- 대상: 전자식 차일드락 제어 SW의 안전 코어 (판단 로직, C17, 정적 메모리)
- 이 이터레이션은 사용자 지시로 `reviewer` 리뷰를 생략했다. (`docs/reviews/iteration-1-review-record.md`)

## 변경 이력

| 버전 | 날짜 | 변경 내용 | 영향 ID |
|---|---|---|---|
| BL-SWA-0.1 | 2026-11-27 | 초판 작성 | C-VAL, C-ARB, C-STA, C-CTL |

## 1. 설계 원칙

| 원칙 | 적용 방법 |
|---|---|
| 인터페이스를 통한 통신 | 컴포넌트는 공개 헤더(`input_validator.h`, `state_manager.h`, `arbiter.h`, `controller.h`)에 선언된 함수로만 서로 호출한다. 내부 함수와 내부 자료는 `static`이며 다른 컴포넌트가 접근하지 않는다. |
| 높은 응집도 | 컴포넌트 하나가 한 가지 일을 한다: 검증(C-VAL), 상태 판정(C-STA), 출력 결정(C-ARB), 조립과 공개(C-CTL). |
| 낮은 결합도 | 컴포넌트 사이의 데이터는 `eclp_types.h`의 값 구조체(`eclp_validated_t`, `eclp_decision_t`)로만 주고받고, 의존은 C-CTL에서 나머지로 향하는 한 방향이다. 순환 의존이 없다. |
| 모듈화, 캡슐화 | 컴포넌트마다 헤더 1개와 구현 파일 1개를 두며, 상태를 가진 컴포넌트(C-ARB)의 내부 상태는 호출자가 정의하는 구조체(`eclp_arb_t`)에 담아 전역 변수를 쓰지 않는다. |
| SOLID: S | 컴포넌트마다 변경 이유가 하나다(입력 규격 변경은 C-VAL, 상태 규칙은 C-STA, 우선순위 규칙은 C-ARB, 출력 형식은 C-CTL). |
| SOLID: O | C-ARB는 규칙 함수 표(`TERMINAL_RULES`, 하위→상위 계층 규칙)를 순서대로 평가한다. 이후 이터레이션의 규칙(화재, ISOFIX, 자동 잠금, 시동 OFF)은 표에 항목을 추가하며 기존 규칙의 코드를 바꾸지 않는다. (SWR-104) |
| SOLID: L, I, D | 규칙은 같은 함수 시그니처(`arb_rule_fn`)를 지킨다(L). 공개 인터페이스는 컴포넌트별로 작게 나눈다(I). C-CTL은 구현 세부가 아니라 각 컴포넌트의 인터페이스에만 의존한다(D). |
| 결정론, 정적 메모리 | 시스템 시계를 읽지 않고 호출자가 시각을 주입한다(SWR-103). 동적 메모리를 쓰지 않는다(SWR-110). |

## 2. 컴포넌트

| ID | 이름 | 책임 | 할당 요구사항 |
|---|---|---|---|
| C-VAL | 입력 검증기 | 입력 스냅샷의 그룹별 형식, 범위, enum 검증과 freshness 판정, 정규화된 입력(`eclp_validated_t`) 생성 | SWR-013, SWR-101 |
| C-STA | 상태 관리기 | 입력 유효성과 `sensor_fault`로 상태(`NORMAL`/`DEGRADED`/`FAULT`)와 경고코드를 판정, 초기 상태 제공 | SWR-105, 122, 123, 124, 126 |
| C-ARB | 판정기 | 우선순위 규칙 평가로 좌, 우 출력과 이긴 규칙(`priority_reason`) 결정, 충돌 래치와 직전 출력 보관, 운전자 요청 처리 | SWR-005, 007, 008, 009, 021, 104, 106, 108, 120, 121, 125 |
| C-CTL | 제어기 | 평가 진입점, 컴포넌트 호출 순서 제어, `priority_reason`->`reason_code` 매핑, 결과 조립과 직렬화, 이름 문자열 | SWR-016, 102, 107, 111, 103, 114 |

제약사항 SWR-109, 110, 112, 115, 116, 117은 네 컴포넌트 모두에 적용된다.

## 3. 인터페이스

모든 인터페이스 함수는 `eclp_status_t`를 반환한다. 오류 상태: `ECLP_OK`(0), `ECLP_ERR_ARG`(널 포인터 등 인자 오류), `ECLP_ERR_RANGE`(값 범위 오류). 출력 인자는 `ECLP_OK`일 때만 유효하다.

### IF-VAL (`input_validator.h`)

| 함수 | 입력 | 출력 | 오류 | 설명 |
|---|---|---|---|---|
| `eclp_val_time_us(t_s, out_us)` | `double t_s`: 초, 유한값, 절댓값 9.0e12 이하 | `int64_t *out_us`: `llround(t_s×1e6)` | NaN/무한대/범위 초과는 `ECLP_ERR_RANGE`, 널은 `ECLP_ERR_ARG` | A-23의 마이크로초 변환 |
| `eclp_val_validate(snap, out)` | `const eclp_snapshot_t *snap` | `eclp_validated_t *out`: 그룹별 `eclp_validity_t`(`VALID`/`STALE`/`INVALID`)와 정규화된 값 | 널은 `ECLP_ERR_ARG`. `now_s`가 변환 불가이면 모든 그룹을 `INVALID`로 하고 `ECLP_OK` | 호출 순서: 평가마다 1번째 |

### IF-STA (`state_manager.h`)

| 함수 | 입력 | 출력 | 오류 | 설명 |
|---|---|---|---|---|
| `eclp_sta_initial(state)` | - | `eclp_state_t *state` = `NORMAL` | 널은 `ECLP_ERR_ARG` | SWR-105 |
| `eclp_sta_evaluate(v, state, warning)` | `const eclp_validated_t *v` | `eclp_state_t *state`, `eclp_warning_t *warning` | 널은 `ECLP_ERR_ARG` | 호출 순서: 평가마다 2번째. `FAULT` > `DEGRADED` > `NORMAL`(A-18), 경고코드 우선순위(A-21) |

### IF-ARB (`arbiter.h`)

| 함수 | 입력 | 출력 | 오류 | 설명 |
|---|---|---|---|---|
| `eclp_arb_init(arb)` | - | `eclp_arb_t *arb`: 직전 출력 `RELEASE`, 충돌 래치 해제 | 널은 `ECLP_ERR_ARG` | A-22 |
| `eclp_arb_decide(arb, v, state, out)` | `eclp_arb_t *arb`(내부 상태 갱신), `const eclp_validated_t *v`, `eclp_state_t state` | `eclp_decision_t *out`: `lock_left`, `lock_right`, `priority_reason`, `driver_action` | 널은 `ECLP_ERR_ARG` | 호출 순서: 평가마다 3번째 |

### IF-CTL (`controller.h`, 외부에 공개되는 유일한 인터페이스)

| 함수 | 입력 | 출력 | 오류 | 설명 |
|---|---|---|---|---|
| `eclp_init(ctl)` | - | `eclp_t *ctl` 초기화 | 널은 `ECLP_ERR_ARG` | 기동 시 1회 |
| `eclp_evaluate(ctl, snap, out)` | `eclp_t *ctl`, `const eclp_snapshot_t *snap`(`now_s`, 입력 그룹 4개, 운전자 요청) | `eclp_result_t *out`: 출력 2개, 상태, 이유, 경고, 입력 유효성 | 널은 `ECLP_ERR_ARG` | 10 ms 주기 호출, 호출당 스냅샷 1건 (SWR-111) |
| `eclp_priority_to_reason(p, action, out)` | `eclp_priority_t p`, `eclp_action_t action` | `eclp_reason_t *out` | 범위 밖 `p`는 `ECLP_ERR_ARG` | SWR-107 |
| `eclp_serialize(r, buf, cap, len)` | `const eclp_result_t *r`, 버퍼와 크기 | 부록 B 문자열, `size_t *len` | 널은 `ECLP_ERR_ARG`, 버퍼 부족은 `ECLP_ERR_RANGE` | SWR-016 |
| `eclp_name_lock/state/reason/warning/validity(v)` | enum 값 | `const char *`: 정의된 이름, 범위 밖이면 `"UNKNOWN"` | - | 직렬화와 표시용 |

## 4. 정적 구조와 동적 동작

- 정적 구조: `docs/uml/components.puml` (컴포넌트와 인터페이스, 의존 방향)
- 평가 호출 순서: `docs/uml/evaluate_sequence.puml`
- 상태 전이: `docs/uml/state.puml`

평가 한 번의 흐름(10 ms 주기): C-CTL이 입력을 받아 C-VAL로 검증하고(freshness 포함), C-STA로 상태와 경고를 구하고, C-ARB로 출력을 결정한 뒤, 이유코드를 매핑해 결과를 반환한다. 시각은 호출자가 주입한다.

C-ARB의 규칙 순서(A-08): 종료 규칙(충돌 래치 ①, `sensor_fault` 보류 ②)이 먼저 평가되고 하나라도 결정하면 끝난다. 종료되지 않으면 하위 계층부터 상위 계층 순으로 적용한다: 운전자 요청 ⑧(DEGRADED, 충돌 PENDING이면 보류) -> 접근위험 ④(좌/우 독립, 해제 요청 억제). 이후 이터레이션의 규칙은 같은 표에 추가한다.

## 5. 설계 결정과 근거

| 결정 | 근거 |
|---|---|
| 상태 `NORMAL`/`DEGRADED`/`FAULT`를 입력에서 매번 새로 계산한다(상태 비저장) | A-10의 즉시 복귀 요구. 히스테리시스가 없으므로 상태를 저장할 필요가 없고 결정론 검증이 쉽다. |
| 상태를 가진 것은 C-ARB의 직전 출력과 충돌 래치뿐이다 | 보류(SWR-021, 125)와 래치(SWR-120)가 이전 값에 의존한다. 상태가 한 곳에 모여 있다. |
| 조건식을 중첩 `if`와 단일 조건으로 쓴다 | MC/DC 100%를 시험으로 입증하기 쉽고 순환복잡도 10 이하를 지킨다. |
| 내부 호출의 반환 상태는 값 오류가 아니라 인자 오류만 낸다 | C-CTL이 유효한 포인터만 넘기므로 내부 호출은 실패하지 않는다. 오류 상태는 병합(`ctl_merge`)해 상위로 돌려준다. |

## 6. 추적성

요구사항과 컴포넌트의 연결은 `docs/traceability/trace.csv`의 `components` 열이 정본이며, 이 문서 2장의 표와 일치한다. 컴포넌트 ID는 `docs/traceability/README.md`에 정의되어 있다.
