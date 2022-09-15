
// Copyright (C) 2022 fleroviux

#pragma once

#include <functional>

namespace lunar {

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

  virtual bool Poll(Key key) = 0;
  virtual void SetOnChangeCallback(std::function<void(void)> callback) = 0;
  virtual auto GetTouchPoint() -> Point const& = 0;

  static constexpr int kKeyCount = 13;
};

struct BasicInputDevice final : InputDevice {
  bool Poll(Key key) override {
    return keys[static_cast<int>(key)];
  }

  void SetOnChangeCallback(std::function<void(void)> callback) final {
    keypress_callback = callback;
  }

  void SetKeyDown(Key key, bool down) {
    keys[static_cast<int>(key)] = down;
    keypress_callback();
  }

  auto GetTouchPoint() -> Point& override {
    return point;
  }

private:
  Point point;
  std::function<void(void)> keypress_callback = []() {};
  bool keys[kKeyCount] {false};
};

} // namespace lunar
