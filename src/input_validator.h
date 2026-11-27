/**
 * @file    input_validator.h
 * @brief   C-VAL 입력 검증기의 인터페이스(IF-VAL).
 * @module  M-VAL-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#ifndef ECLP_INPUT_VALIDATOR_H
#define ECLP_INPUT_VALIDATOR_H

#include "eclp_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   초 단위 시각을 마이크로초 정수로 바꾼다. (A-23)
 * @param[in]  t_s     시각(초). 유한값이고 절댓값이 `ECLP_TIME_LIMIT_S` 이하여야 한다.
 * @param[out] out_us  `llround(t_s × 1,000,000)`. 반환이 `ECLP_OK`일 때만 유효하다.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`, 유한하지 않거나 범위를 넘으면 `ECLP_ERR_RANGE`.
 * @requirement SWR-013
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_val_time_us(double t_s, int64_t *out_us);

/**
 * @brief   입력 스냅샷을 그룹별로 검증하고 정규화한다.
 * @param[in]  snap  입력 스냅샷.
 * @param[out] out   검증된 입력. `now_s`를 변환할 수 없으면 모든 그룹이 `INVALID`다.
 * @return  `ECLP_OK`, 널이면 `ECLP_ERR_ARG`.
 * @requirement SWR-101
 * @requirement SWR-013
 * @requirement SWR-106
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
eclp_status_t eclp_val_validate(const eclp_snapshot_t *snap, eclp_validated_t *out);

#ifdef __cplusplus
}
#endif

#endif /* ECLP_INPUT_VALIDATOR_H */
