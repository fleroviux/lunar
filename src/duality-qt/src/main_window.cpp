/*
 * Copyright (C) 2021 fleroviux
 */

#include <QApplication>
#include <QFileDialog>
#include <QStatusBar>
#include <utility>

#include "main_window.hpp"

MainWindow::MainWindow(QApplication& app) : QMainWindow(nullptr) {
  setWindowTitle("Duality");

  screen = new Screen{this};
  setCentralWidget(screen);
  screen->resize(512, 768);
  screen->installEventFilter(this);

  auto menu = new QMenuBar{this};
  CreateFileMenu(menu);
  CreateEmulationMenu(menu);
  setMenuBar(menu);

  app.installEventFilter(this);
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
    core->Reset();
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
    // TODO: make it so that we don't have to dynamically allocate the core,
    // which is pretty pointless.
    screen->CancelDraw();
    if (emu_thread) {
      emu_thread->Stop();
    }
    core = std::make_unique<Duality::core::Core>(
      file_dialog.selectedFiles().at(0).toStdString());
    core->SetVideoDevice(*screen);
    core->SetAudioDevice(audio_device);
    core->SetInputDevice(input_device);
    emu_thread = std::make_unique<Duality::EmulatorThread>(*core.get());
    emu_thread->Start();

    action_reset->setEnabled(true);
    action_pause->setEnabled(true);
    action_stop->setEnabled(true);
  }
}

bool MainWindow::UpdateKeyInput(QObject* watched, QKeyEvent* event) {
  using namespace Duality::core;

  // TODO: make the key mapping configurable.
  static constexpr std::pair<int, InputDevice::Key> kKeyMap[] {
    { Qt::Key_A, InputDevice::Key::A },
    { Qt::Key_S, InputDevice::Key::B },
    { Qt::Key_D, InputDevice::Key::L },
    { Qt::Key_F, InputDevice::Key::R },
    { Qt::Key_Q, InputDevice::Key::X },
    { Qt::Key_W, InputDevice::Key::Y },
    { Qt::Key_Backspace, InputDevice::Key::Select },
    { Qt::Key_Return, InputDevice::Key::Start },
    { Qt::Key_Up, InputDevice::Key::Up },
    { Qt::Key_Down, InputDevice::Key::Down },
    { Qt::Key_Left, InputDevice::Key::Left },
    { Qt::Key_Right, InputDevice::Key::Right }
  };

  auto key = event->key();
  bool down = event->type() == QEvent::KeyPress;

  for (auto entry : kKeyMap) {
    if (entry.first == key) {
      input_device.SetKeyDown(entry.second, down);
      return true;
    }
  }

  if (key == Qt::Key_Space) {
    emu_thread->SetFastForward(down);
    return true;
  }

  return QObject::eventFilter(watched, event);
}

bool MainWindow::UpdateTouchInput(QObject* watched, QMouseEvent* event) {
  auto size = screen->size();
  auto scale = 384.0 / size.height();
  auto x = int((event->x() - size.width() / 2) * scale) + 128;
  auto y = int(event->y() * scale) - 192;
  bool down = (event->buttons() & Qt::LeftButton) && y >= 0 && x >= 0 && x <= 255;

  input_device.SetKeyDown(Duality::core::InputDevice::Key::TouchPen, down);
  input_device.GetTouchPoint().x = x;
  input_device.GetTouchPoint().y = y;

  return QObject::eventFilter(watched, event);
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
  auto type = event->type();

  switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      return UpdateKeyInput(watched, dynamic_cast<QKeyEvent*>(event));
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
      if (watched == screen) {
        return UpdateTouchInput(watched, dynamic_cast<QMouseEvent*>(event));
      }
  } 

  return QObject::eventFilter(watched, event);
}