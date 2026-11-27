/**
 * @file    state_manager.h
 * @brief   C-STA 상태 관리기의 인터페이스(IF-STA).
 * @module  M-STA-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#ifndef ECLP_STATE_MANAGER_H
#define ECLP_STATE_MANAGER_H

#include "eclp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   기동 직후의 초기 상태를 제공한다.
 * @param[out] state  초기 상태(`NORMAL`).
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-105
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_sta_initial(eclp_state_t *state);

/**
 * @brief   검증된 입력으로 상태와 경고코드를 판정한다. (`FAULT` > `DEGRADED` > `NORMAL`)
 * @param[in]  v        검증된 입력.
 * @param[out] state    판정한 상태.
 * @param[out] warning  경고코드. `W_SENSOR_FAULT` > `W_INPUT_INVALID` > `W_INPUT_STALE` 순으로 하나만 낸다.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-122
 * @requirement SWR-123
 * @requirement SWR-124
 * @requirement SWR-126
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_sta_evaluate(const eclp_validated_t *v, eclp_state_t *state, eclp_warning_t *warning);

#ifdef __cplusplus
}
#endif

#endif /* ECLP_STATE_MANAGER_H */
