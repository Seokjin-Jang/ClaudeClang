/**
 * @file    test_support.hpp
 * @brief   테스트 공용 도우미: 입력 스냅샷 생성기, 결과 직렬화 해시(SHA-256).
 * @ai_model      claude-sonnet-5-5
 * @ai_date       2026-11-27
 * @ai_requester  Seokjin-Jang
 * @review_status Pending
 */
#ifndef ECLP_TEST_SUPPORT_HPP
#define ECLP_TEST_SUPPORT_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

extern "C" {
#include "controller.h"
}

namespace eclp_test {

/** 평가 주기(초). 10 ms (A-01). */
constexpr double kStep = 0.010;

/**
 * 모든 입력이 유효하고 신선한 중립 스냅샷을 만든다.
 * 속도 0, 기어 P, 충돌 NONE, 접근위험 FALSE/FALSE, 시동 ON, sensor_fault FALSE, 운전자 요청 없음.
 */
inline eclp_snapshot_t Neutral(double now_s)
{
    eclp_snapshot_t s;
    std::memset(&s, 0, sizeof(s));
    s.now_s = now_s;
    s.vehicle.present = true;
    s.vehicle.source_timestamp_s = now_s;
    s.vehicle.vehicle_speed_kph = 0.0;
    s.vehicle.gear = ECLP_GEAR_P;
    s.crash.present = true;
    s.crash.source_timestamp_s = now_s;
    s.crash.crash_status = ECLP_CRASH_NONE;
    s.approach.present = true;
    s.approach.source_timestamp_s = now_s;
    s.approach.rear_left_approach_risk = 0;
    s.approach.rear_right_approach_risk = 0;
    s.fault.present = true;
    s.fault.source_timestamp_s = now_s;
    s.fault.ignition_on = 1;
    s.fault.sensor_fault = 0;
    s.request.present = false;
    return s;
}

/** 운전자 요청을 스냅샷에 넣는다. */
inline eclp_snapshot_t &WithRequest(eclp_snapshot_t &s, int side, int action,
                                    int source = ECLP_SRC_PHYSICAL_BUTTON)
{
    s.request.present = true;
    s.request.side = side;
    s.request.action = action;
    s.request.source = source;
    return s;
}

/** 컨트롤러를 한 번 평가하고 결과를 반환한다. 상태 코드가 OK가 아니면 결과는 0으로 채운다. */
inline eclp_result_t Eval(eclp_t *ctl, const eclp_snapshot_t &s, eclp_status_t *status = nullptr)
{
    eclp_result_t r;
    std::memset(&r, 0, sizeof(r));
    const eclp_status_t st = eclp_evaluate(ctl, &s, &r);
    if (status != nullptr) {
        *status = st;
    }
    return r;
}

/** 공개 결과를 부록 B 형식의 한 줄 문자열로 만든다. */
inline std::string Line(const eclp_result_t &r)
{
    char buf[256];
    size_t len = 0;
    const eclp_status_t st = eclp_serialize(&r, buf, sizeof(buf), &len);
    return (st == ECLP_OK) ? std::string(buf, len) : std::string("<serialize error>");
}

/** SHA-256 (FIPS 180-4) 구현. 결과 해시 비교용이다. */
class Sha256 {
public:
    Sha256() { Reset(); }

    /** 데이터를 추가한다. */
    void Update(const std::string &data)
    {
        for (unsigned char c : data) {
            buffer_[fill_++] = c;
            ++total_;
            if (fill_ == 64U) {
                Block(buffer_.data());
                fill_ = 0U;
            }
        }
    }

    /** 해시를 16진수 문자열로 확정한다. */
    std::string Hex()
    {
        const std::uint64_t bits = total_ * 8U;
        buffer_[fill_++] = 0x80U;
        while (fill_ != 56U) {
            if (fill_ == 64U) {
                Block(buffer_.data());
                fill_ = 0U;
            }
            buffer_[fill_++] = 0U;
        }
        for (int i = 7; i >= 0; --i) {
            buffer_[fill_++] = static_cast<std::uint8_t>(bits >> (8 * i));
        }
        Block(buffer_.data());
        static const char *digits = "0123456789abcdef";
        std::string out;
        for (std::uint32_t word : h_) {
            for (int i = 3; i >= 0; --i) {
                const unsigned byte = (word >> (8 * i)) & 0xFFU;
                out.push_back(digits[byte >> 4]);
                out.push_back(digits[byte & 0xFU]);
            }
        }
        return out;
    }

private:
    static std::uint32_t Rotr(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

    void Reset()
    {
        h_ = {0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
              0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U};
        fill_ = 0U;
        total_ = 0U;
    }

    void Block(const std::uint8_t *p)
    {
        static const std::uint32_t k[64] = {
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U};
        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(p[4 * i]) << 24) | (static_cast<std::uint32_t>(p[4 * i + 1]) << 16) |
                   (static_cast<std::uint32_t>(p[4 * i + 2]) << 8) | static_cast<std::uint32_t>(p[4 * i + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        std::array<std::uint32_t, 8> v = h_;
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = Rotr(v[4], 6) ^ Rotr(v[4], 11) ^ Rotr(v[4], 25);
            const std::uint32_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
            const std::uint32_t t1 = v[7] + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = Rotr(v[0], 2) ^ Rotr(v[0], 13) ^ Rotr(v[0], 22);
            const std::uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            const std::uint32_t t2 = s0 + maj;
            v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
            v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
        }
        for (int i = 0; i < 8; ++i) {
            h_[i] += v[i];
        }
    }

    std::array<std::uint32_t, 8> h_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t fill_ = 0U;
    std::uint64_t total_ = 0U;
};

/**
 * 10 ms 주기로 제어기를 구동하는 시뮬레이터. 시각은 사이클 번호로 계산해 부동소수점 오차가 쌓이지 않는다.
 * 매 주기 중립 스냅샷을 만들고 호출자가 지정한 변경을 적용한 뒤 평가한다.
 */
class Sim {
public:
    /** 첫 사이클의 시각(초). */
    static constexpr double kStart = 1.0;

    Sim() { eclp_init(&ctl_); }

    /** 현재 사이클의 시각(초). */
    double Now() const { return kStart + static_cast<double>(cycle_) * kStep; }

    /** 지금까지 수행한 사이클 수. */
    int Cycle() const { return cycle_; }

    /** 변경을 적용한 스냅샷으로 한 주기 평가한다. */
    eclp_result_t Step(const std::function<void(eclp_snapshot_t &)> &change = nullptr)
    {
        eclp_snapshot_t s = Neutral(Now());
        if (change) {
            change(s);
        }
        const eclp_result_t r = Eval(&ctl_, s, &status_);
        ++cycle_;
        return r;
    }

    /** 마지막 평가의 반환 상태. */
    eclp_status_t Status() const { return status_; }

    /** 제어기에 대한 접근(최근 결과 확인용). */
    const eclp_t &Controller() const { return ctl_; }

private:
    eclp_t ctl_{};
    int cycle_ = 0;
    eclp_status_t status_ = ECLP_OK;
};

}  // namespace eclp_test

#endif  // ECLP_TEST_SUPPORT_HPP
