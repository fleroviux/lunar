/*
 * Copyright (C) 2021 fleroviux
 */

#include "screen.hpp"

Screen::Screen(QWidget* parent)
    : QOpenGLWidget(parent) {
  QSurfaceFormat format;
  format.setSwapInterval(0);
  setFormat(format);

  // The signal-slot structure is needed, so that Screen::Draw
  // may be invoked from another thread without causing trouble.
  connect(this, &Screen::SignalDraw, this, &Screen::OnDraw);
}

Screen::~Screen() {
  glDeleteTextures(2, &textures[0]);
}

void Screen::Draw(u32 const* top, u32 const* bottom) {
  should_draw = true;
  emit SignalDraw(top, bottom);
}

void Screen::CancelDraw() {
  should_draw = false;
}

void Screen::OnDraw(u32 const* top, u32 const* bottom) {
  if (should_draw) {
    should_draw = false;

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    if (top != nullptr) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_BGRA, GL_UNSIGNED_BYTE, top);
    }

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    if (bottom != nullptr) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 192, 0, GL_BGRA, GL_UNSIGNED_BYTE, bottom);
    }

    update();
  }
}


void Screen::initializeGL() {
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D);
  glGenTextures(2, &textures[0]);

  for (uint i = 0; i < 2; i++) {
    glBindTexture(GL_TEXTURE_2D, textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  }

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Screen::paintGL() {
  auto width  = size().width();
  auto height = size().height();
  auto offset_x = 0;
  width = int(height * 256.9 / 384.0);
  offset_x = (size().width() - width) / 2;

  glViewport(offset_x, 0, width, height);
  glClear(GL_COLOR_BUFFER_BIT);

  glBindTexture(GL_TEXTURE_2D, textures[0]);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(-1.0f,  1.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f( 1.0f,  1.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f( 1.0f,  0.0f);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(-1.0f,  0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, textures[1]);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(-1.0f,  0.0f);
  glTexCoord2f(1.0f, 0.0f);
  glVertex2f( 1.0f,  0.0f);
  glTexCoord2f(1.0f, 1.0f);
  glVertex2f( 1.0f, -1.0f);
  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(-1.0f, -1.0f);
  glEnd();
}
