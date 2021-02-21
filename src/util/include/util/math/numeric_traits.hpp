/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

template<typename T>
struct NumericConstants {
  // static constexpr auto zero() -> T;
  // static constexpr auto one()  -> T;
};

template<>
struct NumericConstants<float> {
  static constexpr auto zero() -> float {
    return 0;
  }
  
  static constexpr auto one() -> float {
    return 1;
  }
};

template<>
struct NumericConstants<double> {
  static constexpr auto zero() -> double {
    return 0;
  }
  
  static constexpr auto one() -> double {
    return 1;
  }
};