/**
 * @file    test_tdd.cpp
 * @brief   구현을 이끈 TDD 테스트. 단위 테스트(tests/unit)와 분리하며 별도 결과와 커버리지로 보고한다.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include "test_support.hpp"

using eclp_test::Eval;
using eclp_test::Neutral;
using eclp_test::WithRequest;
using eclp_test::kStep;

namespace {

/**
 * @brief   TD-001 초기화 직후 결과가 RELEASE/NORMAL/INIT 이다.
 * @tdd     TD-001
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(TddEclp, InitialResultIsReleaseNormal)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    EXPECT_EQ(ECLP_RELEASE, ctl.last.lock_left);
    EXPECT_EQ(ECLP_RELEASE, ctl.last.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, ctl.last.state);
    EXPECT_EQ(ECLP_RC_INIT, ctl.last.reason_code);
}

/**
 * @brief   TD-002 중립 입력의 평가는 출력이 변하지 않고 NORMAL 이다.
 * @tdd     TD-002
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(TddEclp, NeutralInputKeepsOutputs)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    const eclp_result_t r = Eval(&ctl, Neutral(1.0));
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
    EXPECT_EQ(ECLP_RC_NONE, r.reason_code);
}

/**
 * @brief   TD-003 충돌 CONFIRMED 는 먼저 LOCK 한 출력도 같은 주기에 RELEASE 로 만든다.
 * @tdd     TD-003
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(TddEclp, CrashConfirmedReleasesLockedOutputs)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    eclp_snapshot_t lock = Neutral(1.0);
    WithRequest(lock, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    ASSERT_EQ(ECLP_LOCK, Eval(&ctl, lock).lock_left);

    eclp_snapshot_t crash = Neutral(1.0 + kStep);
    crash.crash.crash_status = ECLP_CRASH_CONFIRMED;
    const eclp_result_t r = Eval(&ctl, crash);
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_RC_CRASH_RELEASE, r.reason_code);
}

/**
 * @brief   TD-004 왼쪽 접근위험은 왼쪽 출력만 LOCK 으로 만든다.
 * @tdd     TD-004
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(TddEclp, LeftApproachRiskLocksLeftOnly)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    eclp_snapshot_t s = Neutral(1.0);
    s.approach.rear_left_approach_risk = 1;
    const eclp_result_t r = Eval(&ctl, s);
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_RC_APPROACH_RISK_LOCK, r.reason_code);
}

/**
 * @brief   TD-005 갱신된 지 201 ms 지난 입력이 있으면 DEGRADED 가 된다.
 * @tdd     TD-005
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(TddEclp, StaleInputBecomesDegraded)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    eclp_snapshot_t s = Neutral(1.0);
    s.vehicle.source_timestamp_s = 1.0 - 0.201;
    const eclp_result_t r = Eval(&ctl, s);
    EXPECT_EQ(ECLP_STATE_DEGRADED, r.state);
    EXPECT_EQ(ECLP_STALE, r.validity_speed);
    EXPECT_EQ(ECLP_W_INPUT_STALE, r.warning_code);
}

/**
 * @brief   TD-006 sensor_fault 가 TRUE 이면 직전 출력을 유지하고 FAULT 가 된다.
 * @tdd     TD-006
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(TddEclp, SensorFaultHoldsPreviousOutputs)
{
    eclp_t ctl;
    ASSERT_EQ(ECLP_OK, eclp_init(&ctl));
    eclp_snapshot_t lock = Neutral(1.0);
    WithRequest(lock, ECLP_SIDE_LEFT, ECLP_ACTION_LOCK);
    ASSERT_EQ(ECLP_LOCK, Eval(&ctl, lock).lock_left);

    eclp_snapshot_t fault = Neutral(1.0 + kStep);
    fault.fault.sensor_fault = 1;
    WithRequest(fault, ECLP_SIDE_LEFT, ECLP_ACTION_UNLOCK);
    const eclp_result_t r = Eval(&ctl, fault);
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_STATE_FAULT, r.state);
    EXPECT_EQ(ECLP_RC_SENSOR_FAULT_HOLD, r.reason_code);
}

}  // namespace
