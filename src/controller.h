/**
 * @file    controller.h
 * @brief   C-CTL 제어기의 인터페이스(IF-CTL). 이 SW가 외부에 공개하는 유일한 인터페이스다.
 * @module  M-CTL-01
 * @module  M-CTL-02
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#ifndef ECLP_CONTROLLER_H
#define ECLP_CONTROLLER_H

#include "eclp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   제어기를 초기화한다. 두 출력은 `RELEASE`, 상태는 `NORMAL`, 이유코드는 `INIT`이다.
 * @param[out] ctl  초기화할 제어기.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-105
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_init(eclp_t *ctl);

/**
 * @brief   입력 스냅샷 1건으로 평가를 1회 수행한다. 10 ms 주기로 호출한다.
 * @param[in,out] ctl   제어기.
 * @param[in]     snap  입력 스냅샷(시각 `now_s` 포함).
 * @param[out]    out   공개 결과.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-102
 * @requirement SWR-103
 * @requirement SWR-111
 * @requirement SWR-114
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_evaluate(eclp_t *ctl, const eclp_snapshot_t *snap, eclp_result_t *out);

/**
 * @brief   `priority_reason`을 공개 `reason_code`로 매핑한다.
 * @param[in]  priority  이긴 규칙.
 * @param[in]  action    적용된 운전자 요청의 동작. `DRIVER`일 때 `DRIVER_LOCK`/`DRIVER_RELEASE`를 고른다.
 * @param[out] out       이유코드.
 * @return  `ECLP_OK`, 널이거나 범위 밖이면 `ECLP_ERR_ARG`.
 * @requirement SWR-107
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_priority_to_reason(eclp_priority_t priority, eclp_action_t action, eclp_reason_t *out);

/**
 * @brief   결과를 공개 직렬화 한 줄(끝에 `\n`)로 만든다.
 * @param[in]  r    결과.
 * @param[out] buf  출력 버퍼.
 * @param[in]  cap  버퍼 크기(바이트).
 * @param[out] len  쓴 길이(널 종료 제외).
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`, 버퍼가 부족하면 `ECLP_ERR_RANGE`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_serialize(const eclp_result_t *r, char *buf, size_t cap, size_t *len);

/**
 * @brief   잠금 출력의 이름을 반환한다.
 * @param[in] value  잠금 출력.
 * @return  이름. 범위 밖이면 `"UNKNOWN"`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
const char *eclp_name_lock(eclp_lock_t value);

/**
 * @brief   상태의 이름을 반환한다.
 * @param[in] value  상태.
 * @return  이름. 범위 밖이면 `"UNKNOWN"`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
const char *eclp_name_state(eclp_state_t value);

/**
 * @brief   이유코드의 이름을 반환한다.
 * @param[in] value  이유코드.
 * @return  이름. 범위 밖이면 `"UNKNOWN"`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
const char *eclp_name_reason(eclp_reason_t value);

/**
 * @brief   경고코드의 이름을 반환한다.
 * @param[in] value  경고코드.
 * @return  이름. 범위 밖이면 `"UNKNOWN"`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
const char *eclp_name_warning(eclp_warning_t value);

/**
 * @brief   입력 유효성의 이름을 반환한다.
 * @param[in] value  입력 유효성.
 * @return  이름. 범위 밖이면 `"UNKNOWN"`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
const char *eclp_name_validity(eclp_validity_t value);

#ifdef __cplusplus
}
#endif

#endif /* ECLP_CONTROLLER_H */
