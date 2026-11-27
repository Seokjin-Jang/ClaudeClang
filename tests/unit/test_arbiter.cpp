/**
 * @file    test_arbiter.cpp
 * @brief   C-ARB 판정기(M-ARB-01)의 단위 테스트.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include "test_support.hpp"

extern "C" {
#include "arbiter.h"
}

namespace {

/** 모든 그룹이 유효하고 요청이 없는 검증 입력을 만든다. */
eclp_validated_t AllValid()
{
    eclp_validated_t v;
    v.speed = ECLP_VALID;
    v.crash = ECLP_VALID;
    v.approach = ECLP_VALID;
    v.fault = ECLP_VALID;
    v.crash_status = ECLP_CRASH_NONE;
    v.left_risk = false;
    v.right_risk = false;
    v.ignition_on = true;
    v.sensor_fault = false;
    v.request = ECLP_REQ_NONE;
    v.req_side = ECLP_SIDE_LEFT;
    v.req_action = ECLP_ACTION_LOCK;
    return v;
}

/** 유효한 운전자 요청을 넣는다. */
eclp_validated_t WithReq(eclp_validated_t v, eclp_side_t side, eclp_action_t action)
{
    v.request = ECLP_REQ_VALID;
    v.req_side = side;
    v.req_action = action;
    return v;
}

/** 판정을 한 번 수행한다. */
eclp_decision_t Decide(eclp_arb_t *arb, const eclp_validated_t &v, eclp_state_t state = ECLP_STATE_NORMAL)
{
    eclp_decision_t d;
    EXPECT_EQ(ECLP_OK, eclp_arb_decide(arb, &v, state, &d));
    return d;
}

/** 판정기를 초기화해 반환한다. */
eclp_arb_t Fresh()
{
    eclp_arb_t arb;
    EXPECT_EQ(ECLP_OK, eclp_arb_init(&arb));
    return arb;
}

/** 두 도어 모두 LOCK 상태로 만든 판정기를 반환한다. */
eclp_arb_t BothLocked()
{
    eclp_arb_t arb = Fresh();
    const eclp_decision_t d = Decide(&arb, WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_LOCK));
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
    return arb;
}

/**
 * @test        UT-021-02
 * @requirement SWR-021
 * @brief       [목적] 판정기 초기화 후 직전 출력이 RELEASE이고 충돌 래치가 해제 상태이며 널 인자가 오류 처리되는지 확인한다.
 * @input       초기화 호출, 널 포인터
 * @procedure   1. eclp_arb_init을 호출한다. 2. 중립 입력으로 판정한다. 3. 널 인자 조합을 호출한다.
 * @expected    초기화 후 출력 RELEASE/RELEASE, 이유 NONE. 널이면 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, InitAndNullArguments)
{
    eclp_arb_t arb = Fresh();
    EXPECT_FALSE(arb.crash_latched);
    const eclp_decision_t d = Decide(&arb, AllValid());
    EXPECT_EQ(ECLP_RELEASE, d.lock_left);
    EXPECT_EQ(ECLP_RELEASE, d.lock_right);
    EXPECT_EQ(ECLP_PR_NONE, d.priority_reason);

    EXPECT_EQ(ECLP_ERR_ARG, eclp_arb_init(nullptr));
    const eclp_validated_t v = AllValid();
    eclp_decision_t out;
    EXPECT_EQ(ECLP_ERR_ARG, eclp_arb_decide(nullptr, &v, ECLP_STATE_NORMAL, &out));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_arb_decide(&arb, nullptr, ECLP_STATE_NORMAL, &out));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_arb_decide(&arb, &v, ECLP_STATE_NORMAL, nullptr));
}

/**
 * @test        UT-007-01
 * @requirement SWR-007
 * @brief       [목적] 유효한 충돌 CONFIRMED가 다른 모든 입력(LOCK 요청, 접근위험, FAULT 상태)보다 우선해 같은 주기에 두 출력을 RELEASE로 만드는지 확인한다.
 * @input       직전 LOCK/LOCK, 충돌 CONFIRMED + (LOCK 요청 | 접근위험 좌/우 | 상태 FAULT)
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. 조건을 바꿔 충돌 CONFIRMED를 판정한다.
 * @expected    출력 RELEASE/RELEASE, 이긴 규칙 CRASH
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, CrashConfirmedOverridesEverything)
{
    for (int variant = 0; variant < 3; ++variant) {
        eclp_arb_t arb = BothLocked();
        eclp_validated_t v = AllValid();
        v.crash_status = ECLP_CRASH_CONFIRMED;
        eclp_state_t state = ECLP_STATE_NORMAL;
        if (variant == 0) {
            v = WithReq(v, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
        } else if (variant == 1) {
            v.left_risk = true;
            v.right_risk = true;
        } else {
            v.sensor_fault = true;
            state = ECLP_STATE_FAULT;
        }
        const eclp_decision_t d = Decide(&arb, v, state);
        EXPECT_EQ(ECLP_RELEASE, d.lock_left) << variant;
        EXPECT_EQ(ECLP_RELEASE, d.lock_right) << variant;
        EXPECT_EQ(ECLP_PR_CRASH, d.priority_reason) << variant;
    }
}

/**
 * @test        UT-120-01
 * @requirement SWR-120
 * @brief       [목적] 충돌 해제 후 충돌 입력이 사라지거나 STALE/INVALID가 되고 LOCK 요청과 접근위험이 들어와도 RELEASE가 유지되는지 확인한다.
 * @input       CONFIRMED 이후 충돌 그룹: NONE, PENDING, STALE, INVALID. 각 주기에 LOCK 요청과 접근위험
 * @procedure   1. 충돌 CONFIRMED를 판정한다. 2. 충돌 입력을 바꿔 판정을 반복한다.
 * @expected    모든 주기에 RELEASE/RELEASE, 이유 CRASH
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, LatchSurvivesCrashSignalLoss)
{
    eclp_arb_t arb = BothLocked();
    eclp_validated_t crash = AllValid();
    crash.crash_status = ECLP_CRASH_CONFIRMED;
    ASSERT_EQ(ECLP_PR_CRASH, Decide(&arb, crash).priority_reason);
    ASSERT_TRUE(arb.crash_latched);

    for (int variant = 0; variant < 4; ++variant) {
        eclp_validated_t v = WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
        v.left_risk = true;
        if (variant == 1) {
            v.crash_status = ECLP_CRASH_PENDING;
        } else if (variant == 2) {
            v.crash = ECLP_STALE;
        } else if (variant == 3) {
            v.crash = ECLP_INVALID;
        }
        const eclp_decision_t d = Decide(&arb, v);
        EXPECT_EQ(ECLP_RELEASE, d.lock_left) << variant;
        EXPECT_EQ(ECLP_RELEASE, d.lock_right) << variant;
        EXPECT_EQ(ECLP_PR_CRASH, d.priority_reason) << variant;
    }
}

/**
 * @test        UT-120-02
 * @requirement SWR-120
 * @brief       [목적] ignition_on이 FALSE인 주기부터 래치가 풀리고 그 뒤 규칙이 정상 평가되는지, 고장 그룹이 유효하지 않으면 풀리지 않는지, 같은 주기의 충돌 CONFIRMED는 다시 래치하는지 확인한다.
 * @input       래치 후 (고장 그룹 STALE + ignition FALSE), (고장 VALID + ignition FALSE), (ignition FALSE + 충돌 CONFIRMED)
 * @procedure   1. 충돌 래치를 건다. 2. 조건별로 판정한다. 3. 풀린 뒤 LOCK 요청을 판정한다.
 * @expected    STALE이면 유지, VALID+OFF이면 해제(이유 NONE), OFF+CONFIRMED이면 유지, 해제 후 LOCK 요청이 적용됨
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, LatchReleasedByIgnitionOff)
{
    eclp_arb_t arb = Fresh();
    eclp_validated_t crash = AllValid();
    crash.crash_status = ECLP_CRASH_CONFIRMED;
    Decide(&arb, crash);
    ASSERT_TRUE(arb.crash_latched);

    eclp_validated_t stale = AllValid();
    stale.fault = ECLP_STALE;
    stale.ignition_on = false;
    EXPECT_EQ(ECLP_PR_CRASH, Decide(&arb, stale, ECLP_STATE_DEGRADED).priority_reason);
    EXPECT_TRUE(arb.crash_latched);

    eclp_validated_t off = AllValid();
    off.ignition_on = false;
    EXPECT_EQ(ECLP_PR_NONE, Decide(&arb, off).priority_reason);
    EXPECT_FALSE(arb.crash_latched);

    const eclp_decision_t locked = Decide(&arb, WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_LOCK));
    EXPECT_EQ(ECLP_LOCK, locked.lock_left);

    eclp_validated_t both = AllValid();
    both.ignition_on = false;
    both.crash_status = ECLP_CRASH_CONFIRMED;
    EXPECT_EQ(ECLP_PR_CRASH, Decide(&arb, both).priority_reason);
    EXPECT_TRUE(arb.crash_latched);
}

/**
 * @test        UT-008-01
 * @requirement SWR-008
 * @brief       [목적] 유효한 충돌 PENDING이 출력을 바꾸지 않고 같은 주기의 운전자 요청을 보류하며, PENDING 입력이 STALE이면 보류하지 않는지 확인한다.
 * @input       직전 LOCK/LOCK, 충돌 PENDING + 해제 요청 ALL. 충돌 그룹 STALE + PENDING + 해제 요청
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. PENDING과 해제 요청을 판정한다. 3. STALE 조건을 판정한다.
 * @expected    VALID PENDING: 출력 유지(LOCK/LOCK), 이유 CRASH_PENDING, 요청 보류. STALE: PENDING으로 보지 않음
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, CrashPendingHoldsOutputsAndRequests)
{
    eclp_arb_t arb = BothLocked();
    eclp_validated_t v = WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    v.crash_status = ECLP_CRASH_PENDING;
    const eclp_decision_t d = Decide(&arb, v);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
    EXPECT_EQ(ECLP_PR_CRASH_PENDING, d.priority_reason);
    EXPECT_EQ(ECLP_OUT_HELD_PENDING, d.outcome);

    v.crash = ECLP_STALE;
    const eclp_decision_t stale = Decide(&arb, v, ECLP_STATE_DEGRADED);
    EXPECT_EQ(ECLP_OUT_HELD_DEGRADED, stale.outcome);
    EXPECT_NE(ECLP_PR_CRASH_PENDING, stale.priority_reason);

    /* 상태 판정기와 모순된 입력(NORMAL인데 충돌 그룹이 STALE)에서도 PENDING으로 보지 않는다. */
    const eclp_decision_t inconsistent = Decide(&arb, v, ECLP_STATE_NORMAL);
    EXPECT_EQ(ECLP_OUT_APPLIED, inconsistent.outcome);
    EXPECT_EQ(ECLP_PR_DRIVER, inconsistent.priority_reason);
}

/**
 * @test        UT-005-01
 * @requirement SWR-005
 * @brief       [목적] 유효한 접근위험 TRUE인 도어만 LOCK이 되고, 접근위험 그룹이 유효하지 않으면 적용하지 않으며, DEGRADED에서도 유효한 접근위험은 적용되는지 확인한다.
 * @input       (좌,우) = (T,F), (F,T), (T,T), (F,F). 접근위험 그룹 STALE. 상태 DEGRADED + 유효한 접근위험
 * @procedure   1. 입력별로 새 판정기로 판정한다. 2. 출력과 이유를 확인한다.
 * @expected    위험 도어만 LOCK, 이유 APPROACH_RISK, 둘 다 FALSE면 NONE, STALE이면 적용 안 함, DEGRADED+유효이면 LOCK
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, ApproachRiskLocksOnlyRiskyDoor)
{
    const bool cases[][2] = {{true, false}, {false, true}, {true, true}, {false, false}};
    for (const auto &c : cases) {
        eclp_arb_t arb = Fresh();
        eclp_validated_t v = AllValid();
        v.left_risk = c[0];
        v.right_risk = c[1];
        const eclp_decision_t d = Decide(&arb, v);
        EXPECT_EQ(c[0] ? ECLP_LOCK : ECLP_RELEASE, d.lock_left);
        EXPECT_EQ(c[1] ? ECLP_LOCK : ECLP_RELEASE, d.lock_right);
        EXPECT_EQ((c[0] || c[1]) ? ECLP_PR_APPROACH_RISK : ECLP_PR_NONE, d.priority_reason);
    }

    eclp_arb_t arb = Fresh();
    eclp_validated_t stale = AllValid();
    stale.approach = ECLP_STALE;
    stale.left_risk = true;
    EXPECT_EQ(ECLP_RELEASE, Decide(&arb, stale, ECLP_STATE_DEGRADED).lock_left);

    arb = Fresh();
    eclp_validated_t degraded = AllValid();
    degraded.speed = ECLP_STALE;
    degraded.right_risk = true;
    const eclp_decision_t d = Decide(&arb, degraded, ECLP_STATE_DEGRADED);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, d.priority_reason);
}

/**
 * @test        UT-121-01
 * @requirement SWR-121
 * @brief       [목적] 접근위험 TRUE인 도어에 대한 해제 요청이 억제되고 이유가 기록되며, 대상이 아닌 도어와 LOCK 요청, 보류된 요청은 억제로 보지 않는지 확인한다.
 * @input       좌측 위험 + 해제 요청 ALL, 해제 요청 RIGHT, LOCK 요청 ALL, DEGRADED의 해제 요청 ALL
 * @procedure   1. 직전 LOCK/LOCK에서 판정한다. 2. 출력과 이유를 확인한다.
 * @expected    ALL 해제: 좌 LOCK 유지/우 RELEASE, 이유 APPROACH_RISK_SUPPRESS. RIGHT 해제: 억제 아님. LOCK 요청: 억제 아님. DEGRADED: 억제 아님
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, ApproachRiskSuppressesRelease)
{
    eclp_arb_t arb = BothLocked();
    eclp_validated_t v = WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    v.left_risk = true;
    eclp_decision_t d = Decide(&arb, v);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_RELEASE, d.lock_right);
    EXPECT_TRUE(d.suppressed);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK_SUPPRESS, d.priority_reason);

    arb = BothLocked();
    v = WithReq(AllValid(), ECLP_SIDE_RIGHT, ECLP_ACTION_UNLOCK);
    v.left_risk = true;
    d = Decide(&arb, v);
    EXPECT_FALSE(d.suppressed);
    EXPECT_EQ(ECLP_RELEASE, d.lock_right);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, d.priority_reason);

    arb = Fresh();
    v = WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    v.left_risk = true;
    d = Decide(&arb, v);
    EXPECT_FALSE(d.suppressed);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);

    arb = BothLocked();
    v = WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    v.left_risk = true;
    d = Decide(&arb, v, ECLP_STATE_DEGRADED);
    EXPECT_FALSE(d.suppressed);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, d.priority_reason);
}

/**
 * @test        UT-009-01
 * @requirement SWR-009
 * @brief       [목적] 좌, 우 도어를 독립적으로 판정하는지 확인한다. 한쪽 접근위험이 다른 쪽 출력이나 운전자 요청을 바꾸지 않는다.
 * @input       좌측 위험 + 우측 LOCK 요청, 우측 위험 + 직전 좌 LOCK
 * @procedure   1. 입력별로 판정한다. 2. 두 출력을 확인한다.
 * @expected    좌측 위험: 좌 LOCK(위험), 우 LOCK(요청). 우측 위험: 우 LOCK, 좌는 직전 LOCK 유지
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, DoorsAreIndependent)
{
    eclp_arb_t arb = Fresh();
    eclp_validated_t v = WithReq(AllValid(), ECLP_SIDE_RIGHT, ECLP_ACTION_LOCK);
    v.left_risk = true;
    eclp_decision_t d = Decide(&arb, v);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);

    arb = Fresh();
    Decide(&arb, WithReq(AllValid(), ECLP_SIDE_LEFT, ECLP_ACTION_LOCK));
    v = AllValid();
    v.right_risk = true;
    d = Decide(&arb, v);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
}

/**
 * @test        UT-021-01
 * @requirement SWR-021
 * @brief       [목적] FAULT 상태에서 새 운전자 요청과 접근위험 변화를 적용하지 않고 직전 출력을 유지하는지 확인한다.
 * @input       직전 LOCK/LOCK, 상태 FAULT, 해제 요청 ALL, 접근위험(우)
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. FAULT로 판정한다. 3. 직전 RELEASE에서도 반복한다.
 * @expected    출력이 직전 값과 같고 이유 SENSOR_FAULT
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, FaultHoldsPreviousOutputs)
{
    eclp_arb_t arb = BothLocked();
    eclp_validated_t v = WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    v.sensor_fault = true;
    v.right_risk = true;
    eclp_decision_t d = Decide(&arb, v, ECLP_STATE_FAULT);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
    EXPECT_EQ(ECLP_PR_SENSOR_FAULT, d.priority_reason);

    arb = Fresh();
    d = Decide(&arb, WithReq(v, ECLP_SIDE_ALL, ECLP_ACTION_LOCK), ECLP_STATE_FAULT);
    EXPECT_EQ(ECLP_RELEASE, d.lock_left);
    EXPECT_EQ(ECLP_RELEASE, d.lock_right);
}

/**
 * @test        UT-125-01
 * @requirement SWR-125
 * @brief       [목적] DEGRADED에서 운전자 요청이 적용되지 않고 직전 출력이 유지되며 이유가 DEGRADED인지 확인한다.
 * @input       직전 LOCK/LOCK, 상태 DEGRADED, 해제 요청 ALL, 접근위험 없음
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. DEGRADED에서 판정한다.
 * @expected    출력 LOCK/LOCK 유지, 요청 HELD_DEGRADED, 이유 DEGRADED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, DegradedHoldsRequests)
{
    eclp_arb_t arb = BothLocked();
    const eclp_decision_t d =
        Decide(&arb, WithReq(AllValid(), ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK), ECLP_STATE_DEGRADED);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
    EXPECT_EQ(ECLP_OUT_HELD_DEGRADED, d.outcome);
    EXPECT_EQ(ECLP_PR_DEGRADED, d.priority_reason);
}

/**
 * @test        UT-106-02
 * @requirement SWR-106
 * @brief       [목적] 값이 잘못된 요청이 정상 상태에서 거절되고 출력이 변하지 않으며, DEGRADED에서는 보류로 처리되는지 확인한다.
 * @input       요청 상태 INVALID (상태 NORMAL, DEGRADED)
 * @procedure   1. 직전 LOCK/LOCK에서 판정한다. 2. 출력과 이유를 확인한다.
 * @expected    NORMAL: REJECTED, 출력 유지. DEGRADED: HELD_DEGRADED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, InvalidRequestIsRejected)
{
    eclp_arb_t arb = BothLocked();
    eclp_validated_t v = AllValid();
    v.request = ECLP_REQ_INVALID;
    eclp_decision_t d = Decide(&arb, v);
    EXPECT_EQ(ECLP_OUT_REJECTED, d.outcome);
    EXPECT_EQ(ECLP_PR_REJECTED, d.priority_reason);
    EXPECT_EQ(ECLP_LOCK, d.lock_left);

    d = Decide(&arb, v, ECLP_STATE_DEGRADED);
    EXPECT_EQ(ECLP_OUT_HELD_DEGRADED, d.outcome);
}

/**
 * @test        UT-108-01
 * @requirement SWR-108
 * @brief       [목적] 유효한 요청이 선택한 도어에만 적용되고 이유(동작)가 기록되는지 확인한다.
 * @input       (LEFT|RIGHT|ALL) x (LOCK|UNLOCK)
 * @procedure   1. 직전 반대 상태에서 요청을 판정한다. 2. 두 출력과 driver_action, 이유를 확인한다.
 * @expected    선택한 도어만 요청대로 바뀌고 나머지는 직전 값, 이유 DRIVER
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, RequestAppliesToSelectedDoors)
{
    for (int side = 0; side < 3; ++side) {
        for (int action = 0; action < 2; ++action) {
            eclp_arb_t arb = (action == ECLP_ACTION_LOCK) ? Fresh() : BothLocked();
            const eclp_decision_t d =
                Decide(&arb, WithReq(AllValid(), static_cast<eclp_side_t>(side), static_cast<eclp_action_t>(action)));
            const eclp_lock_t changed = (action == ECLP_ACTION_LOCK) ? ECLP_LOCK : ECLP_RELEASE;
            const eclp_lock_t kept = (action == ECLP_ACTION_LOCK) ? ECLP_RELEASE : ECLP_LOCK;
            const bool left = (side == ECLP_SIDE_LEFT) || (side == ECLP_SIDE_ALL);
            const bool right = (side == ECLP_SIDE_RIGHT) || (side == ECLP_SIDE_ALL);
            EXPECT_EQ(left ? changed : kept, d.lock_left) << side << "/" << action;
            EXPECT_EQ(right ? changed : kept, d.lock_right) << side << "/" << action;
            EXPECT_EQ(ECLP_PR_DRIVER, d.priority_reason);
            EXPECT_EQ(static_cast<eclp_action_t>(action), d.driver_action);
        }
    }
}

/**
 * @test        UT-104-01
 * @requirement SWR-104
 * @brief       [목적] 규칙 충돌 시 상위 규칙이 이기는지 확인한다. (충돌 > sensor_fault > 접근위험 > 운전자 요청)
 * @input       충돌 CONFIRMED + 상태 FAULT, FAULT + 접근위험, 접근위험 + LOCK 요청(반대 도어)
 * @procedure   1. 조합별로 판정한다. 2. 이긴 규칙을 확인한다.
 * @expected    CRASH, SENSOR_FAULT, APPROACH_RISK
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, HigherRuleWins)
{
    eclp_arb_t arb = Fresh();
    eclp_validated_t v = AllValid();
    v.crash_status = ECLP_CRASH_CONFIRMED;
    v.sensor_fault = true;
    EXPECT_EQ(ECLP_PR_CRASH, Decide(&arb, v, ECLP_STATE_FAULT).priority_reason);

    arb = Fresh();
    v = AllValid();
    v.sensor_fault = true;
    v.left_risk = true;
    EXPECT_EQ(ECLP_PR_SENSOR_FAULT, Decide(&arb, v, ECLP_STATE_FAULT).priority_reason);

    arb = Fresh();
    v = WithReq(AllValid(), ECLP_SIDE_RIGHT, ECLP_ACTION_LOCK);
    v.left_risk = true;
    const eclp_decision_t d = Decide(&arb, v);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, d.priority_reason);
    EXPECT_EQ(ECLP_LOCK, d.lock_right);
}

/**
 * @test        UT-104-02
 * @requirement SWR-104
 * @brief       [목적] 우측 접근위험만 있을 때도 접근위험 규칙이 이기고, 어떤 규칙도 해당되지 않으면 NONE인지 확인한다.
 * @input       우측 접근위험만, 중립 입력
 * @procedure   1. 각각 판정한다. 2. 이긴 규칙을 확인한다.
 * @expected    APPROACH_RISK, NONE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Arbiter, RightRiskOnlyAndNone)
{
    eclp_arb_t arb = Fresh();
    eclp_validated_t v = AllValid();
    v.right_risk = true;
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, Decide(&arb, v).priority_reason);

    arb = Fresh();
    EXPECT_EQ(ECLP_PR_NONE, Decide(&arb, AllValid()).priority_reason);
}

}  // namespace
