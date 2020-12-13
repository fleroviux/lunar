/*
 * Copyright (C) 2020 fleroviux
 */

#include <string.h>

#include "video_unit.hpp"

namespace fauxDS::core {

static constexpr int kDrawingLines = 192;
static constexpr int kBlankingLines = 71;
static constexpr int kTotalLines = kDrawingLines + kBlankingLines;

VideoUnit::VideoUnit(Scheduler& scheduler, IRQ& irq7, IRQ& irq9, DMA7& dma7, DMA9& dma9)
    : gpu(scheduler, irq9)
    , ppu_a(0, vram, &pram[0x000], &oam[0x000], gpu.GetOutput())
    , ppu_b(1, vram, &pram[0x400], &oam[0x400])
    , scheduler(scheduler)
    , irq7(irq7)
    , irq9(irq9)
    , dma7(dma7)
    , dma9(dma9) {
  Reset();
}

void VideoUnit::Reset() {
  memset(pram, 0, sizeof(pram));
  memset(oam, 0, sizeof(oam));
  vram.Reset();
  dispstat7 = {};
  dispstat9 = {};
  vcount = {};
  powcnt1 = {};

  dispstat7.write_cb = [this]() {
    CheckVerticalCounterIRQ(dispstat7, irq7);
  };

  dispstat9.write_cb = [this]() {
    CheckVerticalCounterIRQ(dispstat9, irq9);
  };

  ppu_a.Reset();
  ppu_b.Reset();

  OnHdrawBegin(0);
}

auto VideoUnit::GetOutput(Screen screen) -> u32 const* {
  switch (screen) {
    case Screen::Top:
      return powcnt1.display_swap ? ppu_a.GetOutput() : ppu_b.GetOutput();
    case Screen::Bottom:
      return powcnt1.display_swap ? ppu_b.GetOutput() : ppu_a.GetOutput();
  }
}

void VideoUnit::CheckVerticalCounterIRQ(DisplayStatus& dispstat, IRQ& irq) {
  auto flag_new = dispstat.vcount_setting == vcount.value;

  if (dispstat.vcount.enable_irq && !dispstat.vcount.flag && flag_new) {
    irq.Raise(IRQ::Source::VCount);
  }
  
  dispstat.vcount.flag = flag_new;
}

void VideoUnit::OnHdrawBegin(int late) {
  if (++vcount.value == kTotalLines) {
    vcount.value = 0;
  }

  CheckVerticalCounterIRQ(dispstat7, irq7);
  CheckVerticalCounterIRQ(dispstat9, irq9);

  if (dispstat7.vcount.enable_irq && dispstat7.vcount.flag) {
    irq7.Raise(IRQ::Source::VCount);
  }

  if (dispstat9.vcount.enable_irq && dispstat9.vcount.flag) {
    irq9.Raise(IRQ::Source::VCount);
  }

  if (vcount.value == kDrawingLines) {
    dma7.Request(DMA7::Time::VBlank);
    dma9.Request(DMA9::Time::VBlank);

    if (dispstat7.vblank.enable_irq) {
      irq7.Raise(IRQ::Source::VBlank);
    }

    if (dispstat9.vblank.enable_irq) {
      irq9.Raise(IRQ::Source::VBlank);
    }

    dispstat7.vblank.flag = true;
    dispstat9.vblank.flag = true;
  }
  
  if (vcount.value == kTotalLines - 1) {
    dispstat7.vblank.flag = false;
    dispstat9.vblank.flag = false;
  }

  dispstat7.hblank.flag = false;
  dispstat9.hblank.flag = false;

  if (vcount.value <= kDrawingLines - 1) {
    ppu_a.OnDrawScanlineBegin(vcount.value);
    ppu_b.OnDrawScanlineBegin(vcount.value);
  } else {
    ppu_a.OnBlankScanlineBegin(vcount.value);
    ppu_b.OnBlankScanlineBegin(vcount.value);    
  }

  scheduler.Add(1606 - late, this, &VideoUnit::OnHblankBegin);
}

void VideoUnit::OnHblankBegin(int late) {
  dispstat7.hblank.flag = true;
  dispstat9.hblank.flag = true;

  if (dispstat7.hblank.enable_irq) {
    irq7.Raise(IRQ::Source::HBlank);
  }

  if (dispstat9.hblank.enable_irq) {
    irq9.Raise(IRQ::Source::HBlank);
  }

  if (vcount.value <= kDrawingLines - 1) {
    dma9.Request(DMA9::Time::HBlank);
    ppu_a.OnDrawScanlineEnd();
    ppu_b.OnDrawScanlineEnd();
  }

  scheduler.Add(594 - late, this, &VideoUnit::OnHdrawBegin);
}

auto VideoUnit::DisplayStatus::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return (vblank.flag ? 1 : 0) |
             (hblank.flag ? 2 : 0) |
             (vcount.flag ? 4 : 0) |
             (vblank.enable_irq ?  8 : 0) |
             (hblank.enable_irq ? 16 : 0) |
             (vcount.enable_irq ? 32 : 0) |
             ((vcount_setting >> 1) & 128);
    case 1:
      return vcount_setting & 0xFF;
  }
 
  UNREACHABLE;
}

void VideoUnit::DisplayStatus::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      vblank.enable_irq = value &  8;
      hblank.enable_irq = value & 16;
      vcount.enable_irq = value & 32;
      vcount_setting = (vcount_setting & 0xFF) | ((value << 1) & 0x100);
      break;
    case 1:
      vcount_setting = (vcount_setting & 0x100) | value;
      break;
    default:
      UNREACHABLE;
  }

  write_cb();
}

auto VideoUnit::VCOUNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return value & 0xFF;
    case 1:
      return (value >> 8) & 1;
  }

  UNREACHABLE;
}

auto VideoUnit::PowerControl::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return (enable_lcds  ? 1 : 0) |
             (enable_ppu_a ? 2 : 0) |
             (enable_ppu_b ? 4 : 0) |
             (enable_gpu_geometry ? 8 : 0);
    case 1:
      return (enable_gpu_render ? 2 : 0) |
             (display_swap ? 128 : 0);
  }

  UNREACHABLE;
}

void VideoUnit::PowerControl::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      enable_lcds = value & 1;
      enable_ppu_a = value & 2;
      enable_ppu_b = value & 4;
      enable_gpu_geometry = value & 8;
      break;
    case 1:
      enable_gpu_render = value & 2;
      display_swap = value & 128;
      break;
    default:
      UNREACHABLE;
  }
}

} // namespace fauxDS::core
