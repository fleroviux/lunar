/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>
#include <string.h>

#include "dma.hpp"

namespace fauxDS::core {

void DMA::Reset() {
  memset(filldata, 0, sizeof(filldata));

  for (auto& channel : channels) {
    channel = {};
  }
}

auto DMA::Read(uint chan_id, uint offset) -> u8 {
  auto const& channel = channels[chan_id];

  switch (offset) {
    case REG_DMAXSAD|0: return (channel.src >>  0) & 0xFF;
    case REG_DMAXSAD|1: return (channel.src >>  8) & 0xFF;
    case REG_DMAXSAD|2: return (channel.src >> 16) & 0xFF;
    case REG_DMAXSAD|3: return (channel.src >> 24) & 0xFF;
    case REG_DMAXDAD|0: return (channel.dst >>  0) & 0xFF;
    case REG_DMAXDAD|1: return (channel.dst >>  8) & 0xFF;
    case REG_DMAXDAD|2: return (channel.dst >> 16) & 0xFF;
    case REG_DMAXDAD|3: return (channel.dst >> 24) & 0xFF;
    case REG_DMAXCNT_L|0: return (channel.length >> 0) & 0xFF;
    case REG_DMAXCNT_L|1: return (channel.length >> 8) & 0xFF;
    case REG_DMAXCNT_H|0: {
      return (channel.length >> 16) |
             (static_cast<u8>(channel.dst_mode) << 5) |
             (static_cast<u8>(channel.src_mode) << 7);
    }
    case REG_DMAXCNT_H|1: {
      return (static_cast<u8>(channel.src_mode) >> 1) |
             (channel.repeat ? 2 : 0) |
             (static_cast<u8>(channel.size) << 2) |
             (static_cast<u8>(channel.time) << 3) |
             (channel.interrupt ? 64 : 0) |
             (channel.enable ? 128 : 0); 
    }
  }

  UNREACHABLE;
}

void DMA::Write(uint chan_id, uint offset, u8 value) {
  auto& channel = channels[chan_id];

  switch (offset) {
    case REG_DMAXSAD|0:
    case REG_DMAXSAD|1:
    case REG_DMAXSAD|2:
    case REG_DMAXSAD|3: {
      int shift = offset * 8;
      channel.src &= ~(0xFFUL << shift);
      channel.src |= (value << shift) & 0x0FFFFFFF;
      break;
    }
    case REG_DMAXDAD|0:
    case REG_DMAXDAD|1:
    case REG_DMAXDAD|2:
    case REG_DMAXDAD|3: {
      int shift = (offset - 4) * 8;
      channel.dst &= ~(0xFFUL << shift);
      channel.dst |= (value << shift) & 0x0FFFFFFF;
      break;
    }
    case REG_DMAXCNT_L|0: {
      channel.length &= 0x1FFF00;
      channel.length |= value;
      break;
    }
    case REG_DMAXCNT_L|1: {
      channel.length &= 0x1F00FF;
      channel.length |= value << 8;
      break;
    }
    case REG_DMAXCNT_H|0: {
      channel.length &= 0x00FFFF;
      channel.length |= (value & 0x1F) << 16;
      channel.dst_mode = static_cast<Channel::AddressMode>((value >> 5) & 3);
      channel.src_mode = static_cast<Channel::AddressMode>((channel.src_mode & 2) | (value >> 7));
      break;
    }
    case REG_DMAXCNT_H|1: {
      bool enable_old = channel.enable;

      channel.src_mode = static_cast<Channel::AddressMode>((channel.src_mode & 1) | ((value & 1) << 1));
      channel.size = static_cast<Channel::Size>((value >> 2) & 1);
      channel.time = static_cast<Channel::Timing>((value >> 3) & 7);
      channel.repeat = value & 2;
      channel.interrupt = value & 64;
      channel.enable = value & 128;

      ASSERT(channel.time == Channel::Timing::Immediate,
        "DMA: unhandled start timing {0}", channel.time);

      if (!enable_old && channel.enable && channel.time == Channel::Timing::Immediate) {
        LOG_INFO("DMA: immediate transfer, src=0x{0:08X} dst=0x{1:08X}", channel.src, channel.dst);
        //channel.enable = false;
      }
      break;
    }
  }
}

auto DMA::ReadFill(uint offset) -> u8 {
  if (offset >= 16)
    UNREACHABLE;
  return filldata[offset];
}

void DMA::WriteFill(uint offset, u8 value) {
  if (offset >= 16)
    UNREACHABLE;
  filldata[offset] = value;
}

} // namespace fauxDS::core
