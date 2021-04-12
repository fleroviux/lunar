/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/backup_file.hpp>

#include "hw/spi/spi_device.hpp"

namespace Duality::Core {

/// EEPROM (and FRAM) memory emulation
struct EEPROM final : SPIDevice {
  enum class Size {
    _8K,
    _32K,
    _64K,
    _128K
  };

  EEPROM(std::string const& save_path, Size size_hint);

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
    Read         = 0x03, // RD
    Write        = 0x02  // WR
  };

  enum class State {
    Idle,
    Deselected,
    ReceiveCommand,
    ReadStatus,
    WriteStatus,
    ReadAddress0,
    ReadAddress1,
    ReadAddress2,
    Read,
    Write
  } state;

  void ParseCommand(Command cmd);

  Command current_cmd;
  u16 address;
  bool write_enable_latch;
  int  write_protect_mode;
  bool status_reg_write_disable;

  std::string save_path;
  Size size_hint;
  Size size;
  size_t mask;
  size_t page_mask;

  std::unique_ptr<BackupFile> file;
};

} // namespace Duality::Core
