/**
 * @file    arbiter.c
 * @brief   C-ARB 판정기 구현. 종료 규칙 표와 하위->상위 계층 규칙으로 우선순위(A-08)를 구현한다.
 * @module  M-ARB-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include "arbiter.h"

/** 규칙이 한 번의 판정에서 참조하는 입력 묶음. */
typedef struct {
    const eclp_arb_t *arb;        /**< 판정기 상태(읽기 전용) */
    const eclp_validated_t *v;    /**< 검증된 입력 */
    eclp_state_t state;           /**< 상태 관리기가 판정한 상태 */
} arb_ctx_t;

/** 종료 규칙의 함수 형식. 결정을 끝냈으면 1, 아니면 0을 반환한다. (SOLID: L) */
typedef int (*arb_rule_fn)(const arb_ctx_t *ctx, eclp_decision_t *d);

/**
 * @brief   충돌 해제 래치를 갱신한다. 시동 OFF이면 풀고, 충돌 CONFIRMED이면 건다.
 * @param[in] latched  현재 래치 상태.
 * @param[in] v        검증된 입력.
 * @return  갱신된 래치 상태.
 * @requirement SWR-007
 * @requirement SWR-120
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static bool arb_next_latch(bool latched, const eclp_validated_t *v)
{
    bool next = latched;

    if (v->fault == ECLP_VALID) {
        if (!v->ignition_on) {
            next = false;
        }
    }
    if (v->crash == ECLP_VALID) {
        if (v->crash_status == ECLP_CRASH_CONFIRMED) {
            next = true;
        }
    }
    return next;
}

/**
 * @brief   유효한 충돌 PENDING인지 확인한다.
 * @param[in] ctx  판정 컨텍스트.
 * @return  유효한 `PENDING`이면 true.
 * @requirement SWR-008
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static bool arb_crash_pending(const arb_ctx_t *ctx)
{
    bool pending = false;

    if (ctx->v->crash == ECLP_VALID) {
        if (ctx->v->crash_status == ECLP_CRASH_PENDING) {
            pending = true;
        }
    }
    return pending;
}

/**
 * @brief   종료 규칙 ①: 충돌 래치가 걸려 있으면 두 출력을 RELEASE로 한다.
 * @param[in]     ctx  판정 컨텍스트.
 * @param[in,out] d    결정.
 * @return  결정을 끝냈으면 1, 아니면 0.
 * @requirement SWR-007
 * @requirement SWR-120
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static int arb_rule_crash(const arb_ctx_t *ctx, eclp_decision_t *d)
{
    int decided = 0;

    if (ctx->arb->crash_latched) {
        d->lock_left = ECLP_RELEASE;
        d->lock_right = ECLP_RELEASE;
        d->priority_reason = ECLP_PR_CRASH;
        decided = 1;
    }
    return decided;
}

/**
 * @brief   종료 규칙 ②: 상태가 FAULT이면 직전 출력을 유지한다.
 * @param[in]     ctx  판정 컨텍스트.
 * @param[in,out] d    결정(직전 출력으로 초기화되어 있다).
 * @return  결정을 끝냈으면 1, 아니면 0.
 * @requirement SWR-021
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static int arb_rule_fault(const arb_ctx_t *ctx, eclp_decision_t *d)
{
    int decided = 0;

    if (ctx->state == ECLP_STATE_FAULT) {
        d->priority_reason = ECLP_PR_SENSOR_FAULT;
        decided = 1;
    }
    return decided;
}

/** 종료 규칙 표. 우선순위(A-08) 순서이며 이후 규칙(③ 화재 등)은 여기에 추가한다. (SOLID: O) */
static const arb_rule_fn TERMINAL_RULES[] = {arb_rule_crash, arb_rule_fault};

/** 종료 규칙 표의 항목 수. */
#define ARB_TERMINAL_COUNT (sizeof(TERMINAL_RULES) / sizeof(TERMINAL_RULES[0]))

/**
 * @brief   종료 규칙을 순서대로 평가한다. 하나가 결정하면 이후 규칙은 호출하지 않는다.
 * @param[in]     ctx  판정 컨텍스트.
 * @param[in,out] d    결정.
 * @return  결정이 끝났으면 1, 아니면 0.
 * @requirement SWR-104
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static int arb_run_terminal(const arb_ctx_t *ctx, eclp_decision_t *d)
{
    int decided = 0;
    size_t i = 0U;

    for (i = 0U; i < ARB_TERMINAL_COUNT; i++) {
        if (decided == 0) {
            decided = TERMINAL_RULES[i](ctx, d);
        }
    }
    return decided;
}

/**
 * @brief   운전자 요청의 처리 결과를 정한다. (DEGRADED, 충돌 PENDING이면 보류)
 * @param[in] ctx  판정 컨텍스트.
 * @return  요청 처리 결과.
 * @requirement SWR-106
 * @requirement SWR-108
 * @requirement SWR-125
 * @requirement SWR-008
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_req_outcome_t arb_request_outcome(const arb_ctx_t *ctx)
{
    eclp_req_outcome_t outcome = ECLP_OUT_APPLIED;

    if (ctx->v->request == ECLP_REQ_NONE) {
        outcome = ECLP_OUT_NONE;
    } else if (ctx->state == ECLP_STATE_DEGRADED) {
        outcome = ECLP_OUT_HELD_DEGRADED;
    } else if (arb_crash_pending(ctx)) {
        outcome = ECLP_OUT_HELD_PENDING;
    } else if (ctx->v->request == ECLP_REQ_INVALID) {
        outcome = ECLP_OUT_REJECTED;
    }
    return outcome;
}

/**
 * @brief   요청의 대상 도어가 지정한 도어를 포함하는지 확인한다.
 * @param[in] side    요청 대상(`LEFT`/`RIGHT`/`ALL`).
 * @param[in] target  확인할 도어(`LEFT` 또는 `RIGHT`).
 * @return  `ALL`이거나 대상과 같으면 true.
 * @requirement SWR-108
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static bool arb_side_selected(eclp_side_t side, eclp_side_t target)
{
    bool selected = false;

    if (side == ECLP_SIDE_ALL) {
        selected = true;
    } else if (side == target) {
        selected = true;
    }
    return selected;
}

/**
 * @brief   계층 규칙 ⑧: 운전자 요청을 선택한 도어에 적용한다.
 * @param[in]     ctx  판정 컨텍스트.
 * @param[in,out] d    결정.
 * @return  요청을 적용했으면 1, 아니면 0.
 * @requirement SWR-108
 * @requirement SWR-106
 * @requirement SWR-125
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static int arb_layer_driver(const arb_ctx_t *ctx, eclp_decision_t *d)
{
    int applied = 0;
    eclp_lock_t lock = ECLP_RELEASE;

    d->outcome = arb_request_outcome(ctx);
    if (d->outcome == ECLP_OUT_APPLIED) {
        d->driver_action = ctx->v->req_action;
        if (ctx->v->req_action == ECLP_ACTION_LOCK) {
            lock = ECLP_LOCK;
        }
        if (arb_side_selected(ctx->v->req_side, ECLP_SIDE_LEFT)) {
            d->lock_left = lock;
        }
        if (arb_side_selected(ctx->v->req_side, ECLP_SIDE_RIGHT)) {
            d->lock_right = lock;
        }
        applied = 1;
    }
    return applied;
}

/**
 * @brief   한 도어에 접근위험을 적용한다. 해제 요청이 적용되었다면 억제로 기록한다.
 * @param[in]     ctx   판정 컨텍스트.
 * @param[in,out] d     결정.
 * @param[in]     door  도어(`LEFT` 또는 `RIGHT`).
 * @param[in]     risk  이 도어의 접근위험이 적용되는지.
 * @param[in,out] lock  이 도어의 출력.
 * @return  접근위험을 적용했으면 1, 아니면 0.
 * @requirement SWR-005
 * @requirement SWR-121
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static int arb_apply_risk(const arb_ctx_t *ctx, eclp_decision_t *d, eclp_side_t door, bool risk,
                          eclp_lock_t *lock)
{
    int applied = 0;

    if (risk) {
        if (d->outcome == ECLP_OUT_APPLIED) {
            if (d->driver_action == ECLP_ACTION_UNLOCK) {
                if (arb_side_selected(ctx->v->req_side, door)) {
                    d->suppressed = true;
                }
            }
        }
        *lock = ECLP_LOCK;
        applied = 1;
    }
    return applied;
}

/**
 * @brief   계층 규칙 ④: 유효하고 신선한 접근위험 TRUE인 도어를 도어별로 LOCK한다.
 * @param[in]     ctx  판정 컨텍스트.
 * @param[in,out] d    결정.
 * @return  접근위험이 한 도어라도 적용되었으면 1, 아니면 0.
 * @requirement SWR-005
 * @requirement SWR-009
 * @requirement SWR-121
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static int arb_layer_approach(const arb_ctx_t *ctx, eclp_decision_t *d)
{
    bool left = false;
    bool right = false;

    if (ctx->v->approach == ECLP_VALID) {
        left = ctx->v->left_risk;
        right = ctx->v->right_risk;
    }
    d->risk_left = (arb_apply_risk(ctx, d, ECLP_SIDE_LEFT, left, &d->lock_left) != 0);
    d->risk_right = (arb_apply_risk(ctx, d, ECLP_SIDE_RIGHT, right, &d->lock_right) != 0);
    return (int)(d->risk_left || d->risk_right);
}

/**
 * @brief   이번 평가주기에 출력에 영향을 준 가장 높은 규칙(priority_reason)을 고른다.
 * @param[in] ctx  판정 컨텍스트.
 * @param[in] d    계층 규칙을 적용한 결정.
 * @return  이긴 규칙.
 * @requirement SWR-104
 * @requirement SWR-107
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
static eclp_priority_t arb_select_priority(const arb_ctx_t *ctx, const eclp_decision_t *d)
{
    eclp_priority_t reason = ECLP_PR_NONE;

    if (d->suppressed) {
        reason = ECLP_PR_APPROACH_RISK_SUPPRESS;
    } else if (d->risk_left) {
        reason = ECLP_PR_APPROACH_RISK;
    } else if (d->risk_right) {
        reason = ECLP_PR_APPROACH_RISK;
    } else if (ctx->state == ECLP_STATE_DEGRADED) {
        reason = ECLP_PR_DEGRADED;
    } else if (arb_crash_pending(ctx)) {
        reason = ECLP_PR_CRASH_PENDING;
    } else if (d->outcome == ECLP_OUT_REJECTED) {
        reason = ECLP_PR_REJECTED;
    } else if (d->outcome == ECLP_OUT_APPLIED) {
        reason = ECLP_PR_DRIVER;
    }
    return reason;
}

eclp_status_t eclp_arb_init(eclp_arb_t *arb)
{
    eclp_status_t status = ECLP_OK;

    if (arb == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        arb->lock_left = ECLP_RELEASE;
        arb->lock_right = ECLP_RELEASE;
        arb->crash_latched = false;
    }
    return status;
}

eclp_status_t eclp_arb_decide(eclp_arb_t *arb, const eclp_validated_t *v, eclp_state_t state,
                              eclp_decision_t *out)
{
    eclp_status_t status = ECLP_OK;
    arb_ctx_t ctx;
    eclp_decision_t d;

    if (arb == NULL) {
        status = ECLP_ERR_ARG;
    } else if (v == NULL) {
        status = ECLP_ERR_ARG;
    } else if (out == NULL) {
        status = ECLP_ERR_ARG;
    } else {
        arb->crash_latched = arb_next_latch(arb->crash_latched, v);
        ctx.arb = arb;
        ctx.v = v;
        ctx.state = state;
        d = (eclp_decision_t){arb->lock_left, arb->lock_right, ECLP_PR_NONE, ECLP_ACTION_LOCK,
                              ECLP_OUT_NONE, false, false, false};
        const int decided = arb_run_terminal(&ctx, &d);
        if (decided == 0) {
            (void)arb_layer_driver(&ctx, &d);
            (void)arb_layer_approach(&ctx, &d);
            d.priority_reason = arb_select_priority(&ctx, &d);
        }
        arb->lock_left = d.lock_left;
        arb->lock_right = d.lock_right;
        *out = d;
    }
    return status;
}
