/*
 * Copyright (C) 2020 fleroviux
 */

#include "video_unit.hpp"

namespace fauxDS::core {

VideoUnit::VideoUnit(Scheduler* scheduler)
    : scheduler(scheduler) {
  Reset();
}

void VideoUnit::Reset() {
  dispstat = {};
  vcount = {};
}

} // namespace fauxDS::core
