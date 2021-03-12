/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <core/device/video_device.hpp>
#include <util/integer.hpp>
#include <QOpenGLWidget>

struct Screen final : QOpenGLWidget, Duality::Core::VideoDevice {
  Screen(QWidget* parent);
 ~Screen() override;

  auto sizeHint() const -> QSize override {
    return size();
  }

  void Draw(u32 const* top, u32 const* bottom) override;
  void CancelDraw();

protected:
  void initializeGL() override;
  void paintGL() override;

private slots:
  void OnDraw(u32 const* top, u32 const* bottom);

signals:
  void SignalDraw(u32 const* top, u32 const* bottom);

private:
  bool should_draw = false;
  GLuint textures[2];

  Q_OBJECT;
};
