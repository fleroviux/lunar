/*
 * Copyright (C) 2020 fleroviux
 */

#include "video_unit.hpp"

namespace fauxDS::core {

static constexpr int kDrawingLines = 192;
static constexpr int kBlankingLines = 71;
static constexpr int kTotalLines = kDrawingLines + kBlankingLines;

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
  OnHdrawBegin(0);
}

void VideoUnit::OnHdrawBegin(int late) {
  if (++vcount.value == kTotalLines) {
    vcount.value = 0;
  }

  dispstat.vcount.flag = vcount.value == dispstat.vcount_setting;

  if (dispstat.vcount.enable_irq && dispstat.vcount.flag) {
    irq7.Raise(IRQ::Source::VCount);
    irq9.Raise(IRQ::Source::VCount);
  }

  if (vcount.value == kDrawingLines) {
    if (dispstat.vblank.enable_irq) {
      irq7.Raise(IRQ::Source::VBlank);
      irq9.Raise(IRQ::Source::VBlank);
    }
    dispstat.vblank.flag = true;
  }
  
  if (vcount.value == kTotalLines - 1) {
    dispstat.vblank.flag = false;
  }

  dispstat.hblank.flag = false;

  scheduler->Add(1536 - late, [this](int late) {
    this->OnHblankBegin(late);
  });
}

void VideoUnit::OnHblankBegin(int late) {
  if (dispstat.hblank.enable_irq) {
    irq7.Raise(IRQ::Source::HBlank);
    irq9.Raise(IRQ::Source::HBlank);
  }

  scheduler->Add(70 - late, [this](int late) {
    this->OnHblankFlagSet(late);
  });
}

void VideoUnit::OnHblankFlagSet(int late) {
  dispstat.hblank.flag = true;

  scheduler->Add(594 - late, [this](int late) {
    this->OnHdrawBegin(late);
  });
}

} // namespace fauxDS::core
