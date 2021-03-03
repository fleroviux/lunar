/*
 * Copyright (C) 2021 fleroviux
 */

#include "screen.hpp"

Screen::Screen(QWidget* parent)
    : QOpenGLWidget(parent) {
  QSurfaceFormat format;
  format.setSwapInterval(0);
  setFormat(format);
}

void Screen::initializeGL() {
}

void Screen::paintGL() {
}

void Screen::Draw(u32 const* top, u32 const* bottom) {
}