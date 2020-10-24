#include <algorithm>
#include <fstream>
#include <memory>
#include <stdio.h>

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/arm/cpu_core.hpp>
#include <core/arm/interpreter/interpreter.hpp>
#include <core/interconnect.hpp>
#include <core/arm7/bus.hpp>
#include <core/arm9/bus.hpp>

#include <SDL2/SDL.h>

using namespace fauxDS::core;
using namespace fauxDS::core::arm;

struct NDSHeader {
  // TODO: add remaining header fields. 
  // This should be enough for basic direct boot for now though.
  // http://problemkaputt.de/gbatek.htm#dscartridgeheader
  u8 game_title[12];
  u8 game_code[4];
  u8 maker_code[2];
  u8 unit_code;
  u8 encryption_seed_select;
  u8 device_capacity;
  u8 reserved0[8];
  u8 nds_region;
  u8 version;
  u8 autostart;
  struct Binary {
    u32 file_address;
    u32 entrypoint;
    u32 load_address;
    u32 size;
  } arm9, arm7;
} __attribute__((packed));

void loop(CPUCoreInterpreter* arm7, CPUCoreInterpreter* arm9, Interconnect* interconnect, u8* vram) {
  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "fauxDS",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    256 * 2,
    384 * 2,
    SDL_WINDOW_ALLOW_HIGHDPI);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  auto tex_top = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, 256, 192);
  auto tex_bottom = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, 256, 192);

  SDL_Rect dest_top {
    .x = 0,
    .y = 0,
    .w = 256,
    .h = 192
  };

  SDL_RenderSetLogicalSize(renderer, 256, 384);

  SDL_Event event;

  auto& scheduler = interconnect->scheduler;

  for (;;) {
    // 355 dots-per-line * 263 lines-per-frame * 6 cycles-per-dot = 560190
    static constexpr int kCyclesPerFrame = 560190;

    //auto t0 = SDL_GetTicks();
    auto frame_target = scheduler.GetTimestampNow() + kCyclesPerFrame;

    while (scheduler.GetTimestampNow() < frame_target) {
      while (scheduler.GetTimestampNow() < std::min(frame_target, scheduler.GetTimestampTarget())) {
        arm9->Run(2);
        arm7->Run(1);
        scheduler.AddCycles(1);
      }

      scheduler.Step();
    }

    //auto t1 = SDL_GetTicks();
    //fmt::print("frame time: {0} ms\n", t1 - t0); 

    SDL_UpdateTexture(tex_top, nullptr, vram, sizeof(u16) * 256);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tex_top, nullptr, &dest_top);
    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto cleanup;
      if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
        int key = -1;
        bool pressed = event.type == SDL_KEYDOWN;

        auto& keyinput = interconnect->keyinput;

        switch (reinterpret_cast<SDL_KeyboardEvent*>(&event)->keysym.sym) {
          case SDLK_a: keyinput.a = pressed; break;
          case SDLK_s: keyinput.b = pressed; break;
          case SDLK_BACKSPACE: keyinput.select = pressed; break;
          case SDLK_RETURN: keyinput.start = pressed; break;
          case SDLK_RIGHT: keyinput.right = pressed; break;
          case SDLK_LEFT: keyinput.left = pressed; break;
          case SDLK_UP: keyinput.up = pressed; break;
          case SDLK_DOWN: keyinput.down = pressed; break;
          case SDLK_d: keyinput.l = pressed; break;
          case SDLK_f: keyinput.r = pressed; break;
        }
      }
    }
  }

cleanup:
  SDL_DestroyTexture(tex_top);
  SDL_DestroyTexture(tex_bottom);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
} 

auto main(int argc, const char** argv) -> int {
  const char* rom_path;

  if (argc <= 1) {
    rom_path = "armwrestler.nds";
  } else {
    rom_path = argv[1];
  }

  std::ifstream rom { rom_path, std::ios::in | std::ios::binary };
  if (!rom.good()) {
    printf("failed to open ROM: %s\n", rom_path);
    return -1;
  }

  auto header = std::make_unique<NDSHeader>();
  rom.read((char*)header.get(), sizeof(NDSHeader));
  if (!rom.good()) {
    puts("failed to read ROM header, not enough data.");
    return -2;
  }

  printf("ARM9 load_address=0x%08X size=0x%08X file_address=0x%08X\n",
    header->arm9.load_address, header->arm9.size, header->arm9.file_address);

  printf("ARM7 load_address=0x%08X size=0x%08X file_address=0x%08X\n",
    header->arm7.load_address, header->arm7.size, header->arm7.file_address);

  auto interconnect = std::make_unique<Interconnect>();
  auto arm7_mem = std::make_unique<ARM7MemoryBus>(interconnect.get());
  auto arm9_mem = std::make_unique<ARM9MemoryBus>(interconnect.get());
  auto arm7 = std::make_unique<CPUCoreInterpreter>(0, CPUCoreBase::Architecture::ARMv5TE, arm7_mem.get());
  auto arm9 = std::make_unique<CPUCoreInterpreter>(0, CPUCoreBase::Architecture::ARMv5TE, arm9_mem.get());
  
  {
    u8 data;
    u32 dst = header->arm7.load_address;
    rom.seekg(header->arm7.file_address);
    for (u32 i = 0; i < header->arm7.size; i++) {
      rom.read((char*)&data, 1);
      if (!rom.good()) {
        puts("failed to read ARM7 binary from ROM into ARM7 memory");
        return -3;
      }
      arm7_mem->WriteByte(dst++, data, 0);
    }

    arm7->ExceptionBase(0);
    arm7->Reset();
    arm7->SetPC(header->arm7.entrypoint);
  }

  {
    u8 data;
    u32 dst = header->arm9.load_address;
    rom.seekg(header->arm9.file_address);
    for (u32 i = 0; i < header->arm9.size; i++) {
      rom.read((char*)&data, 1);
      if (!rom.good()) {
        puts("failed to read ARM9 binary from ROM into ARM9 memory");
        return -3;
      }
      arm9_mem->WriteByte(dst++, data, 0);
    }

    arm9->ExceptionBase(0xFFFF0000);
    arm9->Reset();
    arm9->SetPC(header->arm9.entrypoint);
  }

  loop(arm7.get(), arm9.get(), interconnect.get(), arm9_mem->vram);
  return 0;
} 