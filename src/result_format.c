/**
 * @file    result_format.c
 * @brief   결과의 이름 문자열과 공개 직렬화(SRS 부록 B) 구현.
 * @module  M-CTL-02
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <stdio.h>

#include "controller.h"

/** 값의 범위가 맞지 않을 때 쓰는 이름. */
#define FMT_UNKNOWN "UNKNOWN"

/** 잠금 출력의 이름 표. */
static const char *const LOCK_NAMES[ECLP_LOCK_COUNT] = {"RELEASE", "LOCK"};

/** 상태의 이름 표. */
static const char *const STATE_NAMES[ECLP_STATE_COUNT] = {"NORMAL", "DEGRADED", "FAULT", "OFF"};

/** 이유코드의 이름 표. */
static const char *const REASON_NAMES[ECLP_RC_COUNT] = {
    "NONE", "INIT", "CRASH_RELEASE", "CRASH_PENDING_HOLD", "SENSOR_FAULT_HOLD", "INPUT_DEGRADED_HOLD",
    "APPROACH_RISK_LOCK", "RELEASE_SUPPRESSED", "DRIVER_LOCK", "DRIVER_RELEASE", "REQUEST_REJECTED",
    "IGNITION_OFF"};

/** 경고코드의 이름 표. */
static const char *const WARNING_NAMES[ECLP_W_COUNT] = {"NONE", "W_INPUT_STALE", "W_INPUT_INVALID",
                                                        "W_SENSOR_FAULT"};

/** 입력 유효성의 이름 표. */
static const char *const VALIDITY_NAMES[ECLP_VALIDITY_COUNT] = {"VALID", "STALE", "INVALID"};

/**
 * @brief   이름 표에서 값에 해당하는 이름을 찾는다.
 * @param[in] table  이름 표.
 * @param[in] count  표의 항목 수.
 * @param[in] index  값.
 * @return  표 안이면 이름, 아니면 `"UNKNOWN"`.
 * @requirement SWR-016
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static const char *fmt_lookup(const char *const *table, unsigned int count, unsigned int index)
{
    const char *name = FMT_UNKNOWN;

    if (index < count) {
        name = table[index];
    }
    return name;
}

const char *eclp_name_lock(eclp_lock_t value)
{
    return fmt_lookup(LOCK_NAMES, (unsigned int)ECLP_LOCK_COUNT, (unsigned int)value);
}

const char *eclp_name_state(eclp_state_t value)
{
    return fmt_lookup(STATE_NAMES, (unsigned int)ECLP_STATE_COUNT, (unsigned int)value);
}

const char *eclp_name_reason(eclp_reason_t value)
{
    return fmt_lookup(REASON_NAMES, (unsigned int)ECLP_RC_COUNT, (unsigned int)value);
}

const char *eclp_name_warning(eclp_warning_t value)
{
    return fmt_lookup(WARNING_NAMES, (unsigned int)ECLP_W_COUNT, (unsigned int)value);
}

const char *eclp_name_validity(eclp_validity_t value)
{
    return fmt_lookup(VALIDITY_NAMES, (unsigned int)ECLP_VALIDITY_COUNT, (unsigned int)value);
}

eclp_status_t eclp_serialize(const eclp_result_t *r, char *buf, size_t cap, size_t *len)
{
    eclp_status_t status = ECLP_OK;

    if (r == NULL) {
        status = ECLP_ERR_ARG;
    } else if (buf == NULL) {
        status = ECLP_ERR_ARG;
    } else if (len == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        const int written = snprintf(buf, cap, "%s,%s,%s,%s,%s,%s,%s,%s,%s\n", eclp_name_lock(r->lock_left),
                           eclp_name_lock(r->lock_right), eclp_name_state(r->state),
                           eclp_name_reason(r->reason_code), eclp_name_warning(r->warning_code),
                           eclp_name_validity(r->validity_speed), eclp_name_validity(r->validity_crash),
                           eclp_name_validity(r->validity_approach), eclp_name_validity(r->validity_fault));
        if ((size_t)written >= cap) {
            status = ECLP_ERR_RANGE;
        } else {
            *len = (size_t)written;
        }
    }
    return status;
}
