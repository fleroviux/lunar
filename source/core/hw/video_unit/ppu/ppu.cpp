/*
 * Copyright (C) 2020 fleroviux
 */

#include "ppu.hpp"

namespace fauxDS::core {

PPU::PPU(Region<32> const& vram_bg, Region<16> const& vram_obj, u8 const* pram) 
    : vram_bg(vram_bg)
    , vram_obj(vram_obj)
    , pram(pram) {
  Reset();
}

void PPU::Reset() {
}

} // namespace fauxDS::core
