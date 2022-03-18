/*
 * Copyright (C) 2020 fleroviux
 */

#include <string.h>

#include "video_unit.hpp"

namespace Duality::Core {

static constexpr int kDrawingLines = 192;
static constexpr int kBlankingLines = 71;
static constexpr int kTotalLines = kDrawingLines + kBlankingLines;

VideoUnit::VideoUnit(Scheduler& scheduler, IRQ& irq7, IRQ& irq9, DMA7& dma7, DMA9& dma9)
    : gpu(scheduler, irq9, dma9, vram)
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
  dispcapcnt = {};
  capturing = false;
  display_swap = false;

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

void VideoUnit::SetVideoDevice(VideoDevice& device) {
  video_device = &device;
}

auto VideoUnit::GetOutput(Screen screen) -> u32 const* {
  switch (screen) {
    case Screen::Top:
      return display_swap ? ppu_a.GetOutput() : ppu_b.GetOutput();
    default:
      return display_swap ? ppu_b.GetOutput() : ppu_a.GetOutput();
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
    capturing = dispcapcnt.busy;
    display_swap = powcnt1.display_swap;
  }

  CheckVerticalCounterIRQ(dispstat7, irq7);
  CheckVerticalCounterIRQ(dispstat9, irq9);

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

    if (video_device != nullptr) {
      video_device->Draw(GetOutput(Screen::Top), GetOutput(Screen::Bottom));
    }

    gpu.SwapBuffers();

    dispcapcnt.busy = false;
    capturing = false;
  }
  
  if (vcount.value == kTotalLines - 1) {
    dispstat7.vblank.flag = false;
    dispstat9.vblank.flag = false;
  }

  if (vcount.value == kTotalLines - 48) {
    gpu.Render();
  }

  dispstat7.hblank.flag = false;
  dispstat9.hblank.flag = false;

  if (vcount.value <= kDrawingLines - 1) {
    // TODO: check if display capture actually reads BG+3D
    ppu_a.OnDrawScanlineBegin(vcount.value, dispcapcnt.busy);
    ppu_b.OnDrawScanlineBegin(vcount.value, false);
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
    if (capturing) {
      RunDisplayCapture();
    }
  }

  scheduler.Add(524 - late, this, &VideoUnit::OnHdrawBegin);
}

void VideoUnit::RunDisplayCapture() {
  constexpr int kCaptureWidthLUT[4]{ 128, 256, 256, 256 };
  constexpr int kCaptureHeightLUT[4]{ 128, 64, 128, 192 };

  auto height = kCaptureHeightLUT[dispcapcnt.capture_size];

  if (vcount.value < height) {
    auto width = kCaptureWidthLUT[dispcapcnt.capture_size];
    auto capture_source = dispcapcnt.capture_source;
    auto line_offset = vcount.value * 256;

    auto vram_write_base = dispcapcnt.vram_write_block << 17;
    auto vram_write_offset = dispcapcnt.vram_write_offset << 15;
    auto buffer_dst = vram.region_lcdc.GetUnsafePointer<u16>(
      vram_write_base + ((vram_write_offset + line_offset * sizeof(u16)) & 0x1FFFF));

    if (unlikely(buffer_dst == nullptr)) {
      return;
    }

    auto capture_a = [&](u16* dst) {
      if (dispcapcnt.source_a == CaptureControl::SourceA::GPUAndPPU) {
        std::memcpy(dst, ppu_a.GetComposerOutput(), sizeof(u16) * width);
      } else {
        auto src = gpu.GetOutput() + line_offset;

        for (int x = 0; x < width; x++) {
          auto& color = src[x];
          dst[x] = color.to_rgb555();
          if (color.a() != 0) {
            dst[x] |= 0x8000;
          }
        }
      }
    };

    auto capture_b = [&](u16* dst) {
      if (dispcapcnt.source_b == CaptureControl::SourceB::VRAM) {
        auto vram_read_base = ppu_a.mmio.dispcnt.vram_block << 17;
        auto vram_read_offset = dispcapcnt.vram_read_offset << 15;
        auto src = vram.region_lcdc.GetUnsafePointer<u16>(
          vram_read_base + ((vram_read_offset + line_offset * sizeof(u16)) & 0x1FFFF));

        if (likely(src != nullptr)) {
          std::memcpy(dst, src, sizeof(u16) * width);
        } else {
          std::memset(dst, 0, sizeof(u16) * width);
        }
      } else {
        ASSERT(false, "VideoUnit: unhandled main memory display FIFO capture");
      }
    };

    switch (capture_source) {
      case CaptureControl::CaptureSource::A: {
        capture_a(buffer_dst);
        break;
      }
      case CaptureControl::CaptureSource::B: {
        capture_b(buffer_dst);
        break;
      }
      default: {
        u16 buffer_a[width];
        u16 buffer_b[width];

        capture_a(buffer_a);
        capture_b(buffer_b);

        auto eva = std::min(dispcapcnt.eva, 16);
        auto evb = std::min(dispcapcnt.evb, 16);
        bool need_clamp = (eva + evb) > 16;

        for (int x = 0; x < width; x++) {
          auto color_a = buffer_a[x];
          auto color_b = buffer_b[x];

          auto r_a = (color_a >>  0) & 31;
          auto g_a = (color_a >>  5) & 31;
          auto b_a = (color_a >> 10) & 31;
          auto a_a =  color_a >> 15;

          auto r_b = (color_b >>  0) & 31;
          auto g_b = (color_b >>  5) & 31;
          auto b_b = (color_b >> 10) & 31;
          auto a_b =  color_b >> 15;

          auto factor_a = a_a * eva;
          auto factor_b = a_b * evb;

          auto r_out = (r_a * factor_a + r_b * factor_b) >> 4;
          auto g_out = (g_a * factor_a + g_b * factor_b) >> 4;
          auto b_out = (b_a * factor_a + b_b * factor_b) >> 4;
          auto a_out = (a_a & ((eva > 0) ? 1 : 0)) | (a_b & ((evb > 0) ? 1 : 0));

          if (need_clamp) {
            r_out = std::min(r_out, 15);
            g_out = std::min(g_out, 15);
            b_out = std::min(b_out, 15);
          }

          buffer_dst[x] = r_out | (g_out << 5) | (b_out << 10) | (a_out << 15);
        }
        break;
      }
    }
  }
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

auto VideoUnit::CaptureControl::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0: {
      return eva;
    }
    case 1: {
      return evb;
    }
    case 2: {
      return vram_write_block |
            (vram_write_offset << 2) |
            (capture_size      << 4);
    }
    case 3: {
      return (int)source_a |
            ((int)source_b         << 1) |
            (     vram_read_offset << 2) |
            ((int)capture_source   << 5) |
            (busy ? 128 : 0);
    }
  }

  UNREACHABLE;
}

void VideoUnit::CaptureControl::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      eva = value & 31;
      break;
    }
    case 1: {
      evb = value & 31;
      break;
    }
    case 2: {
      vram_write_block = value & 3;
      vram_write_offset = (value >> 2) & 3;
      capture_size = (value >> 4) & 3;
      break;
    }
    case 3: {
      source_a = (SourceA)(value & 1);
      source_b = (SourceB)((value >> 1) & 1);
      vram_read_offset = (value >> 2) & 3;
      capture_source = (CaptureSource)((value >> 5) & 3);
      busy = value & 128;
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

} // namespace Duality::Core
