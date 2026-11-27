---
name: implementation
description: 상세 설계를 코드로 구현하는 스킬(A-SPICE SWE.3 구현). 함수 반환 규칙(항상 반환값, 단일 return)을 지키며 C/C++ 코드를 작성하거나 수정할 때 사용한다.
---

# 구현

상세 설계서를 기준으로 구현한다. 설계에 없는 동작을 임의로 추가하지 않는다. 설계가 잘못되었거나 부족하면 코드를 먼저 고치지 말고 설계를 갱신한 뒤 구현한다.

## 함수 반환 규칙 (필수)

1. **모든 함수는 반드시 반환값을 가진다.**
2. **`return`은 함수 마지막에 단 한 번만** 호출한다. 중간 return, 조기 탈출은 금지한다. (분기는 결과 변수를 설정하고 끝에서 반환)
3. 의미 있는 반환값이 없으면 `void` 대신 **정상 실행 여부를 판단할 수 있는 값**(상태 코드, `bool`, 열거형 등)을 반환한다.
4. 호출자는 반환값을 확인하고 처리한다.

```c
/* 나쁜 예 */
void set_speed(int v) {
    if (v < 0) return;      /* 중간 return, void */
    g_speed = v;
}

/* 좋은 예 */
Status_t set_speed(int v) {
    Status_t result = STATUS_OK;
    if (v < 0) {
        result = STATUS_INVALID_ARG;
    } else {
        g_speed = v;
    }
    return result;           /* 유일한 return */
}
```

## 그 외
- 상세 설계의 인터페이스 시그니처를 그대로 구현한다. 인터페이스가 없는 내부 함수는 `static`으로 숨기고, 다른 모듈의 내부에 직접 접근하지 않는다.
- 클린코드 원칙과 단일 책임 원칙을 지킨다.
- 소스 파일/함수 머리에 설계 모듈 ID와 요구사항 ID를 Doxygen 주석으로 남겨 추적성을 유지한다.
- 구현 후 `unit-test` 스킬로 단위 검증하고, 코드는 `review` 스킬의 코드 체크리스트로 리뷰한다.
