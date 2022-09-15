/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */.

#pragma once

#include <array>
#include <fmt/format.h>
#include <string_view>
#include <utility>

namespace lunar {

enum Level {
  Trace =  1,
  Debug =  2,
  Info  =  4,
  Warn  =  8,
  Error = 16,
  Fatal = 32,

  All = Trace | Debug | Info | Warn | Error | Fatal
};

namespace detail {

#if defined(NDEBUG)
  static constexpr int kLogMask = 0;//Info | Warn | Error | Fatal;
#else
  static constexpr int kLogMask = All;
#endif

} // namespace lunar::detail

template<Level level, typename... Args>
inline void Log(std::string_view format, Args... args) {
  if constexpr((detail::kLogMask & level) != 0) {
    char const* prefix = "[?]";

    if constexpr(level == Trace) prefix = "\e[36m[T]";
    if constexpr(level == Debug) prefix = "\e[34m[D]";
    if constexpr(level ==  Info) prefix = "\e[37m[I]";
    if constexpr(level ==  Warn) prefix = "\e[33m[W]";
    if constexpr(level == Error) prefix = "\e[35m[E]";
    if constexpr(level == Fatal) prefix = "\e[31m[F]";

    fmt::print("{} {}\n", prefix, fmt::format(format, args...));
  }
}

template<typename... Args>
inline void Assert(bool condition, Args... args) {
  if (!condition) {
    Log<Fatal>(args...);
    std::exit(-1);
  }
}

// For compatibility with Duality logging code:
// TODO: remove this once we're done migrating the code.
#define LOG_TRACE lunar::Log<Trace>
#define LOG_DEBUG lunar::Log<Debug>
#define LOG_INFO lunar::Log<Info>
#define LOG_WARN lunar::Log<Warn>
#define LOG_ERROR lunar::Log<Error>
#define LOG_FATAL lunar::Log<Fatal>
#define ASSERT lunar::Assert
#define UNREACHABLE ASSERT(false, "reached supposedly unreachable code.");

} // namespace lunar
