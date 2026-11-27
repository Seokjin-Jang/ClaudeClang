/**
 * @file    input_validator.c
 * @brief   C-VAL 입력 검증기 구현. 입력 그룹의 형식, 범위, enum, freshness를 판정한다.
 * @module  M-VAL-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include "input_validator.h"

#include <math.h>

/**
 * @brief   마이크로초로 바꾼 시각이 입력 시각보다 얼마나 지났는지로 신선도를 판정한다.
 * @param[in] now_us  평가 시각(마이크로초).
 * @param[in] ts_s    입력 생성 시각(초).
 * @return  변환 실패나 미래 시각이면 `INVALID`, 200000 µs 초과이면 `STALE`, 아니면 `VALID`.
 * @requirement SWR-013
 * @requirement SWR-101
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_validity_t val_freshness(int64_t now_us, double ts_s)
{
    eclp_validity_t result = ECLP_VALID;
    int64_t ts_us = 0;

    if (eclp_val_time_us(ts_s, &ts_us) != ECLP_OK) {
        result = ECLP_INVALID;
    } else {
        const int64_t age_us = now_us - ts_us;
        if (age_us < 0) {
            result = ECLP_INVALID;
        } else if (age_us > ECLP_STALE_LIMIT_US) {
            result = ECLP_STALE;
        } else {
            result = ECLP_VALID;
        }
    }
    return result;
}

/**
 * @brief   정수 값이 boolean(0 또는 1)인지 확인한다.
 * @param[in] value  확인할 값.
 * @return  0 또는 1이면 true.
 * @requirement SWR-101
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static bool val_bool_ok(int value)
{
    return ((unsigned int)value <= 1U);
}

/**
 * @brief   차속이 유한하고 0.0 이상 300.0 이하인지 확인한다.
 * @param[in] speed  차속(km/h).
 * @return  범위 안이면 true.
 * @requirement SWR-101
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static bool val_speed_ok(double speed)
{
    bool ok = false;

    if (isfinite(speed)) {
        if (speed >= 0.0) {
            if (speed <= ECLP_SPEED_MAX_KPH) {
                ok = true;
            }
        }
    }
    return ok;
}

/**
 * @brief   차량 상태 그룹을 검증한다.
 * @param[in] in      차량 상태 입력.
 * @param[in] now_us  평가 시각(마이크로초).
 * @return  그룹의 유효성.
 * @requirement SWR-101
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_validity_t val_vehicle(const eclp_vehicle_in_t *in, int64_t now_us)
{
    eclp_validity_t result = ECLP_INVALID;

    if (in->present) {
        bool ok = val_speed_ok(in->vehicle_speed_kph);
        if (ok) {
            ok = ((unsigned int)in->gear <= (unsigned int)ECLP_GEAR_R);
        }
        if (ok) {
            result = val_freshness(now_us, in->source_timestamp_s);
        }
    }
    return result;
}

/**
 * @brief   충돌 그룹을 검증한다.
 * @param[in] in      충돌 입력.
 * @param[in] now_us  평가 시각(마이크로초).
 * @return  그룹의 유효성.
 * @requirement SWR-101
 * @requirement SWR-008
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_validity_t val_crash(const eclp_crash_in_t *in, int64_t now_us)
{
    eclp_validity_t result = ECLP_INVALID;

    if (in->present) {
        const bool ok = ((unsigned int)in->crash_status <= (unsigned int)ECLP_CRASH_CONFIRMED);
        if (ok) {
            result = val_freshness(now_us, in->source_timestamp_s);
        }
    }
    return result;
}

/**
 * @brief   접근위험 그룹을 검증한다.
 * @param[in] in      접근위험 입력.
 * @param[in] now_us  평가 시각(마이크로초).
 * @return  그룹의 유효성.
 * @requirement SWR-101
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_validity_t val_approach(const eclp_approach_in_t *in, int64_t now_us)
{
    eclp_validity_t result = ECLP_INVALID;

    if (in->present) {
        bool ok = val_bool_ok(in->rear_left_approach_risk);
        if (ok) {
            ok = val_bool_ok(in->rear_right_approach_risk);
        }
        if (ok) {
            result = val_freshness(now_us, in->source_timestamp_s);
        }
    }
    return result;
}

/**
 * @brief   고장과 시동 그룹을 검증한다.
 * @param[in] in      고장/시동 입력.
 * @param[in] now_us  평가 시각(마이크로초).
 * @return  그룹의 유효성.
 * @requirement SWR-101
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_validity_t val_fault(const eclp_fault_in_t *in, int64_t now_us)
{
    eclp_validity_t result = ECLP_INVALID;

    if (in->present) {
        bool ok = val_bool_ok(in->ignition_on);
        if (ok) {
            ok = val_bool_ok(in->sensor_fault);
        }
        if (ok) {
            result = val_freshness(now_us, in->source_timestamp_s);
        }
    }
    return result;
}

/**
 * @brief   운전자 요청의 값이 등록된 집합 안에 있는지 확인한다.
 * @param[in] in  운전자 요청 입력.
 * @return  요청이 없으면 `NONE`, 값이 모두 등록되어 있으면 `VALID`, 아니면 `INVALID`.
 * @requirement SWR-106
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_req_state_t val_request(const eclp_request_in_t *in)
{
    eclp_req_state_t result = ECLP_REQ_NONE;

    if (in->present) {
        bool ok = ((unsigned int)in->side <= (unsigned int)ECLP_SIDE_ALL);
        if (ok) {
            ok = ((unsigned int)in->action <= (unsigned int)ECLP_ACTION_UNLOCK);
        }
        if (ok) {
            ok = ((unsigned int)in->source <= (unsigned int)ECLP_SRC_MOBILE_APP);
        }
        result = ECLP_REQ_INVALID;
        if (ok) {
            result = ECLP_REQ_VALID;
        }
    }
    return result;
}

eclp_status_t eclp_val_time_us(double t_s, int64_t *out_us)
{
    eclp_status_t status = ECLP_OK;

    if (out_us == NULL) {
        status = ECLP_ERR_ARG;
    } else if (!isfinite(t_s)) {
        status = ECLP_ERR_RANGE;
    } else if (fabs(t_s) > ECLP_TIME_LIMIT_S) {
        status = ECLP_ERR_RANGE;
    } else {
        *out_us = (int64_t)llround(t_s * ECLP_US_PER_S);
    }
    return status;
}

eclp_status_t eclp_val_validate(const eclp_snapshot_t *snap, eclp_validated_t *out)
{
    eclp_status_t status = ECLP_OK;
    int64_t now_us = 0;

    if (snap == NULL) {
        status = ECLP_ERR_ARG;
    } else if (out == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        /* 기본값: 모든 그룹 INVALID, 불리언 FALSE, 요청 없음 */
        *out = (eclp_validated_t){ECLP_INVALID, ECLP_INVALID, ECLP_INVALID, ECLP_INVALID,
                                  ECLP_CRASH_NONE, false, false, false, false,
                                  ECLP_REQ_NONE, ECLP_SIDE_LEFT, ECLP_ACTION_LOCK};
        if (eclp_val_time_us(snap->now_s, &now_us) == ECLP_OK) {
            out->speed = val_vehicle(&snap->vehicle, now_us);
            out->crash = val_crash(&snap->crash, now_us);
            out->approach = val_approach(&snap->approach, now_us);
            out->fault = val_fault(&snap->fault, now_us);
            out->request = val_request(&snap->request);
        }
        if (out->crash != ECLP_INVALID) {
            out->crash_status = (eclp_crash_t)snap->crash.crash_status;
        }
        if (out->approach != ECLP_INVALID) {
            out->left_risk = (snap->approach.rear_left_approach_risk != 0);
            out->right_risk = (snap->approach.rear_right_approach_risk != 0);
        }
        if (out->fault != ECLP_INVALID) {
            out->ignition_on = (snap->fault.ignition_on != 0);
            out->sensor_fault = (snap->fault.sensor_fault != 0);
        }
        if (out->request == ECLP_REQ_VALID) {
            out->req_side = (eclp_side_t)snap->request.side;
            out->req_action = (eclp_action_t)snap->request.action;
        }
    }
    return status;
}
