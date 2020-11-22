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
    : ppu_a(0, vram.region_ppu_a_bg, vram.region_ppu_a_obj, &pram[0x000])
    , ppu_b(1, vram.region_ppu_b_bg, vram.region_ppu_b_obj, &pram[0x400]) 
    , scheduler(scheduler)
    , irq7(irq7)
    , irq9(irq9)
    , dma7(dma7)
    , dma9(dma9) {
  Reset();
}

void VideoUnit::Reset() {
  memset(pram, 0, sizeof(pram));
  vram.Reset();
  dispstat7 = {};
  dispstat9 = {};
  vcount = {};

  ppu_a.Reset();
  ppu_b.Reset();

  OnHdrawBegin(0);
}

void VideoUnit::OnHdrawBegin(int late) {
  if (++vcount.value == kTotalLines) {
    vcount.value = 0;
  }

  dispstat7.vcount.flag = vcount.value == dispstat7.vcount_setting;
  dispstat9.vcount.flag = vcount.value == dispstat9.vcount_setting;

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

  if (vcount.value < kDrawingLines) {
    ppu_a.RenderScanline(vcount.value);
    ppu_b.RenderScanline(vcount.value);
  }

  scheduler.Add(1536 - late, this, &VideoUnit::OnHblankBegin);
}

void VideoUnit::OnHblankBegin(int late) {
  if (dispstat7.hblank.enable_irq) {
    irq7.Raise(IRQ::Source::HBlank);
  }

  if (dispstat9.hblank.enable_irq) {
    irq9.Raise(IRQ::Source::HBlank);
  }

  scheduler.Add(70 - late, this, &VideoUnit::OnHblankFlagSet);
}

void VideoUnit::OnHblankFlagSet(int late) {
  // TODO: should Hblank DMA be triggered here or in "OnHblankBegin"? 
  if (vcount.value <= kDrawingLines - 1) {
    dma9.Request(DMA9::Time::HBlank);
  }

  dispstat7.hblank.flag = true;
  dispstat9.hblank.flag = true;

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

} // namespace fauxDS::core
