/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <QMainWindow>
#include <QMenuBar>

#include "screen.hpp"

struct MainWindow : QMainWindow {
  MainWindow();

private:
  void CreateFileMenu(QMenuBar* menu);
  void OnOpenFile();

  Screen* screen;

  Q_OBJECT;
};
