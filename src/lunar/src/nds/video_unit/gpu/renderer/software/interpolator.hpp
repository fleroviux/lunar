/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/integer.hpp>

#include "nds/video_unit/gpu/color.hpp"

namespace lunar::nds {

template<int precision>
struct Interpolator {
  void Setup(u16 w0, u16 w1, s32 x, s32 x_min, s32 x_max) {
    CalculateLerpFactor(x, x_min, x_max);
    CalculatePerpFactor(w0, w1, x, x_min, x_max);

    // TODO: overwrite the perp factor instead of setting a flag?
    if constexpr (precision == 9) {
      force_lerp = w0 == w1 && (w0 & 0x7E) == 0 && (w1 & 0x7E) == 0;
    } else {
      force_lerp = w0 == w1 && (w0 & 0x7F) == 0 && (w1 & 0x7F) == 0;
    }
  }

  // TODO: use correct formulas and clean this up.

  template<typename T>
  auto Interpolate(T a, T b) -> T {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    return (a * ((1 << precision) - factor) + b * factor) >> precision;
  }

  template<typename T>
  auto InterpolateLinear(T a, T b) -> T {
    return (a * ((1 << precision) - factor_lerp) + b * factor_lerp) >> precision;
  }

  // TODO: generalize this for arbitrary vectors?

  auto Interpolate(Color4 const& color_a, Color4 const& color_b, Color4& color_out) {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    for (int i = 0; i < 3; i++) {
      color_out[i] = (color_a[i].raw() * ((1 << precision) - factor) + color_b[i].raw() * factor) >> precision;

      // // Formula from melonDS interpolation article.
      // // is there actually a mathematical difference?
      // // This certainly causes interpolation issues unless we expand the interpolated values to higher precision.
      // auto a = color_a[i].raw();
      // auto b = color_b[i].raw();
      // if (a < b) {
      //   color_outi] = (a + (b - a) * factor) >> precision;
      // } else {
      //   color_out[i] = (b + (a - b) * (((1 << precision) - 1) - factor)) >> precision;
      // }
    }
  }

  auto Interpolate(Vector2<Fixed12x4> const& uv_a, Vector2<Fixed12x4> const& uv_b, Vector2<Fixed12x4>& uv_out) {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    for (int i = 0; i < 2; i++) {
      uv_out[i] = (uv_a[i].raw() * ((1 << precision) - factor) + uv_b[i].raw() * factor) >> precision;
    }
  }

private:
  void CalculateLerpFactor(s32 x, s32 x_min, s32 x_max) {
    // TODO: make sure that this uses the correct amount of precision.
    u32 denominator = x_max - x_min;
    u32 numerator = (x - x_min) << precision;

    // TODO: how does the DS GPU deal with division-by-zero?
    if (denominator != 0) {
      factor_lerp = numerator / denominator;
    } else {
      factor_lerp = 0;
    }
  }

  void CalculatePerpFactor(u16 w0, u16 w1, s32 x, s32 x_min, s32 x_max) {
    u16 w0_num = w0;
    u16 w0_denom = w0;
    u16 w1_denom = w1;

    if constexpr(precision == 9) {
      w0_num   >>= 1;
      w0_denom >>= 1;
      w1_denom >>= 1;

      if ((w0 & 1) && !(w1 & 1)) {
        w0_denom++;
      }
    }

    u32 t0 = x - x_min;
    u32 t1 = x_max - x;
    u32 denominator = t1 * w1_denom + t0 * w0_denom;
    u32 numerator = (t0 << precision) * w0_num;

    // TODO: how does the DS handle division-by-zero here?
    if (denominator != 0) {
      factor_perp = numerator / denominator;
    } else {
      factor_perp = 0;
    }
  }

  u32 factor_lerp;
  u32 factor_perp;
  bool force_lerp;
};

} // namespace lunar::nds
