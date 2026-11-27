/**
 * @file    arbiter.h
 * @brief   C-ARB 판정기의 인터페이스(IF-ARB).
 * @module  M-ARB-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#ifndef ECLP_ARBITER_H
#define ECLP_ARBITER_H

#include "eclp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   판정기를 초기화한다. 직전 출력은 `RELEASE`, 충돌 래치는 해제 상태다.
 * @param[out] arb  초기화할 판정기 상태.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-021
 * @requirement SWR-105
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_arb_init(eclp_arb_t *arb);

/**
 * @brief   우선순위 규칙을 평가해 좌, 우 출력을 결정한다. 판정기의 내부 상태(직전 출력, 충돌 래치)를 갱신한다.
 * @param[in,out] arb    판정기 상태.
 * @param[in]     v      검증된 입력.
 * @param[in]     state  상태 관리기가 판정한 상태.
 * @param[out]    out    결정. 이긴 규칙(`priority_reason`)을 포함한다.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-104
 * @requirement SWR-007
 * @requirement SWR-120
 * @requirement SWR-008
 * @requirement SWR-021
 * @requirement SWR-125
 * @requirement SWR-005
 * @requirement SWR-121
 * @requirement SWR-009
 * @requirement SWR-106
 * @requirement SWR-108
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_arb_decide(eclp_arb_t *arb, const eclp_validated_t *v, eclp_state_t state,
                              eclp_decision_t *out);

#ifdef __cplusplus
}
#endif

#endif /* ECLP_ARBITER_H */
