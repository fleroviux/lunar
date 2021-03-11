/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <duality/emulator_thread.hpp>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <memory>

#include "audio_device.hpp"
#include "screen.hpp"

struct MainWindow : QMainWindow {
  MainWindow(QApplication& app);

protected:
  bool eventFilter(QObject* target, QEvent* event) override;

private slots:
  void OnOpenFile();

private:
  void CreateFileMenu(QMenuBar* menu);
  void CreateEmulationMenu(QMenuBar* menu);

  Screen* screen;
  QAction* action_reset;
  QAction* action_pause;
  QAction* action_stop;
  MAAudioDevice audio_device;
  Duality::core::BasicInputDevice input_device;
  std::unique_ptr<Duality::core::Core> core;
  std::unique_ptr<Duality::EmulatorThread> emu_thread;

  Q_OBJECT;
};
