---
name: review
description: A-SPICE 기준 산출물 리뷰. 요구사항, 아키텍처, 상세 설계, 소스 코드, 단위/통합 테스트 산출물을 산출물별 체크리스트로 검토할 때 사용한다. 모든 산출물은 이 스킬로 리뷰해야 한다.
---

# 산출물 리뷰

**모든 산출물은 이 스킬로 리뷰한다.** 리뷰는 산출물 유형별 체크리스트를 기준으로 하며, 체크리스트는 A-SPICE 요구(일관성, 완전성, 추적성, 검증 가능성)를 반영한다.

## 체크리스트

| 산출물 | 체크리스트 | A-SPICE 프로세스 |
|---|---|---|
| 요구사항 명세 | [checklists/requirements.md](checklists/requirements.md) | SWE.1 |
| 아키텍처 설계 | [checklists/architecture.md](checklists/architecture.md) | SWE.2 |
| 상세 설계 | [checklists/detailed-design.md](checklists/detailed-design.md) | SWE.3 |
| 소스 코드 | [checklists/code.md](checklists/code.md) | SWE.3 |
| 단위 테스트 | [checklists/unit-test.md](checklists/unit-test.md) | SWE.4 |
| 통합 테스트 | [checklists/integration-test.md](checklists/integration-test.md) | SWE.5 |

체크리스트가 없는 산출물 유형이면, 먼저 같은 형식으로 체크리스트를 개발해 `checklists/`에 추가한 뒤 리뷰한다.

## 절차
1. 대상 산출물과 해당 체크리스트를 정한다.
2. 체크리스트 항목을 하나씩 대조해 판정한다: **Pass / Fail / N/A** (N/A는 사유 기재).
3. Fail 항목마다 위치(파일/ID/줄), 문제, 근거 규칙, 수정 제안을 적는다.
4. 결과를 아래 형식으로 기록한다.
5. 작성자가 수정한 뒤 Fail 항목을 재검토한다. 모든 Fail이 해소될 때까지 반복한다.

## 결과 형식

```
리뷰 대상: <산출물 이름, 버전>
체크리스트: <파일>
요약: Pass N / Fail N / N/A N  -> 판정: 승인 | 조건부 승인 | 반려

| 항목 ID | 판정 | 위치 | 지적 사항 | 수정 제안 |
```

리뷰는 증거에 근거한다. 확인하지 않은 항목을 Pass로 처리하지 않는다.
