/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <functional>
#include <lunar/device/video_device.hpp>
#include <lunar/integer.hpp>
#include <lunar/log.hpp>

#include "common/scheduler.hpp"
#include "nds/arm7/dma/dma.hpp"
#include "nds/arm9/dma/dma.hpp"
#include "nds/irq/irq.hpp"
#include "gpu/gpu.hpp"
#include "ppu/ppu.hpp"
#include "vram.hpp"

namespace lunar::nds {

// Graphics subsystem which contains two 2D PPUs (A and B) and a 3D GPU.
struct VideoUnit {
  enum class Screen { Top, Bottom };

  VideoUnit(Scheduler& scheduler, IRQ& irq7, IRQ& irq9, DMA7& dma7, DMA9& dma9);

  void Reset();
  void SetVideoDevice(VideoDevice& device);
  auto GetOutput(Screen screen) -> u32 const*;

  // Graphics status and IRQ control.
  struct DisplayStatus {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct lunar::nds::VideoUnit;

    struct {
      bool flag = false;
      bool enable_irq = false;
    } vblank = {}, hblank = {}, vcount = {};

    u16 vcount_setting = 0;

    std::function<void(void)> write_cb;
  } dispstat7, dispstat9;

  // Currently rendered scanline.
  // TODO: "VCOUNT register is write-able, allowing to synchronize linked DS consoles."
  struct VCOUNT {
    auto ReadByte(uint offset) -> u8;

  private:
    friend struct lunar::nds::VideoUnit;
    u16 value = 0xFFFF;
  } vcount;

  // Graphics power control register
  // TODO: right now all bits except for "display_swap" will be ignored.
  struct PowerControl {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct lunar::nds::VideoUnit;

    bool enable_lcds = false;
    bool enable_ppu_a = false;
    bool enable_ppu_b = false;
    bool enable_gpu_geometry = false;
    bool enable_gpu_render = false;
    bool display_swap = false;
  } powcnt1;

  struct CaptureControl {
    enum class SourceA {
      GPUAndPPU = 0,
      GPU = 1
    };

    enum class SourceB {
      VRAM = 0,
      FIFO = 1
    };

    enum class CaptureSource {
      A = 0,
      B = 1,
      Both = 2
    };

    int eva = 0;
    int evb = 0;
    int vram_write_block = 0;
    int vram_write_offset = 0;
    int capture_size = 0;
    SourceA source_a = SourceA::GPUAndPPU;
    SourceB source_b = SourceB::VRAM;
    int vram_read_offset = 0;
    CaptureSource capture_source = CaptureSource::A;
    bool busy = false;

    auto ReadByte(uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
  } dispcapcnt;

  bool capturing;
  bool display_swap;

  u8 pram[0x800];
  u8 oam[0x800];
  VRAM vram;
  GPU gpu;
  PPU ppu_a;
  PPU ppu_b;

private:
  void CheckVerticalCounterIRQ(DisplayStatus& dispstat, IRQ& irq);
  void OnHdrawBegin(int late);
  void OnHblankBegin(int late);
  void RunDisplayCapture();

  Scheduler& scheduler;
  IRQ& irq7;
  IRQ& irq9;
  DMA7& dma7;
  DMA9& dma9;
  VideoDevice* video_device = nullptr;
};

} // namespace lunar::nds
