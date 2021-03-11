/*
 * Copyright (C) 2021 fleroviux
 */

#include <QApplication>

#include "main_window.hpp"

int main(int argc, char** argv) {
  QApplication app{argc, argv};

  QCoreApplication::setOrganizationName("fleroviux");
  QCoreApplication::setApplicationName("Duality");

  MainWindow window{app};
  window.show();

  return app.exec();
}