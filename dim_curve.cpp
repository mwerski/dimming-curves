#include "dim_curve.h"
#include <math.h>

namespace DimCurve {

static inline float clamp01(float v) {
  if (v < 0.0f) return 0.0f;
  if (v > 1.0f) return 1.0f;
  return v;
}

uint16_t clampPwm(int32_t v) {
  if (v < 0) return 0;
  if (v > (int32_t)PWM_MAX) return PWM_MAX;
  return (uint16_t)v;
}

static inline void enforce_endpoints(uint16_t out[TABLE_SIZE]) {
  out[0] = 0;
  out[TABLE_SIZE - 1] = PWM_MAX;
}

void linear(uint16_t out[TABLE_SIZE]) {
  for (size_t i = 0; i < TABLE_SIZE; i++) {
    float x = (float)i / (float)(TABLE_SIZE - 1);     // 0..1
    int32_t pwm = (int32_t)lroundf(x * (float)PWM_MAX);
    out[i] = clampPwm(pwm);
  }
  enforce_endpoints(out);
}

void gamma(uint16_t out[TABLE_SIZE], float g) {
  if (!(g > 0.0f)) { // includes NaN
    linear(out);
    return;
  }

  for (size_t i = 0; i < TABLE_SIZE; i++) {
    float x = (float)i / (float)(TABLE_SIZE - 1);
    float y = powf(x, g);
    int32_t pwm = (int32_t)lroundf(y * (float)PWM_MAX);
    out[i] = clampPwm(pwm);
  }
  enforce_endpoints(out);
}

void exponential(uint16_t out[TABLE_SIZE], float k) {
  if (!(k >= 0.0f)) { // includes NaN
    linear(out);
    return;
  }
  if (k == 0.0f) {
    linear(out);
    return;
  }

  const float denom = expf(k) - 1.0f;
  if (denom == 0.0f || !isfinite(denom)) {
    linear(out);
    return;
  }

  for (size_t i = 0; i < TABLE_SIZE; i++) {
    float x = (float)i / (float)(TABLE_SIZE - 1);
    float y = (expf(k * x) - 1.0f) / denom;
    y = clamp01(y);
    int32_t pwm = (int32_t)lroundf(y * (float)PWM_MAX);
    out[i] = clampPwm(pwm);
  }
  enforce_endpoints(out);
}

void daliLog(uint16_t out[TABLE_SIZE]) {
  out[0] = 0;

  // For i=1..255: clamp to DALI arc power range 1..254.
  // fraction = 1000^((level-254)/253) -> 0.001..1 (approx 0.1% at level 1)
  for (size_t i = 1; i < TABLE_SIZE; i++) {
    const int dali = (i > 254) ? 254 : (int)i;
    const float exponent = ((float)dali - 254.0f) / 253.0f;
    const float frac = powf(1000.0f, exponent);  // ~0.001..1
    int32_t pwm = (int32_t)lroundf(frac * (float)PWM_MAX);
    out[i] = clampPwm(pwm);
  }

  enforce_endpoints(out);
}

void ledLowEndBoost(uint16_t out[TABLE_SIZE], float g, uint16_t pwmMin) {
  if (pwmMin > PWM_MAX) pwmMin = PWM_MAX;
  if (!(g > 0.0f)) {
    g = 2.2f;
  }

  for (size_t i = 0; i < TABLE_SIZE; i++) {
    if (i == 0) {
      out[i] = 0;
      continue;
    }
    float x = (float)i / (float)(TABLE_SIZE - 1);
    float y = powf(x, g);
    float scaled = (float)pwmMin + y * (float)(PWM_MAX - pwmMin);
    out[i] = clampPwm((int32_t)lroundf(scaled));
  }
  enforce_endpoints(out);
}

void ledHybrid(uint16_t out[TABLE_SIZE], float t, float gammaLow, float gammaHigh) {
  // sanitize parameters
  if (!(t > 0.0f)) t = 0.2f;
  if (t > 1.0f) t = 1.0f;
  if (!(gammaLow > 0.0f)) gammaLow = 3.0f;
  if (!(gammaHigh > 0.0f)) gammaHigh = 2.2f;

  const float y_t = powf(t, gammaHigh); // value of the high-gamma curve at x=t

  for (size_t i = 0; i < TABLE_SIZE; i++) {
    float x = (float)i / (float)(TABLE_SIZE - 1);
    float y;
    if (x < t) {
      // Normalize 0..t into 0..1, apply stronger low gamma, then scale to meet at (t, y_t)
      float xn = (t > 0.0f) ? (x / t) : 0.0f;
      y = powf(xn, gammaLow) * y_t;
    } else {
      y = powf(x, gammaHigh);
    }
    y = clamp01(y);
    out[i] = clampPwm((int32_t)lroundf(y * (float)PWM_MAX));
  }
  enforce_endpoints(out);
}

void ledSCurve(uint16_t out[TABLE_SIZE], float g) {
  if (!(g > 0.0f)) g = 2.2f;

  for (size_t i = 0; i < TABLE_SIZE; i++) {
    float x = (float)i / (float)(TABLE_SIZE - 1);
    // smoothstep S-curve: x^2*(3-2x)
    float s = x * x * (3.0f - 2.0f * x);
    float y = powf(clamp01(s), g);
    out[i] = clampPwm((int32_t)lroundf(y * (float)PWM_MAX));
  }
  enforce_endpoints(out);
}

} // namespace DimCurve
