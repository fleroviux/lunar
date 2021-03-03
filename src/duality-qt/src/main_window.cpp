/*
 * Copyright (C) 2021 fleroviux
 */

#include <QMenuBar>

#include "main_window.hpp"

MainWindow::MainWindow() : QMainWindow(nullptr) {
  setWindowTitle("Duality");
  setFixedSize(512, 768);

  auto menu = new QMenuBar{this};
  auto file_menu = menu->addMenu(tr("File"));
  file_menu->addAction(tr("Open"));
  //file_menu->addSeparator();
  file_menu->addAction(tr("Exit"));
  setMenuBar(menu);
}