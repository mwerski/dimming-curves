#pragma once
#include <stdint.h>
#include <stddef.h>

// Simple, platform-neutral dim curve generator.
// Output table is always 256 entries (index 0..255) with 10-bit PWM values (0..1023).
//
// Design goals:
// - No Arduino/ESP dependencies
// - Deterministic output
// - Guarantees: out[0] == 0, out[255] == PWM_MAX
//
// Notes:
// - This library generates curves only. It does not apply minBrightness/clamping logic.
// - If you want a per-light minimum effective output, keep that in your application or device layer.

namespace DimCurve {

static constexpr uint16_t PWM_MAX = 1023;
static constexpr size_t   TABLE_SIZE = 256;

// Utility: clamp to 0..PWM_MAX
uint16_t clampPwm(int32_t v);

// Fills out with a linear mapping: y = x
void linear(uint16_t out[TABLE_SIZE]);

// Gamma curve: y = x^gamma
// gamma > 0 (typical: 2.0..2.8)
void gamma(uint16_t out[TABLE_SIZE], float gamma);

// Exponential (often used as "log-like" perceptual curve):
// y = (exp(k*x) - 1) / (exp(k) - 1)
// k >= 0  (k==0 -> linear)
void exponential(uint16_t out[TABLE_SIZE], float k);

// DALI log-like curve approximation for Arc Power levels.
// - out[0] = 0
// - for i=1..255, map i -> dali=min(i,254) and use
//   fraction = 1000^((dali-254)/253)  => approx 0.1% at level 1, 100% at 254
void daliLog(uint16_t out[TABLE_SIZE]);

// LED-optimized presets

// LED Low-End Boost: Gamma curve but enforce a minimum PWM output for any non-zero level.
// This can help avoid dead-zones or instability at very low PWM.
// pwmMin: 0..PWM_MAX
void ledLowEndBoost(uint16_t out[TABLE_SIZE], float gamma, uint16_t pwmMin);

// LED Hybrid: stronger shaping at the very low end, normal gamma afterwards.
// - For x < t: apply gammaLow on the normalized segment, scaled to meet gammaHigh at t
// - For x >= t: apply gammaHigh on full range
// t in (0,1], typical 0.15..0.30
void ledHybrid(uint16_t out[TABLE_SIZE], float t, float gammaLow, float gammaHigh);

// LED S-curve: smoothstep shaping (S-curve) followed by gamma.
// smoothstep(x) = x^2 * (3 - 2x)
void ledSCurve(uint16_t out[TABLE_SIZE], float gamma);

// Validate a generated curve.
// Returns true if:
// - out[0]==0 and out[255]==PWM_MAX
// - all entries are within 0..PWM_MAX
// - curve is monotonic non-decreasing
bool validate(const uint16_t out[TABLE_SIZE]);

} // namespace DimCurve
