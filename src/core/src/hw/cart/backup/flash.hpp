/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/backup_file.hpp>

#include "backup.hpp"

namespace Duality::Core {

/// FLASH memory emulation
struct FLASH final : Backup {
  enum class Size {
    _256K = 0,
    _512K,
    _1024K,
    _8192K
  };

  FLASH(std::string const& save_path, Size size_hint);

  void Reset() override;
  void Select() override;
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
    Idle,
    Deselected,
    ReceiveCommand,
    ReadJEDEC,
    ReadStatus,
    SendAddress0,
    SendAddress1,
    SendAddress2,
    DummyByte,
    ReadData,
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
  u8 jedec_id[3] { 0x20, 0x40, 0x12 };

  std::string save_path;
  Size size_hint;
  size_t mask;

  std::unique_ptr<BackupFile> file;
};

} // namespace Duality::Core
