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

void MainWindow::OnOpenFile() {
  QFileDialog file_dialog{this};
  file_dialog.setAcceptMode(QFileDialog::AcceptOpen);
  file_dialog.setFileMode(QFileDialog::ExistingFile);
  file_dialog.setNameFilter("Nintendo DS ROM (*.nds)");
  
  if (file_dialog.exec()) {
  }
}