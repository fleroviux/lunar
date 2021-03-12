/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

namespace Duality::Core {

struct InputDevice {
  enum class Key {
    A,
    B,
    X,
    Y,
    L,
    R,
    Start,
    Select,
    Up,
    Down,
    Left,
    Right,
    TouchPen
  };

  struct Point {
    int x = 0;
    int y = 0;
  };

  virtual ~InputDevice() = default;

  virtual bool IsKeyDown(Key key) = 0;
  virtual auto GetTouchPoint() -> Point const& = 0;

  static constexpr int kKeyCount = 13;
};

struct BasicInputDevice final : InputDevice {
  bool IsKeyDown(Key key) override {
    return keys[static_cast<int>(key)];
  }

  void SetKeyDown(Key key, bool down) {
    keys[static_cast<int>(key)] = down;
  }

  auto GetTouchPoint() -> Point& override {
    return point;
  }

private:
  Point point;
  bool keys[kKeyCount] {false};
};

} // namespace Duality::Core
