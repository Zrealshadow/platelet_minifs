#ifndef FS_FILE_H
#define FS_FILE_H

#include <fs_base.h>
#include <fs_macro.h>

namespace fs {

struct FSFileAttr;

class FSFile {
  //析构？？？？
 public:
  // R读,W写,RW读和写,A文件尾部添加
  typedef enum { R, W, RW, A } OpenMode;

  friend void change_current_dir(FSBase* pfs, QString path);
  friend QStringList list_dir(FSBase* pfs, QString dir_path);
  friend QList<QPair<FSFileAttr, QString>> list_dir_with_attr(FSBase* pfs,
                                                              QString dir_path);
  friend void _create_dir(FSBase* pfs, FSFile& parent, QString name,
                          bool force);
  friend void _create_file(FSBase* pfs, FSFile& parent, QString name);
  friend void create_dir(FSBase* pfs, QString path, bool last_force);
  friend void create_file(FSBase* pfs, QString path);

  friend void change_auth_mod(FSBase* pfs, QString path, FileAuth auth);
  friend void change_auth_mod(FSFile* file, FileAuth auth);

  friend int8 get_dir_layer(FSBase* pfs, QString path);
  friend int8 get_tree_layer(FSBase* pfs, QString path);

  friend void move(FSBase* pfs, QString former_path, QString new_path);
  friend void move(FSFile* file, QString new_path);

  friend FSFileAttr get_file_attr(FSBase* pfs, QString path);
  friend FSFileAttr get_file_attr(FSFile* file);

  friend FSFile* open(FSBase* pfs, QString path, FSFile::OpenMode mode);
  friend FSFile* open(FSBase* pfs, QString path);
  ////
  friend void close(FSFile*& file);
  //////

  friend int64 read(FSFile* file, char* data, int64 offset_in_file,
                    int64 max_size);
  friend void write(FSFile* file, char* data, int64 offset_in_file,
                    int64 max_size);
  friend void add_content(FSFile* file, char* data, int64 max_size);
  friend void clear(FSFile* file);
  friend void remove(FSBase* pfs, QString name);
  friend void remove(FSFile* file);

  friend QList<index_t> get_block_id_list(FSBase* pfs, QString path);

  //文件夹删除还没搞

  static bool check_path_name(QString path_name);
  static bool check_dir_name(QString dir_name);
  static bool check_file_name(QString file_name);
  static bool check_folder_name(QString folder_name);
  static bool check_file_or_folder_name(QString name);

  //是否要取绝对路径信息

 private:
  bool deleted;
  OpenMode open_mode;
  FSBase* pfs;
  FSInode inode;
  FSInode data_inode;  //数据

  //  char type;

  //绝对路径信息，但顺序从根目录到该文件是从右到左（反的）
  // i_route_reversed[0]为当前文件，i_route_reversed[1]为上一级
  // root没有上一级
  QVector<index_t> i_route_reversed;
  QString name;

  //////原public
  FSFile();
  FSFile(FSBase* p_fsbase, QString path,
         FSFile::OpenMode open_mode);  //文件或文件夹（对文件夹，open_mode没用）
  FSFile(FSBase* p_fsbase, QString path);  //默认权限为RW
  FSFile(FSBase* p_fsbase);                //主要用于打开当前目录
  ~FSFile();
  offset_t read(char* data, int64 offset_in_file, int64 max_size);
  void write(char* data, int64 offset_in_file, int64 max_size);
  void add_content(char* data, int64 max_size);
  void clear();
  void remove();
  void mv_dir_to(QString new_parent_dir, QString new_name);
  void mv_file_to(QString new_parent_dir, QString new_name);
  void change_auth_mod(FileAuth auth);
  //  void close();
  FSFileAttr get_attr();
  FSFileAttr _get_attr(index_t inode_id);
  //  offset_t get_file_size();
  /////

  FSFile(FSBase* p_fsbase, FSDirRecord record, FSFile::OpenMode open_mode,
         bool get_abs_path_info = false);

  //由于add和write的工作几乎一样，但前提不同。所以这里把主要部分提出来
  void _write(char* data, int64 offset_in_file, int64 max_size);

  static bool _check_path_str(QString path_str);
  static bool _check_name_str(QString name_str);

  // bi表示在文件级别下从文件开头向后第几个块，返回值表示文件系统级别下block的实际索引
  //下面两个函数不能用混了！！！！
  index_t locate_bi(FSInode* inode, index_t bi, FSIRoute& r);
  index_t locate_bi(FSInode* inode, index_t bi);
  index_t locate_bi(index_t bi);

  QList<index_t> get_block_id_list();

  int parse_path_layer(QString path);
  //没找到返回false
  bool parse_path(QString path, FSInode& return_inode);
  //返回file对应inode索引，没找到返回-1(即fs::ILLEGLE)；
  index_t find_file_in_dir(FSInode* dir_inode, QString file_name);
  index_t find_file_in_dir_block(index_t block_id, QString file_name,
                                 index_t illegel_id);
  index_t find_file_in_dir(FSInode* dir_inode, QString file_name,
                           FSIRoute& return_route, index_t& return_bi,
                           index_t& return_block_id);
  index_t find_file_in_dir_block(index_t block_id, QString file_name,
                                 int& pos_in_block, index_t illegel_id);
  index_t get_parent_inode_id(index_t cur_inode_id);

  void file_rm(bool rm_parent_record = true);
  void dir_rm(bool rm_parent_record = true);

  void _dir_add_sub_dir(FSInode* dir_inode, index_t inode_id,
                        QString sub_dir_name, int8 sub_tree_layer);
  void _dir_remove_sub_dir(FSInode* dir_inode, QString sub_dir_name);
  int8 _dir_calcu_tree_layer(FSInode* parent_dir_inode,
                             index_t abandon_tree_inode_id = fs::ILLEGAL);

  void _dir_recurrently_change_dir_layer(FSInode* dir_inode,
                                         int8 former_base_dir_layer,
                                         int8 new_base_dir_layer);

  // file_type:directory
  void _dir_add_record(FSInode* dir_inode, index_t inode_id, QString file_name);
  //找到返回所删除记录的inode_id，否则返回fs::ILLEGAL
  index_t _dir_remove_record(FSInode* dir_inode, QString file_name);
  //  bool _dir_reorganize_blocks(index_t dir_inode_id);
  // located_block_id是辅助变量，防止过多的重复locate
  bool _dir_remove_block(FSInode* dir_inode, index_t bi,
                         index_t located_block_id = fs::ILLEGAL);

  QStringList _dir_list(FSInode* dir_inode);
  QList<QPair<FSFileAttr, QString>> _dir_list_with_attr(FSInode* dir_inode);

  void _file_sync_di_to_i(bool dump = true);
  void _file_truncate_by_bi(index_t cut_start_bi);
  void _file_truncate_by_bi(FSInode* file_inode, index_t cut_start_bi);

  void remove_index_block_level_one(index_t index_block_id, index_t bi_cnt);
  void remove_index_block_level_two(index_t index_block_id, index_t bi_cnt);
  void remove_index_block_level_one(FSBlock& tmp_block, index_t index_block_id,
                                    index_t bi_cnt);  //辅助level_two删除
  void truncate_index_block_level_one(index_t index_block_id, index_t bi_cnt,
                                      index_t cut_start);

  index_t add_block(FSInode* inode, bool dump);
  void add_block_link(FSInode* inode, index_t block_id, FSIRoute& route,
                      bool add_block_num);
  index_t add_index_block(index_t first_block_id = fs::ILLEGAL);
};

//-----------------------2018/9/11/22:10
typedef struct FSFileAttr {
  char file_type;     //文件类型
  FileAuth auth;      //文件权限数
  index_t block_num;  //占用块数
  offset_t size;      //文件大小
  int64 create_time;  //创建时间
  int64 change_time;  //最近访问时间
  FSFileAttr operator=(FSFileAttr const other) {
    file_type = other.file_type;
    auth = other.auth;
    block_num = other.block_num;
    size = other.size;
    create_time = other.create_time;
    change_time = other.change_time;
    return *this;
  }
} FSFileAttr;

}  // namespace fs

#endif  // FS_FILE_H
