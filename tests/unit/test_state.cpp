/**
 * @file    test_state.cpp
 * @brief   C-STA 상태 관리기(M-STA-01)의 단위 테스트.
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#include <gtest/gtest.h>

#include "test_support.hpp"

extern "C" {
#include "state_manager.h"
}

namespace {

/** 모든 그룹이 유효한 검증 입력을 만든다. */
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

/** 지정한 그룹의 유효성을 바꾼다. group: 0 speed, 1 crash, 2 approach, 3 fault. */
void SetGroup(eclp_validated_t *v, int group, eclp_validity_t validity)
{
    eclp_validity_t *slots[4] = {&v->speed, &v->crash, &v->approach, &v->fault};
    *slots[group] = validity;
}

/** 상태와 경고를 평가해 쌍으로 반환한다. */
std::pair<eclp_state_t, eclp_warning_t> RunSta(const eclp_validated_t &v)
{
    eclp_state_t state = ECLP_STATE_OFF;
    eclp_warning_t warning = ECLP_W_COUNT;
    EXPECT_EQ(ECLP_OK, eclp_sta_evaluate(&v, &state, &warning));
    return {state, warning};
}

/**
 * @test        UT-105-01
 * @requirement SWR-105
 * @brief       [목적] 초기 상태가 NORMAL이고 널 포인터가 인자 오류로 처리되는지 확인한다.
 * @input       state 포인터 정상/널
 * @procedure   1. eclp_sta_initial을 호출한다. 2. 상태와 반환 코드를 확인한다.
 * @expected    NORMAL과 ECLP_OK, 널이면 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(State, InitialIsNormal)
{
    eclp_state_t state = ECLP_STATE_FAULT;
    EXPECT_EQ(ECLP_OK, eclp_sta_initial(&state));
    EXPECT_EQ(ECLP_STATE_NORMAL, state);
    EXPECT_EQ(ECLP_ERR_ARG, eclp_sta_initial(nullptr));
}

/**
 * @test        UT-123-01
 * @requirement SWR-123
 * @brief       [목적] 모든 입력이 유효하고 sensor_fault가 없으면 NORMAL이며, FAULT/DEGRADED였다가 조건이 해소된 첫 주기에 NORMAL로 복귀하는지 확인한다.
 * @input       모든 그룹 VALID, sensor_fault FALSE. 직전에 FAULT, DEGRADED였던 입력 순서
 * @procedure   1. FAULT 입력, 정상 입력 순으로 평가한다. 2. DEGRADED 입력, 정상 입력 순으로 평가한다.
 * @expected    정상 입력을 평가한 주기에 NORMAL과 경고 NONE
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(State, RecoversToNormalOnFirstCleanCycle)
{
    eclp_validated_t v = AllValid();
    EXPECT_EQ(ECLP_STATE_NORMAL, RunSta(v).first);
    EXPECT_EQ(ECLP_W_NONE, RunSta(v).second);

    v.sensor_fault = true;
    EXPECT_EQ(ECLP_STATE_FAULT, RunSta(v).first);
    v.sensor_fault = false;
    EXPECT_EQ(ECLP_STATE_NORMAL, RunSta(v).first);

    SetGroup(&v, 0, ECLP_STALE);
    EXPECT_EQ(ECLP_STATE_DEGRADED, RunSta(v).first);
    SetGroup(&v, 0, ECLP_VALID);
    const auto result = RunSta(v);
    EXPECT_EQ(ECLP_STATE_NORMAL, result.first);
    EXPECT_EQ(ECLP_W_NONE, result.second);
}

/**
 * @test        UT-122-01
 * @requirement SWR-122
 * @brief       [목적] 필수 입력 어느 그룹이든 STALE 또는 INVALID이면 DEGRADED가 되는지 확인한다.
 * @input       그룹 4개 각각을 STALE, INVALID로 바꾼 입력
 * @procedure   1. 그룹과 유효성을 바꿔 평가한다. 2. 상태를 확인한다.
 * @expected    모든 경우에 DEGRADED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(State, AnyStaleOrInvalidGroupDegrades)
{
    for (int group = 0; group < 4; ++group) {
        for (eclp_validity_t bad : {ECLP_STALE, ECLP_INVALID}) {
            eclp_validated_t v = AllValid();
            SetGroup(&v, group, bad);
            EXPECT_EQ(ECLP_STATE_DEGRADED, RunSta(v).first) << "group=" << group << " bad=" << bad;
        }
    }
}

/**
 * @test        UT-124-01
 * @requirement SWR-124
 * @brief       [목적] 유효한 고장 그룹에서 sensor_fault가 TRUE이면 FAULT이고 STALE이 함께 있어도 FAULT이며, 고장 그룹이 유효하지 않으면 FAULT가 아닌지 확인한다.
 * @input       sensor_fault TRUE + 고장 그룹 VALID, 속도 STALE 동반, 고장 그룹 STALE/INVALID
 * @procedure   1. 입력을 만들어 평가한다. 2. 상태를 확인한다.
 * @expected    VALID이면 FAULT(STALE 동반도 FAULT), 고장 그룹이 STALE/INVALID이면 DEGRADED
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(State, SensorFaultGivesFault)
{
    eclp_validated_t v = AllValid();
    v.sensor_fault = true;
    EXPECT_EQ(ECLP_STATE_FAULT, RunSta(v).first);

    SetGroup(&v, 0, ECLP_STALE);
    EXPECT_EQ(ECLP_STATE_FAULT, RunSta(v).first);

    v = AllValid();
    v.sensor_fault = true;
    SetGroup(&v, 3, ECLP_STALE);
    EXPECT_EQ(ECLP_STATE_DEGRADED, RunSta(v).first);
    SetGroup(&v, 3, ECLP_INVALID);
    EXPECT_EQ(ECLP_STATE_DEGRADED, RunSta(v).first);
}

/**
 * @test        UT-126-01
 * @requirement SWR-126
 * @brief       [목적] 경고코드가 W_SENSOR_FAULT > W_INPUT_INVALID > W_INPUT_STALE 순으로 하나만 나오고 정상이면 NONE인지 확인한다.
 * @input       정상, STALE만, INVALID만, STALE+INVALID, sensor_fault, sensor_fault+INVALID
 * @procedure   1. 입력을 바꿔 평가한다. 2. 경고코드를 확인한다.
 * @expected    NONE, W_INPUT_STALE, W_INPUT_INVALID, W_INPUT_INVALID, W_SENSOR_FAULT, W_SENSOR_FAULT
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(State, WarningPrecedence)
{
    eclp_validated_t v = AllValid();
    EXPECT_EQ(ECLP_W_NONE, RunSta(v).second);

    SetGroup(&v, 0, ECLP_STALE);
    EXPECT_EQ(ECLP_W_INPUT_STALE, RunSta(v).second);

    SetGroup(&v, 1, ECLP_INVALID);
    EXPECT_EQ(ECLP_W_INPUT_INVALID, RunSta(v).second);

    v = AllValid();
    SetGroup(&v, 2, ECLP_INVALID);
    EXPECT_EQ(ECLP_W_INPUT_INVALID, RunSta(v).second);

    v = AllValid();
    v.sensor_fault = true;
    EXPECT_EQ(ECLP_W_SENSOR_FAULT, RunSta(v).second);
    SetGroup(&v, 0, ECLP_INVALID);
    EXPECT_EQ(ECLP_W_SENSOR_FAULT, RunSta(v).second);
}

/**
 * @test        UT-126-02
 * @requirement SWR-126
 * @brief       [목적] 상태 평가의 널 포인터 인자가 ECLP_ERR_ARG로 처리되는지 확인한다.
 * @input       v, state, warning 중 하나씩 널
 * @procedure   1. 각 널 조합으로 호출한다. 2. 반환 코드를 확인한다.
 * @expected    모두 ECLP_ERR_ARG
 * @ai_model claude-sonnet-5-5 @ai_date 2026-11-27 @ai_requester Seokjin-Jang @review_status Pending
 */
TEST(State, NullArguments)
{
    const eclp_validated_t v = AllValid();
    eclp_state_t state = ECLP_STATE_NORMAL;
    eclp_warning_t warning = ECLP_W_NONE;
    EXPECT_EQ(ECLP_ERR_ARG, eclp_sta_evaluate(nullptr, &state, &warning));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_sta_evaluate(&v, nullptr, &warning));
    EXPECT_EQ(ECLP_ERR_ARG, eclp_sta_evaluate(&v, &state, nullptr));
}

}  // namespace
