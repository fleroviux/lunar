/*
 * Copyright (C) 2020 fleroviux
 */

#include "video_unit.hpp"

namespace fauxDS::core {

VideoUnit::VideoUnit(Scheduler* scheduler, IRQ& irq7, IRQ& irq9)
    : scheduler(scheduler)
    , irq7(irq7)
    , irq9(irq9) {
  Reset();
}

void VideoUnit::Reset() {
  dispstat = {};
  vcount = {};

  // HACK: make the VCOUNT value start at zero for the first scanline.
  vcount.value = 0xFFFF;
  OnHdrawBegin();
}

void VideoUnit::OnHdrawBegin() {
  if (++vcount.value == 263) {
    vcount.value = 0;
  }

  dispstat.vcount.flag = vcount.value == dispstat.vcount_setting;

  if (dispstat.vcount.enable_irq && dispstat.vcount.flag) {
    irq7.Raise(IRQ::Source::VCount);
    irq9.Raise(IRQ::Source::VCount);
  }

  if (vcount.value == 192) {
    if (dispstat.vblank.enable_irq) {
      irq7.Raise(IRQ::Source::VBlank);
      irq9.Raise(IRQ::Source::VBlank);
    }
    dispstat.vblank.flag = true;
  }
  
  if (vcount.value == 261) {
    dispstat.vblank.flag = false;
  }

  dispstat.hblank.flag = false;

  scheduler->Add(1536, [this](int late) {
    this->OnHblankBegin();
  });
}

void VideoUnit::OnHblankBegin() {
  if (dispstat.hblank.enable_irq) {
    irq7.Raise(IRQ::Source::HBlank);
    irq9.Raise(IRQ::Source::HBlank);
  }

  // TODO: according to GBATEK the H-blank flag toggle is slightly delay on NDS7.
  scheduler->Add(70, [this](int late) {
    this->OnHblankFlagSet();
  });
}

void VideoUnit::OnHblankFlagSet() {
  dispstat.hblank.flag = true;

  scheduler->Add(594, [this](int late) {
    this->OnHdrawBegin();
  });
}
  

} // namespace fauxDS::core
