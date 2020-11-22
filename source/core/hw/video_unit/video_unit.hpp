/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/hw/dma/dma7.hpp>
#include <core/hw/dma/dma9.hpp>
#include <core/hw/irq/irq.hpp>
#include <core/scheduler.hpp>
#include <functional>

#include "ppu/ppu.hpp"
#include "vram.hpp"

namespace fauxDS::core {

/// Graphics subsystem which contains two 2D PPUs (A and B) and a 3D GPU.
struct VideoUnit {
  VideoUnit(Scheduler& scheduler, IRQ& irq7, IRQ& irq9, DMA7& dma7, DMA9& dma9);

  void Reset();

  /// Graphics status and IRQ control.
  struct DisplayStatus {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::VideoUnit;

    struct {
      bool flag = false;
      bool enable_irq = false;
    } vblank = {}, hblank = {}, vcount = {};

    u16 vcount_setting = 0;

    std::function<void(void)> write_cb;
  } dispstat7, dispstat9;

  /// Currently rendered scanline.
  /// TODO: "VCOUNT register is write-able, allowing to synchronize linked DS consoles."
  struct VCOUNT {
    auto ReadByte(uint offset) -> u8;

  private:
    friend struct fauxDS::core::VideoUnit;
    u16 value = 0xFFFF;
  } vcount;

  u8 pram[0x800];
  u8 oam[0x800];
  VRAM vram;
  PPU ppu_a;
  PPU ppu_b;

private:
  void CheckVerticalCounterIRQ(DisplayStatus& dispstat, IRQ& irq);
  void OnHdrawBegin(int late);
  void OnHblankBegin(int late);

  Scheduler& scheduler;
  IRQ& irq7;
  IRQ& irq9;
  DMA7& dma7;
  DMA9& dma9;
};

} // namespace fauxDS::core
