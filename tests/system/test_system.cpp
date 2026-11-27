/**
 * @file    test_system.cpp
 * @brief   공개 인터페이스(IF-CTL)만 사용하는 요구사항 기반 시스템 테스트(PC/SIL).
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include <cmath>
#include <string>

#include "test_support.hpp"

using eclp_test::Sim;
using eclp_test::WithRequest;

namespace {

using Snap = eclp_snapshot_t;

/** 두 도어를 LOCK으로 만든 시뮬레이터를 반환한다. */
void LockBoth(Sim *sim)
{
    const eclp_result_t r = sim->Step([](Snap &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK); });
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_LOCK, r.lock_right);
}

/**
 * @test        ST-001
 * @requirement SWR-007
 * @brief       [목적] 충돌 CONFIRMED가 접근위험, LOCK 요청, sensor_fault와 동시에 와도 같은 평가주기에 두 출력이 RELEASE가 되는지 확인한다.
 * @input       두 도어 LOCK 상태에서 충돌 CONFIRMED + 접근위험 좌/우 + LOCK 요청 + sensor_fault
 * @procedure   1. 두 도어를 LOCK한다. 2. 충돌 CONFIRMED와 방해 입력을 같은 주기에 주입한다.
 * @expected    같은 주기에 RELEASE/RELEASE, 이유 CRASH_RELEASE (OEM 상한 300 ms 이내)
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, CrashConfirmedReleasesImmediately)
{
    Sim sim;
    LockBoth(&sim);
    const int before = sim.Cycle();
    const eclp_result_t r = sim.Step([](Snap &s) {
        s.crash.crash_status = ECLP_CRASH_CONFIRMED;
        s.approach.rear_left_approach_risk = 1;
        s.approach.rear_right_approach_risk = 1;
        s.fault.sensor_fault = 1;
        WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
    });
    EXPECT_EQ(before, sim.Cycle() - 1);
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_RC_CRASH_RELEASE, r.reason_code);
}

/**
 * @test        ST-002
 * @requirement SWR-120
 * @brief       [목적] 충돌 해제가 충돌 신호가 사라진 뒤에도 ignition OFF까지 유지되고 이후 정상 동작으로 돌아오는지 확인한다.
 * @input       충돌 CONFIRMED 후 50주기 동안 충돌 NONE + LOCK 요청 + 접근위험, 이어서 ignition OFF, 다시 ON 후 LOCK 요청
 * @procedure   1. 충돌을 주입한다. 2. 50주기 동안 방해 입력을 준다. 3. ignition을 껐다 켠다. 4. LOCK을 요청한다.
 * @expected    50주기 모두 RELEASE(CRASH_RELEASE), ignition OFF 이후 LOCK 요청이 적용됨
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, CrashReleaseHoldsUntilIgnitionOff)
{
    Sim sim;
    sim.Step([](Snap &s) { s.crash.crash_status = ECLP_CRASH_CONFIRMED; });
    for (int i = 0; i < 50; ++i) {
        const eclp_result_t r = sim.Step([](Snap &s) {
            WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK);
            s.approach.rear_left_approach_risk = 1;
        });
        ASSERT_EQ(ECLP_RELEASE, r.lock_left) << i;
        ASSERT_EQ(ECLP_RELEASE, r.lock_right) << i;
        ASSERT_EQ(ECLP_RC_CRASH_RELEASE, r.reason_code) << i;
    }
    sim.Step([](Snap &s) { s.fault.ignition_on = 0; });
    const eclp_result_t after = sim.Step([](Snap &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK); });
    EXPECT_EQ(ECLP_LOCK, after.lock_left);
    EXPECT_EQ(ECLP_LOCK, after.lock_right);
    EXPECT_EQ(ECLP_RC_DRIVER_LOCK, after.reason_code);
}

/**
 * @test        ST-003
 * @requirement SWR-008
 * @brief       [목적] 충돌 PENDING이 출력을 바꾸지 않고 운전자 요청을 보류하며, PENDING이 풀리면 요청이 다시 적용되는지 확인한다.
 * @input       두 도어 LOCK 후 충돌 PENDING + 해제 요청, 이어서 충돌 NONE + 해제 요청
 * @procedure   1. 두 도어를 LOCK한다. 2. PENDING과 해제 요청을 주입한다. 3. PENDING을 해제하고 요청을 다시 준다.
 * @expected    PENDING 동안 LOCK/LOCK 유지(CRASH_PENDING_HOLD), 해제 후 RELEASE 적용
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, CrashPendingHoldsAndResumes)
{
    Sim sim;
    LockBoth(&sim);
    const eclp_result_t held = sim.Step([](Snap &s) {
        s.crash.crash_status = ECLP_CRASH_PENDING;
        WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    });
    EXPECT_EQ(ECLP_LOCK, held.lock_left);
    EXPECT_EQ(ECLP_LOCK, held.lock_right);
    EXPECT_EQ(ECLP_RC_CRASH_PENDING_HOLD, held.reason_code);

    const eclp_result_t resumed = sim.Step([](Snap &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK); });
    EXPECT_EQ(ECLP_RELEASE, resumed.lock_left);
    EXPECT_EQ(ECLP_RELEASE, resumed.lock_right);
}

/**
 * @test        ST-004
 * @requirement SWR-005
 * @brief       [목적] 접근위험 TRUE인 도어가 같은 주기에 LOCK되고 이유가 기록되며 DEGRADED에서도 유효한 접근위험은 적용되는지 확인한다.
 * @input       좌측 접근위험, 우측 접근위험, 속도 STALE + 우측 접근위험
 * @procedure   1. 입력별로 새 시뮬레이터에서 평가한다. 2. 출력과 이유를 확인한다.
 * @expected    위험 도어 LOCK, 이유 APPROACH_RISK_LOCK, DEGRADED에서도 동일
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, ApproachRiskLocksDoor)
{
    Sim left;
    eclp_result_t r = left.Step([](Snap &s) { s.approach.rear_left_approach_risk = 1; });
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_RC_APPROACH_RISK_LOCK, r.reason_code);

    Sim right;
    r = right.Step([](Snap &s) { s.approach.rear_right_approach_risk = 1; });
    EXPECT_EQ(ECLP_LOCK, r.lock_right);

    Sim degraded;
    r = degraded.Step([](Snap &s) {
        s.vehicle.source_timestamp_s = s.now_s - 0.3;
        s.approach.rear_right_approach_risk = 1;
    });
    EXPECT_EQ(ECLP_STATE_DEGRADED, r.state);
    EXPECT_EQ(ECLP_LOCK, r.lock_right);
    EXPECT_EQ(ECLP_RC_APPROACH_RISK_LOCK, r.reason_code);
}

/**
 * @test        ST-005
 * @requirement SWR-121
 * @brief       [목적] 접근위험 도어에 대한 해제 요청이 억제되고 원인이 기록되며 반대쪽 도어는 영향받지 않는지 확인한다.
 * @input       두 도어 LOCK 후 좌측 접근위험 + 해제 요청 ALL
 * @procedure   1. 두 도어를 LOCK한다. 2. 접근위험과 해제 요청을 주입한다.
 * @expected    좌 LOCK 유지, 우 RELEASE, 이유 RELEASE_SUPPRESSED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, ReleaseRequestSuppressedByRisk)
{
    Sim sim;
    LockBoth(&sim);
    const eclp_result_t r = sim.Step([](Snap &s) {
        s.approach.rear_left_approach_risk = 1;
        WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    });
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_RC_RELEASE_SUPPRESSED, r.reason_code);
}

/**
 * @test        ST-006
 * @requirement SWR-009
 * @brief       [목적] 한쪽 도어의 접근위험이 반대쪽 도어의 요청 처리에 영향을 주지 않는지 확인한다.
 * @input       좌측 접근위험 + 우측 LOCK 요청, 이어서 우측 해제 요청
 * @procedure   1. 좌측 위험과 우측 LOCK을 주입한다. 2. 우측 해제 요청을 준다.
 * @expected    좌 LOCK(위험)과 우 LOCK(요청), 이어서 우 RELEASE(요청)이고 좌는 LOCK 유지
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, DoorsAreJudgedIndependently)
{
    Sim sim;
    eclp_result_t r = sim.Step([](Snap &s) {
        s.approach.rear_left_approach_risk = 1;
        WithRequest(s, ECLP_SIDE_RIGHT, ECLP_ACTION_LOCK);
    });
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_LOCK, r.lock_right);

    r = sim.Step([](Snap &s) {
        s.approach.rear_left_approach_risk = 1;
        WithRequest(s, ECLP_SIDE_RIGHT, ECLP_ACTION_UNLOCK);
    });
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
}

/**
 * @test        ST-007
 * @requirement SWR-013
 * @brief       [목적] 입력이 갱신을 멈춘 뒤 나이 200 ms까지는 신선하고 210 ms 주기부터 STALE로 판정되는지 시간 경과로 확인한다.
 * @input       속도 입력의 시각이 시작 시각에서 멈춘 상태로 21주기 진행
 * @procedure   1. 매 주기 속도 시각을 고정한 채 평가한다. 2. 주기별 속도 유효성을 확인한다.
 * @expected    나이 0~200 ms(주기 0~20) VALID, 210 ms(주기 21) STALE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, InputBecomesStaleAfter200ms)
{
    Sim sim;
    for (int c = 0; c <= 21; ++c) {
        const eclp_result_t r = sim.Step([](Snap &s) { s.vehicle.source_timestamp_s = Sim::kStart; });
        EXPECT_EQ(c <= 20 ? ECLP_VALID : ECLP_STALE, r.validity_speed) << "cycle " << c;
    }
}

/**
 * @test        ST-008
 * @requirement SWR-122
 * @brief       [목적] 필수 입력이 STALE이 된 첫 평가주기에 DEGRADED로 전환되고 INVALID 입력도 같은 주기에 DEGRADED가 되는지 확인한다.
 * @input       속도 입력 갱신 중단(주기 21에 STALE), 충돌 입력 값 5(INVALID)
 * @procedure   1. 속도 시각을 고정해 STALE 직전과 직후 주기를 평가한다. 2. INVALID 입력을 주입한다.
 * @expected    주기 20 NORMAL, 주기 21 DEGRADED (OEM 기준 200 ms 초과 후 100 ms 이내), INVALID 주입 주기에 DEGRADED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, StaleOrInvalidInputGivesDegradedInSameCycle)
{
    Sim sim;
    eclp_result_t r = {};
    for (int c = 0; c <= 21; ++c) {
        r = sim.Step([](Snap &s) { s.vehicle.source_timestamp_s = Sim::kStart; });
        if (c == 20) {
            EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
        }
    }
    EXPECT_EQ(ECLP_STATE_DEGRADED, r.state);

    Sim other;
    r = other.Step([](Snap &s) { s.crash.crash_status = 5; });
    EXPECT_EQ(ECLP_STATE_DEGRADED, r.state);
    EXPECT_EQ(ECLP_INVALID, r.validity_crash);
}

/**
 * @test        ST-009
 * @requirement SWR-123
 * @brief       [목적] 입력과 sensor_fault가 정상으로 돌아온 첫 평가주기에 NORMAL로 복귀하는지 확인한다.
 * @input       DEGRADED 입력 후 정상 입력, FAULT 입력 후 정상 입력
 * @procedure   1. 각각 이상 입력 뒤 정상 입력을 주입한다. 2. 복귀 주기의 상태를 확인한다.
 * @expected    정상 입력의 첫 주기에 NORMAL이고 경고 NONE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, ReturnsToNormalOnFirstCleanCycle)
{
    Sim degraded;
    degraded.Step([](Snap &s) { s.vehicle.present = false; });
    eclp_result_t r = degraded.Step();
    EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
    EXPECT_EQ(ECLP_W_NONE, r.warning_code);

    Sim fault;
    fault.Step([](Snap &s) { s.fault.sensor_fault = 1; });
    r = fault.Step();
    EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
    EXPECT_EQ(ECLP_W_NONE, r.warning_code);
}

/**
 * @test        ST-010
 * @requirement SWR-125
 * @brief       [목적] DEGRADED에서 운전자 요청이 보류되고 출력이 유지되며 이유가 기록되는지 확인한다.
 * @input       두 도어 LOCK 후 속도 입력 누락 + 해제 요청 ALL
 * @procedure   1. 두 도어를 LOCK한다. 2. 입력 누락과 해제 요청을 주입한다.
 * @expected    LOCK/LOCK 유지, 상태 DEGRADED, 이유 INPUT_DEGRADED_HOLD
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, DegradedHoldsOutputs)
{
    Sim sim;
    LockBoth(&sim);
    const eclp_result_t r = sim.Step([](Snap &s) {
        s.vehicle.present = false;
        WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
    });
    EXPECT_EQ(ECLP_STATE_DEGRADED, r.state);
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_LOCK, r.lock_right);
    EXPECT_EQ(ECLP_RC_INPUT_DEGRADED_HOLD, r.reason_code);
}

/**
 * @test        ST-011
 * @requirement SWR-021
 * @brief       [목적] sensor_fault가 TRUE인 첫 평가주기에 직전 출력이 유지되고 새 요청이 적용되지 않으며 충돌 CONFIRMED는 이를 이기는지 확인한다.
 * @input       좌 LOCK 후 sensor_fault + 해제 요청, 이어서 sensor_fault + 충돌 CONFIRMED
 * @procedure   1. 좌를 LOCK한다. 2. sensor_fault와 요청을 주입한다. 3. 충돌을 추가한다.
 * @expected    좌 LOCK 유지(SENSOR_FAULT_HOLD), 충돌 시 RELEASE(CRASH_RELEASE)
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, SensorFaultHoldsOutputs)
{
    Sim sim;
    sim.Step([](Snap &s) { WithRequest(s, ECLP_SIDE_LEFT, ECLP_ACTION_LOCK); });
    eclp_result_t r = sim.Step([](Snap &s) {
        s.fault.sensor_fault = 1;
        WithRequest(s, ECLP_SIDE_LEFT, ECLP_ACTION_UNLOCK);
    });
    EXPECT_EQ(ECLP_LOCK, r.lock_left);
    EXPECT_EQ(ECLP_RC_SENSOR_FAULT_HOLD, r.reason_code);

    r = sim.Step([](Snap &s) {
        s.fault.sensor_fault = 1;
        s.crash.crash_status = ECLP_CRASH_CONFIRMED;
    });
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_RC_CRASH_RELEASE, r.reason_code);
}

/**
 * @test        ST-012
 * @requirement SWR-124
 * @brief       [목적] sensor_fault가 TRUE인 첫 평가주기에 상태가 FAULT이고 입력 STALE이 함께 있어도 FAULT인지 확인한다.
 * @input       sensor_fault TRUE, sensor_fault TRUE + 속도 입력 누락
 * @procedure   1. 각각 새 시뮬레이터에서 평가한다. 2. 상태를 확인한다.
 * @expected    두 경우 모두 FAULT
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, SensorFaultGivesFaultState)
{
    Sim a;
    EXPECT_EQ(ECLP_STATE_FAULT, a.Step([](Snap &s) { s.fault.sensor_fault = 1; }).state);
    Sim b;
    EXPECT_EQ(ECLP_STATE_FAULT, b.Step([](Snap &s) {
                  s.fault.sensor_fault = 1;
                  s.vehicle.present = false;
              }).state);
}

/**
 * @test        ST-013
 * @requirement SWR-126
 * @brief       [목적] 상태에 따라 경고코드가 정의된 우선순위(sensor_fault > INVALID > STALE)로 하나만 공개되는지 확인한다.
 * @input       정상, STALE, INVALID, STALE+INVALID, sensor_fault, sensor_fault+INVALID
 * @procedure   1. 입력별로 새 시뮬레이터에서 평가한다. 2. 경고코드를 확인한다.
 * @expected    NONE, W_INPUT_STALE, W_INPUT_INVALID, W_INPUT_INVALID, W_SENSOR_FAULT, W_SENSOR_FAULT
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, WarningCodes)
{
    auto warning = [](const std::function<void(Snap &)> &f) {
        Sim sim;
        return sim.Step(f).warning_code;
    };
    EXPECT_EQ(ECLP_W_NONE, warning([](Snap &) {}));
    EXPECT_EQ(ECLP_W_INPUT_STALE, warning([](Snap &s) { s.vehicle.source_timestamp_s = s.now_s - 0.3; }));
    EXPECT_EQ(ECLP_W_INPUT_INVALID, warning([](Snap &s) { s.crash.crash_status = 9; }));
    EXPECT_EQ(ECLP_W_INPUT_INVALID, warning([](Snap &s) {
                  s.vehicle.source_timestamp_s = s.now_s - 0.3;
                  s.crash.crash_status = 9;
              }));
    EXPECT_EQ(ECLP_W_SENSOR_FAULT, warning([](Snap &s) { s.fault.sensor_fault = 1; }));
    EXPECT_EQ(ECLP_W_SENSOR_FAULT, warning([](Snap &s) {
                  s.fault.sensor_fault = 1;
                  s.crash.crash_status = 9;
              }));
}

/**
 * @test        ST-014
 * @requirement SWR-114
 * @brief       [목적] 유효, STALE, INVALID, 누락, 범위 밖 값이 섞인 3000주기 동안 출력이 항상 LOCK 또는 RELEASE로 정의되는지 확인한다.
 * @input       고정 시드의 무작위 입력 3000주기(NaN, 범위 밖, 누락, 오래된 시각 포함)
 * @procedure   1. 무작위 입력을 주입한다. 2. 매 주기 반환 상태와 출력 값을 확인한다.
 * @expected    모든 주기에 ECLP_OK이고 출력이 LOCK 또는 RELEASE, 이유코드와 상태가 정의 범위
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, OutputsAlwaysDefined)
{
    Sim sim;
    std::uint32_t seed = 4242U;
    auto next = [&seed]() {
        seed = seed * 1664525U + 1013904223U;
        return seed >> 8;
    };
    for (int i = 0; i < 3000; ++i) {
        const eclp_result_t r = sim.Step([&next](Snap &s) {
            s.vehicle.vehicle_speed_kph = (next() % 5U == 0U) ? std::nan("") : static_cast<double>(next() % 400U) - 50.0;
            s.vehicle.present = (next() % 6U) != 0U;
            s.crash.crash_status = static_cast<int>(next() % 6U) - 1;
            s.approach.rear_left_approach_risk = static_cast<int>(next() % 4U) - 1;
            s.fault.sensor_fault = static_cast<int>(next() % 4U) - 1;
            s.fault.source_timestamp_s -= (next() % 4U == 0U) ? 0.5 : 0.0;
            if (next() % 3U == 0U) {
                WithRequest(s, static_cast<int>(next() % 5U) - 1, static_cast<int>(next() % 3U) - 1);
            }
        });
        ASSERT_EQ(ECLP_OK, sim.Status()) << i;
        ASSERT_TRUE(r.lock_left == ECLP_LOCK || r.lock_left == ECLP_RELEASE) << i;
        ASSERT_TRUE(r.lock_right == ECLP_LOCK || r.lock_right == ECLP_RELEASE) << i;
        ASSERT_LT(static_cast<unsigned>(r.reason_code), static_cast<unsigned>(ECLP_RC_COUNT)) << i;
        ASSERT_LT(static_cast<unsigned>(r.state), static_cast<unsigned>(ECLP_STATE_COUNT)) << i;
    }
}

/**
 * @test        ST-015
 * @requirement SWR-101
 * @brief       [목적] 형식, 범위, enum 오류와 누락이 입력 유효성 INVALID로 공개되고 그 값이 판정에 쓰이지 않는지 확인한다.
 * @input       속도 301, 기어 7, 충돌 5, 접근위험 2, ignition 3, 그룹 누락
 * @procedure   1. 항목별로 새 시뮬레이터에서 평가한다. 2. 해당 그룹의 유효성과 출력을 확인한다.
 * @expected    해당 그룹 INVALID, 접근위험 값 2는 LOCK을 만들지 않는다
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, InvalidInputsAreReportedAndIgnored)
{
    auto run = [](const std::function<void(Snap &)> &f) {
        Sim sim;
        return sim.Step(f);
    };
    EXPECT_EQ(ECLP_INVALID, run([](Snap &s) { s.vehicle.vehicle_speed_kph = 301.0; }).validity_speed);
    EXPECT_EQ(ECLP_INVALID, run([](Snap &s) { s.vehicle.gear = 7; }).validity_speed);
    EXPECT_EQ(ECLP_INVALID, run([](Snap &s) { s.crash.crash_status = 5; }).validity_crash);
    const eclp_result_t risk = run([](Snap &s) { s.approach.rear_left_approach_risk = 2; });
    EXPECT_EQ(ECLP_INVALID, risk.validity_approach);
    EXPECT_EQ(ECLP_RELEASE, risk.lock_left);
    EXPECT_EQ(ECLP_INVALID, run([](Snap &s) { s.fault.ignition_on = 3; }).validity_fault);
    EXPECT_EQ(ECLP_INVALID, run([](Snap &s) { s.crash.present = false; }).validity_crash);
    EXPECT_EQ(ECLP_VALID, run([](Snap &s) { s.vehicle.vehicle_speed_kph = 300.0; }).validity_speed);
}

/**
 * @test        ST-016
 * @requirement SWR-102
 * @brief       [목적] 평가 결과가 출력, 상태, 이유, 경고, 입력 유효성 필드를 모두 정의된 값으로 채우고 공개 직렬화가 9개 필드인지 확인한다.
 * @input       접근위험이 있는 한 주기
 * @procedure   1. 평가한다. 2. 모든 필드와 직렬화 문자열을 확인한다.
 * @expected    모든 필드가 정의된 값이고 직렬화가 쉼표로 구분된 9개 필드와 줄바꿈 하나
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, ResultCarriesAllPublicFields)
{
    Sim sim;
    const eclp_result_t r = sim.Step([](Snap &s) { s.approach.rear_right_approach_risk = 1; });
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_LOCK, r.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
    EXPECT_EQ(ECLP_RC_APPROACH_RISK_LOCK, r.reason_code);
    EXPECT_EQ(ECLP_W_NONE, r.warning_code);
    const std::string line = eclp_test::Line(r);
    EXPECT_EQ("RELEASE,LOCK,NORMAL,APPROACH_RISK_LOCK,NONE,VALID,VALID,VALID,VALID\n", line);
    size_t commas = 0;
    for (char ch : line) {
        commas += (ch == ',') ? 1U : 0U;
    }
    EXPECT_EQ(8U, commas);
}

/**
 * @test        ST-017
 * @requirement SWR-107
 * @brief       [목적] 공개된 이유코드가 판정 규칙과 SRS 부록 A의 매핑대로 나오는지 시나리오로 확인한다.
 * @input       충돌, PENDING, sensor_fault, DEGRADED, 접근위험, 억제, LOCK 요청, 해제 요청, 거절 시나리오
 * @procedure   1. 시나리오별로 새 시뮬레이터에서 평가한다. 2. priority_reason과 reason_code를 확인한다.
 * @expected    CRASH->CRASH_RELEASE, CRASH_PENDING->CRASH_PENDING_HOLD, SENSOR_FAULT->SENSOR_FAULT_HOLD, DEGRADED->INPUT_DEGRADED_HOLD, APPROACH_RISK->APPROACH_RISK_LOCK, APPROACH_RISK_SUPPRESS->RELEASE_SUPPRESSED, DRIVER->DRIVER_LOCK/DRIVER_RELEASE, REJECTED->REQUEST_REJECTED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, ReasonCodeMapping)
{
    struct Case {
        eclp_priority_t priority;
        eclp_reason_t reason;
        std::function<void(Snap &)> setup;
    };
    const Case cases[] = {
        {ECLP_PR_CRASH, ECLP_RC_CRASH_RELEASE, [](Snap &s) { s.crash.crash_status = ECLP_CRASH_CONFIRMED; }},
        {ECLP_PR_CRASH_PENDING, ECLP_RC_CRASH_PENDING_HOLD, [](Snap &s) { s.crash.crash_status = ECLP_CRASH_PENDING; }},
        {ECLP_PR_SENSOR_FAULT, ECLP_RC_SENSOR_FAULT_HOLD, [](Snap &s) { s.fault.sensor_fault = 1; }},
        {ECLP_PR_DEGRADED, ECLP_RC_INPUT_DEGRADED_HOLD, [](Snap &s) { s.vehicle.present = false; }},
        {ECLP_PR_APPROACH_RISK, ECLP_RC_APPROACH_RISK_LOCK, [](Snap &s) { s.approach.rear_left_approach_risk = 1; }},
        {ECLP_PR_APPROACH_RISK_SUPPRESS, ECLP_RC_RELEASE_SUPPRESSED,
         [](Snap &s) {
             s.approach.rear_left_approach_risk = 1;
             WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK);
         }},
        {ECLP_PR_DRIVER, ECLP_RC_DRIVER_LOCK, [](Snap &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_LOCK); }},
        {ECLP_PR_DRIVER, ECLP_RC_DRIVER_RELEASE, [](Snap &s) { WithRequest(s, ECLP_SIDE_ALL, ECLP_ACTION_UNLOCK); }},
        {ECLP_PR_REJECTED, ECLP_RC_REQUEST_REJECTED, [](Snap &s) { WithRequest(s, 9, 9); }},
    };
    for (const Case &c : cases) {
        Sim sim;
        const eclp_result_t r = sim.Step(c.setup);
        EXPECT_EQ(c.priority, r.priority_reason) << eclp_name_reason(c.reason);
        EXPECT_EQ(c.reason, r.reason_code) << eclp_name_reason(c.reason);
    }
}

/**
 * @test        ST-018
 * @requirement SWR-104
 * @brief       [목적] 규칙이 동시에 성립할 때 충돌 > sensor_fault > 접근위험 > 운전자 요청 순으로 상위 규칙의 결과가 공개되는지 확인한다.
 * @input       접근위험 + LOCK 요청, + sensor_fault, + 충돌 CONFIRMED를 차례로 추가
 * @procedure   1. 조합별로 새 시뮬레이터에서 평가한다. 2. 이긴 규칙을 확인한다.
 * @expected    APPROACH_RISK, SENSOR_FAULT, CRASH
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, HigherPriorityRuleWins)
{
    auto base = [](Snap &s) {
        s.approach.rear_left_approach_risk = 1;
        WithRequest(s, ECLP_SIDE_RIGHT, ECLP_ACTION_LOCK);
    };
    Sim a;
    EXPECT_EQ(ECLP_PR_APPROACH_RISK, a.Step(base).priority_reason);
    Sim b;
    EXPECT_EQ(ECLP_PR_SENSOR_FAULT, b.Step([&base](Snap &s) {
                  base(s);
                  s.fault.sensor_fault = 1;
              }).priority_reason);
    Sim c;
    EXPECT_EQ(ECLP_PR_CRASH, c.Step([&base](Snap &s) {
                  base(s);
                  s.fault.sensor_fault = 1;
                  s.crash.crash_status = ECLP_CRASH_CONFIRMED;
              }).priority_reason);
}

/**
 * @test        ST-019
 * @requirement SWR-105
 * @brief       [목적] 기동 직후 결과가 RELEASE/RELEASE, NORMAL, INIT이고 중립 입력의 첫 평가도 RELEASE/RELEASE, NORMAL인지 확인한다.
 * @input       제어기 초기화 직후, 중립 입력 첫 평가
 * @procedure   1. 초기화 직후 최근 결과를 확인한다. 2. 중립 입력으로 평가한다.
 * @expected    초기 결과 RELEASE/RELEASE/NORMAL/INIT, 첫 평가 RELEASE/RELEASE/NORMAL
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, StartsReleasedAndNormal)
{
    Sim sim;
    const eclp_result_t &initial = sim.Controller().last;
    EXPECT_EQ(ECLP_RELEASE, initial.lock_left);
    EXPECT_EQ(ECLP_RELEASE, initial.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, initial.state);
    EXPECT_EQ(ECLP_RC_INIT, initial.reason_code);
    const eclp_result_t r = sim.Step();
    EXPECT_EQ(ECLP_RELEASE, r.lock_left);
    EXPECT_EQ(ECLP_RELEASE, r.lock_right);
    EXPECT_EQ(ECLP_STATE_NORMAL, r.state);
}

/**
 * @test        ST-020
 * @requirement SWR-106
 * @brief       [목적] 등록되지 않은 side/action/source 값의 요청이 거절되어 출력이 변하지 않고 이유가 기록되는지 확인한다.
 * @input       두 도어 LOCK 후 side=7, action=5, source=9 요청
 * @procedure   1. 두 도어를 LOCK한다. 2. 잘못된 요청을 항목별로 주입한다.
 * @expected    출력 유지, 이유 REQUEST_REJECTED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, UnregisteredRequestIsRejected)
{
    const int bad[][3] = {{7, 0, 0}, {0, 5, 0}, {0, 0, 9}};
    for (const auto &b : bad) {
        Sim sim;
        LockBoth(&sim);
        const eclp_result_t r = sim.Step([&b](Snap &s) { WithRequest(s, b[0], b[1], b[2]); });
        EXPECT_EQ(ECLP_LOCK, r.lock_left);
        EXPECT_EQ(ECLP_LOCK, r.lock_right);
        EXPECT_EQ(ECLP_RC_REQUEST_REJECTED, r.reason_code);
    }
}

/**
 * @test        ST-021
 * @requirement SWR-108
 * @brief       [목적] 등록된 4개 경로의 운전자 요청이 선택한 도어에만 적용되고 동작에 따라 이유가 기록되는지 확인한다.
 * @input       (LEFT|RIGHT|ALL) x (LOCK|UNLOCK) x 경로 4종
 * @procedure   1. 직전 반대 상태에서 요청을 주입한다. 2. 두 출력과 이유를 확인한다.
 * @expected    선택한 도어만 바뀌고 나머지는 유지, 이유 DRIVER_LOCK 또는 DRIVER_RELEASE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, DriverRequestAppliesToSelectedDoors)
{
    for (int source = 0; source < 4; ++source) {
        for (int side = 0; side < 3; ++side) {
            for (int action = 0; action < 2; ++action) {
                Sim sim;
                if (action == ECLP_ACTION_UNLOCK) {
                    LockBoth(&sim);
                }
                const eclp_result_t r = sim.Step([=](Snap &s) { WithRequest(s, side, action, source); });
                const eclp_lock_t changed = (action == ECLP_ACTION_LOCK) ? ECLP_LOCK : ECLP_RELEASE;
                const eclp_lock_t kept = (action == ECLP_ACTION_LOCK) ? ECLP_RELEASE : ECLP_LOCK;
                const bool left = (side == ECLP_SIDE_LEFT) || (side == ECLP_SIDE_ALL);
                const bool right = (side == ECLP_SIDE_RIGHT) || (side == ECLP_SIDE_ALL);
                EXPECT_EQ(left ? changed : kept, r.lock_left) << source << "/" << side << "/" << action;
                EXPECT_EQ(right ? changed : kept, r.lock_right) << source << "/" << side << "/" << action;
                EXPECT_EQ(action == ECLP_ACTION_LOCK ? ECLP_RC_DRIVER_LOCK : ECLP_RC_DRIVER_RELEASE, r.reason_code);
            }
        }
    }
}

/**
 * @test        ST-022
 * @requirement SWR-016
 * @brief       [목적] 같은 입력 벡터를 고정 시계로 1,000회 재생했을 때 공개 결과(부록 B 직렬화)의 SHA-256이 모두 같은지 확인한다.
 * @input       충돌, 접근위험, STALE, 요청, sensor_fault가 섞인 200주기 벡터 x 1,000회
 * @procedure   1. 벡터를 새 제어기로 재생하고 직렬화한 줄을 이어 붙여 SHA-256을 구한다. 2. 1,000회의 해시를 비교한다.
 * @expected    1,000개의 해시가 모두 같고 64자리 16진수다
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(System, ReplayHashIsStable)
{
    std::string first;
    for (int run = 0; run < 1000; ++run) {
        Sim sim;
        eclp_test::Sha256 hash;
        for (int i = 0; i < 200; ++i) {
            const eclp_result_t r = sim.Step([i](Snap &s) {
                if (i % 37 == 5) {
                    s.crash.crash_status = (i % 2 == 0) ? ECLP_CRASH_PENDING : ECLP_CRASH_CONFIRMED;
                }
                if (i % 11 == 3) {
                    s.approach.rear_left_approach_risk = 1;
                }
                if (i % 23 == 7) {
                    s.vehicle.source_timestamp_s = s.now_s - 0.25;
                }
                if (i % 29 == 11) {
                    s.fault.sensor_fault = 1;
                }
                if (i % 5 == 0) {
                    WithRequest(s, i % 3, i % 2, i % 4);
                }
                if (i == 150) {
                    s.fault.ignition_on = 0;
                }
            });
            hash.Update(eclp_test::Line(r));
        }
        const std::string hex = hash.Hex();
        if (run == 0) {
            first = hex;
            ASSERT_EQ(64U, first.size());
        }
        ASSERT_EQ(first, hex) << "run " << run;
    }
}

}  // namespace
