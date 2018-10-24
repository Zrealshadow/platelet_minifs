#include "fs_bitmap.h"
#include "fs_macro.h"

namespace {
const uint8 False = uint8(0);
const uint8 True = uint8(1);
// 0-254中各数的二进制形式下，第一个0的位置
const uint8 FIRST_ZERO[255] = {
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6,
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4,
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0};
const uint8 ZERO_NUM[256] = {
    8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4, 7, 6, 6, 5, 6, 5, 5, 4,
    6, 5, 5, 4, 5, 4, 4, 3, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
    6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 7, 6, 6, 5, 6, 5, 5, 4,
    6, 5, 5, 4, 5, 4, 4, 3, 6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2,
    6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2,
    4, 3, 3, 2, 3, 2, 2, 1, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,
    6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 6, 5, 5, 4, 5, 4, 4, 3,
    5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
    6, 5, 5, 4, 5, 4, 4, 3, 5, 4, 4, 3, 4, 3, 3, 2, 5, 4, 4, 3, 4, 3, 3, 2,
    4, 3, 3, 2, 3, 2, 2, 1, 5, 4, 4, 3, 4, 3, 3, 2, 4, 3, 3, 2, 3, 2, 2, 1,
    4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0};

//位反转
// byte reverse8(byte c) {
//  c = (c & byte(0x55)) << 1 | (c & byte(0xAA)) >> 1;
//  c = (c & byte(0x33)) << 2 | (c & byte(0xCC)) >> 2;
//  c = (c & byte(0x0F)) << 4 | (c & byte(0xF0)) >> 4;
//  return c;
//}
}  // namespace

namespace _fs {
/*
 *
 * public
 *
 */
FSBitmap::FSBitmap(index_t bit_num, byte byte_array[], QFile* fs_file,
                   offset_t const bitmap_offsets[], index_t group_num) {
  index_t size = bit_num / 8;
  this->bit_num = bit_num;
  this->size = size;
  this->bitmap = new byte[size];
  this->rest_free = 0;
  for (index_t i = 0; i < size; i++) {
    this->rest_free += ZERO_NUM[this->bitmap[i]];
  }
  // this->fs_file = new QFile(fs_file);
  // 注意此处不能新建一个QFile，因为该文件已经被打开了，新申请的QFile无权打开这个文件（资源被占用）
  this->fs_file = fs_file;
  this->group_num = group_num;
  // (debug)类内部const*只是在类内部不能改而已，如果这个const*指向外部可变区域，外部的改变还会影响它
  //  offset_t const* bitmap_offsets;
  //    this->bitmap_offsets = bitmap_offsets;
  this->bitmap_offsets = new offset_t[group_num];
  for (int i = 0; i < this->group_num; i++) {
    this->bitmap_offsets[i] = bitmap_offsets[i];
  }
  for (index_t i = 0; i < size; i++) {
    // 对L2R(从左到右顺序，即正常顺序)的二进制串要从左向右读取
    // 但bitmap数组是8位一组从左向右，而每个byte内bits是从右向左，这矛盾了
    // 因此第一步就要把L2R二进制串转为好处理的R2L型
    //(debug)现在不用了
    this->bitmap[i] = byte_array[i];
  }

  //  this->first_free = 0;
  find_first_free_from_begin();
}

FSBitmap::~FSBitmap() {
  delete[] this->bitmap;
  delete[] this->bitmap_offsets;
  // 不能删除fs_file，因为它的空间是在类外申请的
  //  delete this->fs_file;
  // 这个是不能delete的，因为他不是在类内new的
  // delete this->bitmap_offsets;
}

index_t FSBitmap::get_rest_free_num() { return this->rest_free; }

bool FSBitmap::get_by_index(index_t bit_id) {
  //我们不考虑锁
  return 1 & ((this->bitmap[bit_id / 8]) >> (bit_id % 8));
}

void FSBitmap::set_by_index(index_t bit_id, bool flag_used,
                            bool dump_to_outer_device) {
  this->rest_free -= ZERO_NUM[this->bitmap[bit_id / 8]];
  if (flag_used) {  //置1
    this->bitmap[bit_id / 8] |= (1 << (bit_id % 8));
    if (bit_id == this->first_free) {
      find_first_free_from_cur();
    }

  } else {  //置0
    byte a = this->bitmap[bit_id / 8];
    this->bitmap[bit_id / 8] &= byte(~(1 << (bit_id % 8)));
    if (bit_id < this->first_free) {
      this->first_free = bit_id;
    }
    //    if (this->first_free == -1)
    //      qDebug()
    //          <<
    //          "-------------------------------------------------------------";
  }

  this->rest_free += ZERO_NUM[this->bitmap[bit_id / 8]];
  if (dump_to_outer_device) {
    dump_to_outer_device_by_id(bit_id);
  }
  return;
}

index_t FSBitmap::get_first_free_id(bool set_one) {
  // first_valid_byte_id
  index_t ans = fs::ILLEGAL;
  ans = this->first_free;

  if (ans == fs::ILLEGAL) {
    return ans;
  }
  if (set_one) {
    //要写入外存
    set_by_index(ans, true, true);
  }
  //  qDebug() << ans;
  return ans;
}

index_t FSBitmap::get_first_free_id_and_set_one() {
  return get_first_free_id(true);
}

void FSBitmap::dump_to_outer_device_by_id(index_t bit_id) {
  dump_to_outer_device_by_byte_id(bit_id / 8);
}

void FSBitmap::dump_to_outer_device_by_byte_id(index_t byte_id) {
  //注意，如果最后一组block数大于default_block_num，则应有所变化
  offset_t offset;
  if (byte_id / fs::default_bitmap_bytes >= this->group_num) {
    offset = this->bitmap_offsets[byte_id / fs::default_bitmap_bytes - 1] +
             (byte_id % fs::default_bitmap_bytes + fs::default_bitmap_bytes) *
                 fs::byte_size;
  } else {
    offset = this->bitmap_offsets[byte_id / fs::default_bitmap_bytes] +
             (byte_id % fs::default_bitmap_bytes) * fs::byte_size;
  }

  byte write_info = this->bitmap[byte_id];
  this->fs_file->seek(offset);
  this->fs_file->write(reinterpret_cast<char*>(&write_info), fs::byte_size);
}

inline index_t FSBitmap::get_rest_free_n() { return this->rest_free; }

inline uint8 FSBitmap::have_free_bit(byte byte_content) {
  return (fs::FULL_BYTE == byte_content) ? False : True;
}

void FSBitmap::find_first_free_from_cur() {
  index_t byte_id = this->first_free / 8;
  for (index_t i = byte_id; i < this->size; i++) {
    if (ZERO_NUM[this->bitmap[i]]) {
      this->first_free = FIRST_ZERO[this->bitmap[i]] + i * 8;
      return;
    }
  }
  this->first_free = fs::ILLEGAL;
}

void FSBitmap::find_first_free_from_begin() {
  for (index_t i = 0; i < this->size; i++) {
    if (ZERO_NUM[this->bitmap[i]]) {
      this->first_free = FIRST_ZERO[this->bitmap[i]] + i * 8;
      return;
    }
  }
  this->first_free = fs::ILLEGAL;
}

}  // namespace _fs
