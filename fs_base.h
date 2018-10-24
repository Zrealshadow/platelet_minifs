/**
 * author: 欧阳峻彦
 * version:1.0.1
 */
#ifndef FS_BASE_H
#define FS_BASE_H

#include "fs_bitmap.h"
#include "fs_hash.h"
#include "fs_macro.h"

namespace fs {

class FSFile;  //先声明

class FSBase {
 public:
  ~FSBase();

  friend class FSFile;
  friend class MiniFSGroup;
  friend QString get_current_dir(FSBase* pfs);
  friend void change_current_dir(FSBase* pfs, QString path);
  friend QStringList list_dir(FSBase* pfs, QString dir_path);

  friend void _create_dir(FSBase* pfs, FSFile& parent, QString name,
                          bool force);
  friend void _create_file(FSBase* pfs, FSFile& parent, QString name);
  friend void create_dir(FSBase* pfs, QString path, bool last_force);
  friend void create_file(FSBase* pfs, QString path);

  friend void change_auth_mod(FSBase* pfs, QString path, FileAuth auth);
  friend void change_auth_mod(FSFile* file, FileAuth auth);
  friend void move_file(FSBase* pfs, QString former_path, QString new_path);
  friend void move_file(FSFile* file, QString new_path);

  //
  friend void write(FSFile* file, char* data, int64 offset_in_file,
                    int64 max_size);
  friend index_t get_rest_inode_num(FSBase* pfs);
  friend index_t get_rest_block_num(FSBase* pfs);

 private:
  QFile* ptr_fs;
  //当前路径
  index_t cur_inode_id;
  QStringList* cur_dir_path;
  QList<index_t>* cur_route;

  // static是类的对象共享，所以不能static
  //  static _fs::FSBitmap* inode_bitmap;void
  //  static _fs::FSBitmap* block_bitmap;
  _fs::FSBitmap* inode_bitmap;
  _fs::FSBitmap* block_bitmap;
  _fs::FSHash<FSInode, fs::inode_hash_size>* inode_hash;
  _fs::FSHash<FSBlock, fs::block_hash_size>* block_hash;

  int32 version;
  int32 group_num;
  int32 total_block_num;
  offset_t group_offset_array[100];
  int32 normal_group_block_num;
  offset_t normal_group_size;
  int32 last_group_block_num;
  offset_t last_group_size;

  //////////原public
  FSBase();
  FSBase(QString outer_path);
  // hash表和外存操作配合

  // memcpy会有野指针问题（内容被覆盖到未知区域），所以传的时引用，以保证内存已经被分配了(通过ide的警报)
  // QT的
  // 文件read似乎会把野指针认为时空指针？？？但是不报错，只是不能运行下去？？？

  //我觉得写操作还是应该有个指针的。。。。。。。。。。。。。。。要不然不方便
  void read_inode(FSInode& data, index_t inode_id);
  void write_inode(FSInode& data, index_t inode_id);
  void read_block(FSBlock& data, index_t block_id);
  void write_block(FSBlock& data, index_t block_id);
  void calcu_cur_route();
  index_t get_cur_inode_id();
  void set_cur_inode_id(index_t new_cur_inode_id);
  QString get_cur_dir();
  //看inode_id是否在cur_route中
  bool check_in_cur_route(index_t inode_id);
  //////////

  index_t add_new_block(bool dump);
  index_t add_new_inode(bool dump);
  void remove_block(index_t block_id, bool dump);
  void remove_inode(index_t inode_id, bool dump);
  void dump_block_bitmap_by_id(index_t block_id);
  void dump_inode_bitmap_by_id(index_t inode_id);

  //外存操作
  void load(offset_t offset, void* data, offset_t size);
  void dump(offset_t offset, const void* data, offset_t size);
  offset_t seek_inode(index_t inode_id);
  void load_inode(FSInode* data, index_t inode_id);
  void dump_inode(FSInode* data, index_t inode_id);
  offset_t seek_block(index_t block_id);
  void load_block(FSBlock* data, index_t block_id);
  void dump_block(FSBlock* data, index_t block_id);

  //用于写文件提速
  void read_block_fast(void* block_data, offset_t offset_in_block,
                       offset_t size, index_t block_id);
  void write_block_fast(void* block_data, offset_t offset_in_block,
                        offset_t size, index_t block_id);
};

}  // namespace fs

#endif  // FS_BASE_H
