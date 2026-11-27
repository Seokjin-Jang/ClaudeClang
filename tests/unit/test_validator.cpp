/**
 * @file    test_validator.cpp
 * @brief   C-VAL 입력 검증기(M-VAL-01)의 단위 테스트.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "test_support.hpp"

extern "C" {
#include "input_validator.h"
}

using eclp_test::Neutral;

namespace {

const double kNan = std::numeric_limits<double>::quiet_NaN();
const double kInf = std::numeric_limits<double>::infinity();

/** 스냅샷을 검증하고 결과를 반환한다. */
eclp_validated_t Validate(const eclp_snapshot_t &s)
{
    eclp_validated_t v;
    EXPECT_EQ(ECLP_OK, eclp_val_validate(&s, &v));
    return v;
}

/**
 * @test        UT-013-01
 * @requirement SWR-013
 * @brief       [목적] 초 단위 시각이 마이크로초 정수로 변환되고 경계값과 오류 입력이 정의대로 처리되는지 확인한다.
 * @input       0.0, 1.0, 1.2345678, -1.5, 9.0e12, 9.0000001e12, NaN, +inf, 널 출력 포인터
 * @procedure   1. 각 값으로 eclp_val_time_us를 호출한다. 2. 상태 코드와 변환 결과를 확인한다.
 * @expected    0->0, 1.0->1000000, 1.2345678->1234568, -1.5->-1500000, 9.0e12->OK, 9.0000001e12->ERR_RANGE, NaN/inf->ERR_RANGE, 널->ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, TimeConversion)
{
    int64_t us = -1;
    EXPECT_EQ(ECLP_OK, eclp_val_time_us(0.0, &us));
    EXPECT_EQ(0, us);
    EXPECT_EQ(ECLP_OK, eclp_val_time_us(1.0, &us));
    EXPECT_EQ(1000000, us);
    EXPECT_EQ(ECLP_OK, eclp_val_time_us(1.2345678, &us));
    EXPECT_EQ(1234568, us);
    EXPECT_EQ(ECLP_OK, eclp_val_time_us(-1.5, &us));
    EXPECT_EQ(-1500000, us);
    EXPECT_EQ(ECLP_OK, eclp_val_time_us(9.0e12, &us));
    EXPECT_EQ(ECLP_ERR_RANGE, eclp_val_time_us(9.0000001e12, &us));
    EXPECT_EQ(ECLP_ERR_RANGE, eclp_val_time_us(kNan, &us));
    EXPECT_EQ(ECLP_ERR_RANGE, eclp_val_time_us(kInf, &us));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_val_time_us(1.0, nullptr));
}

/**
 * @test        UT-013-02
 * @requirement SWR-013
 * @brief       [목적] 입력 나이 200000 µs는 신선하고 200001 µs는 STALE이며 미래 시각은 INVALID인지 경계값으로 확인한다.
 * @input       now_s=10.0, source_timestamp_s = 10.0, 9.8, 9.799999, 9.799, 10.000001
 * @procedure   1. 차량 입력의 시각만 바꿔 검증한다. 2. 차량 그룹 유효성을 확인한다.
 * @expected    나이 0->VALID, 200000->VALID, 200001->STALE, 201000->STALE, -1->INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, FreshnessBoundary)
{
    const double ts[] = {10.0, 9.8, 9.799999, 9.799, 10.000001};
    const eclp_validity_t expected[] = {ECLP_VALID, ECLP_VALID, ECLP_STALE, ECLP_STALE, ECLP_INVALID};
    for (size_t i = 0; i < 5; ++i) {
        eclp_snapshot_t s = Neutral(10.0);
        s.vehicle.source_timestamp_s = ts[i];
        EXPECT_EQ(expected[i], Validate(s).speed) << "ts=" << ts[i];
    }
}

/**
 * @test        UT-013-03
 * @requirement SWR-013
 * @requirement SWR-101
 * @brief       [목적] 입력 시각 자체가 변환 불가이면 해당 그룹이 INVALID가 되는지 확인한다.
 * @input       각 그룹의 source_timestamp_s = NaN
 * @procedure   1. 그룹마다 시각을 NaN으로 바꿔 검증한다. 2. 해당 그룹의 유효성을 확인한다.
 * @expected    해당 그룹만 INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, UnconvertibleTimestampIsInvalid)
{
    eclp_snapshot_t s = Neutral(1.0);
    s.vehicle.source_timestamp_s = kNan;
    EXPECT_EQ(ECLP_INVALID, Validate(s).speed);
    s = Neutral(1.0);
    s.crash.source_timestamp_s = kInf;
    EXPECT_EQ(ECLP_INVALID, Validate(s).crash);
    s = Neutral(1.0);
    s.approach.source_timestamp_s = 1.0e13;
    EXPECT_EQ(ECLP_INVALID, Validate(s).approach);
    s = Neutral(1.0);
    s.fault.source_timestamp_s = kNan;
    const eclp_validated_t v = Validate(s);
    EXPECT_EQ(ECLP_INVALID, v.fault);
    EXPECT_EQ(ECLP_VALID, v.speed);
}

/**
 * @test        UT-101-01
 * @requirement SWR-101
 * @brief       [목적] 차속의 경계값(0.0, 300.0)은 유효하고 범위 밖과 NaN/무한대는 INVALID인지 확인한다.
 * @input       속도 0.0, 150.0, 300.0, -0.001, 300.001, NaN, +inf, -inf
 * @procedure   1. 속도만 바꿔 검증한다. 2. 차량 그룹 유효성을 확인한다.
 * @expected    0.0/150.0/300.0->VALID, 나머지->INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, SpeedRange)
{
    const double speeds[] = {0.0, 150.0, 300.0, -0.001, 300.001, kNan, kInf, -kInf};
    for (size_t i = 0; i < 8; ++i) {
        eclp_snapshot_t s = Neutral(1.0);
        s.vehicle.vehicle_speed_kph = speeds[i];
        const eclp_validity_t expected = (i < 3) ? ECLP_VALID : ECLP_INVALID;
        EXPECT_EQ(expected, Validate(s).speed) << "speed=" << speeds[i];
    }
}

/**
 * @test        UT-101-02
 * @requirement SWR-101
 * @brief       [목적] 기어 enum(P/N/D/R)만 유효하고 그 밖의 값은 INVALID인지 확인한다.
 * @input       기어 0, 1, 2, 3, -1, 4
 * @procedure   1. 기어만 바꿔 검증한다. 2. 차량 그룹 유효성을 확인한다.
 * @expected    0~3->VALID, -1/4->INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, GearEnum)
{
    const int gears[] = {0, 1, 2, 3, -1, 4};
    for (size_t i = 0; i < 6; ++i) {
        eclp_snapshot_t s = Neutral(1.0);
        s.vehicle.gear = gears[i];
        EXPECT_EQ((i < 4) ? ECLP_VALID : ECLP_INVALID, Validate(s).speed) << "gear=" << gears[i];
    }
}

/**
 * @test        UT-101-03
 * @requirement SWR-101
 * @requirement SWR-008
 * @brief       [목적] 충돌 enum(NONE/PENDING/CONFIRMED)만 유효하고 값이 정규화되어 복사되는지 확인한다.
 * @input       충돌 상태 0, 1, 2, 3, -1
 * @procedure   1. 충돌 상태만 바꿔 검증한다. 2. 충돌 그룹 유효성과 정규화된 값을 확인한다.
 * @expected    0~2->VALID이고 같은 값으로 복사, 3/-1->INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, CrashEnumAndNormalization)
{
    const int values[] = {0, 1, 2, 3, -1};
    for (size_t i = 0; i < 5; ++i) {
        eclp_snapshot_t s = Neutral(1.0);
        s.crash.crash_status = values[i];
        const eclp_validated_t v = Validate(s);
        if (i < 3) {
            EXPECT_EQ(ECLP_VALID, v.crash);
            EXPECT_EQ(values[i], static_cast<int>(v.crash_status));
        } else {
            EXPECT_EQ(ECLP_INVALID, v.crash);
            EXPECT_EQ(ECLP_CRASH_NONE, v.crash_status);
        }
    }
}

/**
 * @test        UT-101-04
 * @requirement SWR-101
 * @brief       [목적] 접근위험 boolean(0/1) 외의 값이 좌, 우 어느 쪽에 있어도 그룹이 INVALID가 되고 값이 정규화되는지 확인한다.
 * @input       (좌,우) = (0,0), (1,0), (0,1), (1,1), (2,0), (0,2), (-1,1)
 * @procedure   1. 값을 바꿔 검증한다. 2. 접근위험 그룹 유효성과 정규화된 값을 확인한다.
 * @expected    앞의 4개는 VALID이고 좌/우 위험이 입력과 같다, 나머지는 INVALID이고 위험은 FALSE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, ApproachBooleanAndNormalization)
{
    const int pairs[][2] = {{0, 0}, {1, 0}, {0, 1}, {1, 1}, {2, 0}, {0, 2}, {-1, 1}};
    for (size_t i = 0; i < 7; ++i) {
        eclp_snapshot_t s = Neutral(1.0);
        s.approach.rear_left_approach_risk = pairs[i][0];
        s.approach.rear_right_approach_risk = pairs[i][1];
        const eclp_validated_t v = Validate(s);
        if (i < 4) {
            EXPECT_EQ(ECLP_VALID, v.approach);
            EXPECT_EQ(pairs[i][0] == 1, v.left_risk);
            EXPECT_EQ(pairs[i][1] == 1, v.right_risk);
        } else {
            EXPECT_EQ(ECLP_INVALID, v.approach);
            EXPECT_FALSE(v.left_risk);
            EXPECT_FALSE(v.right_risk);
        }
    }
}

/**
 * @test        UT-101-05
 * @requirement SWR-101
 * @brief       [목적] 시동과 고장 boolean 외의 값이 INVALID이고 정규화된 값이 복사되는지 확인한다.
 * @input       (ignition_on, sensor_fault) = (1,0), (0,1), (2,0), (1,2)
 * @procedure   1. 값을 바꿔 검증한다. 2. 고장 그룹 유효성과 정규화된 값을 확인한다.
 * @expected    (1,0), (0,1)->VALID이고 값이 같다, (2,0), (1,2)->INVALID이고 값은 FALSE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, FaultBooleanAndNormalization)
{
    const int pairs[][2] = {{1, 0}, {0, 1}, {2, 0}, {1, 2}};
    for (size_t i = 0; i < 4; ++i) {
        eclp_snapshot_t s = Neutral(1.0);
        s.fault.ignition_on = pairs[i][0];
        s.fault.sensor_fault = pairs[i][1];
        const eclp_validated_t v = Validate(s);
        if (i < 2) {
            EXPECT_EQ(ECLP_VALID, v.fault);
            EXPECT_EQ(pairs[i][0] == 1, v.ignition_on);
            EXPECT_EQ(pairs[i][1] == 1, v.sensor_fault);
        } else {
            EXPECT_EQ(ECLP_INVALID, v.fault);
            EXPECT_FALSE(v.ignition_on);
            EXPECT_FALSE(v.sensor_fault);
        }
    }
}

/**
 * @test        UT-101-06
 * @requirement SWR-101
 * @brief       [목적] 도착하지 않은(누락) 입력 그룹이 INVALID로 처리되는지 확인한다.
 * @input       그룹별 present=false
 * @procedure   1. 그룹 하나씩 누락시켜 검증한다. 2. 해당 그룹만 INVALID인지 확인한다.
 * @expected    누락된 그룹만 INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, MissingGroupIsInvalid)
{
    eclp_snapshot_t s = Neutral(1.0);
    s.vehicle.present = false;
    EXPECT_EQ(ECLP_INVALID, Validate(s).speed);
    s = Neutral(1.0);
    s.crash.present = false;
    EXPECT_EQ(ECLP_INVALID, Validate(s).crash);
    s = Neutral(1.0);
    s.approach.present = false;
    EXPECT_EQ(ECLP_INVALID, Validate(s).approach);
    s = Neutral(1.0);
    s.fault.present = false;
    const eclp_validated_t v = Validate(s);
    EXPECT_EQ(ECLP_INVALID, v.fault);
    EXPECT_EQ(ECLP_VALID, v.crash);
}

/**
 * @test        UT-101-07
 * @requirement SWR-101
 * @requirement SWR-114
 * @brief       [목적] 평가 시각(now_s)을 변환할 수 없으면 모든 그룹이 INVALID이고 요청이 없음으로 정의되는지 확인한다.
 * @input       now_s = NaN, 운전자 요청 있음
 * @procedure   1. 검증한다. 2. 모든 그룹과 요청 상태를 확인한다.
 * @expected    speed/crash/approach/fault 모두 INVALID, 요청 NONE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, InvalidNowInvalidatesEverything)
{
    eclp_snapshot_t s = Neutral(1.0);
    s.now_s = kNan;
    eclp_test::WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    const eclp_validated_t v = Validate(s);
    EXPECT_EQ(ECLP_INVALID, v.speed);
    EXPECT_EQ(ECLP_INVALID, v.crash);
    EXPECT_EQ(ECLP_INVALID, v.approach);
    EXPECT_EQ(ECLP_INVALID, v.fault);
    EXPECT_EQ(ECLP_REQ_NONE, v.request);
}

/**
 * @test        UT-101-08
 * @requirement SWR-101
 * @brief       [목적] 널 포인터 인자가 ECLP_ERR_ARG로 처리되는지 확인한다.
 * @input       snap=NULL, out=NULL
 * @procedure   1. 각 널 조합으로 호출한다. 2. 상태 코드를 확인한다.
 * @expected    모두 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, NullArguments)
{
    const eclp_snapshot_t s = Neutral(1.0);
    eclp_validated_t v;
    EXPECT_EQ(ECLP_ERR_ARG, eclp_val_validate(nullptr, &v));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_val_validate(&s, nullptr));
}

/**
 * @test        UT-106-01
 * @requirement SWR-106
 * @brief       [목적] 운전자 요청의 side/action/source가 등록 집합이면 VALID, 아니면 INVALID, 없으면 NONE인지 확인한다.
 * @input       요청 없음, (0,0,0), (2,1,3), (3,0,0), (0,2,0), (0,0,4), (-1,0,0)
 * @procedure   1. 요청을 바꿔 검증한다. 2. 요청 상태와 정규화된 값을 확인한다.
 * @expected    없음->NONE, 앞의 두 개->VALID(값 복사), 나머지->INVALID
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(Validator, RequestValues)
{
    eclp_snapshot_t s = Neutral(1.0);
    EXPECT_EQ(ECLP_REQ_NONE, Validate(s).request);

    const int cases[][3] = {{0, 0, 0}, {2, 1, 3}, {3, 0, 0}, {0, 2, 0}, {0, 0, 4}, {-1, 0, 0}};
    for (size_t i = 0; i < 6; ++i) {
        s = Neutral(1.0);
        eclp_test::WithRequest(s, cases[i][0], cases[i][1], cases[i][2]);
        const eclp_validated_t v = Validate(s);
        if (i < 2) {
            EXPECT_EQ(ECLP_REQ_VALID, v.request);
            EXPECT_EQ(cases[i][0], static_cast<int>(v.req_side));
            EXPECT_EQ(cases[i][1], static_cast<int>(v.req_action));
        } else {
            EXPECT_EQ(ECLP_REQ_INVALID, v.request);
        }
    }
}

}  // namespace
