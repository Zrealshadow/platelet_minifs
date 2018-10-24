#ifndef FS_BITMAP_H
#define FS_BITMAP_H

#include "fs_macro.h"

//
//这里用的是最小堆方法
//但应该有更好的方法
//用记号的方法之后再尝试
//

namespace _fs {

//我们不考虑锁
class FSBitmap {
 public:
  FSBitmap(index_t bit_num, byte byte_array[], QFile* fs_file,
           offset_t const bitmap_offsets[], index_t group_num);
  ~FSBitmap();

  bool get_by_index(index_t bit_id);
  void set_by_index(index_t bit_id, bool flag_used,
                    bool dump_to_outer_device = false);
  index_t get_first_free_id_and_set_one();
  void dump_to_outer_device_by_id(index_t bit_id);
  void dump_to_outer_device_by_byte_id(index_t byte_id);

  index_t get_rest_free_num();

 private:
  index_t first_free;

  byte* bitmap;
  index_t bit_num;
  index_t size;
  offset_t* bitmap_offsets;
  index_t group_num;
  index_t rest_free;
  QFile* fs_file;

  index_t get_first_free_id(bool set_one);
  index_t get_first_zero_bit_id_by_byte(index_t byte_id);
  index_t get_rest_free_n();
  uint8 have_free_bit(byte);
  void find_first_free_from_cur();
  void find_first_free_from_begin();
};

}  // namespace _fs

#endif  // FS_BITMAP_H
