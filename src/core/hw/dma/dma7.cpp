/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "dma7.hpp"

namespace Duality::core {

static constexpr u32 g_dma_src_mask[] = {
  0x07FFFFFF, 0x0FFFFFFF, 0x0FFFFFFF, 0x0FFFFFFF
};

static constexpr u32 g_dma_dst_mask[] = {
  0x07FFFFFF, 0x07FFFFFF, 0x07FFFFFF, 0x0FFFFFFF
};

static constexpr u16 g_dma_len_mask[] = {
  0x3FFF, 0x3FFF, 0x3FFF, 0xFFFF
};

void DMA7::Reset() {
  for (uint i = 0; i < 4; i++) {
    channels[i] = {i};
  }
}

auto DMA7::Read(uint chan_id, uint offset) -> u8 {
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
      return (static_cast<u8>(channel.dst_mode) << 5) |
             (static_cast<u8>(channel.src_mode) << 7);
    }
    case REG_DMAXCNT_H|1: {
      return (static_cast<u8>(channel.src_mode) >> 1) |
             (channel.repeat ? 2 : 0) |
             (static_cast<u8>(channel.size) << 2) |
             (static_cast<u8>(channel.time) << 4) |
             (channel.interrupt ? 64 : 0) |
             (channel.enable ? 128 : 0); 
    }
  }

  UNREACHABLE;
}

void DMA7::Write(uint chan_id, uint offset, u8 value) {
  auto& channel = channels[chan_id];

  switch (offset) {
    case REG_DMAXSAD|0:
    case REG_DMAXSAD|1:
    case REG_DMAXSAD|2:
    case REG_DMAXSAD|3: {
      int shift = offset * 8;
      channel.src &= ~(0xFFUL << shift);
      channel.src |= (value << shift) & g_dma_src_mask[chan_id];
      break;
    }
    case REG_DMAXDAD|0:
    case REG_DMAXDAD|1:
    case REG_DMAXDAD|2:
    case REG_DMAXDAD|3: {
      int shift = (offset - 4) * 8;
      channel.dst &= ~(0xFFUL << shift);
      channel.dst |= (value << shift) & g_dma_dst_mask[chan_id];
      break;
    }
    case REG_DMAXCNT_L|0: {
      channel.length &= 0x1FFF00;
      channel.length |= value;
      break;
    }
    case REG_DMAXCNT_L|1: {
      channel.length &= 0x1F00FF;
      channel.length |= (value << 8) & g_dma_len_mask[chan_id];
      break;
    }
    case REG_DMAXCNT_H|0: {
      channel.dst_mode = static_cast<Channel::AddressMode>((value >> 5) & 3);
      channel.src_mode = static_cast<Channel::AddressMode>((channel.src_mode & 2) | (value >> 7));
      break;
    }
    case REG_DMAXCNT_H|1: {
      bool enable_old = channel.enable;

      channel.src_mode = static_cast<Channel::AddressMode>((channel.src_mode & 1) | ((value & 1) << 1));
      channel.size = static_cast<Channel::Size>((value >> 2) & 1);
      channel.time = static_cast<Time>((value >> 4) & 7);
      channel.repeat = value & 2;
      channel.interrupt = value & 64;
      channel.enable = value & 128;

      if (!enable_old && channel.enable) {
        u32 mask = channel.size == Channel::Size::Word ? ~3 : ~1;
        channel.latch.src = channel.src & mask;
        channel.latch.dst = channel.dst & mask;
        channel.latch.length = channel.length;
        if (channel.latch.length == 0) {
          channel.latch.length = g_dma_len_mask[channel.id] + 1;
        }
        if (channel.time == Time::Immediate) {
          RunChannel(channel);
        }
        // Diagnostics, get rid of it once emulation is more stable...
        switch (channel.time) {
          case Time::Immediate:
          case Time::VBlank:
          case Time::Slot1:
            break;
          default:
            ASSERT(false, "DMA7: unhandled start time: {0}", channel.time);
            break;
        }
      }
      break;
    }
  }
}

void DMA7::Request(Time time) {
  for (auto& channel : channels) {
    if (channel.enable && channel.time == time)
      RunChannel(channel);
  }
}

void DMA7::RunChannel(Channel& channel) {
  // FIXME: what happens if source control is set to reload?
  static constexpr int dma_modify[2][4] = {
    { 2, -2, 0, 2 },
    { 4, -4, 0, 4 }
  };

  int dst_offset = dma_modify[channel.size][channel.dst_mode];
  int src_offset = dma_modify[channel.size][channel.src_mode];

  LOG_INFO("DMA7: transfer src=0x{0:08X} dst=0x{1:08X} length=0x{2:08X} size={3}",
    channel.latch.src, channel.latch.dst, channel.latch.length, channel.size);

  // TODO: read and write full 64-bit words at once as long as possible?
  if (channel.size == Channel::Size::Word) {
    while (channel.latch.length-- != 0) {
      memory->FastWrite<u32, Bus::System>(channel.latch.dst, memory->FastRead<u32, Bus::System>(channel.latch.src));
      channel.latch.dst += dst_offset;
      channel.latch.src += src_offset;
    }
  } else {
    while (channel.latch.length-- != 0) {
      memory->FastWrite<u16, Bus::System>(channel.latch.dst, memory->FastRead<u16, Bus::System>(channel.latch.src));
      channel.latch.dst += dst_offset;
      channel.latch.src += src_offset;
    }
  }

  if (channel.repeat && channel.time != Time::Immediate) {
    channel.latch.length = channel.length;
    if (channel.latch.length == 0) {
      channel.latch.length = g_dma_len_mask[channel.id] + 1;
    }
    if (channel.dst_mode == Channel::AddressMode::Reload) {
      channel.latch.dst = channel.dst & (channel.size == Channel::Size::Word ? ~3 : ~1);
    }
  } else {
    channel.enable = false;
  }

  if (channel.interrupt) {
    switch (channel.id) {
      case 0: irq.Raise(IRQ::Source::DMA0); break;
      case 1: irq.Raise(IRQ::Source::DMA1); break;
      case 2: irq.Raise(IRQ::Source::DMA2); break;
      case 3: irq.Raise(IRQ::Source::DMA3); break;
    }
  }
}

} // namespace Duality::core
