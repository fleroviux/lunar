/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/arm/memory.hpp>
#include <core/scheduler.hpp>
#include <mutex>

namespace fauxDS::core {

// FIXME
void AudioCallback(struct APU* this_, s16* stream, int length);

struct APU {
  APU(Scheduler& scheduler) : scheduler(scheduler) { Reset(); }

  void Reset();
  void SetMemory(arm::MemoryBase* memory) { this->memory = memory; }
  auto Read (uint chan_id, uint offset) -> u8;
  void Write(uint chan_id, uint offset, u8 value);

private:
  friend void fauxDS::core::AudioCallback(APU* this_, s16* stream, int length);

  static constexpr int kRingBufferSize = 8192;

  enum Registers {
    REG_SOUNDXCNT = 0x0,
    REG_SOUNDXSAD = 0x4,
    REG_SOUNDXTMR = 0x8,
    REG_SOUNDXPNT = 0xA,
    REG_SOUNDXLEN = 0xC
  };

  struct Channel {
    enum class RepeatMode {
      Manual = 0,
      Infinite = 1,
      OneShot = 2,
      Prohibited = 3
    };

    enum class Format {
      PCM8 = 0,
      PCM16 = 1,
      ADPCM = 2,
      PSG = 3
    };

    int volume_mul = 0; // 0 - 127
    int volume_div = 0; // 0 - 3
    bool hold = false;
    int panning = 0; // 0 - 127
    int psg_wave_duty = 0; // 0 - 7
    RepeatMode repeat_mode = RepeatMode::Manual;
    Format format = Format::PCM8;
    bool running = false;

    u32 src_address = 0;
    u16 timer_duty = 0;
    u16 loop_start = 0;
    u32 length = 0;

    // internal
    float sample = 0;
    s32 adpcm_sample = 0;
    int adpcm_index = 0;
    u32 adpcm_latch;
    u32 adpcm_header;
    u32 cur_address = 0;
    int t = 0;
    Scheduler::Event* event = nullptr;
  } channels[16];

  void StepMixer(int cycles_late);
  void StepChannel(uint chan_id, int cycles_late);

  s16 buffer[2][kRingBufferSize];
  int buffer_rd_pos;
  int buffer_wr_pos;
  int buffer_count;
  std::mutex buffer_lock;
  Scheduler& scheduler;
  arm::MemoryBase* memory = nullptr;
};

} // namespace fauxDS::core
