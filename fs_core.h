#ifndef FS_CORE_H
#define FS_CORE_H

#include "fs_base.h"
#include "fs_file.h"

//#include "app_console.h"

namespace fs {

//管理MINIFS，设计上最多挂载3个，使用单例模式
//但是实际上失败了。。。。。
class MiniFSGroup {
 public:
  ~MiniFSGroup();
  //获取单例
  //  static MiniFSGroup& GetInstance();
  // 好像不能放到源文件那里定义
  static MiniFSGroup& GetInstance() {
    //    static MiniFSGroup instance;
    return instance;
  }

  //挂载MINIFS
  //参数：
  //  outer_path: MINIFS 文件路径
  //  name: 给MINIFS命名（挂载时的临时名字）
  FSBase* mount(QString outer_path, QString name);
  //根据索引获取minifs的指针
  FSBase* get_fs(int fs_index);
  //根据名字获取MINIFS的指针
  FSBase* get_fs(QString name);
  //根据索引获取MINIFS名字
  QString get_name(int fs_index);
  //根据名字获取索引
  int get_fs_id(QString name);
  //获取索引和名字的键值对list
  QList<QPair<int, QString>> list_fs();
  //根据索引卸除文件系统
  void unmount(int fs_index);
  //根据名字卸除文件系统
  void unmount(QString name);
  //-----------------------------2018/9/11/22:07
  //获取当前已经挂载的minifs数量
  int get_current_pfs_num() { return cnt; }

 private:
  //  MiniFSGroup();
  //  MiniFSGroup(const MiniFSGroup&);
  int cnt = 0;
  static MiniFSGroup instance;
  FSBase* ptr_fs_array[3] = {nullptr, nullptr, nullptr};
  QString minifs_name_array[3] = {"", "", ""};
  QMap<QString, int> map_name_to_id;
};

//解析版本号（如 “13.5.6”）
QString parse_version(int32 version);
int32 parse_version(int v1, int v2, int v3);
int32 parse_version(QString version);

//创建文件系统
//参数：
//  outer_path: MINIFS 文件路径
//  fs_size: byte数量，暂时只能768MB-2GB
void create_minifs(QString outer_path, int64 fs_size);

//格式化文件系统
void format_minifs(QString outer_path);
void format_minifs(QFile* outer_file);

//改变指定文件系统的当前路径
//参数：
//  pfs: MINIFS（FSBase类型，这个通过MiniFSGroup的函数可以获取）
//  path: MINIFS内部路径
void change_current_dir(FSBase* pfs, QString path);
//获取指定MINIFS当前路径
QString get_current_dir(FSBase* pfs);

//指定pfs，将dir_path目录下的文件名列表返回
QStringList list_dir(FSBase* pfs, QString dir_path);

//------------------------------------------2018/9/11/21:30
//指定pfs，将dir_path目录下的文件名以及文件信息列表返回
//<QPair<FSFileAttr, QString>>两值分别为属性和文件名
QList<QPair<FSFileAttr, QString>> list_dir_with_attr(FSBase* pfs,
                                                     QString dir_path);

//指定pfs,按照相应路径创建文件夹
void create_dir(FSBase* pfs, QString path, bool last_force = false);
//指定pfs,按照相应路径创建文件
void create_file(FSBase* pfs, QString path);

//改变文件读写权限,详细见FileAuth类型
void change_auth_mod(FSBase* pfs, QString path, FileAuth auth);
void change_auth_mod(FSFile* file, FileAuth auth);

// 9/12
//获取文件夹层级（从root到此）,如果输入路径表示的不是文件夹，返回-1
//根目录层级为0,根目录下一级目录层级为1，以此类推
//普通文件不计层级
int8 get_dir_layer(FSBase* pfs, QString path);

// 9/12
//获取以此文件夹为根时目录树的层级,,如果输入路径表示的不是文件夹，返回-1
//普通文件不计层级
int8 get_tree_layer(FSBase* pfs, QString path);

// 9/12
//移动文件夹或文件
void move(FSBase* pfs, QString former_path, QString new_path);
void move(FSFile* file, QString new_path);

//获取文件属性
FSFileAttr get_file_attr(FSBase* pfs, QString path);
FSFileAttr get_file_attr(FSFile* file);

//打开文件
FSFile* open(FSBase* pfs, QString path, FSFile::OpenMode mode);
FSFile* open(FSBase* pfs, QString path);
//关闭文件
//(因为涉及到指针的更改，所以传入的时指针的引用)
void close(FSFile*& file);

//读操作（对文件夹无效）
//参数：
//  file: FSFile文件指针
//  data: 数据指针
//  offset_in_file: 在文件中的偏置
//  max_size: 数据字节数;
int64 read(FSFile* file, char* data, int64 offset_in_file, int64 max_size);

//写操作（内部实现为以block大小为单位写入）（对文件夹无效）
//参数：
//  file: FSFile文件指针
//  data: 数据指针
//  offset_in_file: 在文件中的偏置
//  max_size: 数据字节数;
void write(FSFile* file, char* data, int64 offset_in_file, int64 max_size);

//添加操作，在文件尾部添加内容（对文件夹无效）
//参数：
//  file: FSFile文件指针
//  data: 数据指针
//  max_size: 数据字节数;
void add_content(FSFile* file, char* data, int64 max_size);

//清空文件（对文件夹无效）
void clear(FSFile* file);

//删除文件
void remove(FSBase* pfs, QString path);
void remove(FSFile* file);

//
//
//似乎还要获取剩余空闲块的功能
//
//
//

//// 2018/9/13
index_t get_rest_inode_num(FSBase* pfs);
index_t get_rest_block_num(FSBase* pfs);
QList<index_t> get_block_id_list(FSBase* pfs, QString path);
// QList<index_t> get_block_id_list(FSBase* pfs, QString path);
// void optimize_dir(FSBase* pfs, QString path);

}  // namespace fs

#endif  // FS_CORE_H
