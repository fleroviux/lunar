/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <duality/emulator_thread.hpp>
#include <QMainWindow>
#include <QMenuBar>
#include <memory>

#include "audio_device.hpp"
#include "screen.hpp"

struct MainWindow : QMainWindow {
  MainWindow();

private slots:
  void OnOpenFile();

private:
  void CreateFileMenu(QMenuBar* menu);

  Screen* screen;
  MAAudioDevice audio_device;
  std::unique_ptr<Duality::core::Core> core;
  std::unique_ptr<Duality::EmulatorThread> emu_thread;

  Q_OBJECT;
};
