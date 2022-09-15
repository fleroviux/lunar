/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <algorithm>
#include <string.h>

#include "ppu.hpp"

namespace lunar::nds {

PPU::PPU(
  int id,
  VRAM const& vram,
  u8   const* pram,
  u8   const* oam,
  Color4 const* gpu_output
)   : id(id)
    , vram_bg(vram.region_ppu_bg[id])
    , vram_obj(vram.region_ppu_obj[id])
    , extpal_bg(vram.region_ppu_bg_extpal[id])
    , extpal_obj(vram.region_ppu_obj_extpal[id])
    , vram_lcdc(vram.region_lcdc)
    , pram(pram)
    , oam(oam)
    , gpu_output(gpu_output) {
  if (id == 0) {
    mmio.dispcnt = {};
  } else {
    mmio.dispcnt = {0xC033FFF7};
  }
  Reset();
  RegisterMapUnmapCallbacks();
}

PPU::~PPU() {
  StopRenderWorker();
}

void PPU::Reset() {
  memset(output, 0, sizeof(output));

  mmio.dispcnt.Reset();
  
  for (int i = 0; i < 4; i++) {
    mmio.bgcnt[i].Reset();
    mmio.bghofs[i].Reset();
    mmio.bgvofs[i].Reset();
  }

  for (int i = 0; i < 2; i++) {
    mmio.bgpa[i].WriteByte(0, 0);
    mmio.bgpa[i].WriteByte(1, 1);
    mmio.bgpb[i].WriteByte(0, 0);
    mmio.bgpb[i].WriteByte(1, 0);
    mmio.bgpc[i].WriteByte(0, 0);
    mmio.bgpc[i].WriteByte(1, 0);
    mmio.bgpd[i].WriteByte(0, 0);
    mmio.bgpd[i].WriteByte(1, 1);
    mmio.bgx[i].Reset();
    mmio.bgy[i].Reset();
    mmio.winh[i].Reset();
    mmio.winv[i].Reset();
  }

  mmio.winin.Reset();
  mmio.winout.Reset();

  mmio.bldcnt.Reset();
  mmio.bldalpha.Reset();
  mmio.bldy.Reset();

  mmio.mosaic.Reset();

  current_vcount = 0;

  vram_bg_dirty = {0, sizeof(render_vram_bg)};
  vram_obj_dirty = {0, sizeof(render_vram_obj)};
  extpal_bg_dirty = {0, sizeof(render_extpal_bg)};
  extpal_obj_dirty = {0, sizeof(render_extpal_obj)};
  vram_lcdc_dirty = {0, sizeof(render_vram_lcdc)};
  pram_dirty = {0,sizeof(render_pram)};
  oam_dirty = {0, sizeof(render_oam)};

  SetupRenderWorker();
}

void PPU::OnDrawScanlineBegin(u16 vcount, bool capture_bg_and_3d) {
  current_vcount = vcount;

  SubmitScanline(vcount, capture_bg_and_3d);
}

void PPU::OnDrawScanlineEnd() {
  auto& dispcnt = mmio.dispcnt;
  auto& bgx = mmio.bgx;
  auto& bgy = mmio.bgy;
  auto& mosaic = mmio.mosaic;

  // Advance vertical background mosaic counter
  if (++mosaic.bg._counter_y == mosaic.bg.size_y) {
    mosaic.bg._counter_y = 0;
  }

  // Advance vertical OBJ mosaic counter
  if (++mosaic.obj._counter_y == mosaic.obj.size_y) {
    mosaic.obj._counter_y = 0;
  }

  /* Mode 0 doesn't have any affine backgrounds,
   * in that case the internal registers seemingly aren't updated.
   * TODO: needs more research, e.g. what happens if all affine backgrounds are disabled?
   */
  if (dispcnt.bg_mode != 0) {
    // Advance internal affine registers and apply vertical mosaic if applicable.
    for (int i = 0; i < 2; i++) {
      if (mmio.bgcnt[2 + i].enable_mosaic) {
        if (mosaic.bg._counter_y == 0) {
          bgx[i]._current += mosaic.bg.size_y * mmio.bgpb[i].value;
          bgy[i]._current += mosaic.bg.size_y * mmio.bgpd[i].value;
        }
      } else {
        bgx[i]._current += mmio.bgpb[i].value;
        bgy[i]._current += mmio.bgpd[i].value;
      }
    }
  }
}

void PPU::OnBlankScanlineBegin(u16 vcount) {
  auto& bgx = mmio.bgx;
  auto& bgy = mmio.bgy;
  auto& mosaic = mmio.mosaic;

  current_vcount = vcount;

  // TODO: when exactly are these registers reloaded?
  if (vcount == 192) {
    // Reset vertical mosaic counters
    mosaic.bg._counter_y = 0;
    mosaic.obj._counter_y = 0;

    // Reload internal affine registers
    bgx[0]._current = bgx[0].initial;
    bgy[0]._current = bgy[0].initial;
    bgx[1]._current = bgx[1].initial;
    bgy[1]._current = bgy[1].initial;
  }

  SubmitScanline(vcount, false);
}

void PPU::RenderScanline(u16 vcount, bool capture_bg_and_3d) {
  auto display_mode = mmio_copy[vcount].dispcnt.display_mode;

  if (capture_bg_and_3d || display_mode == 1) {
    RenderBackgroundsAndComposite(vcount);
  }

  switch (display_mode) {
    case 0:
      RenderDisplayOff(vcount);
      break;
    case 1:
      RenderNormal(vcount);
      break;
    case 2:
      RenderVideoMemoryDisplay(vcount);
      break;
    case 3:
      RenderMainMemoryDisplay(vcount);
      break;
  }
}

void PPU::RenderDisplayOff(u16 vcount) {
  u32* line = &output[vcount * 256];

  for (uint x = 0; x < 256; x++) {
    line[x] = ConvertColor(0x7FFF);
  }
}

void PPU::RenderNormal(u16 vcount) {
  u32* line = &output[vcount * 256];

  for (uint x = 0; x < 256; x++) {
    line[x] = ConvertColor(buffer_compose[x]);
  }
}

void PPU::RenderVideoMemoryDisplay(u16 vcount) {
  u32* line = &output[vcount * 256];
  auto vram_block = mmio_copy[vcount].dispcnt.vram_block;
  u16 const* source = (u16 const*)&render_vram_lcdc[vram_block * 0x20000 + vcount * 256 * sizeof(u16)];

  if (source != nullptr) {
    for (uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(*source++);
    }
  } else {
    for (uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(0);
    }
  }
}

void PPU::RenderMainMemoryDisplay(u16 vcount) {
  ASSERT(false, "PPU: unimplemented main memory display mode.");
}

void PPU::RenderBackgroundsAndComposite(u16 vcount) {
  auto const& mmio = mmio_copy[vcount];

  if (mmio.dispcnt.forced_blank) {
    for (uint x = 0; x < 256; x++) {
      buffer_compose[x] = 0xFFFF;
    }
    return;
  }

  // TODO: on a real Nintendo DS all sprites are rendered one scanline ahead.
  if (mmio.dispcnt.enable[ENABLE_OBJ]) {
    RenderLayerOAM(vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_BG0]) {
    // TODO: what does HW do if "enable BG0 3D" is disabled in mode 6.
    if (mmio.dispcnt.enable_bg0_3d || mmio.dispcnt.bg_mode == 6) {
      for (uint x = 0; x < 256; x++) {
        auto& color = gpu_output[vcount * 256 + x];
        if (color.a() != 0) {
          buffer_bg[0][x] = color.to_rgb555();
        } else {
          buffer_bg[0][x] = s_color_transparent;
        }
      }
    } else {
      RenderLayerText(0, vcount);
    }
  }

  if (mmio.dispcnt.enable[ENABLE_BG1] && mmio.dispcnt.bg_mode != 6) {
    RenderLayerText(1, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_BG2]) {
    switch (mmio.dispcnt.bg_mode) {
      case 0:
      case 1:
      case 3:
        RenderLayerText(2, vcount);
        break;
      case 2:
      case 4:
        RenderLayerAffine(0, vcount);
        break;
      case 5:
        RenderLayerExtended(0, vcount);
        break;
      case 6:
        RenderLayerLarge(vcount);
        break;
    }
  }

  if (mmio.dispcnt.enable[ENABLE_BG3]) {
    switch (mmio.dispcnt.bg_mode) {
      case 0:
        RenderLayerText(3, vcount);
        break;
      case 1:
      case 2:
        RenderLayerAffine(1, vcount);
        break;
      case 3:
      case 4:
      case 5:
        RenderLayerExtended(1, vcount);
        break;
    }
  }

  ComposeScanline(vcount, 0, 3);
}

void PPU::SetupRenderWorker() {
  StopRenderWorker();

  render_worker.vcount = 0;
  render_worker.vcount_max = -1;
  render_worker.running = true;
  render_worker.ready = false;

  render_worker.thread = std::thread([this]() {
    while (render_worker.running.load()) {
      while (render_worker.vcount <= render_worker.vcount_max) {
        // TODO: this might be racy with SubmitScanline() resetting render_thread_vcount.
        int vcount = render_worker.vcount;

        if (mmio.dispcnt.enable[ENABLE_WIN0]) {
          RenderWindow(0, vcount);
        }

        if (mmio.dispcnt.enable[ENABLE_WIN1]) {
          RenderWindow(1, vcount);
        }

        if (vcount < 192) {
          RenderScanline(vcount, mmio_copy[vcount].capture_bg_and_3d);
        }

        render_worker.vcount++;
      }

      // Wait for the emulation thread to submit more work:
      std::unique_lock lock{render_worker.mutex};
      render_worker.cv.wait(lock, [this]() {return render_worker.ready;});
      render_worker.ready = false;
    }
  });
}

void PPU::StopRenderWorker() {
  if (!render_worker.running) {
    return;
  }

  // Wake the render worker thread up if it is working for new data:
  render_worker.mutex.lock();
  render_worker.ready = true;
  render_worker.cv.notify_one();
  render_worker.mutex.unlock();

  // Tell the render worker thread to quit and join it:
  render_worker.running = false;
  render_worker.thread.join();
}

void PPU::SubmitScanline(u16 vcount, bool capture_bg_and_3d) {
  mmio.capture_bg_and_3d = capture_bg_and_3d;

  if (vcount < 192) {
    mmio_copy[vcount] = mmio;
  } else {
    mmio_copy[vcount].dispcnt = mmio.dispcnt;
    mmio_copy[vcount].winh[0] = mmio.winh[0];
    mmio_copy[vcount].winh[1] = mmio.winh[1];
    mmio_copy[vcount].winv[0] = mmio.winv[0];
    mmio_copy[vcount].winv[1] = mmio.winv[1];
  }

  if (vcount == 0) {
    CopyVRAM(vram_bg, render_vram_bg, vram_bg_dirty);
    CopyVRAM(vram_obj, render_vram_obj, vram_obj_dirty);
    CopyVRAM(extpal_bg, render_extpal_bg, extpal_bg_dirty);
    CopyVRAM(extpal_obj, render_extpal_obj, extpal_obj_dirty);
    CopyVRAM(vram_lcdc, render_vram_lcdc, vram_lcdc_dirty);
    CopyVRAM(pram, render_pram, pram_dirty);
    CopyVRAM(oam, render_oam, oam_dirty);

    vram_bg_dirty = {};
    vram_obj_dirty = {};
    extpal_bg_dirty = {};
    extpal_obj_dirty = {};
    vram_lcdc_dirty = {};
    pram_dirty = {};
    oam_dirty = {};

    render_worker.vcount = 0;
  }

  render_worker.vcount_max = vcount;

  std::lock_guard lock{render_worker.mutex};
  render_worker.ready = true;
  render_worker.cv.notify_one();
}

void PPU::RegisterMapUnmapCallbacks() {
  vram_bg.AddCallback([this](u32 offset, size_t size) {
    OnWriteVRAM_BG(offset, offset + size);
  });

  vram_obj.AddCallback([this](u32 offset, size_t size) {
    OnWriteVRAM_OBJ(offset, offset + size);
  });

  extpal_bg.AddCallback([this](u32 offset, size_t size) {
    OnWriteExtPal_BG(offset, offset + size);
  });

  extpal_obj.AddCallback([this](u32 offset, size_t size) {
    OnWriteExtPal_OBJ(offset, offset + size);
  });

  vram_lcdc.AddCallback([this](u32 offset, size_t size) {
    OnWriteVRAM_LCDC(offset, offset + size);
  });
}

} // namespace lunar::nds
