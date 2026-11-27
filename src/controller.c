/**
 * @file    controller.c
 * @brief   C-CTL 제어기 구현. 검증, 상태 판정, 결정의 호출 순서를 제어하고 결과를 조립한다.
 * @module  M-CTL-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include "controller.h"

#include "arbiter.h"
#include "input_validator.h"
#include "state_manager.h"

/** `priority_reason`에서 `reason_code`로의 매핑 표. (SWR-107, SRS 부록 A) */
static const eclp_reason_t REASON_FOR_PRIORITY[ECLP_PR_COUNT] = {
    [ECLP_PR_NONE] = ECLP_RC_NONE,
    [ECLP_PR_INIT] = ECLP_RC_INIT,
    [ECLP_PR_CRASH] = ECLP_RC_CRASH_RELEASE,
    [ECLP_PR_CRASH_PENDING] = ECLP_RC_CRASH_PENDING_HOLD,
    [ECLP_PR_SENSOR_FAULT] = ECLP_RC_SENSOR_FAULT_HOLD,
    [ECLP_PR_DEGRADED] = ECLP_RC_INPUT_DEGRADED_HOLD,
    [ECLP_PR_APPROACH_RISK] = ECLP_RC_APPROACH_RISK_LOCK,
    [ECLP_PR_APPROACH_RISK_SUPPRESS] = ECLP_RC_RELEASE_SUPPRESSED,
    [ECLP_PR_DRIVER] = ECLP_RC_DRIVER_LOCK,
    [ECLP_PR_REJECTED] = ECLP_RC_REQUEST_REJECTED,
    [ECLP_PR_IGNITION_OFF] = ECLP_RC_IGNITION_OFF,
};

/**
 * @brief   두 반환 상태를 분기 없이 합친다. 하나라도 오류면 결과도 오류다.
 * @param[in] a  첫째 상태.
 * @param[in] b  둘째 상태.
 * @return  비트 OR로 합친 상태.
 * @requirement SWR-114
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_status_t ctl_merge(eclp_status_t a, eclp_status_t b)
{
    return (eclp_status_t)((int)a | (int)b);
}

/**
 * @brief   컴포넌트의 출력으로 공개 결과를 조립한다.
 * @param[in] v        검증된 입력.
 * @param[in] state    상태.
 * @param[in] warning  경고코드.
 * @param[in] d        판정기의 결정.
 * @param[in] reason   이유코드.
 * @return  조립한 결과.
 * @requirement SWR-102
 * @requirement SWR-114
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_result_t ctl_assemble(const eclp_validated_t *v, eclp_state_t state, eclp_warning_t warning,
                                  const eclp_decision_t *d, eclp_reason_t reason)
{
    eclp_result_t r;

    r.lock_left = d->lock_left;
    r.lock_right = d->lock_right;
    r.state = state;
    r.priority_reason = d->priority_reason;
    r.reason_code = reason;
    r.warning_code = warning;
    r.validity_speed = v->speed;
    r.validity_crash = v->crash;
    r.validity_approach = v->approach;
    r.validity_fault = v->fault;
    return r;
}

eclp_status_t eclp_priority_to_reason(eclp_priority_t priority, eclp_action_t action, eclp_reason_t *out)
{
    eclp_status_t status = ECLP_OK;

    if (out == NULL) {
        status = ECLP_ERR_ARG;
    } else if ((unsigned int)priority >= (unsigned int)ECLP_PR_COUNT) {
        status = ECLP_ERR_ARG;
    } else {
        *out = REASON_FOR_PRIORITY[priority];
        if (priority == ECLP_PR_DRIVER) {
            if (action == ECLP_ACTION_UNLOCK) {
                *out = ECLP_RC_DRIVER_RELEASE;
            }
        }
    }
    return status;
}

eclp_status_t eclp_init(eclp_t *ctl)
{
    eclp_status_t status = ECLP_OK;
    eclp_state_t state = ECLP_STATE_NORMAL;

    if (ctl == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        status = ctl_merge(status, eclp_arb_init(&ctl->arb));
        status = ctl_merge(status, eclp_sta_initial(&state));
        ctl->last = (eclp_result_t){ECLP_RELEASE, ECLP_RELEASE, state, ECLP_PR_INIT, ECLP_RC_INIT,
                                    ECLP_W_NONE, ECLP_VALID, ECLP_VALID, ECLP_VALID, ECLP_VALID};
    }
    return status;
}

eclp_status_t eclp_evaluate(eclp_t *ctl, const eclp_snapshot_t *snap, eclp_result_t *out)
{
    eclp_status_t status = ECLP_OK;
    eclp_validated_t v;
    eclp_decision_t d;
    eclp_state_t state = ECLP_STATE_NORMAL;
    eclp_warning_t warning = ECLP_W_NONE;
    eclp_reason_t reason = ECLP_RC_NONE;

    if (ctl == NULL) {
        status = ECLP_ERR_ARG;
    } else if (snap == NULL) {
        status = ECLP_ERR_ARG;
    } else if (out == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        /* 내부 호출은 항상 유효한 포인터를 받으므로 오류 상태는 합쳐서 돌려준다. */
        status = ctl_merge(status, eclp_val_validate(snap, &v));
        status = ctl_merge(status, eclp_sta_evaluate(&v, &state, &warning));
        status = ctl_merge(status, eclp_arb_decide(&ctl->arb, &v, state, &d));
        status = ctl_merge(status, eclp_priority_to_reason(d.priority_reason, d.driver_action, &reason));
        *out = ctl_assemble(&v, state, warning, &d, reason);
        ctl->last = *out;
    }
    return status;
}
