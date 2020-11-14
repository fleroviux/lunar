#include <algorithm>
#include <fstream>
#include <memory>
#include <stdio.h>

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/arm/arm.hpp>
#include <core/interconnect.hpp>
#include <core/arm7/bus.hpp>
#include <core/arm9/bus.hpp>
#include <core/arm9/cp15.hpp>

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

void loop(ARM* arm7, ARM* arm9, Interconnect* interconnect) {
  SDL_Init(SDL_INIT_VIDEO);

  auto window = SDL_CreateWindow(
    "fauxDS",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    256 * 2,
    384 * 2,
    SDL_WINDOW_ALLOW_HIGHDPI);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  auto tex_top = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);
  auto tex_bottom = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);

  // TODO: handle screen swapping
  u32* framebuffer_top = interconnect->video_unit.ppu_a.GetFramebuffer();
  u32* framebuffer_bottom = interconnect->video_unit.ppu_b.GetFramebuffer();

  SDL_Rect dest_top {
    .x = 0,
    .y = 0,
    .w = 256,
    .h = 192
  };

  SDL_Rect dest_bottom {
    .x = 0,
    .y = 192,
    .w = 256,
    .h = 192
  };

  SDL_RenderSetLogicalSize(renderer, 256, 384);

  SDL_Event event;

  auto& scheduler = interconnect->scheduler;
  auto& irq7 = interconnect->irq7;
  auto& irq9 = interconnect->irq9;
  auto& tsc = interconnect->spi.tsc;

  int frames = 0;
  auto t0 = SDL_GetTicks();

  for (;;) {
    // 355 dots-per-line * 263 lines-per-frame * 6 cycles-per-dot = 560190
    static constexpr int kCyclesPerFrame = 560190;

    auto frame_target = scheduler.GetTimestampNow() + kCyclesPerFrame;

    while (scheduler.GetTimestampNow() < frame_target) {
      while (scheduler.GetTimestampNow() < std::min(frame_target, scheduler.GetTimestampTarget())) {
        if (irq7.IsEnabled() && irq7.HasPendingIRQ()) {
          arm7->SignalIRQ();
        }
        if (irq9.IsEnabled() && irq9.HasPendingIRQ()) {
          arm9->SignalIRQ();
        }
        arm9->Run(2);
        arm7->Run(1);
        scheduler.AddCycles(1);
      }

      scheduler.Step();
    }

    auto t1 = SDL_GetTicks();
    frames++;
    if ((t1 - t0) >= 1000) {
      LOG_INFO("framerate: {0} fps", frames);
      frames = 0;
      t0 = SDL_GetTicks();
    }

    SDL_UpdateTexture(tex_top, nullptr, framebuffer_top, sizeof(u32) * 256);
    SDL_UpdateTexture(tex_bottom, nullptr, framebuffer_bottom, sizeof(u32) * 256);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tex_top, nullptr, &dest_top);
    SDL_RenderCopy(renderer, tex_bottom, nullptr, &dest_bottom);
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
      if (event.type == SDL_MOUSEMOTION) {
        auto mouse_event = reinterpret_cast<SDL_MouseMotionEvent*>(&event);
        s32 x = mouse_event->x;
        s32 y = mouse_event->y - 192;
        tsc.SetTouchState((mouse_event->state & SDL_BUTTON_LMASK) && y >= 0, x, y);
      }
    }
  }

cleanup:
  SDL_DestroyTexture(tex_top);
  SDL_DestroyTexture(tex_bottom);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

/// No-operation stub for the CP14 coprocessor
struct CP14Stub : arm::Coprocessor {
  void Reset() override {}
  auto Read(int opcode1, int cn, int cm, int opcode2) -> u32 override { return 0; }
  void Write(int opcode1, int cn, int cm, int opcode2, u32 value) override {}
};

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

  printf("ARM9 load_address=0x%08X size=0x%08X file_address=0x%08X entrypoint=0x%08X\n",
    header->arm9.load_address, header->arm9.size, header->arm9.file_address, header->arm9.entrypoint);

  printf("ARM7 load_address=0x%08X size=0x%08X file_address=0x%08X entrypoint=0x%08X\n",
    header->arm7.load_address, header->arm7.size, header->arm7.file_address, header->arm7.entrypoint);

  auto interconnect = std::make_unique<Interconnect>();
  auto arm7_mem = std::make_unique<ARM7MemoryBus>(interconnect.get());
  auto arm9_mem = std::make_unique<ARM9MemoryBus>(interconnect.get());
  auto arm7 = std::make_unique<ARM>(ARM::Architecture::ARMv4T, arm7_mem.get());
  auto arm9 = std::make_unique<ARM>(ARM::Architecture::ARMv5TE, arm9_mem.get());
  auto arm7_cp14 = std::make_unique<CP14Stub>();
  auto arm9_cp15 = std::make_unique<CP15>(arm9.get(), arm9_mem.get());

  arm7->AttachCoprocessor(14, arm7_cp14.get());
  arm9->AttachCoprocessor(15, arm9_cp15.get());

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
      arm7_mem->WriteByte(dst++, data);
    }

    arm7->ExceptionBase(0);
    arm7->Reset();
    arm7->GetState().r13 = 0x0380FD80;
    arm7->GetState().bank[BANK_IRQ][BANK_R13] = 0x0380FF80;
    arm7->GetState().bank[BANK_SVC][BANK_R13] = 0x0380FFC0;
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
      arm9_mem->WriteByte(dst++, data);
    }

    arm9->ExceptionBase(0xFFFF0000);
    arm9->Reset();
    // TODO: ARM9 stack is in shared WRAM by default,
    // but afaik at cartridge boot time all of SWRAM is mapped to NDS7?
    arm9->GetState().r13 = 0x03002F7C;
    arm9->GetState().bank[BANK_IRQ][BANK_R13] = 0x03003F80;
    arm9->GetState().bank[BANK_SVC][BANK_R13] = 0x03003FC0;
    arm9->SetPC(header->arm9.entrypoint);
  }

  u8 data;
  u32 dst = 0x02FFFE00;
  rom.seekg(0);
  for (u32 i = 0; i < 0x170; i++) {
    rom.read((char*)&data, 1);
    if (!rom.good()) {
      puts("failed to load cartridge header into memory");
      return -4;
    }
    arm9_mem->WriteByte(dst++, data);
  }

  rom.close();

  // Load cartridge ROM into Slot 1
  interconnect->cart.Load(rom_path);

  // Huge thanks to Hydr8gon for pointing this out:
  arm9_mem->WriteWord(0x027FF800, 0x1FC2); // Chip ID 1
  arm9_mem->WriteWord(0x027FF804, 0x1FC2); // Chip ID 2
  arm9_mem->WriteHalf(0x027FF850, 0x5835); // ARM7 BIOS CRC
  arm9_mem->WriteHalf(0x027FF880, 0x0007); // Message from ARM9 to ARM7
  arm9_mem->WriteHalf(0x027FF884, 0x0006); // ARM7 boot task
  arm9_mem->WriteWord(0x027FFC00, 0x1FC2); // Copy of chip ID 1
  arm9_mem->WriteWord(0x027FFC04, 0x1FC2); // Copy of chip ID 2
  arm9_mem->WriteHalf(0x027FFC10, 0x5835); // Copy of ARM7 BIOS CRC
  arm9_mem->WriteHalf(0x027FFC40, 0x0001); // Boot indicator

  loop(arm7.get(), arm9.get(), interconnect.get());
  return 0;
}
