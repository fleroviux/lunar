/*
 * Copyright (C) 2021 fleroviux
 */

#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>

#include "main_window.hpp"

MainWindow::MainWindow() : QMainWindow(nullptr) {
  setWindowTitle("Duality");

  screen = new Screen{this};
  setCentralWidget(screen);
  screen->resize(512, 768);

  auto menu = new QMenuBar{this};
  CreateFileMenu(menu);
  CreateEmulationMenu(menu);
  setMenuBar(menu);
}

void MainWindow::CreateFileMenu(QMenuBar* menu) {
  auto file_menu = menu->addMenu(tr("File"));

  auto open = file_menu->addAction(tr("Open"));
  file_menu->addSeparator();
  auto exit = file_menu->addAction(tr("Close"));

  connect(open, &QAction::triggered, this, &MainWindow::OnOpenFile);
  connect(exit, &QAction::triggered, &QApplication::quit);
}

void MainWindow::CreateEmulationMenu(QMenuBar* menu) {
  auto emu_menu = menu->addMenu(tr("Emulation"));

  action_reset = emu_menu->addAction(tr("Reset"));
  action_pause = emu_menu->addAction(tr("Pause"));
  action_stop  = emu_menu->addAction(tr("Stop"));

  action_reset->setEnabled(false);
  action_pause->setEnabled(false);
  action_pause->setCheckable(true);
  action_stop->setEnabled(false);

  connect(action_reset, &QAction::triggered, [this]() {
    emu_thread->Stop();
    //core->Reset();
    emu_thread->Start();
  });

  connect(action_pause, &QAction::triggered, [this]() {
    if (emu_thread->IsRunning()) {
      emu_thread->Stop();
      audio_device.SetPaused(true);
    } else {
      emu_thread->Start();
      audio_device.SetPaused(false);
    }
  });

  connect(action_stop, &QAction::triggered, [this]() {
    if (emu_thread) {
      action_reset->setEnabled(false);
      action_pause->setEnabled(false);
      action_stop->setEnabled(false);
      screen->CancelDraw();
      emu_thread->Stop();
      core = {};
      emu_thread = {};
    }
  });
}

void MainWindow::OnOpenFile() {
  QFileDialog file_dialog{this};
  file_dialog.setAcceptMode(QFileDialog::AcceptOpen);
  file_dialog.setFileMode(QFileDialog::ExistingFile);
  file_dialog.setNameFilter("Nintendo DS ROM (*.nds)");
  
  if (file_dialog.exec()) {
    // TODO: handle errors while attemping to load the ROM, firmware or BIOSes.
    // TODO; make it so that we don't have to dynamically allocate the core,
    // which is pretty pointless.
    screen->CancelDraw();
    if (emu_thread) {
      emu_thread->Stop();
    }
    core = std::make_unique<Duality::core::Core>(
      file_dialog.selectedFiles().at(0).toStdString());
    core->SetVideoDevice(*screen);
    core->SetAudioDevice(audio_device);
    emu_thread = std::make_unique<Duality::EmulatorThread>(*core.get());
    emu_thread->Start();

    action_reset->setEnabled(true);
    action_pause->setEnabled(true);
    action_stop->setEnabled(true);
  }
}