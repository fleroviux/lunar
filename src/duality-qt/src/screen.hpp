/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>
#include <QOpenGLWidget>

struct Screen : QOpenGLWidget {
  Screen(QWidget* parent);

  auto sizeHint() const -> QSize override {
    return size();
  }

protected:
  void initializeGL() override;
  void paintGL() override;

public slots:
  void Draw(u32 const* top, u32 const* bottom);

private:
  Q_OBJECT;
};
