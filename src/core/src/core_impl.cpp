/*
 * Copyright (C) 2021 fleroviux
 */

#include <core/core.hpp>
#include <fstream>
#include <stdexcept>

#include "arm/arm.hpp"
#include "arm7/bus.hpp"
#include "arm9/bus.hpp"
#include "arm9/cp15.hpp"
#include "interconnect.hpp"

namespace Duality::core {

/// NDS file header
struct Header {
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

/// No-operation stub for the CP14 coprocessor
struct CP14 : arm::Coprocessor {
  void Reset() override {}

  auto Read(
    int opcode1,
    int cn,
    int cm,
    int opcode2
  ) -> u32 override { return 0; }
  
  void Write(
    int opcode1,
    int cn,
    int cm,
    int opcode2,
    u32 value
  ) override {}
};

struct CoreImpl {
  CoreImpl(std::string_view rom_path) 
      : arm7_mem(&interconnect)
      , arm9_mem(&interconnect)
      , arm7(arm::ARM::Architecture::ARMv4T, &arm7_mem)
      , arm9(arm::ARM::Architecture::ARMv5TE, &arm9_mem)
      , cp15(&arm9, &arm9_mem) {
    arm7.AttachCoprocessor(14, &cp14);
    arm9.AttachCoprocessor(15, &cp15);

    interconnect.irq7.SetCore(arm7);
    interconnect.irq9.SetCore(arm9);
    interconnect.dma7.SetMemory(&arm7_mem);
    interconnect.dma9.SetMemory(&arm9_mem);

    Load(rom_path);
  }

  void Load(std::string_view rom_path) {
    using Bus = arm::MemoryBase::Bus;

    std::ifstream rom { rom_path, std::ios::in | std::ios::binary };
    if (!rom.good()) {
      throw std::runtime_error("failed to open ROM");
    }

    auto header = Header{};
    rom.read((char*)&header, sizeof(Header));
    if (!rom.good()) {
      throw std::runtime_error("failed to read ROM header, not enough data.");
    }

    {
      u8 data;
      u32 dst = header.arm7.load_address;
      rom.seekg(header.arm7.file_address);
      for (u32 i = 0; i < header.arm7.size; i++) {
        rom.read((char*)&data, 1);
        if (!rom.good()) {
          throw std::runtime_error("failed to read ARM7 binary from ROM into ARM7 memory");
        }
        arm7_mem.WriteByte(dst++, data, Bus::Data);
      }

      arm7.ExceptionBase(0);
      arm7.Reset();
      arm7.GetState().r13 = 0x0380FD80;
      arm7.GetState().bank[arm::BANK_IRQ][arm::BANK_R13] = 0x0380FF80;
      arm7.GetState().bank[arm::BANK_SVC][arm::BANK_R13] = 0x0380FFC0;
      arm7.SetPC(header.arm7.entrypoint);
    }

    {
      u8 data;
      u32 dst = header.arm9.load_address;
      rom.seekg(header.arm9.file_address);
      for (u32 i = 0; i < header.arm9.size; i++) {
        rom.read((char*)&data, 1);
        if (!rom.good()) {
          throw std::runtime_error("failed to read ARM9 binary from ROM into ARM9 memory");
        }
        arm9_mem.WriteByte(dst++, data, Bus::Data);
      }

      arm9.ExceptionBase(0xFFFF0000);
      arm9.Reset();
      // TODO: ARM9 stack is in shared WRAM by default,
      // but afaik at cartridge boot time all of SWRAM is mapped to NDS7?
      arm9.GetState().r13 = 0x03002F7C;
      arm9.GetState().bank[arm::BANK_IRQ][arm::BANK_R13] = 0x03003F80;
      arm9.GetState().bank[arm::BANK_SVC][arm::BANK_R13] = 0x03003FC0;
      arm9.SetPC(header.arm9.entrypoint);
    }

    u8 data;
    u32 dst = 0x02FFFE00;
    rom.seekg(0);
    for (u32 i = 0; i < 0x170; i++) {
      rom.read((char*)&data, 1);
      if (!rom.good()) {
        throw std::runtime_error("failed to load cartridge header into memory");
      }
      arm9_mem.WriteByte(dst++, data, Bus::Data);
    }

    rom.close();

    // Load cartridge ROM into Slot 1
    interconnect.cart.Load(rom_path);

    // Huge thanks to Hydr8gon for pointing this out:
    arm9_mem.WriteWord(0x027FF800, 0x1FC2, Bus::Data); // Chip ID 1
    arm9_mem.WriteWord(0x027FF804, 0x1FC2, Bus::Data); // Chip ID 2
    arm9_mem.WriteHalf(0x027FF850, 0x5835, Bus::Data); // ARM7 BIOS CRC
    arm9_mem.WriteHalf(0x027FF880, 0x0007, Bus::Data); // Message from ARM9 to ARM7
    arm9_mem.WriteHalf(0x027FF884, 0x0006, Bus::Data); // ARM7 boot task
    arm9_mem.WriteWord(0x027FFC00, 0x1FC2, Bus::Data); // Copy of chip ID 1
    arm9_mem.WriteWord(0x027FFC04, 0x1FC2, Bus::Data); // Copy of chip ID 2
    arm9_mem.WriteHalf(0x027FFC10, 0x5835, Bus::Data); // Copy of ARM7 BIOS CRC
    arm9_mem.WriteHalf(0x027FFC40, 0x0001, Bus::Data); // Boot indicator
  }

  Interconnect interconnect;
  ARM7MemoryBus arm7_mem;
  ARM9MemoryBus arm9_mem;
  arm::ARM arm7;
  arm::ARM arm9;
  CP14 cp14;
  CP15 cp15; 
};

Core::Core(std::string_view rom_path) {
  pimpl = new CoreImpl(rom_path);
}

Core::~Core() {
  delete pimpl;
}

} // namespace Duality::core
