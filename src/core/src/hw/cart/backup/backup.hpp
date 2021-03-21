/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

namespace Duality::Core {

struct Backup {
  virtual ~Backup() = default;

  virtual void Reset() = 0;
  virtual void Deselect() = 0;
  virtual auto Transfer(u8 data) -> u8 = 0;
};

} // namespace Duality::Core
