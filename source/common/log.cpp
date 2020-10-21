/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "log.hpp"

namespace common::logger {

auto trim_filepath(const char* file) -> std::string {
  auto tmp = std::string{file};
#ifdef WIN32
  auto pos = tmp.find("\\source\\");
#else
  auto pos = tmp.find("/source/");
#endif
  if (pos == std::string::npos) {
    return "???";
  }
  return tmp.substr(pos);
}

void append(Level level,
            const char* file,
            int line,
            std::string const& message) {
  std::string prefix;

  switch (level) {
    case Level::Trace:
      prefix = "\e[36m[T]";
      break;
    case Level::Debug:
      prefix = "\e[34m[D]";
      break;
    case Level::Info:
      prefix = "\e[37m[I]";
      break;
    case Level::Warn:
      prefix = "\e[33m[W]";
      break;
    case Level::Error:
      prefix = "\e[35m[E]";
      break;
    case Level::Fatal:
      prefix = "\e[31m[F]";
      break;
  }

  fmt::print("{0} {1}:{2}: {3}\e[39m\n", prefix, trim_filepath(file), line, message);
}

} // namespace common::logger

