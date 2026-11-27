---
name: unit-test
description: A-SPICE SWE.4 기준 단위 테스트 작성 및 커버리지 달성. 테스트 케이스 설계(경계값, 동치 클래스, PICT), Doxygen 테스트 주석, LLVM 기반 MC/DC 커버리지 확인이 필요할 때 사용한다.
---

# 단위 테스트 (A-SPICE SWE.4)

## 원칙
1. **정해진 커버리지 목표를 100% 달성할 때까지 반복**해서 테스트 케이스를 추가한다. (측정 -> 미커버 분기 분석 -> 케이스 추가 -> 재측정)
2. Stub/Mock은 필요할 때만 쓰고 **과도하게 의존하지 않는다.** 가능하면 실제 모듈을 사용한다.
3. **커버리지를 올리기 위한 무의미한 테스트는 금지**한다. (단언 없음, 결과를 검증하지 않는 호출 등) 미커버 코드는 먼저 "필요한 코드인가, 도달 가능한 코드인가"를 확인하고, 불필요하면 코드를 제거하거나 설계를 수정한다.
4. 테스트 케이스는 **상세 설계와 요구사항에서 도출**한다. 구현 코드를 보고 역으로 맞추지 않는다.

## 테스트 설계 기법
- **경계값 분석**: 최솟값, 최댓값, 각 경계의 바로 안/밖
- **동치 클래스 분할**: 유효/무효 클래스별 대표값 1개 이상
- **PICT 조합**: 파라미터가 여러 개일 때 pairwise 조합 생성. 모델 파일 예시는 [pict-example.md](pict-example.md) 참고.

## Doxygen 주석 (모든 테스트 함수 필수)
테스트 목적, 입력값, 절차, 예상 출력값을 반드시 적는다.

```c
/**
 * @test      TC_SET_SPEED_001
 * @brief     [목적] 허용 범위 하한 경계값에서 set_speed가 성공하는지 확인
 * @requirement SWR-012
 * @design    MOD_SPEED_CTRL
 * @input     v = 0
 * @procedure 1. g_speed를 초기화한다.
 *            2. set_speed(0)을 호출한다.
 *            3. 반환값과 g_speed를 확인한다.
 * @expected  반환값 == STATUS_OK, g_speed == 0
 */
void test_set_speed_lower_bound(void) { ... }
```

## 커버리지
- 기본 목표는 프로젝트 정의에 따른다. (구문 / 분기 100% 등)
- **MC/DC 목표가 수립된 경우 LLVM 도구를 사용**해 케이스를 추가한다. 스킬에 포함된 스크립트를 쓴다.

```bash
bash .claude/skills/unit-test/scripts/mcdc.sh <소스.c> <테스트.c> [출력디렉터리]
```

스크립트는 `clang -fprofile-instr-generate -fcoverage-mapping -fcoverage-mcdc`로 빌드·실행하고 `llvm-profdata`, `llvm-cov`로 MC/DC 결과를 출력한다. LLVM 18 이상이 필요하다. 보고서에서 독립성이 입증되지 않은 조건을 찾아, 해당 조건만 바꾸어 결과가 달라지는 케이스를 추가하고 다시 실행한다.

## 추적성
테스트 케이스 <-> 상세 설계 모듈 <-> 요구사항을 양방향 연결하고, 결과(통과/실패, 커버리지)를 기록한다. 완료 후 `review` 스킬로 단위 테스트 체크리스트 리뷰.
