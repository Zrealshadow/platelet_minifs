#ifndef FS_DEBUG_H
#define FS_DEBUG_H

#include <QDebug>
//#include <ctime>
#include <iostream>

namespace test {

template <typename T>
void print_array(int size, T array[]) {
  for (int i = 0; i < size; i++) std::cout << array[i] << ' ';
  std::cout << '\n';
}
}  // namespace test

#endif  // FS_DEBUG_H
