
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

struct Register {
  virtual auto GetWidth() const -> uint = 0;
  
  virtual auto Read(uint offset) -> u8 {
    LOG_WARN("Register: attempted to read write-only register.");
    return 0;
  }
  
  virtual void Write(uint offset, u8 value) {
    LOG_WARN("Register: attempted to write read-only register.");
  }
};

struct RegisterHalf : Register {
  auto GetWidth() const -> uint override { return sizeof(u16); }
};

struct RegisterWord : Register {
  auto GetWidth() const -> uint override { return sizeof(u32); }
};

struct RegisterSet {
  RegisterSet(uint capacity) : capacity(capacity) {
    views = std::make_unique<View[]>(capacity);
  }

  void Map(uint offset, Register& reg) {
    auto width = reg.GetWidth();

    for (uint i = 0; i < width; i++) {
      ASSERT(offset < capacity, "cannot map register to out-of-bounds offset.");
      ASSERT(views[offset].reg == nullptr ||
             views[offset].reg == &reg, "cannot map two registers to the same location.");
      views[offset++] = { &reg, i };
    }
  }

  void Map(uint offset, RegisterSet const& set) {
    for (uint i = 0; i < set.capacity; i++) {
      if (set.views[i].reg == nullptr)
        continue;
      ASSERT(offset < capacity, "cannot map register to out-of-bounds offset.");
      ASSERT(views[offset].reg == nullptr ||
             views[offset].reg == set.views[i].reg, "cannot map two registers to the same location.");
      views[offset++] = set.views[i];
    }
  }

  auto Read(uint offset) -> u8 {
    if (offset >= capacity) {
      LOG_WARN("RegisterSet: out-of-bounds read to offset 0x{0:08X}", offset);
      return 0;
    }

    auto const& view = views[offset];
    if (view.reg == nullptr) {
      LOG_WARN("RegisterSet: read to unmapped offset 0x{0:08X}", offset);
      return 0;
    }

    return view.reg->Read(view.offset);
  }

  void Write(uint offset, u8 value) {
    if (offset >= capacity) {
      LOG_WARN("RegisterSet: out-of-bounds write to offset 0x{0:08X} = 0x{1:02X}",
        offset, value);
      return;
    }

    auto const& view = views[offset];
    if (view.reg == nullptr) {
      LOG_WARN("RegisterSet: write to unmapped offset 0x{0:08X} = 0x{1:02X}",
        offset, value);
      return;
    }

    view.reg->Write(view.offset, value);
  }

private:
  uint capacity;

  struct View {
    Register* reg = nullptr;
    uint offset = 0;
  };

  std::unique_ptr<View[]> views;
};

#include <string.h>

struct Interconnect {
  Interconnect() { Reset(); }

  void Reset() {
    memset(swram, 0, sizeof(swram));
  }

  struct FakeDISPSTAT : RegisterHalf {
    int vblank_flag = 0;

    auto Read(uint offset) -> u8 override {
      if (offset == 0)
        return (vblank_flag ^= 1);
      return 0;
    }

    void Write(uint offset, u8 value) {}
  } fake_dispstat = {};

  struct KeyInput : RegisterHalf {
    bool a = false;
    bool b = false;
    bool select = false;
    bool start = false;
    bool right = false;
    bool left = false;
    bool up = false;
    bool down = false;
    bool r = false;
    bool l = false;

    auto Read(uint offset) -> u8 override {
      return (a      ? 0 :   1) |
             (b      ? 0 :   2) |
             (select ? 0 :   4) |
             (start  ? 0 :   8) |
             (right  ? 0 :  16) |
             (left   ? 0 :  32) |
             (up     ? 0 :  64) |
             (down   ? 0 : 128) |
             (r      ? 0 : 256) |
             (l      ? 0 : 512);
    }
  } keyinput = {};

  u8 swram[0x8000];
};

struct ARM9MemoryBus final : MemoryBase {
  ARM9MemoryBus(Interconnect* interconnect) : interconnect(interconnect) {
    mmio.Map(0x0004, interconnect->fake_dispstat);
    mmio.Map(0x0130, interconnect->keyinput);
  }

  auto GetMemoryModel() const -> MemoryModel override {
    return MemoryModel::ProtectionUnit;
  }

  void SetDTCM(std::uint32_t base, std::uint32_t limit) override {
    dtcm_base = base;
    dtcm_limit = limit;
  }
  
  void SetITCM(std::uint32_t base, std::uint32_t limit) override {
    itcm_base = base;
    itcm_limit = limit;
  }

  auto ReadByte(u32 address, Bus bus, int core) -> u8 override {
    if (address >= itcm_base && address <= itcm_limit)
      return itcm[(address - itcm_base) & 0x7FFF];

    if (bus == Bus::Data && address >= dtcm_base && address <= dtcm_limit)
      return dtcm[(address - dtcm_base) & 0x3FFF];

    switch (address >> 24) {
      case 0x02:
        return ewram[address & 0x3FFFFF];
      case 0x04:
        return mmio.Read(address & 0x00FFFFFF);
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
    if (address >= itcm_base && address <= itcm_limit) {
      itcm[(address - itcm_base) & 0x7FFF] = value;
      return;
    }

    if (address >= dtcm_base && address <= dtcm_limit) {
      dtcm[(address - dtcm_base) & 0x3FFF] = value;
      return;
    }

    switch (address >> 24) {
      case 0x02:
        ewram[address & 0x3FFFFF] = value;
        break;
      case 0x04:
        mmio.Write(address & 0xFFFFFF, value);
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

  u32 dtcm_base  = 0;
  u32 dtcm_limit = 0;
  u8 dtcm[0x4000] {0};

  u32 itcm_base  = 0;
  u32 itcm_limit = 0;
  u8 itcm[0x8000] {0};

  u8 ewram[0x400000] {0};
  u8 vblank_flag = 0;

  // TODO: this is completely stupid and inaccurate as hell.
  u8 vram[0x200000];

  RegisterSet mmio {0x106E};
  Interconnect* interconnect;
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

        auto& keyinput = arm9_mem->interconnect->keyinput;

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

  auto interconnect = std::make_unique<Interconnect>();
  auto arm9_mem = std::make_unique<ARM9MemoryBus>(interconnect.get());
  auto arm9 = std::make_unique<CPUCoreInterpreter>(0, CPUCoreBase::Architecture::ARMv5TE, arm9_mem.get());

  // TODO: this is really messed up but it works for now :cringeharold:
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

  loop(arm9.get(), arm9_mem.get());
  return 0;
} 