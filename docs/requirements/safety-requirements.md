# 소프트웨어 안전 요구사항 (ASIL B)

기능 안전 요구사항은 일반 SW 요구사항 명세서(`software-requirements.md`)와 분리해 이 문서에서 관리한다.
OEM이 ASIL B로 할당한 입력(OEM-SR-001~004)과, 이를 구현하는 입력 검증·결과·판정 요구(SWR-101, 102, 104)를 ASIL B로 관리한다. HARA와 ASIL 도출은 범위 밖이다.

- 문서 ID: SSR-001, 베이스라인 BL-SWR-0.7 (이터레이션 1)
- 입력: OEM-SWR-001 (BL-OEM-1.0), `assumptions.md`
- 시간 기준: 평가 주기 10 ms (A-01). "같은 평가주기"는 해당 입력이 주입된 평가 호출을 말한다.
- 이 문서의 요구사항 ID: SWR-005, 007, 008, 009, 013, 021, 101, 102, 104, 114, 120, 121, 122, 123, 124, 125, 126
- 같은 SW 요소의 QM 요구사항도 ASIL B 수준의 개발 기준을 적용한다. (A-17)

## 변경 이력

| 버전 | 날짜 | 변경 내용 | 영향 ID |
|---|---|---|---|
| BL-SWR-0.1 | 2026-11-27 | 초판 작성 | SWR-005, 007, 008, 009, 013, 021 |
| BL-SWR-0.2 | 2026-11-27 | 리뷰 반려 조치: 복합 요구 분할(SWR-120~126 신설), SWR-114 이동(ASIL B), 우선순위 열 추가, 수용기준 수치·경계 명확화, 출처에 가정 ID 기재 | SWR-005, 007, 008, 013, 021, 114, 120~126 |
| BL-SWR-0.3 | 2026-11-27 | 재리뷰 반려 조치: SWR-101, 102, 104를 SRS에서 이동(ASIL B 분리), DEGRADED에서도 유효한 접근위험 LOCK 적용(SWR-005, 125), 입력 나이를 마이크로초 정수 비교로 변경(SWR-013, 101, 122), SWR-008 조건 명확화, SWR-125 출처에 A-12 추가 | SWR-005, 008, 013, 101, 102, 104, 122, 125 |
| BL-SWR-0.4 | 2026-11-27 | 베이스라인 정렬(요구사항 내용 변경 없음, SRS의 SWR-112 분할 반영) | 없음 |
| BL-SWR-0.5 | 2026-11-27 | 베이스라인 정렬(요구사항 내용 변경 없음, SRS 부록 A, C 추가 반영) | 없음 |
| BL-SWR-0.6 | 2026-11-27 | 베이스라인 정렬(요구사항 내용 변경 없음) | 없음 |
| BL-SWR-0.7 | 2026-11-27 | 베이스라인 정렬(요구사항 내용 변경 없음) | 없음 |

## 요구사항

| ID | 출처 | ASIL | 우선순위 | 요구사항 | 수용기준 | 검증 |
|---|---|---|---|---|---|---|
| SWR-007 | OEM-SR-001, A-08 | B | 필수 | SW는 충돌상태가 `CONFIRMED`인 평가주기에 후석 좌, 우 출력을 모두 `RELEASE`로 설정해야 한다. | 유효한 `CONFIRMED` 입력이 주입된 같은 평가주기의 두 출력이 `RELEASE`다. (OEM 상한 300 ms를 만족한다.) 이유코드는 `CRASH_RELEASE`다. `sensor_fault=TRUE`, 접근위험 TRUE, LOCK 요청이 동시에 있어도 `RELEASE`다. | 단위, 통합, 시스템 |
| SWR-120 | OEM-SR-001, A-04 | B | 필수 | SW는 충돌 해제 후 `ignition_on`이 FALSE가 될 때까지 두 출력을 `RELEASE`로 유지해야 한다. | `CONFIRMED` 이후 충돌 입력이 `NONE`, `PENDING`, `INVALID`, `STALE`로 바뀌거나, LOCK 요청, 접근위험 TRUE가 들어와도 출력은 `RELEASE`이고 이유코드는 `CRASH_RELEASE`다. `ignition_on=FALSE`인 평가주기부터 래치가 풀리고 이후 규칙이 정상 평가된다. | 단위, 통합, 시스템 |
| SWR-008 | OEM-SR-001, OEM-IF-002, A-05 | B | 필수 | SW는 충돌상태 `PENDING`을 이유로 출력을 변경하지 않아야 하며 같은 평가주기의 운전자 요청을 보류해야 한다. | 접근위험 FALSE, `sensor_fault` FALSE, 필수 입력 신선·유효, 충돌 래치 없음인 상태에서 충돌 `PENDING`을 주입하면 두 출력이 직전 평가주기 출력과 같고, 같은 주기의 운전자 요청은 적용되지 않으며 이유코드는 `CRASH_PENDING_HOLD`다. | 단위, 통합, 시스템 |
| SWR-005 | OEM-SR-002, OEM-IF-003, A-06, A-08 | B | 필수 | SW는 유효하고 신선한 접근위험이 TRUE인 도어의 출력을 `LOCK`으로 설정해야 한다. | 접근위험 TRUE인 도어의 출력이 같은 평가주기에 `LOCK`이고 이유코드는 `APPROACH_RISK_LOCK`이다. `DEGRADED` 상태(다른 입력이 STALE)에서도 접근위험 입력이 유효하고 신선하면 적용된다. 충돌 `CONFIRMED` 래치(SWR-007, 120) 중이거나 `sensor_fault=TRUE`(SWR-021)이면 적용하지 않는다. 접근위험 입력 자체가 `STALE`/`INVALID`이면 적용하지 않는다(SWR-125). | 단위, 통합, 시스템 |
| SWR-121 | OEM-SR-002, A-08 | B | 필수 | SW는 접근위험이 TRUE인 도어에 대한 `RELEASE` 요청을 억제하고 원인을 기록해야 한다. | 접근위험 TRUE인 도어에 `RELEASE` 요청이 들어와도 그 도어의 출력은 `LOCK`을 유지하고 이유코드는 `RELEASE_SUPPRESSED`다. 같은 요청의 반대쪽 도어는 영향받지 않는다. | 단위, 통합, 시스템 |
| SWR-009 | OEM-SR-002 | B | 필수 | SW는 좌, 우 도어를 독립적으로 판정해야 한다. | 한쪽 도어에만 접근위험이 TRUE이면 그 도어의 출력만 `LOCK`으로 강제되고 반대쪽 출력은 직전 값 또는 다른 규칙의 결과를 따른다. | 단위, 통합, 시스템 |
| SWR-013 | OEM-SR-003, A-02, A-03, A-23 | B | 필수 | SW는 필수 입력(A-03)의 나이를 `age_us = us(now_s) − us(source_timestamp_s)`(A-23의 마이크로초 정수 변환)로 계산하고 `age_us > 200000`이면 해당 입력을 `STALE`로 판정해야 한다. | `age_us = 200000`은 신선하고 `age_us = 200001`은 `STALE`이다. `age_us < 0`은 `INVALID`다(SWR-101). 판정은 입력 그룹마다 독립이다. | 단위, 통합, 시스템 |
| SWR-122 | OEM-SR-003, A-06, A-18 | B | 필수 | SW는 필수 입력 중 `STALE` 또는 `INVALID`가 있으면 상태를 `DEGRADED`로 전환해야 한다. | `STALE`이 처음 판정된 평가주기에 상태가 `DEGRADED`다. (OEM 기준 "200 ms 초과 후 100 ms 이내"를 만족한다.) `INVALID`인 필수 입력이 있어도 같은 평가주기에 `DEGRADED`다. `sensor_fault=TRUE`이면 상태는 `FAULT`가 우선한다(A-18). | 단위, 통합, 시스템 |
| SWR-125 | OEM-SR-003, A-06, A-12 | B | 필수 | SW는 `DEGRADED` 상태에서 새 운전자 요청을 적용하지 않고, `STALE`/`INVALID` 입력에 의존하는 변화를 적용하지 않아 직전 출력을 유지해야 한다. | `DEGRADED` 평가주기에 운전자 요청은 적용되지 않고 출력은 직전 평가주기 출력과 같으며 이유코드는 `INPUT_DEGRADED_HOLD`다. 단 유효하고 신선한 접근위험 TRUE는 SWR-005에 따라 LOCK이 적용된다. 접근위험 입력이 `STALE`/`INVALID`이면 출력을 해제하지 않는다(A-12). 충돌 `CONFIRMED` 래치는 우선한다. | 단위, 통합, 시스템 |
| SWR-123 | OEM-SR-003, A-10 | B | 필수 | SW는 모든 필수 입력이 유효하고 신선하며 `sensor_fault=FALSE`가 된 첫 평가주기에 상태를 `NORMAL`로 복귀시켜야 한다. | 복귀 조건을 만족한 첫 평가주기에 상태가 `NORMAL`이고 그 주기부터 새 요청이 다시 적용된다. 조건이 하나라도 미충족이면 복귀하지 않는다. | 단위, 통합, 시스템 |
| SWR-021 | OEM-SR-004, OEM-IF-009, A-07, A-22 | B | 필수 | SW는 `sensor_fault`가 TRUE이면 새 운전자 요청과 접근위험 변화를 적용하지 않고 직전 확정 출력(직전 평가주기에 공개된 출력)을 유지해야 한다. | `sensor_fault=TRUE`인 첫 평가주기에 두 출력이 직전 평가주기 출력과 같고 이유코드는 `SENSOR_FAULT_HOLD`다. 같은 주기의 유효한 충돌 `CONFIRMED`는 출력을 `RELEASE`로 만든다(SWR-007). | 단위, 통합, 시스템 |
| SWR-124 | OEM-SR-004, A-07, A-18 | B | 필수 | SW는 `sensor_fault`가 TRUE이면 상태를 `FAULT`로 전환해야 한다. | `sensor_fault=TRUE`인 첫 평가주기에 상태가 `FAULT`다. 입력 STALE이 함께 있어도 `FAULT`다. | 단위, 통합, 시스템 |
| SWR-126 | OEM-SR-003, OEM-SR-004, A-21 | B | 필수 | SW는 `DEGRADED` 또는 `FAULT` 상태에서 경고코드를 제공해야 한다. | `sensor_fault=TRUE`이면 `W_SENSOR_FAULT`, 필수 입력 `INVALID`이면 `W_INPUT_INVALID`, `STALE`이면 `W_INPUT_STALE`다. 조건이 여럿이면 `W_SENSOR_FAULT` > `W_INPUT_INVALID` > `W_INPUT_STALE` 순으로 하나만 낸다. 정상 상태이면 `NONE`이다. | 단위, 통합, 시스템 |
| SWR-101 | OEM-IF-001, OEM-IF-002, OEM-IF-003, OEM-IF-009, A-11, A-23 | B | 필수 | SW는 입력 그룹(차량 상태, 충돌, 접근위험, 고장/시동)이 형식, 범위, enum 오류이거나 누락이면 해당 그룹을 `INVALID`로 표시하고 사용하지 않아야 한다. | 속도가 0.0 미만 또는 300.0 초과이거나 NaN/무한대, 기어가 `P/N/D/R` 이외, 충돌이 `NONE/PENDING/CONFIRMED` 이외, 접근위험·`ignition_on`·`sensor_fault`가 boolean 이외, 입력 누락, `age_us < 0`(미래 시각)이면 해당 그룹의 `input_validity`가 `INVALID`가 되고 그 값은 판정에 쓰이지 않는다. 경계값 0.0과 300.0은 유효하다. | 단위, 통합, 시스템 |
| SWR-102 | OEM-IF-005, OEM-IF-006 | B | 필수 | SW는 평가 결과로 `lock_left`, `lock_right`(`LOCK`/`RELEASE`), `state`, `priority_reason`, `reason_code`, `warning_code`, 입력 그룹별 `input_validity`를 제공해야 한다. | 평가 호출 한 번이 위 필드를 모두 채운 결과를 반환한다. 값은 SRS 부록 A의 집합 안에 있다. 검증 범위는 논리 출력까지이며 적용 피드백은 범위 밖이다(OEM-IF-005). | 단위, 통합, 시스템 |
| SWR-104 | A-08 | B | 필수 | SW는 규칙 충돌 시 A-08의 우선순위에 따라 상위 규칙의 결과를 채택해야 한다. | 이터레이션 1 규칙(① 충돌 해제, ② `sensor_fault` 보류, ④ 접근위험 LOCK, ⑧ 운전자 요청과 그 적용 조건인 DEGRADED 보류)의 모든 쌍에서 상위 규칙의 결과가 출력이 된다. 이후 이터레이션에서 규칙을 추가해도 이 쌍의 결과는 변하지 않는다(회귀 시험으로 확인한다). | 단위, 통합, 시스템 |
| SWR-114 | OEM-SR-003, OEM-SR-004 | B | 필수 | SW는 어떤 입력 조합에서도 두 출력을 정의된 값(`LOCK` 또는 `RELEASE`)으로 제공해야 한다. | 유효, STALE, INVALID, 누락, 범위 밖 값을 포함한 입력 조합 어디에서도 `lock_left`, `lock_right`가 `LOCK` 또는 `RELEASE`이고 결과 구조의 모든 필드가 채워진다. (ISO 25010 신뢰성 > 결함 허용성) | 단위, 시스템 |

## 안전 요구사항 간 관계
- 우선순위: SWR-007/120 > SWR-021/124(sensor_fault 보류) > SWR-005/121(접근위험) > 운전자 요청(DEGRADED이면 보류, SWR-125). (A-08)
- 충돌 래치(SWR-120)는 DEGRADED/FAULT 보류보다 우선한다. (A-06, A-07)
- 접근위험 LOCK은 DEGRADED에서도 적용되지만 `sensor_fault` 보류에서는 적용되지 않는다. 후자는 OEM-SR-004 수용기준(출력 유지)을 그대로 따른 것이며 OEM 확인이 필요하다. (A-07)
