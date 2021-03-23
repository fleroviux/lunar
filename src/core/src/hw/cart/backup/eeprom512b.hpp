/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/backup_file.hpp>

#include "hw/spi/spi_device.hpp"

namespace Duality::Core {

/// EEPROM 512B memory emulation
struct EEPROM512B final : SPIDevice {
  EEPROM512B(std::string const& save_path);

  void Reset() override;
  void Select() override;
  void Deselect() override;
  auto Transfer(u8 data) -> u8 override;

private:
  enum class Command : u8 {
    WriteEnable  = 0x06, // WREM
    WriteDisable = 0x04, // WRDI
    ReadStatus   = 0x05, // RDSR
    WriteStatus  = 0x01, // WRSR
    Read         = 0x03, // RDLO & RDHI
    Write        = 0x02  // WRLO & WRHI
  };

  enum class State {
    Idle,
    Deselected,
    ReceiveCommand,
    ReadStatus,
    WriteStatus,
    ReadAddress,
    Read,
    Write
  } state;

  void ParseCommand(u8 cmd);

  Command current_cmd;
  u16 address;
  bool write_enable_latch;
  int  write_protect_mode;

  std::string save_path;

  std::unique_ptr<BackupFile> file;
};

} // namespace Duality::Core
