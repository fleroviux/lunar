/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <string.h>

#include "ppu.hpp"

namespace Duality::Core {

PPU::PPU(
  int id,
  VRAM const& vram,
  u8  const* pram,
  u8  const* oam,
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
}

void PPU::OnDrawScanlineBegin(u16 vcount) {
  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }

  RenderScanline(vcount);
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

  if (mmio.dispcnt.enable[ENABLE_WIN0]) {
    RenderWindow(0, vcount);
  }

  if (mmio.dispcnt.enable[ENABLE_WIN1]) {
    RenderWindow(1, vcount);
  }
}

void PPU::RenderScanline(u16 vcount) {
  switch (mmio.dispcnt.display_mode) {
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

  if (mmio.dispcnt.forced_blank) {
    for (uint x = 0; x < 256; x++) {
      line[x] = ConvertColor(0x7FFF);
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
      for (uint x = 0; x < 256; x++)
        buffer_bg[0][x] = gpu_output[vcount * 256 + x].to_rgb555();
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
        RenderLayerAffine(0);
        break;
      case 5:
        RenderLayerExtended(0);
        break;
      case 6:
        RenderLayerLarge();
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
        RenderLayerAffine(1);
        break;
      case 3:
      case 4:
      case 5:
        RenderLayerExtended(1);
        break;
    }
  }

  ComposeScanline(vcount, 0, 3);
}

void PPU::RenderVideoMemoryDisplay(u16 vcount) {
  u32* line = &output[vcount * 256];
  u16 const* source = vram_lcdc.GetUnsafePointer<u16>(mmio.dispcnt.vram_block * 0x20000 + vcount * 256 * sizeof(u16));

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

} // namespace Duality::Core
