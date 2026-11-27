/**
 * @file    test_integration.cpp
 * @brief   컴포넌트 인터페이스(IF-VAL, IF-STA, IF-ARB, IF-CTL) 사이의 연동을 검증하는 통합 테스트.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include <string>

#include "test_support.hpp"

extern "C" {
#include "arbiter.h"
#include "input_validator.h"
#include "state_manager.h"
}

using eclp_test::Eval;
using eclp_test::kStep;
using eclp_test::Neutral;
using eclp_test::WithRequest;

namespace {

/** 컴포넌트 인터페이스를 순서대로 직접 호출한 한 주기의 결과. */
struct Pipe {
    eclp_validated_t v;
    eclp_state_t state;
    eclp_warning_t warning;
    eclp_decision_t d;
};

/** IF-VAL -> IF-STA -> IF-ARB 순서로 한 주기를 수행한다. */
Pipe RunPipe(eclp_arb_t *arb, const eclp_snapshot_t &s)
{
    Pipe p{};
    EXPECT_EQ(ECLP_OK, eclp_val_validate(&s, &p.v));
    EXPECT_EQ(ECLP_OK, eclp_sta_evaluate(&p.v, &p.state, &p.warning));
    EXPECT_EQ(ECLP_OK, eclp_arb_decide(arb, &p.v, p.state, &p.d));
    return p;
}

/** 초기화한 판정기를 반환한다. */
eclp_arb_t NewArb()
{
    eclp_arb_t arb;
    EXPECT_EQ(ECLP_OK, eclp_arb_init(&arb));
    return arb;
}

/**
 * @test        IT-101-01
 * @requirement SWR-013, SWR-101, SWR-122, SWR-123, SWR-126
 * @brief       [목적] IF-VAL의 유효성 결과가 IF-STA에서 상태와 경고코드로 올바르게 해석되는지 확인한다.
 * @interface   IF-VAL -> IF-STA
 * @input       속도 STALE(0.201 s), 충돌 값 3(INVALID), 복구한 정상 입력
 * @procedure   1. 입력별로 eclp_val_validate 결과를 eclp_sta_evaluate에 전달한다. 2. 상태와 경고를 확인한다.
 * @expected    STALE: DEGRADED/W_INPUT_STALE, INVALID: DEGRADED/W_INPUT_INVALID, 정상: NORMAL/NONE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, ValidatorFeedsStateManager)
{
    eclp_arb_t arb = NewArb();
    eclp_snapshot_t stale = Neutral(2.0);
    stale.vehicle.source_timestamp_s = 2.0 - 0.201;
    Pipe p = RunPipe(&arb, stale);
    EXPECT_EQ(ECLP_STALE, p.v.speed);
    EXPECT_EQ(ECLP_STATE_DEGRADED, p.state);
    EXPECT_EQ(ECLP_W_INPUT_STALE, p.warning);

    eclp_snapshot_t bad = Neutral(2.0 + kStep);
    bad.crash.crash_status = 3;
    p = RunPipe(&arb, bad);
    EXPECT_EQ(ECLP_INVALID, p.v.crash);
    EXPECT_EQ(ECLP_STATE_DEGRADED, p.state);
    EXPECT_EQ(ECLP_W_INPUT_INVALID, p.warning);

    p = RunPipe(&arb, Neutral(2.0 + 2 * kStep));
    EXPECT_EQ(ECLP_STATE_NORMAL, p.state);
    EXPECT_EQ(ECLP_W_NONE, p.warning);
}

/**
 * @test        IT-021-01
 * @requirement SWR-021, SWR-124
 * @brief       [목적] sensor_fault가 IF-VAL, IF-STA를 거쳐 IF-ARB에서 직전 출력 유지로 이어지고, 같은 주기의 충돌 CONFIRMED가 이기는지 확인한다.
 * @interface   IF-VAL -> IF-STA -> IF-ARB
 * @input       좌 LOCK 후 sensor_fault TRUE + 해제 요청, 이어서 sensor_fault + 충돌 CONFIRMED
 * @procedure   1. 좌 LOCK을 만든다. 2. FAULT 입력을 주입한다. 3. 충돌을 추가한다.
 * @expected    FAULT/W_SENSOR_FAULT, 출력 유지(SENSOR_FAULT), 충돌 시 RELEASE(CRASH)
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, SensorFaultReachesArbiter)
{
    eclp_arb_t arb = NewArb();
    eclp_snapshot_t lock = Neutral(1.0);
    WithRequest(lock, ECLP_SIDE_LEFT, ECLP_ACTION_LOCK);
    ASSERT_EQ(ECLP_LOCK, RunPipe(&arb, lock).d.lock_left);

    eclp_snapshot_t fault = Neutral(1.0 + kStep);
    fault.fault.sensor_fault = 1;
    WithRequest(fault, ECLP_SIDE_LEFT, ECLP_ACTION_UNLOCK);
    Pipe p = RunPipe(&arb, fault);
    EXPECT_EQ(ECLP_STATE_FAULT, p.state);
    EXPECT_EQ(ECLP_W_SENSOR_FAULT, p.warning);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_left);
    EXPECT_EQ(ECLP_PR_SENSOR_FAULT, p.d.priority_reason);

    fault.now_s = 1.0 + 2 * kStep;
    fault.vehicle.source_timestamp_s = fault.now_s;
    fault.crash.source_timestamp_s = fault.now_s;
    fault.approach.source_timestamp_s = fault.now_s;
    fault.fault.source_timestamp_s = fault.now_s;
    fault.crash.crash_status = ECLP_CRASH_CONFIRMED;
    p = RunPipe(&arb, fault);
    EXPECT_EQ(ECLP_RELEASE, p.d.lock_left);
    EXPECT_EQ(ECLP_PR_CRASH, p.d.priority_reason);
}

/**
 * @test        IT-007-01
 * @requirement SWR-007, SWR-120
 * @brief       [목적] 충돌 CONFIRMED의 해제와 래치가 여러 주기에 걸쳐 IF-VAL/IF-ARB 사이에서 유지되고 ignition OFF로 풀리는지 확인한다.
 * @interface   IF-VAL -> IF-STA -> IF-ARB
 * @input       주기1 LOCK 요청, 주기2 충돌 CONFIRMED, 주기3 충돌 NONE + LOCK 요청, 주기4 ignition OFF, 주기5 LOCK 요청
 * @procedure   1. 주기별 스냅샷을 순서대로 판정한다. 2. 출력과 이유를 확인한다.
 * @expected    주기2~3 RELEASE(CRASH), 주기4 래치 해제, 주기5 LOCK 적용(DRIVER)
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, CrashLatchAcrossCycles)
{
    eclp_arb_t arb = NewArb();

    eclp_snapshot_t s1 = Neutral(1.0);
    WithRequest(s1, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    EXPECT_EQ(ECLP_LOCK, RunPipe(&arb, s1).d.lock_left);

    eclp_snapshot_t s2 = Neutral(1.0 + kStep);
    s2.crash.crash_status = ECLP_CRASH_CONFIRMED;
    Pipe p = RunPipe(&arb, s2);
    EXPECT_EQ(ECLP_RELEASE, p.d.lock_left);
    EXPECT_EQ(ECLP_PR_CRASH, p.d.priority_reason);

    eclp_snapshot_t s3 = Neutral(1.0 + 2 * kStep);
    WithRequest(s3, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    p = RunPipe(&arb, s3);
    EXPECT_EQ(ECLP_RELEASE, p.d.lock_right);
    EXPECT_EQ(ECLP_PR_CRASH, p.d.priority_reason);

    eclp_snapshot_t s4 = Neutral(1.0 + 3 * kStep);
    s4.fault.ignition_on = 0;
    p = RunPipe(&arb, s4);
    EXPECT_FALSE(arb.crash_latched);

    eclp_snapshot_t s5 = Neutral(1.0 + 4 * kStep);
    WithRequest(s5, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    p = RunPipe(&arb, s5);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_left);
    EXPECT_EQ(ECLP_PR_DRIVER, p.d.priority_reason);
}

/**
 * @test        IT-008-01
 * @requirement SWR-008
 * @brief       [목적] 충돌 PENDING이 IF-VAL의 정규화 값으로 IF-ARB에 전달되어 요청이 보류되고 출력이 유지되는지 확인한다.
 * @interface   IF-VAL -> IF-ARB
 * @input       좌우 LOCK 후 충돌 PENDING + 해제 요청 ALL
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. PENDING과 해제 요청을 주입한다.
 * @expected    LOCK/LOCK 유지, 요청 HELD_PENDING, 이유 CRASH_PENDING
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, CrashPendingHoldsRequest)
{
    eclp_arb_t arb = NewArb();
    eclp_snapshot_t lock = Neutral(1.0);
    WithRequest(lock, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    RunPipe(&arb, lock);

    eclp_snapshot_t pending = Neutral(1.0 + kStep);
    pending.crash.crash_status = ECLP_CRASH_PENDING;
    WithRequest(pending, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    const Pipe p = RunPipe(&arb, pending);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_left);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_right);
    EXPECT_EQ(ECLP_OUT_HELD_PENDING, p.d.outcome);
    EXPECT_EQ(ECLP_PR_CRASH_PENDING, p.d.priority_reason);
}

/**
 * @test        IT-005-01
 * @requirement SWR-005, SWR-009, SWR-121
 * @brief       [목적] 접근위험이 IF-VAL을 거쳐 IF-ARB에서 도어별 LOCK과 해제 요청 억제로 이어지는지 확인한다.
 * @interface   IF-VAL -> IF-ARB
 * @input       좌측 접근위험 TRUE + 해제 요청 ALL (직전 LOCK/LOCK)
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. 접근위험과 해제 요청을 주입한다.
 * @expected    좌 LOCK 유지, 우 RELEASE, 이유 APPROACH_RISK_SUPPRESS
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, ApproachRiskSuppressesReleasePerDoor)
{
    eclp_arb_t arb = NewArb();
    eclp_snapshot_t lock = Neutral(1.0);
    WithRequest(lock, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    RunPipe(&arb, lock);

    eclp_snapshot_t risk = Neutral(1.0 + kStep);
    risk.approach.rear_left_approach_risk = 1;
    WithRequest(risk, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    const Pipe p = RunPipe(&arb, risk);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_left);
    EXPECT_EQ(ECLP_RELEASE, p.d.lock_right);
    EXPECT_TRUE(p.d.suppressed);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK_SUPPRESS, p.d.priority_reason);
}

/**
 * @test        IT-125-01
 * @requirement SWR-125
 * @brief       [목적] DEGRADED 상태가 IF-STA에서 IF-ARB로 전달되어 요청이 보류되고 유효한 접근위험은 적용되며, 복구 후 요청이 다시 적용되는지 확인한다.
 * @interface   IF-STA -> IF-ARB
 * @input       속도 STALE + 해제 요청, 속도 STALE + 우측 접근위험, 복구 후 해제 요청
 * @procedure   1. 두 도어를 LOCK으로 만든다. 2. 각 주기를 판정한다.
 * @expected    보류(LOCK 유지, DEGRADED), 우 LOCK(APPROACH_RISK), 복구 후 RELEASE 적용(DRIVER)
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, DegradedHoldsThenRecovers)
{
    eclp_arb_t arb = NewArb();
    eclp_snapshot_t lock = Neutral(3.0);
    WithRequest(lock, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    RunPipe(&arb, lock);

    eclp_snapshot_t held = Neutral(3.0 + kStep);
    held.vehicle.source_timestamp_s = 3.0 - 0.3;
    WithRequest(held, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    Pipe p = RunPipe(&arb, held);
    EXPECT_EQ(ECLP_STATE_DEGRADED, p.state);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_left);
    EXPECT_EQ(ECLP_PR_DEGRADED, p.d.priority_reason);

    eclp_snapshot_t risk = Neutral(3.0 + 2 * kStep);
    risk.vehicle.source_timestamp_s = 3.0 - 0.3;
    risk.approach.rear_right_approach_risk = 1;
    p = RunPipe(&arb, risk);
    EXPECT_EQ(ECLP_LOCK, p.d.lock_right);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, p.d.priority_reason);

    eclp_snapshot_t ok = Neutral(3.0 + 3 * kStep);
    WithRequest(ok, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    p = RunPipe(&arb, ok);
    EXPECT_EQ(ECLP_STATE_NORMAL, p.state);
    EXPECT_EQ(ECLP_RELEASE, p.d.lock_left);
    EXPECT_EQ(ECLP_PR_DRIVER, p.d.priority_reason);
}

/**
 * @test        IT-106-01
 * @requirement SWR-106, SWR-108
 * @brief       [목적] 등록된 4개 경로의 요청이 IF-VAL을 거쳐 IF-ARB에서 적용되고 등록되지 않은 경로는 거절되는지 확인한다.
 * @interface   IF-VAL -> IF-ARB
 * @input       source 0~3의 LOCK/UNLOCK 요청, source 9
 * @procedure   1. 경로별로 요청을 주입한다. 2. 출력과 처리 결과를 확인한다.
 * @expected    등록 경로는 APPLIED(DRIVER), 경로 9는 REJECTED(출력 유지)
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, AllRegisteredSourcesAreApplied)
{
    for (int source = 0; source < 4; ++source) {
        eclp_arb_t arb = NewArb();
        eclp_snapshot_t lock = Neutral(1.0);
        WithRequest(lock, ECLP_SIDE_ALL, ECLP_ACTION_LOCK, source);
        Pipe p = RunPipe(&arb, lock);
        EXPECT_EQ(ECLP_OUT_APPLIED, p.d.outcome) << source;
        EXPECT_EQ(ECLP_LOCK, p.d.lock_left) << source;

        eclp_snapshot_t unlock = Neutral(1.0 + kStep);
        WithRequest(unlock, ECLP_SIDE_LEFT, ECLP_ACTION_UNLOCK, source);
        p = RunPipe(&arb, unlock);
        EXPECT_EQ(ECLP_RELEASE, p.d.lock_left) << source;
        EXPECT_EQ(ECLP_LOCK, p.d.lock_right) << source;
    }
    eclp_arb_t arb = NewArb();
    eclp_snapshot_t bad = Neutral(1.0);
    WithRequest(bad, ECLP_SIDE_ALL, ECLP_ACTION_LOCK, 9);
    const Pipe p = RunPipe(&arb, bad);
    EXPECT_EQ(ECLP_OUT_REJECTED, p.d.outcome);
    EXPECT_EQ(ECLP_RELEASE, p.d.lock_left);
}

/**
 * @test        IT-104-01
 * @requirement SWR-104
 * @brief       [목적] 전체 파이프라인에서 규칙 우선순위(충돌 > sensor_fault > 접근위험 > 운전자 요청)가 지켜지는지 확인한다.
 * @interface   IF-VAL -> IF-STA -> IF-ARB
 * @input       충돌 + sensor_fault + 접근위험 + 요청, sensor_fault + 접근위험 + 요청, 접근위험 + 요청
 * @procedure   1. 새 판정기로 조합별로 판정한다. 2. 이긴 규칙을 확인한다.
 * @expected    CRASH, SENSOR_FAULT, APPROACH_RISK
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, PriorityOrderThroughPipeline)
{
    eclp_snapshot_t s = Neutral(1.0);
    s.approach.rear_left_approach_risk = 1;
    WithRequest(s, ECLP_SIDE_RIGHT, ECLP_ACTION_LOCK);

    eclp_arb_t arb = NewArb();
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, RunPipe(&arb, s).d.priority_reason);

    s.fault.sensor_fault = 1;
    arb = NewArb();
    EXPECT_EQ(ECLP_PR_SENSOR_FAULT, RunPipe(&arb, s).d.priority_reason);

    s.crash.crash_status = ECLP_CRASH_CONFIRMED;
    arb = NewArb();
    EXPECT_EQ(ECLP_PR_CRASH, RunPipe(&arb, s).d.priority_reason);
}

/**
 * @test        IT-102-01
 * @requirement SWR-102, SWR-107, SWR-111
 * @brief       [목적] IF-CTL이 IF-VAL/IF-STA/IF-ARB의 결과를 필드 손실 없이 조립하고 이유코드를 매핑하며 직렬화하는지 확인한다.
 * @interface   IF-CTL <-> IF-VAL, IF-STA, IF-ARB
 * @input       접근위험, 요청 보류, 충돌, FAULT를 포함한 10주기 시나리오
 * @procedure   1. 같은 시나리오를 컴포넌트 인터페이스 직접 호출과 IF-CTL로 각각 수행한다. 2. 결과 필드와 직렬화를 비교한다.
 * @expected    모든 주기에서 출력, 상태, 경고, 유효성, 이유코드가 같고 직렬화가 성공한다
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, ControllerMatchesComponentPipeline)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    eclp_arb_t arb = NewArb();
    for (int i = 0; i < 10; ++i) {
        eclp_snapshot_t s = Neutral(1.0 + i * kStep);
        if (i == 1) {
            WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
        } else if (i == 2) {
            s.approach.rear_left_approach_risk = 1;
        } else if (i == 4) {
            s.vehicle.source_timestamp_s = s.now_s - 0.3;
        } else if (i == 6) {
            s.crash.crash_status = ECLP_CRASH_PENDING;
        } else if (i == 8) {
            s.fault.sensor_fault = 1;
        }
        const Pipe p = RunPipe(&arb, s);
        eclp_reason_t reason = ECLP_RC_COUNT;
        ASSERT_EQ(ECLP_OK, eclp_priority_to_reason(p.d.priority_reason, p.d.driver_action, &reason));
        const eclp_result_t r = Eval(&ctl, s);
        EXPECT_EQ(p.d.lock_left, r.lock_left) << i;
        EXPECT_EQ(p.d.lock_right, r.lock_right) << i;
        EXPECT_EQ(p.state, r.state) << i;
        EXPECT_EQ(p.warning, r.warning_code) << i;
        EXPECT_EQ(reason, r.reason_code) << i;
        EXPECT_EQ(p.v.speed, r.validity_speed) << i;
        EXPECT_EQ(p.v.fault, r.validity_fault) << i;
        char buf[256];
        size_t len = 0;
        EXPECT_EQ(ECLP_OK, eclp_serialize(&r, buf, sizeof(buf), &len)) << i;
        EXPECT_GT(len, 0U);
    }
}

/**
 * @test        IT-105-01
 * @requirement SWR-105
 * @brief       [목적] IF-CTL의 초기화가 IF-STA, IF-ARB의 초기값과 일치하는지 확인한다.
 * @interface   IF-CTL <-> IF-STA, IF-ARB
 * @input       eclp_init 직후
 * @procedure   1. 제어기를 초기화한다. 2. IF-STA, IF-ARB의 초기값과 비교한다.
 * @expected    상태 NORMAL, 두 출력 RELEASE, 래치 해제
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, ControllerInitMatchesComponents)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    eclp_state_t state = ECLP_STATE_FAULT;
    ASSERT_EQ(ECLP_OK, eclp_sta_initial(&state));
    eclp_arb_t arb = NewArb();
    EXPECT_EQ(state, ctl.last.state);
    EXPECT_EQ(arb.lock_left, ctl.arb.lock_left);
    EXPECT_EQ(arb.lock_right, ctl.arb.lock_right);
    EXPECT_EQ(arb.crash_latched, ctl.arb.crash_latched);
}

/**
 * @test        IT-103-01
 * @requirement SWR-103
 * @brief       [목적] 평가가 호출자가 주입한 시각만 사용하므로 절대 시각을 옮겨도 같은 상대 시나리오가 같은 결과를 내는지 확인한다.
 * @interface   IF-CTL (now_s 입력)
 * @input       시작 시각 1.0 s와 500000.0 s에서 같은 상대 시각의 시나리오
 * @procedure   1. 두 제어기에 같은 상대 입력 열을 주입한다. 2. 직렬화 결과를 비교한다.
 * @expected    모든 주기의 결과가 같다
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, ResultsDependOnlyOnInjectedTime)
{
    eclp_t a;
    eclp_t b;
    ASSERT_EQ(ECLP_OK, eclp_init(&a));
    ASSERT_EQ(ECLP_OK, eclp_init(&b));
    for (int i = 0; i < 60; ++i) {
        eclp_snapshot_t sa = Neutral(1.0 + i * kStep);
        eclp_snapshot_t sb = Neutral(500000.0 + i * kStep);
        if (i == 10) {
            sa.vehicle.source_timestamp_s = sa.now_s - 0.25;
            sb.vehicle.source_timestamp_s = sb.now_s - 0.25;
        }
        if (i == 30) {
            WithRequest(sa, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
            WithRequest(sb, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
        }
        EXPECT_EQ(eclp_test::Line(Eval(&a, sa)), eclp_test::Line(Eval(&b, sb))) << i;
    }
}

/**
 * @test        IT-107-01
 * @requirement SWR-107
 * @brief       [목적] 모든 규칙 결과가 IF-CTL을 거쳐 SRS 부록 A의 reason_code로 공개되는지 확인한다.
 * @interface   IF-CTL
 * @input       충돌, PENDING, sensor_fault, DEGRADED, 접근위험, 억제, LOCK 요청, 해제 요청, 거절, 규칙 없음 시나리오
 * @procedure   1. 시나리오별로 새 제어기에서 평가한다. 2. 이유코드를 확인한다.
 * @expected    CRASH_RELEASE, CRASH_PENDING_HOLD, SENSOR_FAULT_HOLD, INPUT_DEGRADED_HOLD, APPROACH_RISK_LOCK, RELEASE_SUPPRESSED, DRIVER_LOCK, DRIVER_RELEASE, REQUEST_REJECTED, NONE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Integration, EveryRuleOutcomeHasReasonCode)
{
    struct Case {
        eclp_reason_t expected;
        std::function<void(eclp_snapshot_t &)> setup;
    };
    const Case cases[] = {
        {ECLP_RC_CRASH_RELEASE, [](eclp_snapshot_t &s) { s.crash.crash_status = ECLP_CRASH_CONFIRMED; }},
        {ECLP_RC_CRASH_PENDING_HOLD, [](eclp_snapshot_t &s) { s.crash.crash_status = ECLP_CRASH_PENDING; }},
        {ECLP_RC_SENSOR_FAULT_HOLD, [](eclp_snapshot_t &s) { s.fault.sensor_fault = 1; }},
        {ECLP_RC_INPUT_DEGRADED_HOLD, [](eclp_snapshot_t &s) { s.vehicle.source_timestamp_s -= 0.3; }},
        {ECLP_RC_APPROACH_RISK_LOCK, [](eclp_snapshot_t &s) { s.approach.rear_right_approach_risk = 1; }},
        {ECLP_RC_RELEASE_SUPPRESSED,
         [](eclp_snapshot_t &s) {
             s.approach.rear_left_approach_risk = 1;
             WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
         }},
        {ECLP_RC_DRIVER_LOCK, [](eclp_snapshot_t &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK); }},
        {ECLP_RC_DRIVER_RELEASE, [](eclp_snapshot_t &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK); }},
        {ECLP_RC_REQUEST_REJECTED, [](eclp_snapshot_t &s) { WithRequest(s, 7, 0); }},
        {ECLP_RC_NONE, [](eclp_snapshot_t &) {}},
    };
    for (const Case &c : cases) {
        eclp_t ctl;
        ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
        eclp_snapshot_t s = Neutral(1.0);
        c.setup(s);
        EXPECT_EQ(c.expected, Eval(&ctl, s).reason_code) << eclp_name_reason(c.expected);
    }
}

}  // namespace
