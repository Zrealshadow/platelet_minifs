#ifndef FS_MACRO_H
#define FS_MACRO_H

#include <QBitArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QPair>
#include <QStack>
#include <QVector>
#define TR QObject::tr

#include <algorithm>
//#include <cstdio>
//#include <cstring>
#include <exception>
#include <iostream>

#include "app_console.h"
#include "fs_debug.h"
#include "version.h"

#define offset_t qint64
#define index_t qint32

#define int8 qint8
#define int16 qint16
#define int32 qint32
#define int64 qint64

#define uint8 quint8
#define byte uint8

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

namespace {
int16 char2auth[256] = {0};
}

namespace fs {

// all bit is one
const uint8 FULL_BYTE = 255;
// illegal index
const index_t ILLEGAL = -1;
// zero block
const byte ZERO_BLOCK[4096] = {0};
// file name max length
const int FILE_NAME_MAX_SIZE = 12;

// 1TB
const offset_t ONE_TB = (offset_t(1) << 40);
// 1GB
const offset_t ONE_GB = (offset_t(1) << 30);
// 1MB
const offset_t ONE_MB = (offset_t(1) << 20);
// 1KB
const offset_t ONE_KB = (offset_t(1) << 10);
// 1.8GB，组分裂的上界
// const offset_t GROUP_UPBOUND = offset_t(1.75 * ONE_GB);
const offset_t GROUP_UPBOUND = offset_t(7 * ONE_GB / 4);
// 4KB，存储的基本单元（用于“4K对齐”）
const offset_t FK = (offset_t(4096));
// 32KB，空间大小必须为32KB整数倍
const offset_t EIGHT_FK = 8 * FK;

// size of inode and block
const offset_t inode_size = 128;
const offset_t block_size = FK;
const offset_t B = FK;
const offset_t byte_size = offset_t(sizeof(byte));

// minimun of file system
// const offset_t min_size = 0.75 * ONE_GB;
// const offset_t min_size = offset_t(3 * ONE_GB / 4);

////test
const offset_t min_size = offset_t(1 * ONE_GB / 4);

// const offset_t max_size = 100 * ONE_GB;
const offset_t max_size = 2 * ONE_GB;

const offset_t default_size = ONE_GB;

// default offsets
//(for format_information,inode_bitmap,block_bitmap,inodes,blocks)
const offset_t default_format_offset = 0;
const offset_t default_ibm_offset = 1 * FK;
const offset_t default_bbm_offset = (1 + 16) * FK;
const offset_t default_inodes_offset = (1 + 16 + 16) * FK;
const offset_t default_blocks_offset = (1 + 16 + 16 + 15415) * FK;
const index_t default_block_num = 246696;
const index_t default_bitmap_bytes = default_block_num / 8;

const int32 header_B_num = 1;
const int32 ibm_B_num = 16;
const int32 bbm_B_num = 16;
const int32 inode_B_num = 15415;

const index_t root_id = 0;
const index_t inode_hash_size = 100007;
const index_t block_hash_size = 10007;

// const offset_t max_file_size = ONE_GB;
// const offset_t max_file_size = 900 * ONE_MB;
const offset_t max_file_size = 500 * ONE_MB;

// 注意：根目录为0级
// const uint8 max_dir_layer = 16;
const uint8 max_dir_layer = 5;

typedef struct FSGroupHeader {
  int32 version;
  int32 group_num;
  int32 total_block_num;
  index_t cur_group_id;
  int32 cur_group_block_num;

  offset_t cur_group_size;
  //  offset_t ibm_offset;
  //  offset_t bbm_offset;
  //  offset_t inodes_offset;
  //  offset_t blocks_offst;
  offset_t group_offset_array[100];
} FSGroupHeader;

//文件权限
class FileAuth {
 public:
  int16 auth_mod;

  typedef enum { AUTH_NONE = 0, AUTH_R = (1 << 0), AUTH_W = (1 << 1) } AuthMod;
  //可以使用位运算，例如：
  //可读可写的文件:FileAuth(FileAuth::AUTH_R|FileAuth::AUTH_W)
  FileAuth(int16 auth_mod) { set_auth(auth_mod); }
  //可读可写的文件:FileAuth("rw")
  FileAuth(QString auth_mod) { set_auth(auth_mod); }
  //默认可读可写
  FileAuth() { set_default(); }
  bool can_read() { return AUTH_R & this->auth_mod; }
  bool can_write() { return AUTH_W & this->auth_mod; }
  void set_default() { set_auth("rw"); }
  void set_auth(int16 auth_mod) { this->auth_mod = auth_mod; }
  void set_auth(QString auth_mod) {
    if (auth_mod.length() > 2)
      throw "FileAuth:set_auth: " + QString("传入字符串长度不能大于2");
    if (auth_mod.length() != auth_mod.toUtf8().length())
      //如果不等，说明有非单字节字符
      throw "FileAuth:set_auth: " + QString("传入字符串不能有非ascii成分");
    char2auth[int('r')] = AUTH_R;
    char2auth[int('w')] = AUTH_W;
    this->auth_mod = 0;
    for (int i = auth_mod.length() - 1; i >= 0; i--) {
      this->auth_mod |= char2auth[auth_mod.at(i).cell()];
    }
  }
  FileAuth operator=(FileAuth const other) {
    this->auth_mod = other.auth_mod;
    return *this;
  }

  // private:
  //  int16 auth_mod;
};

// 17Byte，考虑对齐为20Byte
typedef struct FSDirRecord {
  index_t inode_id;
  char file_name[FILE_NAME_MAX_SIZE + 1];
  char padding[3];  //填充作用，无意义
  void set(index_t inode_id, const char* file_name) {
    //这里没有长度检测！！
    this->inode_id = inode_id;
    strcpy(this->file_name, file_name);
  }
  void set_name(const char* file_name) { strcpy(this->file_name, file_name); }
} FSDirRecord;

// 240 * 17Byte + 16Byte（wrong）
// 204 * 20Byte + 16Byte
const int rn_in_b = 204;
typedef struct FSDirBlock {
  FSDirRecord records[204];
  int16 used;
} FSDirBlock;

// 4096Byte
typedef struct FSDataBlock {
  byte data_byte[block_size];
} FSDataBlock;

// 1024 * 4Byte
const int index_block_i_n = 1024;
typedef struct FSIndexBlock {
  index_t block_id[block_size / sizeof(index_t)];
} FSIndexBlock;

// 4KB
typedef union FSBlock {
  FSDirBlock dir_block;
  FSDataBlock data_block;
  FSIndexBlock index_block;
} FSBlock;

typedef struct FSIRoute {
  //顺序不能变了
  index_t route[3];
  int level;
  FSIRoute operator=(FSIRoute const other) {
    route[0] = other.route[0];
    route[1] = other.route[1];
    route[2] = other.route[2];
    level = other.level;
    return *this;
  }
  bool check() {
    if (this->level == 0) return false;
    return true;  //(debug)之前忘记加return true
  }
} FSIRoute;

// max:128B
const index_t file_max_block_n = fs::max_file_size / fs::block_size;
const index_t dir_max_block_n = 12 + 1024;  //只用一级间接索引
const index_t max_sub_file_n = dir_max_block_n * rn_in_b;
const index_t inode_direc_n = 12;
const index_t inode_first_n = 1024;
const index_t inodex_second_n = 1024 * 1024;
const index_t inode_fst_n = 12 + 1024;
const index_t inode_sec_n = 12 + 1024 + 1024 * 1024;

//-----------------------2018/9/11/22:08
typedef struct FSInode {
  char file_type;  //文件类型（'d':directory,'f':normal_file,'b':数据块）
  int8 dir_layer;      //目录层级（根目录为0层）
  int8 tree_layer;     //以该结点为根，目录树的层级 2018/9/12/
  FileAuth auth;       //权限
  index_t inode_id;    // inode号
  offset_t file_size;  //文件大小（对文件夹，是记录数）
  index_t block_num;   //占用block数量
  int64 create_time;   //创建时间
  int64 change_time;   //改变时间

  //  int64 latest_access_time;  //最近访问时间
  //  int64 latest_content_change_time;  //最近内容修改时间
  //  int64 latest_attr_change_time;     //最近属性变更时间

  index_t direct_ptr[12];  // 直接索引，直接指向block，-1为无效
  index_t first_level_ptr;  // 一级索引，指向FSIndexBlock（一级），-1为无效
  index_t second_level_ptr;  // 二级索引，指向FSIndexBlock（二级），-1为无效

  bool check_route(FSIRoute route);
  FSIRoute next_route(FSIRoute route);
  FSIRoute locate_block(index_t bi);
  index_t parse_route_to_bi(FSIRoute r);
  //根据bi定位二级间接索引块中的一级间接索引块
  index_t locate_l1_in_l2(index_t bi);
  //
  index_t locate_direc_in_l2(index_t bi);
  index_t locate_direc_in_l1(index_t bi);

} FSInode;

}  // namespace fs

#endif  // FS_MACRO_H
