/**
 * @file    test_controller.cpp
 * @brief   C-CTL 제어기(M-CTL-01, M-CTL-02)의 단위 테스트.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include <cmath>
#include <cstring>
#include <string>

#include "test_support.hpp"

using eclp_test::Eval;
using eclp_test::kStep;
using eclp_test::Neutral;
using eclp_test::WithRequest;

namespace {

/** 초기화된 제어기를 반환한다. */
eclp_t Fresh()
{
    eclp_t ctl;
    EXPECT_EQ(ECLP_OK, eclp_init(&ctl));
    return ctl;
}

/**
 * @test        UT-105-02
 * @requirement SWR-105
 * @brief       [목적] 초기화 직후 두 출력이 RELEASE, 상태가 NORMAL, 이유가 INIT이고 널 포인터가 오류 처리되는지 확인한다.
 * @input       eclp_init(ctl), eclp_init(NULL)
 * @procedure   1. 초기화한다. 2. 최근 결과 필드를 확인한다. 3. 중립 입력으로 첫 평가한다.
 * @expected    RELEASE/RELEASE, NORMAL, INIT, 경고 NONE, 첫 평가 후에도 RELEASE/RELEASE/NORMAL. 널이면 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, InitialState)
{
    eclp_t ctl = Fresh();
    EXPECT_EQ(ECLP_RELEASE, ctl.last.lock_left);
    EXPECT_EQ(ECLP_RELEASE, ctl.last.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, ctl.last.state);
    EXPECT_EQ(ECLP_PR_INIT, ctl.last.priority_reason);
    EXPECT_EQ(ECLP_RC_INIT, ctl.last.reason_code);
    EXPECT_EQ(ECLP_W_NONE, ctl.last.warning_code);
    const eclp_result_t r = Eval(&ctl, Neutral(1.0));
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
    EXPECT_EQ(ECLP_ERR_ARG, eclp_init(nullptr));
}

/**
 * @test        UT-107-01
 * @requirement SWR-107
 * @brief       [목적] 모든 priority_reason이 SRS 부록 A의 reason_code로 매핑되고 DRIVER는 동작에 따라 갈리며 범위 밖과 널은 오류인지 확인한다.
 * @input       priority 0~10, DRIVER + LOCK/UNLOCK, 범위 밖 값(11, -1), 널 출력
 * @procedure   1. 각 값으로 eclp_priority_to_reason을 호출한다. 2. 이유코드를 확인한다.
 * @expected    NONE, INIT, CRASH_RELEASE, CRASH_PENDING_HOLD, SENSOR_FAULT_HOLD, INPUT_DEGRADED_HOLD, APPROACH_RISK_LOCK, RELEASE_SUPPRESSED, DRIVER_LOCK/DRIVER_RELEASE, REQUEST_REJECTED, IGNITION_OFF. 오류는 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, PriorityToReasonMapping)
{
    const eclp_reason_t expected[ECLP_PR_COUNT] = {
        ECLP_RC_NONE, ECLP_RC_INIT, ECLP_RC_CRASH_RELEASE, ECLP_RC_CRASH_PENDING_HOLD,
        ECLP_RC_SENSOR_FAULT_HOLD, ECLP_RC_INPUT_DEGRADED_HOLD, ECLP_RC_APPROACH_RISK_LOCK,
        ECLP_RC_RELEASE_SUPPRESSED, ECLP_RC_DRIVER_LOCK, ECLP_RC_REQUEST_REJECTED, ECLP_RC_IGNITION_OFF};
    for (int p = 0; p < ECLP_PR_COUNT; ++p) {
        eclp_reason_t reason = ECLP_RC_COUNT;
        EXPECT_EQ(ECLP_OK, eclp_priority_to_reason(static_cast<eclp_priority_t>(p), ECLP_ACTION_LOCK, &reason));
        EXPECT_EQ(expected[p], reason) << p;
    }
    eclp_reason_t reason = ECLP_RC_COUNT;
    EXPECT_EQ(ECLP_OK, eclp_priority_to_reason(ECLP_PR_DRIVER, ECLP_ACTION_UNLOCK, &reason));
    EXPECT_EQ(ECLP_RC_DRIVER_RELEASE, reason);
    EXPECT_EQ(ECLP_ERR_ARG, eclp_priority_to_reason(ECLP_PR_COUNT, ECLP_ACTION_LOCK, &reason));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_priority_to_reason(static_cast<eclp_priority_t>(-1), ECLP_ACTION_LOCK, &reason));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_priority_to_reason(ECLP_PR_NONE, ECLP_ACTION_LOCK, nullptr));
}

/**
 * @test        UT-102-01
 * @requirement SWR-102
 * @brief       [목적] 평가 결과가 출력, 상태, 이유, 경고, 입력 유효성 필드를 모두 채우고 최근 결과로 저장되는지 확인한다.
 * @input       속도 입력이 STALE이고 좌측 접근위험 TRUE인 스냅샷
 * @procedure   1. 평가한다. 2. 결과의 모든 필드와 ctl.last를 확인한다.
 * @expected    좌 LOCK/우 RELEASE, DEGRADED, APPROACH_RISK, APPROACH_RISK_LOCK, W_INPUT_STALE, 유효성 STALE/VALID/VALID/VALID, ctl.last와 동일
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, ResultCarriesAllFields)
{
    eclp_t ctl = Fresh();
    eclp_snapshot_t s = Neutral(1.0);
    s.vehicle.source_timestamp_s = 1.0 - 0.25;
    s.approach.rear_left_approach_risk = 1;
    const eclp_result_t r = Eval(&ctl, s);
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_STATE_DEGRADED, r.state);
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, r.priority_reason);
    EXPECT_EQ(ECLP_RC_APPROACH_RISK_LOCK, r.reason_code);
    EXPECT_EQ(ECLP_W_INPUT_STALE, r.warning_code);
    EXPECT_EQ(ECLP_STALE, r.validity_speed);
    EXPECT_EQ(ECLP_VALID, r.validity_crash);
    EXPECT_EQ(ECLP_VALID, r.validity_approach);
    EXPECT_EQ(ECLP_VALID, r.validity_fault);
    EXPECT_EQ(0, std::memcmp(&r, &ctl.last, sizeof(r)));
}

/**
 * @test        UT-102-02
 * @requirement SWR-102
 * @brief       [목적] 평가의 널 포인터 인자가 ECLP_ERR_ARG로 처리되는지 확인한다.
 * @input       ctl, snap, out 중 하나씩 널
 * @procedure   1. 각 널 조합으로 eclp_evaluate를 호출한다. 2. 반환 코드를 확인한다.
 * @expected    모두 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, EvaluateNullArguments)
{
    eclp_t ctl = Fresh();
    const eclp_snapshot_t s = Neutral(1.0);
    eclp_result_t r;
    EXPECT_EQ(ECLP_ERR_ARG, eclp_evaluate(nullptr, &s, &r));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_evaluate(&ctl, nullptr, &r));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_evaluate(&ctl, &s, nullptr));
}

/**
 * @test        UT-111-01
 * @requirement SWR-111
 * @brief       [목적] 평가 호출 한 번이 스냅샷 1건만 처리하고 호출 사이에 직전 출력이 유지되는지 확인한다.
 * @input       스냅샷 1: 좌 LOCK 요청, 스냅샷 2: 요청 없음
 * @procedure   1. 첫 호출로 좌를 LOCK한다. 2. 요청 없는 두 번째 호출을 한다.
 * @expected    호출마다 결과가 1건씩 나오고 두 번째 호출에서 좌 LOCK이 유지됨
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, OneSnapshotPerCall)
{
    eclp_t ctl = Fresh();
    eclp_snapshot_t first = Neutral(1.0);
    WithRequest(first, ECLP_SIDE_LEFT, ECLP_ACTION_LOCK);
    EXPECT_EQ(ECLP_LOCK, Eval(&ctl, first).lock_left);
    const eclp_result_t second = Eval(&ctl, Neutral(1.0 + kStep));
    EXPECT_EQ(ECLP_LOCK, second.lock_left);
    EXPECT_EQ(ECLP_PR_NONE, second.priority_reason);
}

/**
 * @test        UT-103-01
 * @requirement SWR-103
 * @brief       [목적] 같은 입력 열과 주입 시각이면 두 제어기가 같은 결과를 내는지 확인한다. (시스템 시계에 의존하지 않음)
 * @input       고정 시드로 만든 입력 400건을 두 제어기에 같은 순서로 주입
 * @procedure   1. 두 제어기를 초기화한다. 2. 같은 스냅샷을 각각 평가한다. 3. 결과를 비교한다.
 * @expected    모든 주기의 직렬화 결과가 같다
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, SameInputsSameResults)
{
    eclp_t a = Fresh();
    eclp_t b = Fresh();
    std::uint32_t seed = 12345U;
    for (int i = 0; i < 400; ++i) {
        seed = seed * 1664525U + 1013904223U;
        eclp_snapshot_t s = Neutral(1.0 + i * kStep);
        s.crash.crash_status = static_cast<int>((seed >> 8) % 3U);
        s.approach.rear_left_approach_risk = static_cast<int>((seed >> 12) % 2U);
        s.fault.sensor_fault = static_cast<int>((seed >> 16) % 2U);
        if (((seed >> 20) % 5U) == 0U) {
            WithRequest(s, static_cast<int>((seed >> 4) % 3U), static_cast<int>((seed >> 6) % 2U));
        }
        EXPECT_EQ(eclp_test::Line(Eval(&a, s)), eclp_test::Line(Eval(&b, s))) << i;
    }
}

/**
 * @test        UT-114-01
 * @requirement SWR-114
 * @brief       [목적] 유효, STALE, INVALID, 누락, 범위 밖 값이 섞인 어떤 입력에서도 출력이 LOCK 또는 RELEASE이고 모든 필드가 정의된 값인지 확인한다.
 * @input       고정 시드로 만든 입력 5000건(범위 밖 값, NaN, 누락, 오래된 시각 포함)
 * @procedure   1. 스냅샷을 무작위로 만든다. 2. 평가한다. 3. 결과의 모든 enum 필드가 정의 범위인지 확인한다.
 * @expected    모든 평가가 ECLP_OK이고 모든 필드가 정의된 범위 안
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, AnyInputGivesDefinedOutputs)
{
    eclp_t ctl = Fresh();
    std::uint32_t seed = 777U;
    auto next = [&seed]() {
        seed = seed * 1664525U + 1013904223U;
        return seed >> 8;
    };
    const double times[] = {0.0, -0.3, 0.3, std::nan(""), 1.0e13};
    for (int i = 0; i < 5000; ++i) {
        eclp_snapshot_t s = Neutral(1.0 + i * kStep);
        s.vehicle.present = (next() % 8U) != 0U;
        s.vehicle.vehicle_speed_kph = static_cast<double>(next() % 400U) - 50.0;
        s.vehicle.gear = static_cast<int>(next() % 7U) - 1;
        s.vehicle.source_timestamp_s += times[next() % 5U];
        s.crash.present = (next() % 8U) != 0U;
        s.crash.crash_status = static_cast<int>(next() % 6U) - 1;
        s.crash.source_timestamp_s += times[next() % 5U];
        s.approach.present = (next() % 8U) != 0U;
        s.approach.rear_left_approach_risk = static_cast<int>(next() % 4U) - 1;
        s.approach.rear_right_approach_risk = static_cast<int>(next() % 4U) - 1;
        s.fault.present = (next() % 8U) != 0U;
        s.fault.ignition_on = static_cast<int>(next() % 4U) - 1;
        s.fault.sensor_fault = static_cast<int>(next() % 4U) - 1;
        s.fault.source_timestamp_s += times[next() % 5U];
        s.request.present = (next() % 3U) == 0U;
        s.request.side = static_cast<int>(next() % 5U) - 1;
        s.request.action = static_cast<int>(next() % 4U) - 1;
        s.request.source = static_cast<int>(next() % 6U) - 1;
        eclp_status_t st = ECLP_ERR_ARG;
        const eclp_result_t r = Eval(&ctl, s, &st);
        ASSERT_EQ(ECLP_OK, st) << i;
        ASSERT_TRUE(r.lock_left == ECLP_LOCK || r.lock_left == ECLP_RELEASE) << i;
        ASSERT_TRUE(r.lock_right == ECLP_LOCK || r.lock_right == ECLP_RELEASE) << i;
        ASSERT_LT(static_cast<unsigned>(r.state), static_cast<unsigned>(ECLP_STATE_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.priority_reason), static_cast<unsigned>(ECLP_PR_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.reason_code), static_cast<unsigned>(ECLP_RC_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.warning_code), static_cast<unsigned>(ECLP_W_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.validity_speed), static_cast<unsigned>(ECLP_VALIDITY_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.validity_crash), static_cast<unsigned>(ECLP_VALIDITY_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.validity_approach), static_cast<unsigned>(ECLP_VALIDITY_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.validity_fault), static_cast<unsigned>(ECLP_VALIDITY_COUNT)) << i;
    }
}

/**
 * @test        UT-016-01
 * @requirement SWR-016
 * @brief       [목적] 모든 enum 이름이 정의대로 반환되고 범위 밖 값은 UNKNOWN인지 확인한다.
 * @input       잠금, 상태, 이유코드, 경고코드, 유효성의 모든 값과 범위 밖 값(COUNT, -1)
 * @procedure   1. 각 이름 함수를 호출한다. 2. 문자열을 확인한다.
 * @expected    부록 A의 이름과 일치, 범위 밖은 "UNKNOWN"
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, EnumNames)
{
    EXPECT_STREQ("RELEASE", eclp_name_lock(ECLP_RELEASE));
    EXPECT_STREQ("LOCK", eclp_name_lock(ECLP_LOCK));
    EXPECT_STREQ("UNKNOWN", eclp_name_lock(ECLP_LOCK_COUNT));

    const char *states[] = {"NORMAL", "DEGRADED", "FAULT", "OFF"};
    for (int i = 0; i < ECLP_STATE_COUNT; ++i) {
        EXPECT_STREQ(states[i], eclp_name_state(static_cast<eclp_state_t>(i)));
    }
    EXPECT_STREQ("UNKNOWN", eclp_name_state(ECLP_STATE_COUNT));

    const char *reasons[] = {"NONE", "INIT", "CRASH_RELEASE", "CRASH_PENDING_HOLD", "SENSOR_FAULT_HOLD",
                             "INPUT_DEGRADED_HOLD", "APPROACH_RISK_LOCK", "RELEASE_SUPPRESSED", "DRIVER_LOCK",
                             "DRIVER_RELEASE", "REQUEST_REJECTED", "IGNITION_OFF"};
    for (int i = 0; i < ECLP_RC_COUNT; ++i) {
        EXPECT_STREQ(reasons[i], eclp_name_reason(static_cast<eclp_reason_t>(i)));
    }
    EXPECT_STREQ("UNKNOWN", eclp_name_reason(ECLP_RC_COUNT));

    const char *warnings[] = {"NONE", "W_INPUT_STALE", "W_INPUT_INVALID", "W_SENSOR_FAULT"};
    for (int i = 0; i < ECLP_W_COUNT; ++i) {
        EXPECT_STREQ(warnings[i], eclp_name_warning(static_cast<eclp_warning_t>(i)));
    }
    EXPECT_STREQ("UNKNOWN", eclp_name_warning(static_cast<eclp_warning_t>(-1)));

    EXPECT_STREQ("VALID", eclp_name_validity(ECLP_VALID));
    EXPECT_STREQ("STALE", eclp_name_validity(ECLP_STALE));
    EXPECT_STREQ("INVALID", eclp_name_validity(ECLP_INVALID));
    EXPECT_STREQ("UNKNOWN", eclp_name_validity(ECLP_VALIDITY_COUNT));
}

/**
 * @test        UT-016-02
 * @requirement SWR-016
 * @brief       [목적] 결과가 SRS 부록 B의 순서와 구분자로 직렬화되고 버퍼 크기 경계와 널 인자가 정의대로 처리되는지 확인한다.
 * @input       좌 LOCK/우 RELEASE/DEGRADED/INPUT_DEGRADED_HOLD/W_INPUT_STALE/VALID,STALE,INVALID,VALID 결과. 버퍼 크기 길이, 길이+1
 * @procedure   1. 직렬화한다. 2. 문자열과 길이를 확인한다. 3. 버퍼 크기 경계와 널 인자로 호출한다.
 * @expected    "LOCK,RELEASE,DEGRADED,INPUT_DEGRADED_HOLD,W_INPUT_STALE,VALID,STALE,INVALID,VALID\n". 크기=길이이면 ECLP_ERR_RANGE, 길이+1이면 OK, 널이면 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Controller, SerializeFormatAndBounds)
{
    eclp_result_t r;
    std::memset(&r, 0, sizeof(r));
    r.lock_left = ECLP_LOCK;
    r.lock_right = ECLP_RELEASE;
    r.state = ECLP_STATE_DEGRADED;
    r.reason_code = ECLP_RC_INPUT_DEGRADED_HOLD;
    r.warning_code = ECLP_W_INPUT_STALE;
    r.validity_speed = ECLP_VALID;
    r.validity_crash = ECLP_STALE;
    r.validity_approach = ECLP_INVALID;
    r.validity_fault = ECLP_VALID;
    const std::string expected = "LOCK,RELEASE,DEGRADED,INPUT_DEGRADED_HOLD,W_INPUT_STALE,VALID,STALE,INVALID,VALID\n";

    char buf[128];
    size_t len = 0;
    ASSERT_EQ(ECLP_OK, eclp_serialize(&r, buf, sizeof(buf), &len));
    EXPECT_EQ(expected, std::string(buf, len));
    EXPECT_EQ(expected.size(), len);

    EXPECT_EQ(ECLP_ERR_RANGE, eclp_serialize(&r, buf, expected.size(), &len));
    EXPECT_EQ(ECLP_OK, eclp_serialize(&r, buf, expected.size() + 1U, &len));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_serialize(nullptr, buf, sizeof(buf), &len));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_serialize(&r, nullptr, sizeof(buf), &len));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_serialize(&r, buf, sizeof(buf), nullptr));
}

}  // namespace
