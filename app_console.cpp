#include "app_console.h"

namespace app {
QTextStream out(stdout, QIODevice::WriteOnly);
QTextStream error(stderr, QIODevice::WriteOnly);
OutDecorator out_decorator(0);
ErrorDecorator error_decorator(0);
}  // namespace app
