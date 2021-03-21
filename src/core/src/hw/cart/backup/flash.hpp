/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/backup_file.hpp>

#include "backup.hpp"

namespace Duality::Core {

struct FLASH : Backup {
  enum class Size {
    _256K = 0,
    _512K,
    _1024K,
    _8192K
  };

  FLASH(std::string const& save_path, Size size_hint);

  void Reset() override;
  void Deselect() override;
  auto Transfer(u8 data) -> u8 override;

private:
  enum class Command : u8 {
    WriteEnable   = 0x06, // WREM
    WriteDisable  = 0x04, // WRDI
    ReadJEDEC     = 0x96, // RDID
    ReadStatus    = 0x05, // RDSR
    ReadData      = 0x03, // READ
    ReadDataFast  = 0x0B, // FAST
    PageWrite     = 0x0A, // PW
    PageProgram   = 0x02, // PP
    PageErase     = 0xDB, // PE
    SectorErase   = 0xD8, // SE
    DeepPowerDown = 0xB9, // DP
    ReleaseDeepPowerDown = 0xAB // RDP
  };

  enum class State {
    ReceiveCommand,
    ReadManufacturerID,
    ReadMemoryType,
    ReadMemoryCapacity,
    ReadStatus,
    SendAddress0,
    SendAddress1,
    SendAddress2,
    ReadFastDummyByte,
    Reading,
    PageWrite,
    PageProgram,
    PageErase,
    SectorErase
  } state;

  void ParseCommand(Command cmd);

  Command current_cmd;
  u32 address;
  bool write_enable_latch;
  bool deep_power_down;

  std::string save_path;
  Size size_hint;
  size_t mask = 0;

  std::unique_ptr<BackupFile> file;
};

} // namespace Duality::Core