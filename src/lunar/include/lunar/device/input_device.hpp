/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <functional>

namespace lunar {

class InputDevice {
  public:
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

    static constexpr int k_key_count = 13;
};

class BasicInputDevice final : public InputDevice {
  public:
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
    bool keys[k_key_count] {false};
};

} // namespace lunar
