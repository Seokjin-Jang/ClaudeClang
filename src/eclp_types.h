/**
 * @file    eclp_types.h
 * @brief   전자식 차일드락 제어 SW의 공유 데이터 타입과 상수. 값 타입만 정의하며 코드는 없다.
 * @module  M-TYP-01
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#ifndef ECLP_TYPES_H
#define ECLP_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** 입력 STALE 기준(마이크로초). 나이가 이 값을 넘으면 STALE이다. (SWR-013, A-23) */
#define ECLP_STALE_LIMIT_US (200000)
/** 차속 상한(km/h). (OEM-IF-001, SWR-101) */
#define ECLP_SPEED_MAX_KPH (300.0)
/** 초를 마이크로초로 바꾸는 계수. (A-23) */
#define ECLP_US_PER_S (1000000.0)
/** 시각 변환에서 `int64_t` 오버플로를 막는 절댓값 상한(초). */
#define ECLP_TIME_LIMIT_S (9.0e12)

/** 인터페이스 함수의 반환 상태. 값은 비트 OR로 합칠 수 있다. */
typedef enum {
    ECLP_OK = 0,        /**< 정상 */
    ECLP_ERR_ARG = 1,   /**< 널 포인터 등 인자 오류 */
    ECLP_ERR_RANGE = 2  /**< 값 범위 오류 또는 버퍼 부족 */
} eclp_status_t;

/** 도어 잠금 출력. */
typedef enum {
    ECLP_RELEASE = 0,  /**< 해제 */
    ECLP_LOCK = 1,     /**< 잠금 */
    ECLP_LOCK_COUNT    /**< 값의 개수 */
} eclp_lock_t;

/** 제어 상태. */
typedef enum {
    ECLP_STATE_NORMAL = 0,  /**< 정상 */
    ECLP_STATE_DEGRADED,    /**< 필수 입력이 STALE 또는 INVALID */
    ECLP_STATE_FAULT,       /**< sensor_fault가 TRUE */
    ECLP_STATE_OFF,         /**< 시동 OFF (이터레이션 2에서 사용) */
    ECLP_STATE_COUNT        /**< 값의 개수 */
} eclp_state_t;

/** 입력 그룹의 유효성. */
typedef enum {
    ECLP_VALID = 0,   /**< 유효하고 신선함 */
    ECLP_STALE,       /**< 갱신 후 200 ms 초과 */
    ECLP_INVALID,     /**< 형식, 범위, enum 오류, 누락, 미래 시각 */
    ECLP_VALIDITY_COUNT  /**< 값의 개수 */
} eclp_validity_t;

/** 충돌 상태 (OEM-IF-002). */
typedef enum {
    ECLP_CRASH_NONE = 0,   /**< 충돌 없음 */
    ECLP_CRASH_PENDING,    /**< 충돌 판정 중 */
    ECLP_CRASH_CONFIRMED   /**< 충돌 확정 */
} eclp_crash_t;

/** 기어 (OEM-IF-001). */
typedef enum {
    ECLP_GEAR_P = 0,  /**< 주차 */
    ECLP_GEAR_N,      /**< 중립 */
    ECLP_GEAR_D,      /**< 주행 */
    ECLP_GEAR_R       /**< 후진 */
} eclp_gear_t;

/** 운전자 요청의 대상 도어 (OEM-IF-004). */
typedef enum {
    ECLP_SIDE_LEFT = 0,  /**< 후석 좌 */
    ECLP_SIDE_RIGHT,     /**< 후석 우 */
    ECLP_SIDE_ALL        /**< 후석 좌, 우 모두 */
} eclp_side_t;

/** 운전자 요청의 동작 (OEM-IF-004). */
typedef enum {
    ECLP_ACTION_LOCK = 0,  /**< 잠금 요청 */
    ECLP_ACTION_UNLOCK     /**< 해제 요청 */
} eclp_action_t;

/** 운전자 요청의 경로 (OEM-IF-004). */
typedef enum {
    ECLP_SRC_PHYSICAL_BUTTON = 0,  /**< 물리 버튼 */
    ECLP_SRC_AVN,                  /**< AVN */
    ECLP_SRC_VOICE,                /**< 음성 */
    ECLP_SRC_MOBILE_APP            /**< 모바일 앱 */
} eclp_source_t;

/** 검증된 운전자 요청의 상태. */
typedef enum {
    ECLP_REQ_NONE = 0,  /**< 요청 없음 */
    ECLP_REQ_VALID,     /**< 값이 모두 등록된 집합 안에 있음 */
    ECLP_REQ_INVALID    /**< 값이 등록된 집합 밖이거나 누락됨 */
} eclp_req_state_t;

/** 운전자 요청의 처리 결과. */
typedef enum {
    ECLP_OUT_NONE = 0,       /**< 요청 없음 */
    ECLP_OUT_APPLIED,        /**< 적용됨 */
    ECLP_OUT_HELD_DEGRADED,  /**< DEGRADED 상태라 보류됨 */
    ECLP_OUT_HELD_PENDING,   /**< 충돌 PENDING이라 보류됨 */
    ECLP_OUT_REJECTED        /**< 값이 잘못되어 거절됨 */
} eclp_req_outcome_t;

/** 판정에서 이긴 규칙(priority_reason). */
typedef enum {
    ECLP_PR_NONE = 0,                 /**< 어떤 규칙도 출력을 바꾸지 않음 */
    ECLP_PR_INIT,                     /**< 초기 상태 */
    ECLP_PR_CRASH,                    /**< 충돌 해제 */
    ECLP_PR_CRASH_PENDING,            /**< 충돌 PENDING 보류 */
    ECLP_PR_SENSOR_FAULT,             /**< sensor_fault 보류 */
    ECLP_PR_DEGRADED,                 /**< DEGRADED 보류 */
    ECLP_PR_APPROACH_RISK,            /**< 접근위험 LOCK */
    ECLP_PR_APPROACH_RISK_SUPPRESS,   /**< 접근위험으로 해제 요청 억제 */
    ECLP_PR_DRIVER,                   /**< 운전자 요청 적용 */
    ECLP_PR_REJECTED,                 /**< 운전자 요청 거절 */
    ECLP_PR_IGNITION_OFF,             /**< 시동 OFF (이터레이션 2에서 사용) */
    ECLP_PR_COUNT                     /**< 값의 개수 */
} eclp_priority_t;

/** 공개 이유코드(reason_code). */
typedef enum {
    ECLP_RC_NONE = 0,             /**< NONE */
    ECLP_RC_INIT,                 /**< INIT */
    ECLP_RC_CRASH_RELEASE,        /**< CRASH_RELEASE */
    ECLP_RC_CRASH_PENDING_HOLD,   /**< CRASH_PENDING_HOLD */
    ECLP_RC_SENSOR_FAULT_HOLD,    /**< SENSOR_FAULT_HOLD */
    ECLP_RC_INPUT_DEGRADED_HOLD,  /**< INPUT_DEGRADED_HOLD */
    ECLP_RC_APPROACH_RISK_LOCK,   /**< APPROACH_RISK_LOCK */
    ECLP_RC_RELEASE_SUPPRESSED,   /**< RELEASE_SUPPRESSED */
    ECLP_RC_DRIVER_LOCK,          /**< DRIVER_LOCK */
    ECLP_RC_DRIVER_RELEASE,       /**< DRIVER_RELEASE */
    ECLP_RC_REQUEST_REJECTED,     /**< REQUEST_REJECTED */
    ECLP_RC_IGNITION_OFF,         /**< IGNITION_OFF (이터레이션 2에서 사용) */
    ECLP_RC_COUNT                 /**< 값의 개수 */
} eclp_reason_t;

/** 경고코드(warning_code). */
typedef enum {
    ECLP_W_NONE = 0,       /**< NONE */
    ECLP_W_INPUT_STALE,    /**< W_INPUT_STALE */
    ECLP_W_INPUT_INVALID,  /**< W_INPUT_INVALID */
    ECLP_W_SENSOR_FAULT,   /**< W_SENSOR_FAULT */
    ECLP_W_COUNT           /**< 값의 개수 */
} eclp_warning_t;

/** 차량 상태 입력 (OEM-IF-001). 값은 검증 전의 원본이다. */
typedef struct {
    bool present;                 /**< 입력이 도착했는지 */
    double source_timestamp_s;    /**< 입력 생성 시각(초) */
    double vehicle_speed_kph;     /**< 차속(km/h) */
    int gear;                     /**< 기어(`eclp_gear_t` 값) */
} eclp_vehicle_in_t;

/** 충돌 입력 (OEM-IF-002). */
typedef struct {
    bool present;                 /**< 입력이 도착했는지 */
    double source_timestamp_s;    /**< 입력 생성 시각(초) */
    int crash_status;             /**< 충돌 상태(`eclp_crash_t` 값) */
} eclp_crash_in_t;

/** 후측방 접근위험 입력 (OEM-IF-003). */
typedef struct {
    bool present;                 /**< 입력이 도착했는지 */
    double source_timestamp_s;    /**< 입력 생성 시각(초) */
    int rear_left_approach_risk;  /**< 좌측 접근위험(0 또는 1) */
    int rear_right_approach_risk; /**< 우측 접근위험(0 또는 1) */
} eclp_approach_in_t;

/** 고장과 시동 입력 (OEM-IF-009). */
typedef struct {
    bool present;                 /**< 입력이 도착했는지 */
    double source_timestamp_s;    /**< 입력 생성 시각(초) */
    int ignition_on;              /**< 시동 상태(0 또는 1) */
    int sensor_fault;             /**< 센서 고장(0 또는 1) */
} eclp_fault_in_t;

/** 운전자 요청 입력 (OEM-IF-004). */
typedef struct {
    bool present;                 /**< 요청이 있는지 */
    int side;                     /**< 대상 도어(`eclp_side_t` 값) */
    int action;                   /**< 동작(`eclp_action_t` 값) */
    int source;                   /**< 경로(`eclp_source_t` 값) */
} eclp_request_in_t;

/** 평가 1회의 입력 스냅샷. */
typedef struct {
    double now_s;                 /**< 평가 시각(초). 호출자가 주입한다. */
    eclp_vehicle_in_t vehicle;    /**< 차량 상태 */
    eclp_crash_in_t crash;        /**< 충돌 */
    eclp_approach_in_t approach;  /**< 접근위험 */
    eclp_fault_in_t fault;        /**< 고장과 시동 */
    eclp_request_in_t request;    /**< 운전자 요청 */
} eclp_snapshot_t;

/** 검증과 정규화를 마친 입력. 값은 유효성이 `INVALID`가 아닌 그룹에서만 의미가 있다. */
typedef struct {
    eclp_validity_t speed;        /**< 차량 상태 그룹의 유효성 */
    eclp_validity_t crash;        /**< 충돌 그룹의 유효성 */
    eclp_validity_t approach;     /**< 접근위험 그룹의 유효성 */
    eclp_validity_t fault;        /**< 고장/시동 그룹의 유효성 */
    eclp_crash_t crash_status;    /**< 충돌 상태 */
    bool left_risk;               /**< 좌측 접근위험 */
    bool right_risk;              /**< 우측 접근위험 */
    bool ignition_on;             /**< 시동 상태 */
    bool sensor_fault;            /**< 센서 고장 */
    eclp_req_state_t request;     /**< 운전자 요청의 상태 */
    eclp_side_t req_side;         /**< 요청 대상 도어 */
    eclp_action_t req_action;     /**< 요청 동작 */
} eclp_validated_t;

/** 판정기의 결정. */
typedef struct {
    eclp_lock_t lock_left;        /**< 좌 출력 */
    eclp_lock_t lock_right;       /**< 우 출력 */
    eclp_priority_t priority_reason;  /**< 이긴 규칙 */
    eclp_action_t driver_action;  /**< 적용된 운전자 요청의 동작 */
    eclp_req_outcome_t outcome;   /**< 운전자 요청의 처리 결과 */
    bool risk_left;               /**< 좌측 접근위험이 적용되었는지 */
    bool risk_right;              /**< 우측 접근위험이 적용되었는지 */
    bool suppressed;              /**< 해제 요청이 억제되었는지 */
} eclp_decision_t;

/** 판정기의 내부 상태. 호출자가 소유하고 판정기만 갱신한다. */
typedef struct {
    eclp_lock_t lock_left;        /**< 직전 평가주기에 공개된 좌 출력 */
    eclp_lock_t lock_right;       /**< 직전 평가주기에 공개된 우 출력 */
    bool crash_latched;           /**< 충돌 해제 래치 */
} eclp_arb_t;

/** 평가 1회의 공개 결과 (OEM-IF-005, OEM-IF-006). */
typedef struct {
    eclp_lock_t lock_left;        /**< 좌 출력 */
    eclp_lock_t lock_right;       /**< 우 출력 */
    eclp_state_t state;           /**< 상태 */
    eclp_priority_t priority_reason;  /**< 이긴 규칙 */
    eclp_reason_t reason_code;    /**< 이유코드 */
    eclp_warning_t warning_code;  /**< 경고코드 */
    eclp_validity_t validity_speed;     /**< 차량 상태 그룹의 유효성 */
    eclp_validity_t validity_crash;     /**< 충돌 그룹의 유효성 */
    eclp_validity_t validity_approach;  /**< 접근위험 그룹의 유효성 */
    eclp_validity_t validity_fault;     /**< 고장/시동 그룹의 유효성 */
} eclp_result_t;

/** 제어기 인스턴스. 호출자가 소유한다. */
typedef struct {
    eclp_arb_t arb;               /**< 판정기 상태 */
    eclp_result_t last;           /**< 최근 결과 */
} eclp_t;

#ifdef __cplusplus
}
#endif

#endif /* ECLP_TYPES_H */
