/*
 * Copyright (C) 2020 fleroviux
 */


#include <algorithm>
#include <util/log.hpp>
//#include <SDL.h>
#include <string.h>

#include "apu.hpp"

namespace Duality::core {

static constexpr float kVolumeDivideLUT[4] { 1.0, 1.0 / 2.0, 1.0 / 4.0, 1.0 / 16.0 };

static constexpr int kAdpcmIndexTab[8] { -1, -1, -1, -1, 2, 4, 6, 8 };

static constexpr s16 kAdpcmTable[89] {
  0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 
  0x0010, 0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F, 
  0x0022, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042,
  0x0049, 0x0050, 0x0058, 0x0061, 0x006B, 0x0076, 0x0082, 0x008F, 
  0x009D, 0x00AD, 0x00BE, 0x00D1, 0x00E6, 0x00FD, 0x0117, 0x0133,
  0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE, 0x0220, 0x0256, 0x0292,
  0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E, 0x0502, 0x0583,
  0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD, 0x0BD0,
  0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
  0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B,
  0x3BB9, 0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462,
  0x7FFF
};

void APU::Reset() {
  for (int i = 0; i < 16; i++)
    channels[i] = {};

  memset(&buffer, 0, sizeof(buffer));
  buffer_rd_pos = 0;
  buffer_wr_pos = 0;
  buffer_count = 0;

  scheduler.Add(1024, this, &APU::StepMixer);
}

void APU::SetAudioDevice(AudioDevice& device) {
  if (audio_device != nullptr) {
    audio_device->Close();
  }
  audio_device = &device;
  audio_device->Open(this, (AudioDevice::Callback)AudioCallback, 32768, 2048);
}

auto APU::Read(uint chan_id, uint offset) -> u8 {
  auto const& channel = channels[chan_id];

  switch (offset) {
    case REG_SOUNDXCNT|0: {
      return channel.volume_mul;
    }
    case REG_SOUNDXCNT|1: {
      return channel.volume_div | (channel.hold ? 0x80 : 0);
    }
    case REG_SOUNDXCNT|2: {
      return channel.panning;
    }
    case REG_SOUNDXCNT|3: {
      return channel.psg_wave_duty |
             (static_cast<u8>(channel.repeat_mode) << 3) |
             (static_cast<u8>(channel.format) << 5) |
             (channel.running ? 0x80 : 0);
    }
  }

  return 0;
}

void APU::Write(uint chan_id, uint offset, u8 value) {
  auto& channel = channels[chan_id];
  auto timer_duty_old = channel.timer_duty;

  switch (offset) {
    case REG_SOUNDXCNT|0: {
      channel.volume_mul = value & 0x7F;
      break;
    }
    case REG_SOUNDXCNT|1: {
      channel.volume_div = value & 3;
      channel.hold = value & 0x80;
      break;
    }
    case REG_SOUNDXCNT|2: {
      channel.panning = value & 0x7F;
      break;
    }
    case REG_SOUNDXCNT|3: {
      channel.psg_wave_duty = value & 7;
      channel.repeat_mode = static_cast<Channel::RepeatMode>((value >> 3) & 3);
      channel.format = static_cast<Channel::Format>((value >> 5) & 3);
      
      if (!channel.running && (value & 0x80)) {
        ASSERT(channel.format == Channel::Format::PSG || 
          channel.repeat_mode != Channel::RepeatMode::Manual, "APU: unimplemented manual repeat mode.");

        channel.running = true;
        channel.cur_address = channel.src_address;
        channel.t = 0;

        if (channel.format == Channel::Format::ADPCM) {
          u32 value = memory->FastRead<u32, arm::MemoryBase::Bus::System>(channel.cur_address);
          channel.adpcm_sample = s16(value & 0xFFFF);
          channel.adpcm_index = (value >> 16) & 0x7F;
          if (channel.adpcm_index > 88)
            channel.adpcm_index = 88;
          channel.cur_address += 4;

          // FIXME
          channel.adpcm_header = value;
        }

        if (channel.format == Channel::Format::PSG && chan_id >= 14) {
          channel.noise_lfsr = 0x7FFF;
        }

        auto delay = 2 * (0x10000 - channel.timer_duty);
        channel.event = scheduler.Add(delay, [chan_id, this](int cycles_late) {
          StepChannel(chan_id, cycles_late);
        });
      }

      if (channel.running && !(value & 0x80)) {
        channel.running = false;
        scheduler.Cancel(channel.event);
        channel.event = nullptr;
      }
      break;
    }
    case REG_SOUNDXSAD|0:
    case REG_SOUNDXSAD|1:
    case REG_SOUNDXSAD|2:
    case REG_SOUNDXSAD|3: {
      auto shift = (offset - REG_SOUNDXSAD) * 8;
      channel.src_address &= ~(0xFFUL << shift);
      channel.src_address |= (value << shift) & 0x07FFFFFC;
      break;
    }
    case REG_SOUNDXTMR|0: {
      channel.timer_duty &= 0xFF00;
      channel.timer_duty |= value;
      break;
    }
    case REG_SOUNDXTMR|1: {
      channel.timer_duty &= 0xFF;
      channel.timer_duty |= value << 8;
      break;
    }
    case REG_SOUNDXPNT|0: {
      channel.loop_start &= 0xFF00;
      channel.loop_start |= value;
      break;
    }
    case REG_SOUNDXPNT|1: {
      channel.loop_start &= 0xFF;
      channel.loop_start |= value << 8;
      break;
    }
    case REG_SOUNDXLEN|0:
    case REG_SOUNDXLEN|1:
    case REG_SOUNDXLEN|2: {
      auto shift = (offset - REG_SOUNDXLEN) * 8;
      channel.length &= ~(0xFFUL << shift);
      channel.length |= (value << shift) & 0x003FFFFF;      
      break;
    }
  }
}

void APU::StepMixer(int cycles_late) {
  float samples[2] { };

  for (auto const& channel : channels) {
    // TODO: interpret volume_mul = 127 as 128.
    float sample = channel.sample * channel.volume_mul * kVolumeDivideLUT[channel.volume_div] / 128.0;
    samples[0] += sample * channel.panning / 128.0;
    samples[1] += sample * (127.0 - channel.panning) / 128.0;
  }

  std::lock_guard<std::mutex> guard { buffer_lock };
  
  for (int i = 0; i < 2; i++) {
    s16 sample_s16 = std::clamp(samples[i] * 32767.0, -32767.0, +32767.0);
    buffer[i][buffer_wr_pos] = sample_s16;
  }

  buffer_wr_pos = (buffer_wr_pos + 1) % kRingBufferSize;
  buffer_count++;

  scheduler.Add(1024 - cycles_late, this, &APU::StepMixer);
}

void APU::StepChannel(uint chan_id, int cycles_late) {
  auto& channel = channels[chan_id];

  if (channel.format == Channel::Format::PSG) {
    ASSERT(chan_id >= 8, "APU: channel #{0} cannot run in PSG mode", chan_id);
  
    if (chan_id <= 13) {
      if (channel.t < (7 - channel.psg_wave_duty)) {
        channel.sample = -1.0;
      } else {
        channel.sample = +1.0;
      }

      if (++channel.t == 8) channel.t = 0;
    } else {
      int carry = channel.noise_lfsr & 1;
      channel.noise_lfsr >>= 1;
      if (carry) {
        channel.noise_lfsr ^= 0x6000;
        channel.sample = -1.0;
      } else {
        channel.sample = +1.0;
      }
    }
  } else {
    u32 loop_address = channel.src_address + sizeof(u32) * channel.loop_start;
    u32 end_address  = loop_address + sizeof(u32) * channel.length;

    if (channel.t == 0) {
      channel.latch = memory->FastRead<u32, arm::MemoryBase::Bus::System>(channel.cur_address);
      channel.cur_address += 4;
    }

    switch (channel.format) {
      case Channel::Format::PCM8: {
        channel.sample = s8(channel.latch & 0xFF) / 128.0;
        channel.latch >>= 8;
        channel.t += 2;
        break;
      }
      case Channel::Format::PCM16: {
        channel.sample = s16(channel.latch & 0xFFFF) / 32768.0;
        channel.latch >>= 16;
        channel.t += 4;
        break;
      }
      case Channel::Format::ADPCM: {
        u8 data = channel.latch & 15;
        channel.latch >>= 4;

        s16 tab = kAdpcmTable[channel.adpcm_index];
        s16 diff = tab >> 3;
        if (data & 1) diff += tab >> 2;
        if (data & 2) diff += tab >> 1;
        if (data & 4) diff += tab;

        if (data & 8) {
          channel.adpcm_sample = std::max(channel.adpcm_sample - diff, -0x7FFF);
        } else {
          channel.adpcm_sample = std::min(channel.adpcm_sample + diff, +0x7FFF);
        }

        channel.adpcm_index = std::clamp(channel.adpcm_index + kAdpcmIndexTab[data & 7], 0, 88);
        channel.sample = channel.adpcm_sample / 32768.0;
        channel.t++;
        break;
      }
    }

    if (channel.t == 8) {
      channel.t = 0;

      if (channel.format == Channel::Format::ADPCM && 
          channel.repeat_mode == Channel::RepeatMode::Infinite &&
          channel.cur_address == loop_address) {
        channel.adpcm_header  = u16(channel.adpcm_sample);
        channel.adpcm_header |= channel.adpcm_index << 16;
      }

      if (channel.cur_address == end_address) {
        switch (channel.repeat_mode) {
          case Channel::RepeatMode::Infinite:
            channel.cur_address = loop_address;

            if (channel.format == Channel::Format::ADPCM) {
              // FIXME
              channel.adpcm_sample = s16(channel.adpcm_header & 0xFFFF);
              channel.adpcm_index = std::min((channel.adpcm_header >> 16) & 0x7F, 88U);
            }
            break;
          case Channel::RepeatMode::OneShot:
            channel.running = false;
            channel.sample = 0;
            break;
        }
      }
    }
  }

  if (channel.running) {
    auto delay = 2 * (0x10000 - channel.timer_duty);
    channel.event = scheduler.Add(delay - cycles_late, [chan_id, this](int cycles_late) {
      StepChannel(chan_id, cycles_late);
    });
  } else {
    channel.event = nullptr;
  }
}

void AudioCallback(APU* this_, s16* stream, int length) {
  int num_samples = length / sizeof(s16) / 2;
  std::lock_guard<std::mutex> guard { this_->buffer_lock };

  if (this_->buffer_count >= num_samples) {
    for (int i = 0; i < num_samples; i++) {
      *stream++ = this_->buffer[0][this_->buffer_rd_pos];
      *stream++ = this_->buffer[1][this_->buffer_rd_pos];
      this_->buffer_rd_pos = (this_->buffer_rd_pos + 1) % APU::kRingBufferSize;
      this_->buffer_count--;
    }
  } else {
    //int j = 0;

    for (int i = 0; i < num_samples; i++) {
      *stream++ = 0;
      *stream++ = 0;
      //*stream++ = this_->buffer[0][this_->buffer_rd_pos + j];
      //*stream++ = this_->buffer[1][this_->buffer_rd_pos + j];
      //if (++j == this_->buffer_count) j = 0;
    }
  }
}

} // namespace Duality::core
