#include <QCoreApplication>
#include <QDebug>

#include "fs_macro.h"
#include "sess_core.h"



int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);
  //  APP_OUT(1)
   sess::user_interface();
  //test::testFunc();
  return a.exec();
}
