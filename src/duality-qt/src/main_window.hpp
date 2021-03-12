/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <duality/emulator_thread.hpp>
#include <QApplication>
#include <QMainWindow>
#include <QKeyEvent>
#include <QMouseEvent>
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
  bool UpdateKeyInput(QObject* watched, QKeyEvent* event);
  bool UpdateTouchInput(QObject* watched, QMouseEvent* event);

  Screen* screen;
  QAction* action_reset;
  QAction* action_pause;
  QAction* action_stop;
  MAAudioDevice audio_device;
  Duality::Core::BasicInputDevice input_device;
  std::unique_ptr<Duality::Core::Core> core;
  std::unique_ptr<Duality::EmulatorThread> emu_thread;

  Q_OBJECT;
};
