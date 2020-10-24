/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/static_for.hpp>

#include "../arm.hpp"
#include "decoder.hpp"

namespace fauxDS::core::arm {

using Handler16 = ARM::Handler16;
using Handler32 = ARM::Handler32;

/** A helper class used to generate lookup tables for
  * the interpreter at compiletime.
  * The motivation is to separate the code used for generation from
  * the interpreter class and its header itself.
  */
struct TableGen {
  #ifdef __clang__
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Weverything"
  #endif

  #include "gen_arm.hpp"
  #include "gen_thumb.hpp"

  #ifdef __clang__
  #pragma clang diagnostic pop
  #endif

  static constexpr auto GenerateTableThumb() -> std::array<Handler16, 2048> {
    std::array<Handler16, 2048> lut = {};

    common::static_for<std::size_t, 0, 2048>([&](auto i) {
      lut[i] = GenerateHandlerThumb<i << 5>();
    });
    return lut;
  }

  static constexpr auto GenerateTableARM() -> std::array<Handler32, 8192> {
    std::array<Handler32, 8192> lut = {};

    // Conditional instructions
    common::static_for<std::size_t, 0, 4096>([&](auto i) {
      // HACK: set bits 16-19 all to one, so that
      // no "status transfer" instructions will be decoded as "hint" 
      // instructions.
      lut[i] = GenerateHandlerARM<
        ((i & 0xFF0) << 16) | 
        ((i & 0xF) << 4) | 
        0xF0000>();
    });

    // Unconditional instructions
    common::static_for<std::size_t, 0, 4096>([&](auto i) {
      lut[4096 + i] = GenerateHandlerARM<
        ((i & 0xFF0) << 16) |
        ((i & 0xF) << 4) |
        0xF0000000>();
    });

    return lut;
  }
};

std::array<Handler16, 2048> ARM::s_opcode_lut_16 = TableGen::GenerateTableThumb();
std::array<Handler32, 8192> ARM::s_opcode_lut_32 = TableGen::GenerateTableARM();

} // namespace fauxDS::core::arm
