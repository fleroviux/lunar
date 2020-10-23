/*
 * Copyright (C) 2020 fleroviux
 */

#include "video_unit.hpp"

namespace fauxDS::core {

VideoUnit::VideoUnit() {
  Reset();
}

void VideoUnit::Reset() {
  dispstat = {};
}

} // namespace fauxDS::core
