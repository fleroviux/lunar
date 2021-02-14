#include <algorithm>
#include <fstream>
#include <memory>
#include <stdio.h>
#include <util/integer.hpp>
#include <util/log.hpp>

#include <arm/arm.hpp>
#include <interconnect.hpp>
#include <arm7/bus.hpp>
#include <arm9/bus.hpp>
#include <arm9/cp15.hpp>

#include <SDL.h>
#include <GL/glew.h>

#undef main

using namespace Duality::core;
using namespace Duality::core::arm;

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

void loop(ARM* arm7, ARM* arm9, Interconnect* interconnect, ARM7MemoryBus* arm7_mem) {
  SDL_Init(SDL_INIT_VIDEO);

  auto scale = 2;
  auto window = SDL_CreateWindow(
    "Duality",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    256 * scale,
    384 * scale,
    SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);

  auto gl_context = SDL_GL_CreateContext(window);

  glewInit();

  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetSwapInterval(1);

  // TODO: replace legacy OpenGL jank with more modern code.
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);

  GLuint textures[2];
  glGenTextures(2, &textures[0]);

  for (int i = 0; i < 2; i++) {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  glClearColor(0, 0, 0, 1);

  SDL_Event event;

  auto& scheduler = interconnect->scheduler;
  auto& irq7 = interconnect->irq7;
  auto& irq9 = interconnect->irq9;
  auto& tsc = interconnect->spi.tsc;
  auto& keyinput = interconnect->keyinput;
  auto& extkeyinput = interconnect->extkeyinput;

  int frames = 0;
  auto t0 = SDL_GetTicks();
  bool fastforward = false;

  for (;;) {
    // 355 dots-per-line * 263 lines-per-frame * 6 cycles-per-dot = 560190
    static constexpr int kCyclesPerFrame = 560190;

    auto frame_target = scheduler.GetTimestampNow() + kCyclesPerFrame;

    while (scheduler.GetTimestampNow() < frame_target) {
      uint cycles = 1;

      // Run both CPUs individually for up to 32 cycles, but make sure
      // that we do not run past any hardware event.
      if (gLooselySynchronizeCPUs) {
        u64 target = std::min(frame_target, scheduler.GetTimestampTarget());
        // Run to the next event if both CPUs are halted.
        // Otherwise run each CPU for up to 32 cycles.
        cycles = target - scheduler.GetTimestampNow();
        if (!arm9->IsWaitingForIRQ() || !arm7_mem->IsHalted()) {
          cycles = std::min(32U, cycles);
        }
      }

      arm9->Run(cycles * 2);

      if (!arm7_mem->IsHalted() || irq7.HasPendingIRQ()) {
        arm7_mem->IsHalted() = false;
        arm7->Run(cycles);
      }

      scheduler.AddCycles(cycles);
      scheduler.Step();
    }

    auto t1 = SDL_GetTicks();
    auto t_diff = t1 - t0;
    frames++;
    if (t_diff >= 1000) {
      auto percent = frames / 60.0 * 100.0;
      auto ms = t_diff / float(frames);
      SDL_SetWindowTitle(window, fmt::format("Duality [{0} fps | {1:.2f} ms]",
        frames,
        ms).c_str());
      fmt::print("framerate: {0} fps, {1} ms, {2}%\n", frames, ms, percent);
      frames = 0;
      t0 = SDL_GetTicks();
    }

    u32 const* output_top = interconnect->video_unit.GetOutput(VideoUnit::Screen::Top);
    u32 const* output_bottom = interconnect->video_unit.GetOutput(VideoUnit::Screen::Bottom);

    glClear(GL_COLOR_BUFFER_BIT);

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_BGRA, GL_UNSIGNED_BYTE, output_top);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f,  1.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f( 1.0f,  1.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f( 1.0f,  0.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f,  0.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_BGRA, GL_UNSIGNED_BYTE, output_bottom);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-1.0f,  0.0f);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f( 1.0f,  0.0f);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f( 1.0f, -1.0f);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-1.0f, -1.0f);
    glEnd();

    SDL_GL_SwapWindow(window);

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT)
        goto cleanup;
      if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {
        int key = -1;
        bool pressed = event.type == SDL_KEYDOWN;

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
          case SDLK_q: extkeyinput.x = pressed; break;
          case SDLK_w: extkeyinput.y = pressed; break;
          case SDLK_SPACE: {
            fastforward = pressed;
            if (fastforward) {
              SDL_GL_SetSwapInterval(0);
            } else {
              SDL_GL_SetSwapInterval(1);
            }
            break;
          }
        }
      }
      if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
        auto mouse_event = reinterpret_cast<SDL_MouseMotionEvent*>(&event);
        s32 x = mouse_event->x / scale;
        s32 y = mouse_event->y / scale - 192;
        bool down = (mouse_event->state & SDL_BUTTON_LMASK) && y >= 0;
        tsc.SetTouchState(down, x, y);
        extkeyinput.pen_down = down;
      }
    }
  }

cleanup:
  glDeleteTextures(2, &textures[0]);
  SDL_GL_DeleteContext(gl_context);
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

  interconnect->irq7.SetCore(*arm7.get());
  interconnect->irq9.SetCore(*arm9.get());
  interconnect->dma7.SetMemory(arm7_mem.get());
  interconnect->dma9.SetMemory(arm9_mem.get());

  using Bus = arm::MemoryBase::Bus;

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
      arm7_mem->WriteByte(dst++, data, Bus::Data);
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
      arm9_mem->WriteByte(dst++, data, Bus::Data);
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
    arm9_mem->WriteByte(dst++, data, Bus::Data);
  }

  rom.close();

  // Load cartridge ROM into Slot 1
  interconnect->cart.Load(rom_path);

  // Huge thanks to Hydr8gon for pointing this out:
  arm9_mem->WriteWord(0x027FF800, 0x1FC2, Bus::Data); // Chip ID 1
  arm9_mem->WriteWord(0x027FF804, 0x1FC2, Bus::Data); // Chip ID 2
  arm9_mem->WriteHalf(0x027FF850, 0x5835, Bus::Data); // ARM7 BIOS CRC
  arm9_mem->WriteHalf(0x027FF880, 0x0007, Bus::Data); // Message from ARM9 to ARM7
  arm9_mem->WriteHalf(0x027FF884, 0x0006, Bus::Data); // ARM7 boot task
  arm9_mem->WriteWord(0x027FFC00, 0x1FC2, Bus::Data); // Copy of chip ID 1
  arm9_mem->WriteWord(0x027FFC04, 0x1FC2, Bus::Data); // Copy of chip ID 2
  arm9_mem->WriteHalf(0x027FFC10, 0x5835, Bus::Data); // Copy of ARM7 BIOS CRC
  arm9_mem->WriteHalf(0x027FFC40, 0x0001, Bus::Data); // Boot indicator

  loop(arm7.get(), arm9.get(), interconnect.get(), arm7_mem.get());
  return 0;
}
