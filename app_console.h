#ifndef APP_CONSOLE_H
#define APP_CONSOLE_H

//#include "fs_macro.h"
#include <QTextStream>
#include <iostream>

#define APP_OUT(content)         \
  {                              \
    app::out_decorator.before(); \
    app::out << (content);       \
    app::out_decorator.after();  \
  }

#define APP_ERROR(content)         \
  {                                \
    app::error_decorator.before(); \
    app::error << (content);       \
    app::error_decorator.after();  \
  }

namespace app {
class OutDecorator;
class ErrorDecorator;
extern QTextStream out;
extern QTextStream error;
extern OutDecorator out_decorator;
extern ErrorDecorator error_decorator;

class OutDecorator {
 public:
  OutDecorator(int tmp) {}
  void before() {}
  void after() { out << endl; }
};
class ErrorDecorator {
 public:
  ErrorDecorator(int tmp) {}
  void before() { error << "ERROR:\n"; }
  void after() { error << endl; }
};

}  // namespace app

#endif  // APP_CONSOLE_H
