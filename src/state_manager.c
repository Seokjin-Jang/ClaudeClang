/**
 * @file    state_manager.c
 * @brief   C-STA 상태 관리기 구현. 상태는 입력에서 매번 새로 계산하며 저장하지 않는다. (A-10)
 * @module  M-STA-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include "state_manager.h"

/** 상태 판정에 쓰는 필수 입력 그룹의 수. (A-03) */
#define STA_GROUP_COUNT (4U)

/**
 * @brief   고장 그룹이 유효하고 `sensor_fault`가 TRUE인지 확인한다.
 * @param[in] v  검증된 입력.
 * @return  `FAULT` 조건이면 true.
 * @requirement SWR-124
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static bool sta_fault_active(const eclp_validated_t *v)
{
    bool active = false;

    if (v->fault == ECLP_VALID) {
        if (v->sensor_fault) {
            active = true;
        }
    }
    return active;
}

/**
 * @brief   필수 입력 그룹 중 지정한 유효성인 그룹의 수를 센다.
 * @param[in] v       검증된 입력.
 * @param[in] target  세려는 유효성.
 * @return  그룹 수(0~4).
 * @requirement SWR-122
 * @requirement SWR-126
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static unsigned int sta_count(const eclp_validated_t *v, eclp_validity_t target)
{
    const eclp_validity_t groups[STA_GROUP_COUNT] = {v->speed, v->crash, v->approach, v->fault};
    unsigned int count = 0U;
    unsigned int i = 0U;

    for (i = 0U; i < STA_GROUP_COUNT; i++) {
        if (groups[i] == target) {
            count++;
        }
    }
    return count;
}

/**
 * @brief   상태를 판정한다.
 * @param[in] v  검증된 입력.
 * @return  `FAULT`, `DEGRADED`, `NORMAL` 중 하나.
 * @requirement SWR-122
 * @requirement SWR-123
 * @requirement SWR-124
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_state_t sta_decide_state(const eclp_validated_t *v)
{
    eclp_state_t state = ECLP_STATE_NORMAL;

    if (sta_fault_active(v)) {
        state = ECLP_STATE_FAULT;
    } else if ((sta_count(v, ECLP_INVALID) + sta_count(v, ECLP_STALE)) > 0U) {
        state = ECLP_STATE_DEGRADED;
    }
    return state;
}

/**
 * @brief   경고코드를 정한다.
 * @param[in] v      검증된 입력.
 * @param[in] state  판정한 상태.
 * @return  경고코드.
 * @requirement SWR-126
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_warning_t sta_decide_warning(const eclp_validated_t *v, eclp_state_t state)
{
    eclp_warning_t warning = ECLP_W_NONE;

    if (state == ECLP_STATE_FAULT) {
        warning = ECLP_W_SENSOR_FAULT;
    } else if (sta_count(v, ECLP_INVALID) > 0U) {
        warning = ECLP_W_INPUT_INVALID;
    } else if (sta_count(v, ECLP_STALE) > 0U) {
        warning = ECLP_W_INPUT_STALE;
    }
    return warning;
}

eclp_status_t eclp_sta_initial(eclp_state_t *state)
{
    eclp_status_t status = ECLP_OK;

    if (state == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        *state = ECLP_STATE_NORMAL;
    }
    return status;
}

eclp_status_t eclp_sta_evaluate(const eclp_validated_t *v, eclp_state_t *state, eclp_warning_t *warning)
{
    eclp_status_t status = ECLP_OK;

    if (v == NULL) {
        status = ECLP_ERR_ARG;
    } else if (state == NULL) {
        status = ECLP_ERR_ARG;
    } else if (warning == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        *state = sta_decide_state(v);
        *warning = sta_decide_warning(v, *state);
    }
    return status;
}
