
#include <fstream>
#include <memory>
#include <stdio.h>

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/arm/cpu_core.hpp>
#include <core/arm/interpreter/interpreter.hpp>

#include <SDL2/SDL.h>

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

struct ARM9MemoryBus final : MemoryBase {
  auto GetMemoryModel() const -> MemoryModel override {
    return MemoryModel::ProtectionUnit;
  }

  auto ReadByte(u32 address, Bus bus, int core) -> u8 override {
    // TODO: this likely isn't the most efficient solution.
    if (bus == Bus::Data && address >= dtcm_base && address <= dtcm_limit)
      return dtcm[address - dtcm_base];

    switch (address >> 24) {
      case 0x02:
        return ewram[address & 0x3FFFFF];
      case 0x04:
        switch (address & 0x00FFFFFF) {
          case 0x04:
            return (vblank_flag ^= 1);
          case 0x130:
            return keyinput & 0xFF;
          case 0x131:
            return keyinput >> 8;
          default:
            LOG_WARN("ARM9: unhandled MMIO register at 0x{0:08X}", address);
        }
        break;
      default:
        LOG_ERROR("ARM9: unhandled read byte from 0x{0:08X}", address);
        for (;;) ;
    }

    return 0;
  }
  
  auto ReadHalf(u32 address, Bus bus, int core) -> u16 override {
    return ReadByte(address + 0, bus, core) | 
          (ReadByte(address + 1, bus, core) << 8);
  }
  
  auto ReadWord(u32 address, Bus bus, int core) -> u32 override {
    return ReadHalf(address + 0, bus, core) | 
          (ReadHalf(address + 2, bus, core) << 16);
  }
  
  void WriteByte(u32 address, u8 value, int core) override {
    // TODO: this likely isn't the most efficient solution.
    if (address >= dtcm_base && address <= dtcm_limit) {
      dtcm[address - dtcm_base] = value;
      return;
    }

    switch (address >> 24) {
      case 0x02:
        ewram[address & 0x3FFFFF] = value;
        break;
      case 0x04:
        LOG_WARN("ARM9: unhandled MMIO write byte 0x{0:08X} = 0x{1:02X}", address, value);
        break;
      case 0x06:
        vram[address & 0x1FFFFF] = value;
        break;
      default:
        LOG_ERROR("ARM9: unhandled write byte 0x{0:08X} = 0x{1:02X}", address, value);
        //for (;;) ;    
    }
    
  }
  
  void WriteHalf(u32 address, u16 value, int core) override {
    WriteByte(address + 0, value & 0xFF, core);
    WriteByte(address + 1, value >> 8, core);
  }

  void WriteWord(u32 address, u32 value, int core) override {
    WriteHalf(address + 0, value & 0xFFFF, core);
    WriteHalf(address + 2, value >> 16, core);
  }

  // TODO: retrive this information from the CP15 coprocessor.
  u32 dtcm_base  = 0x00800000;
  u32 dtcm_limit = 0x00803FFF;
  u8 dtcm[0x4000] {0};

  u8 ewram[0x400000] {0};
  u8 vblank_flag = 0;

  // TODO: this is completely stupid and inaccurate as hell.
  u8 vram[0x200000];

  u16 keyinput = 0x3FF;
};

void loop(CPUCoreBase* arm9, ARM9MemoryBus* arm9_mem) {
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

  for (;;) {
    arm9->Run(1024 * 1024);

    SDL_UpdateTexture(tex_top, nullptr, arm9_mem->vram, sizeof(u16) * 256);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, tex_top, nullptr, &dest_top);
    SDL_RenderPresent(renderer);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto cleanup;
      if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
        int key = -1;
        bool pressed = event.type == SDL_KEYDOWN;

        switch (reinterpret_cast<SDL_KeyboardEvent*>(&event)->keysym.sym) {
          case SDLK_a: key = 1; break;
          case SDLK_s: key = 2; break;
          case SDLK_BACKSPACE: key = 4; break;
          case SDLK_RETURN: key = 8; break;
          case SDLK_RIGHT: key = 16; break;
          case SDLK_LEFT: key = 32; break;
          case SDLK_UP: key = 64; break;
          case SDLK_DOWN: key = 128; break;
          case SDLK_d: key = 256; break;
          case SDLK_f: key = 512; break;
        }

        if (key != -1) {
          if (pressed) 
            arm9_mem->keyinput &= ~key;
          else
            arm9_mem->keyinput |=  key;
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

  auto arm9_mem = std::make_unique<ARM9MemoryBus>();
  auto arm9 = std::make_unique<CPUCoreInterpreter>(0, CPUCoreBase::Architecture::ARMv5TE, arm9_mem.get());

  // TODO: this is really messed up but it works for now :cringeharold:
  rom.seekg(0x200);
  rom.read((char*)&arm9_mem->ewram[0x4000], 0x40D0);
  if (!rom.good()) {
    puts("failed to read ARM9 binary from ROM into ARM9 memory");
    return -3;
  }

  arm9->ExceptionBase(0xFFFF0000);
  arm9->Reset();
  arm9->SetPC(header->arm9.entrypoint);

  loop(arm9.get(), arm9_mem.get());
  return 0;
} 