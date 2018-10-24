#ifndef FS_HASH_H
#define FS_HASH_H

#include <random>
#include "fs_macro.h"

namespace {
// const int hash_size = 2503;  //素数
// int rehash[hash_size];

template <typename T>
struct FSHashNode {
  T data;
  index_t id;
};

template <typename T>
void shuflle(T a[], int size) {
  std::srand(43);
  //  std::shuffle(a, a + hash_size);
  std::random_shuffle(a, a + size);
}
}  // namespace

namespace _fs {

template <typename T, int hash_size>
class FSHash {
 public:
  FSHash() {
    for (int i = 0; i < hash_size; i++) {
      this->rehash[i] = i;
    }
    //保证第一个为0，即取第一个时并非再hash，而是直接hash；
    shuflle<int>(&this->rehash[1], hash_size - 1);
    for (int i = 0; i < hash_size; i++) {
      this->buf[i].id = fs::ILLEGAL;
    }
  }
  ~FSHash() {}

  T* get(index_t key) {
    int cur = key % hash_size, tmp;
    for (int i = 0; i < hash_size; i++) {
      tmp = this->buf[locate(cur, i)].id;
      if (tmp == fs::ILLEGAL)
        return nullptr;
      else if (tmp != key)
        continue;
      else
        return &this->buf[locate(cur, i)].data;
    }
    return nullptr;
  }
  void set(index_t key, T* data) {
    int cur = key % hash_size, tmp;
    for (int i = 0; i < hash_size; i++) {
      tmp = this->buf[locate(cur, i)].id;
      if (tmp == fs::ILLEGAL) {
        //注意这里buf时明确分配了空间的，所以memcpy安全
        memcpy(&this->buf[locate(cur, i)].data, data, sizeof(T));
        this->buf[locate(cur, i)].id = key;
        return;
      } else if (tmp != key) {
        continue;
      } else {
        memcpy(&this->buf[locate(cur, i)].data, data, sizeof(T));
        this->buf[locate(cur, i)].id = key;
        return;
      }
    }
    memcpy(&this->buf[locate(cur, 0)].data, data, sizeof(T));
    this->buf[locate(cur, 0)].id = key;
  }

  void set(index_t key, char* data, int offset, int size) {
    int cur = key % hash_size, tmp;
    for (int i = 0; i < hash_size; i++) {
      tmp = this->buf[locate(cur, i)].id;
      if (tmp == fs::ILLEGAL) {
        //注意这里buf时明确分配了空间的，所以memcpy安全
        char* p = (char*)(&this->buf[locate(cur, i)].data) + offset;
        memcpy(p, data, size);
        this->buf[locate(cur, i)].id = key;
        return;
      } else if (tmp != key) {
        continue;
      } else {
        char* p = (char*)(&this->buf[locate(cur, i)].data) + offset;
        memcpy(p, data, size);
        this->buf[locate(cur, i)].id = key;
        return;
      }
    }
    memcpy(&this->buf[locate(cur, 0)].data, data, sizeof(T));
    this->buf[locate(cur, 0)].id = key;
  }

 private:
  int rehash[hash_size];
  FSHashNode<T> buf[hash_size];
  inline int locate(int first_hash, int rehash_index) {
    //(debug) (x+a)%a才是最保险的做法
    return (first_hash + this->rehash[rehash_index] + hash_size) % hash_size;
  }
};
}  // namespace _fs

#endif  // FS_HASH_H
