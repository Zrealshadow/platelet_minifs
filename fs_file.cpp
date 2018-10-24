#include "fs_file.h"

//改变inode的change_time的操作：
//文件:读写，改变文件权限（暂时没做）
//文件夹：增加记录，删除记录，opt

namespace fs {
//  public:
// public constructor
FSFile::FSFile(FSBase* p_fsbase, QString path, FSFile::OpenMode open_mode) {
  if (path.isEmpty()) throw QString("FSFile: path cannot be empty");
  bool flag = false;
  this->pfs = p_fsbase;
  flag = parse_path(path, this->inode);
  if (false == flag) {
    //    FSInode inode;
    //    this->pfs->read_inode(inode, this->pfs->cur_inode_id);
    //    QStringList list = _dir_list(&inode);
    //    qDebug() << list;
    throw QString("FSFile: cannot find the file");
  }
  if (open_mode == FSFile::OpenMode::RW or open_mode == FSFile::OpenMode::R) {
    if (not this->inode.auth.can_read()) {
      throw "FSFile: " + QString("该文件不是可读文件");
    }
  }
  if (open_mode == FSFile::OpenMode::RW or open_mode == FSFile::OpenMode::R or
      open_mode == FSFile::OpenMode::A) {
    if (not this->inode.auth.can_write()) {
      throw "FSFile: " + QString("该文件不是可写文件");
    }
  }
  FSBlock block;
  this->pfs->read_block(block, this->inode.direct_ptr[0]);
  //  this->type = this->inode.file_type;
  this->name = block.dir_block.records[0].file_name;
  this->i_route_reversed.append(this->inode.inode_id);
  // parent_inode_id
  index_t p_id = this->inode.inode_id;
  while ((p_id = get_parent_inode_id(p_id)) != fs::ILLEGAL) {
    this->i_route_reversed.append(p_id);
  }

  this->open_mode = open_mode;
  if (this->inode.file_type == 'f') {
    FSBlock block;
    this->pfs->read_block(block, this->inode.direct_ptr[0]);
    index_t data_inode_id = block.dir_block.records[0].inode_id;  //
    this->pfs->read_inode(this->data_inode, data_inode_id);
  }
  this->deleted = false;
}

FSFile::FSFile(FSBase* p_fsbase, QString path)
    : FSFile(p_fsbase, path, OpenMode::RW) {}

FSFile::FSFile(FSBase* p_fsbase) {
  this->pfs = p_fsbase;
  this->pfs->read_inode(this->inode, p_fsbase->get_cur_inode_id());
  if (this->inode.file_type != 'd') {
    throw "FSFile: " + QString("严重错误: 当前目录不是文件夹类型");
  }
  //  this->name = QString(".");
  FSBlock block;
  this->pfs->read_block(block, this->inode.direct_ptr[0]);
  this->name = block.dir_block.records[0].file_name;
}

// private constructor
FSFile::FSFile(FSBase* p_fsbase, FSDirRecord record, FSFile::OpenMode open_mode,
               bool get_abs_path_info) {
  index_t inode_id = record.inode_id;
  this->pfs = p_fsbase;
  this->pfs->read_inode(this->inode, inode_id);
  this->name = QString(record.file_name);
  this->i_route_reversed.append(this->inode.inode_id);
  // parent_inode_id
  if (get_abs_path_info) {
    index_t p_id = this->inode.inode_id;
    while ((p_id = get_parent_inode_id(p_id)) != fs::ILLEGAL) {
      this->i_route_reversed.append(p_id);
    }
  }
  this->open_mode = open_mode;
  if (this->inode.file_type == 'f') {
    FSBlock block;
    this->pfs->read_block(block, this->inode.direct_ptr[0]);
    index_t data_inode_id = block.dir_block.records[0].inode_id;
    this->pfs->read_inode(this->data_inode, data_inode_id);
  }
  this->deleted = false;
}

FSFile::~FSFile() {}

offset_t FSFile::read(char* data, offset_t offset_in_file, offset_t max_size) {
  if (this->deleted) throw QString("文件已删除");
  if (this->inode.file_type != 'f') {
    throw "FSFile::read: " + QString("不是文件");
  }
  if (this->open_mode != OpenMode::R and this->open_mode != OpenMode::RW) {
    throw "FSFile::read: " + QString("无权读");
  }
  if (offset_in_file > this->inode.file_size) {
    throw "FSFile::read: " + QString("超过文件范围");
  }
  if (offset_in_file == this->inode.file_size) {
    //(debug)相等时返回0；
    return 0;
  }
  char* p = data;
  index_t bi_off = offset_in_file / fs::block_size;
  index_t bi = bi_off;
  offset_t off_in_block = offset_in_file % fs::block_size;
  //  FSBlock block;
  offset_t cur_size = 0;
  offset_t rest_size = this->inode.file_size - offset_in_file;
  //  offset_t tmp_size;
  //(debug) offset_t(int64;  long lng)放这似乎会有bug
  // "stack smashing detected"---------数组越界

  // 可能时memcpy那里类型转换有问题
  //  unsigned long tmp_size;
  // size似乎有问题
  // memcpy支持long long吗？？？？

  offset_t tmp_size;

  for (; cur_size < max_size and rest_size > 0;) {
    //没检查fs::ILLEGAL
    index_t block_id = locate_bi(bi);  //因为是文件，实际上查的时data_inode

    offset_t cur_max_size = max_size - cur_size;
    tmp_size = MIN(cur_max_size, fs::block_size - off_in_block);
    if (rest_size < tmp_size) tmp_size = rest_size;

    //    this->pfs->read_block(block, block_id);
    //    memcpy(p, &block.data_block.data_byte[off_in_block], tmp_size);

    //快速读++++++++++++++++++++++++++++++++
    this->pfs->read_block_fast(p, off_in_block, tmp_size, block_id);

    cur_size += tmp_size;
    rest_size -= tmp_size;
    off_in_block = 0;  // block开头
    p += tmp_size;
    bi++;
  }
  return cur_size;  //
}

void FSFile::write(char* data, offset_t offset_in_file, offset_t max_size) {
  if (this->deleted) throw QString("文件已删除");
  if (this->inode.file_type != 'f') {
    throw "FSFile::write: " + QString("不是文件");
  }
  if (this->open_mode != OpenMode::W and this->open_mode != OpenMode::RW) {
    throw "FSFile::write: " + QString("无权写");
  }
  //  if (offset_in_file >= this->inode.file_size) {
  //    throw "FSFile::write: " + QString("超过文件范围");
  //  }
  if (offset_in_file > this->inode.file_size) {
    throw "FSFile::write: " +
        QString("超过文件范围，写的开始位置只能是文件内部以及文件末尾");
  }
  if (offset_in_file + max_size > fs::max_file_size) {
    throw "FSFile::write: " +
        QString("文件大小不能超过 %1 MB").arg(fs::max_file_size / fs::ONE_MB);
  }
  _write(data, offset_in_file, max_size);
}

void FSFile::add_content(char* data, int64 max_size) {
  if (this->deleted) throw QString("文件已删除");
  if (this->inode.file_type != 'f') {
    throw "FSFile::add_content: " + QString("不是文件");
  }
  if (this->open_mode != OpenMode::A and this->open_mode != OpenMode::W and
      this->open_mode != OpenMode::RW) {
    throw "FSFile::add_content: " + QString("无权添加");
  }
  if (this->inode.file_size + max_size > fs::max_file_size) {
    throw "FSFile::write: " +
        QString("文件大小不能超过 %1 MB").arg(fs::max_file_size / fs::ONE_MB);
  }
  _write(data, this->inode.file_size, max_size);
}

// private: write主体部分
// write定位有重大BUG 9/11
void FSFile::_write(char* data, offset_t offset_in_file, offset_t max_size) {
  char* p = data;
  index_t bi_off = offset_in_file / fs::block_size;
  index_t bi = bi_off;
  offset_t off_in_block = offset_in_file % fs::block_size;

  FSBlock last_keep_block;
  index_t last_keep_block_id = fs::ILLEGAL;

  //  offset_t file_former_size = this->inode.file_size;
  if (offset_in_file == this->inode.file_size and
      bi_off == this->inode.block_num) {
    ;  //满足此条件说明原本文件的内容就占用整数个block
       //要在这样的文件末尾加内容是不用truncate的
  } else {
    //没写inode
    last_keep_block_id = locate_bi(bi);
    this->pfs->read_block(last_keep_block, last_keep_block_id);
    _file_truncate_by_bi(bi);
  }

  //  //判断文件末尾方式有重大bug，问题在locate_bi找的inode如果未分配，返回的是随机值
  //  index_t block_id = locate_bi(bi);
  //  //没找到说明offset_in_file是文件末尾（靠调用_write的函数限制）

  //  if (block_id != fs::ILLEGAL) {
  //    //    this->pfs->read_block(block, block_id);
  //    if (offset_in_file == this->inode.file_size and
  //        bi_off == this->inode.block_num) {
  //      ;  //满足次条件说明原本文件的内容就占用整数个block
  //         //要在这样的文件末尾加内容是不用truncate的
  //    } else {
  //      //没写inode
  //      _file_truncate_by_bi(bi);
  //    }
  //  }

  offset_t cur_size = 0;
  offset_t tmp_size;
  //
  //  unsigned long tmp_size;
  for (; cur_size < max_size;) {
    index_t new_block_id = add_block(&this->data_inode, false);
    if (cur_size == 0) {
      if (last_keep_block_id != fs::ILLEGAL) {
        //截取后再填，这样只是省力气而已，效率底下
        this->pfs->write_block(last_keep_block, last_keep_block_id);
      }
    }

    offset_t cur_max_size = max_size - cur_size;
    tmp_size = MIN(cur_max_size, fs::block_size - off_in_block);

    //(optimize)此处memcpy是非常耗时间的，我换成快速写后，足足提升了5倍速度
    //    memcpy(&block.data_block.data_byte[off_in_block], p, tmp_size);
    //    this->pfs->write_block(block, new_block_id);

    //快速写++++++++++++++++++++++++++++++++
    this->pfs->write_block_fast(p, off_in_block, tmp_size, new_block_id);

    this->data_inode.file_size += tmp_size + off_in_block;
    //    tmp_file_size += tmp_size;

    cur_size += tmp_size;
    off_in_block = 0;  // block开头
    p += tmp_size;
    bi++;
  }
  //  this->data_inode.file_size += tmp_file_size;
  //写inode
  _file_sync_di_to_i(true);
}

void FSFile::clear() {
  if (this->deleted) throw QString("文件已删除");
  if (this->inode.file_type != 'f') {
    throw "FSFile::write: " + QString("不是文件");
  }
  if (this->open_mode != OpenMode::W and this->open_mode != OpenMode::RW) {
    throw "FSFile::write: " + QString("无权清空");
  }
  _file_truncate_by_bi(0);  //已经同步且写入外存
}

void FSFile::remove() {
  if (this->deleted) throw QString("文件已删除");
  if (this->inode.inode_id == fs::root_id) {
    throw "FSFile::remove: " + QString("试图删除根目录");
  }
  switch (this->inode.file_type) {
    case 'f':
      file_rm(true);
      break;
    case 'd':
      dir_rm(true);
      break;
    default:
      throw "FSFile::remove: " + QString("未知的文件类型");
  }
  this->deleted = true;
}

void FSFile::mv_dir_to(QString new_parent_dir, QString new_name) {
  if (this->deleted) throw QString("已删除");
  /////
  if (this->inode.file_type != 'd')
    throw "FSFile::mv_dir_to: " + QString("不是文件夹类型");
  /////
  if (new_name == "." or new_name == "..") {
    throw "FSFile::mv_dir_to: " + QString("新文件名不能为'.'或'..'");
  }
  ////
  if (this->inode.inode_id == fs::root_id)
    throw "FSFile::mv_dir_to: " + QString("不能移动根目录");
  if (this->pfs->check_in_cur_route(this->inode.inode_id))
    throw "FSFile::mv_dir_to: " + QString("不能移动当前路径中的任何目录");
  if (not check_dir_name(new_parent_dir))
    throw "FSFile::mv_dir_to: check_dir_name: " +
        QString("invalid parent directory name");
  if (not check_file_or_folder_name(new_name)) {
    throw "FSFile::mv_dir_to: check_file_or_folder_name: " +
        QString("invalid new name");
  }
  FSInode new_parent_i, parent_i;
  if (not parse_path(new_parent_dir, new_parent_i))
    throw "FSFile::mv_dir_to: " + QString("找不到目标目录");
  if (new_parent_i.inode_id == this->inode.inode_id)
    throw "FSFile::mv_dir_to: " +
        QString("不可移动到自身");  //这个要改，区分移动目录和移动文件

  if (new_parent_i.dir_layer + this->inode.tree_layer + 1 > fs::max_dir_layer) {
    throw "FSFile::mv_dir_to: " + QString("超过目录层级限制");
  }
  if (find_file_in_dir(&new_parent_i, new_name) != fs::ILLEGAL)
    throw "FSFile::mv_dir_to: " + QString("新目录下 ") + new_name +
        QString(" 已经存在，不可移动");

  FSBlock fb;
  {  // debug
    this->pfs->read_block(fb, this->inode.direct_ptr[0]);
  }

  //子目录层级也要
  //  this->pfs->read_inode(parent_i, this->i_route_reversed[1]);
  _dir_recurrently_change_dir_layer(&this->inode, parent_i.dir_layer,
                                    new_parent_i.dir_layer);
  this->pfs->read_inode(parent_i, this->i_route_reversed[1]);  //读parent_inode
  //先加再删
  _dir_add_sub_dir(&new_parent_i, this->inode.inode_id, new_name,
                   this->inode.tree_layer);    // tree_layer层级变更
  _dir_remove_sub_dir(&parent_i, this->name);  // tree_layer层级变更

  {  // debug
    this->pfs->read_block(fb, this->inode.direct_ptr[0]);
  }

  this->name = new_name;

  FSBlock block;
  //更改原dir_block中两条特殊记录
  this->pfs->read_block(block, this->inode.direct_ptr[0]);
  // 9/13
  //(debug)文件才是设置data_inode的inode_id，这里要inode的inode_id
  block.dir_block.records[0].set(this->inode.inode_id,
                                 new_name.toUtf8().data());

  block.dir_block.records[1].set(new_parent_i.inode_id, "..");

  this->pfs->write_block(block, this->inode.direct_ptr[0]);
  //  this->pfs->write_inode()//写过了?????

  this->pfs->write_inode(this->inode, this->inode.inode_id);

  //修改绝对路径信息
  this->i_route_reversed.clear();
  index_t p_id = this->inode.inode_id;
  while ((p_id = get_parent_inode_id(p_id)) != fs::ILLEGAL) {
    this->i_route_reversed.append(p_id);
  }
}

void FSFile::mv_file_to(QString new_parent_dir, QString new_name) {
  if (this->deleted) throw QString("已删除");
  /////
  if (this->inode.file_type != 'f')
    throw "FSFile::mv_file_to: " + QString("不是普通文件类型");
  /////
  if (new_name == "." or new_name == "..") {
    throw "FSFile::mv_file_to: " + QString("新文件名不能为'.'或'..'");
  }
  ////
  //  if (this->inode.inode_id == fs::root_id)
  //    throw "FSFile::mv_file_to: " + QString("不能移动根目录");
  //  //  if (this->inode.inode_id == this->pfs->get_cur_inode_id())
  //  //    throw "FSFile::mv_to: " + QString("不能移动当前目录");
  //  if (this->pfs->check_in_cur_route(this->inode.inode_id))
  //    throw "FSFile::mv_file_to: " + QString("不能移动当前路径中的任何目录");
  if (not check_dir_name(new_parent_dir))
    throw "FSFile::mv_file_to: check_dir_name: " +
        QString("invalid parent directory name");
  if (not check_file_or_folder_name(new_name)) {
    throw "FSFile::mv_file_to: check_file_or_folder_name: " +
        QString("invalid new name");
  }
  FSInode new_parent_i, parent_i;
  if (not parse_path(new_parent_dir, new_parent_i))
    throw "FSFile::mv_file_to: " + QString("找不到目标目录");
  if (new_parent_i.inode_id == this->inode.inode_id)
    throw "FSFile::mv_file_to: " +
        QString("不可移动到自身");  //这个要改，区分移动目录和移动文件
  //文件不增加层级
  //  if (new_parent_i.dir_layer + 1 > fs::max_dir_layer)
  //    throw "FSFile::mv_file_to: " +
  //        QString("超过目录层级限制");
  if (find_file_in_dir(&new_parent_i, new_name) != fs::ILLEGAL)
    throw "FSFile::mv_file_to: " + QString("新目录下 ") + new_name +
        QString(" 已经存在，不可移动");

  //文件不增加层级
  this->inode.dir_layer =
      new_parent_i.dir_layer + 1;  //没必要，但是也行,统一一下
  this->pfs->read_inode(parent_i, this->i_route_reversed[1]);
  //先加再删
  _dir_add_record(&new_parent_i, this->inode.inode_id, new_name);
  _dir_remove_record(&parent_i, this->name);
  this->name = new_name;

  FSBlock block;
  //更改原dir_block中两条特殊记录
  this->pfs->read_block(block, this->inode.direct_ptr[0]);
  //(debug)要设置data_inode的inode_id,但之前误输入inode的inode_id
  block.dir_block.records[0].set(this->data_inode.inode_id,
                                 new_name.toUtf8().data());

  block.dir_block.records[1].set(new_parent_i.inode_id, "..");

  this->pfs->write_block(block, this->inode.direct_ptr[0]);
  //  this->pfs->write_inode()//写过了?????

  this->pfs->write_inode(this->inode, this->inode.inode_id);

  //修改绝对路径信息
  this->i_route_reversed.clear();
  index_t p_id = this->inode.inode_id;
  while ((p_id = get_parent_inode_id(p_id)) != fs::ILLEGAL) {
    this->i_route_reversed.append(p_id);
  }
}

void FSFile::change_auth_mod(FileAuth auth) {
  this->inode.auth = auth;
  this->pfs->write_inode(this->inode, this->inode.inode_id);
}

FSFileAttr FSFile::get_attr() {
  if (this->deleted) throw QString("文件已删除");
  FSFileAttr a;
  a.file_type = this->inode.file_type;
  a.auth = this->inode.auth;
  a.block_num = this->inode.block_num;
  a.size = this->inode.file_size;
  a.create_time = this->inode.create_time;
  a.change_time = this->inode.change_time;
  return a;
}

FSFileAttr FSFile::_get_attr(index_t inode_id) {
  FSInode inode;
  this->pfs->read_inode(inode, inode_id);
  FSFileAttr a;
  a.file_type = inode.file_type;
  a.auth = inode.auth;
  a.block_num = inode.block_num;
  a.size = inode.file_size;
  a.create_time = inode.create_time;
  a.change_time = inode.change_time;
  return a;
}

bool FSFile::check_path_name(QString path_name) {
  if (not _check_path_str(path_name)) return false;
  QStringList list = path_name.split("/");
  int start = 0, end = list.length();
  if (list[0] == "") start++;
  for (int i = start; i < end - 1; i++) {
    if (not check_folder_name(list[i])) return false;
  }
  if (list[end - 1] == "" or check_file_or_folder_name(list[end - 1])) {
    return true;
  }
  return false;
}

bool FSFile::check_dir_name(QString dir_name) {
  if (not _check_path_str(dir_name)) return false;
  QStringList list = dir_name.split("/");
  int start = 0, end = list.length();
  if (list[0] == "") start++;
  for (int i = start; i < end - 1; i++) {
    if (not check_folder_name(list[i])) return false;
  }
  if (list[end - 1] == "" or check_folder_name(list[end - 1])) {
    return true;
  }
  return false;
}

bool FSFile::check_file_name(QString file_name) {
  if (file_name == "." or file_name == "..") return false;
  //必须有后缀名
  if (not check_file_or_folder_name(file_name)) return false;
  QStringList list = file_name.split(".");
  if (list.length() != 2) return false;
  int l1 = list[0].length(), l2 = list[1].length();
  if (l1 <= 0 or l1 > 8 or l2 <= 0 or l2 > 3) return false;
  return true;
}

bool FSFile::check_folder_name(QString folder_name) {
  if (folder_name == "." or folder_name == "..") return true;
  if (not check_file_or_folder_name(folder_name)) return false;
  for (int i = folder_name.length() - 1; i >= 0; i--)
    if (folder_name.at(i) == '.') return false;
  return true;
}

bool FSFile::check_file_or_folder_name(QString name) {
  //
  return _check_name_str(name);
}

bool FSFile::_check_path_str(QString path_str) {
  int length = path_str.length();
  QChar cur = ' ';
  for (int i = 0; i < length; i++) {
    cur = path_str.at(i);
    if (cur == '/') {
      if (i > 0 and path_str.at(i - 1) == '/') return false;
    } else if (cur == '.') {
      continue;
    } else {
      if (not cur.isLetterOrNumber()) return false;
    }
  }
  return true;
}

bool FSFile::_check_name_str(QString name_str) {
  int length_8bit = name_str.toUtf8().length();
  if (length_8bit > fs::FILE_NAME_MAX_SIZE)
    return false;
  else if (name_str.length() != length_8bit)
    return false;  //不等说明不全是单字节字符
  else
    for (int i = 0; i < length_8bit; i++)
      if (not(name_str[i].isLetterOrNumber() or name_str[i] == '.'))
        return false;
  return true;
}

//  private:
index_t FSFile::locate_bi(index_t bi) {
  FSIRoute r;
  switch (this->inode.file_type) {
    case 'f':
      return locate_bi(&this->data_inode, bi, r);
    default:
      return locate_bi(&this->inode, bi, r);
  }
}
index_t FSFile::locate_bi(FSInode* inode, index_t bi) {
  FSIRoute r;
  return locate_bi(inode, bi, r);
}

index_t FSFile::locate_bi(FSInode* inode, index_t bi_in_file, FSIRoute& r) {
  //  index_t bi = f_offset / fs::block_size,
  //          ofs_in_block = f_offset % fs::block_size;
  r = inode->locate_block(bi_in_file);
  int level = r.level;
  index_t block_id = inode->direct_ptr[r.route[0]];
  FSBlock block;
  for (int i = 1; i < level; i++) {
    this->pfs->read_block(block, block_id);
    block_id = block.index_block.block_id[r.route[i]];
  }
  return block_id;
}

QList<index_t> FSFile::get_block_id_list() {
  index_t bn = this->inode.block_num;  //??
  index_t block_id;
  QList<index_t> list;
  for (index_t bi = 0; bi < bn; bi++) {
    block_id = locate_bi(bi);
    list.append(block_id);
  }
  return list;
}

int FSFile::parse_path_layer(QString path) {
  if (not check_path_name(path))
    throw "FSFile::parse_path:check_path_name: " + QString("illegel path name");
  QStringList plist = path.split("/");
  //当前目录
  index_t fs_cur_i = this->pfs->get_cur_inode_id();
  int layer = 0;
  int start = 0, end = plist.length();
  if (plist[0] == "") {
    start = 1;
    layer = 0;
  } else {
    layer = this->inode.dir_layer;
  }
  if (plist[end - 1] == "") {
    end = end - 1;
  }
  {
    for (int i = start; i < end; i++) {
      if (plist[i] == ".")
        ;  // do nothing
      else if (plist[i] == "..")
        layer--;
      else
        layer++;
    }
  }
  return layer;
}

//文件路径解析中附带了目录层级检测
bool FSFile::parse_path(QString path, FSInode& return_inode) {
  if (not check_path_name(path))
    throw "FSFile::parse_path:check_path_name: " + QString("illegel path name");
  QStringList plist = path.split("/");
  //当前目录
  index_t fs_cur_i = this->pfs->get_cur_inode_id();
  int layer = 0;
  int start = 0, end = plist.length();

  uint8 flag_is_file = 0;
  if (check_file_name(plist[end - 1])) {
    flag_is_file = 1;
  }

  if (plist[0] == "") {
    this->pfs->read_inode(return_inode, 0);
    start = 1;
    layer = 0;
  } else {
    this->pfs->read_inode(return_inode, fs_cur_i);
    layer = return_inode.dir_layer;
  }
  if (plist[end - 1] == "") {
    end = end - 1;
  }
  {
    for (int i = start; i < end; i++) {
      if (plist[i] == ".")
        ;  // do nothing
      else if (plist[i] == "..")
        layer--;
      else
        layer++;
      if (layer < 0) throw "FSFile::parse_path: " + QString("illegel path");
    }
    //注意，文件不用管层级
    if (layer - flag_is_file > fs::max_dir_layer) {
      throw "FSFile::parse_path: " +
          QString("目录层数不能超过 %1").arg(fs::max_dir_layer);
    }
  }

  for (int i = start; i < end; i++) {
    if (plist[i] == ".") continue;
    //由于".."在目录块中(第二个记录)，所以不用额外找。
    //（找的顺序也是从前向后，所以很快找到第二个）
    index_t inode_id = find_file_in_dir(&return_inode, plist[i]);
    if (inode_id == fs::ILLEGAL) return false;
    this->pfs->read_inode(return_inode, inode_id);
  }
  return true;
}

index_t FSFile::find_file_in_dir(FSInode* dir_inode, QString file_name) {
  FSIRoute route_nothing;
  index_t bi_nothing;
  index_t block_id_nothing;
  return find_file_in_dir(dir_inode, file_name, route_nothing, bi_nothing,
                          block_id_nothing);
}

index_t FSFile::find_file_in_dir(FSInode* dir_inode, QString file_name,
                                 FSIRoute& return_route, index_t& return_bi,
                                 index_t& return_block_id) {
  //占用目录块的数量
  index_t bn = dir_inode->block_num;
  //记录的数量
  //  index_t rn = dir_inode->file_size;
  index_t ans_inode_id = fs::ILLEGAL;

  if (file_name == ".") {
    return_bi = 0;
    return_block_id = locate_bi(dir_inode, 0, return_route);
    return dir_inode->inode_id;
  }

  for (index_t bi = 0; bi < bn; bi++) {
    return_block_id = locate_bi(dir_inode, bi, return_route);
    return_bi = bi;
    //在具体块中寻找，但是设置屏蔽id为dir_inode->inode_id，防止父子重名
    ans_inode_id =
        find_file_in_dir_block(return_block_id, file_name, dir_inode->inode_id);
    if (ans_inode_id != fs::ILLEGAL) return ans_inode_id;
  }
  return ans_inode_id;
}

index_t FSFile::find_file_in_dir_block(index_t block_id, QString file_name,
                                       index_t illegal_id) {
  int tmp;
  return find_file_in_dir_block(block_id, file_name, tmp, illegal_id);
}

index_t FSFile::find_file_in_dir_block(index_t block_id, QString file_name,
                                       int& pos_in_block, index_t illegal_id) {
  // illegal_id：如果子文件名和当前文件名重了，如果按照原来的设计，这样时无法定位子文件的
  //所幸的时可以通过比较inode_id来判断是子文件还是当前目录
  FSBlock block;  //循环里这样做太慢
  this->pfs->read_block(block, block_id);
  int end = block.dir_block.used;
  for (int i = 0; i < end; i++) {
    if (block.dir_block.records[i].file_name == file_name) {
      //排除当前目录和子文件重名的情况
      if (block.dir_block.records[i].inode_id == illegal_id) continue;
      pos_in_block = i;
      return block.dir_block.records[i].inode_id;
    }
  }
  return fs::ILLEGAL;
}

index_t FSFile::get_parent_inode_id(index_t cur_inode_id) {
  FSInode inode;
  this->pfs->read_inode(inode, cur_inode_id);
  index_t block_id = inode.direct_ptr[0];
  FSBlock block;
  this->pfs->read_block(block, block_id);
  return block.dir_block.records[1].inode_id;  //对于根，返回的应该为ILLEGAL
}

//有问题
void FSFile::file_rm(bool rm_parent_record) {
  _file_truncate_by_bi(0);
  if (rm_parent_record) {
    FSInode parent_i;
    this->pfs->read_inode(parent_i, this->i_route_reversed[1]);
    _dir_remove_record(&parent_i, this->name);
  }
  this->pfs->remove_inode(this->data_inode.inode_id, true);
  this->pfs->remove_block(this->inode.direct_ptr[0], true);
  this->pfs->remove_block(this->inode.inode_id, true);
}

//有问题
void FSFile::dir_rm(bool rm_parent_record) {
  // dir是只占用到一级间接索引,所以之后只要清空至多一级间接索引
  //
  if (rm_parent_record) {
    FSInode parent_i;
    this->pfs->read_inode(parent_i, this->i_route_reversed[1]);
    //因为涉及到层级，不能简单地删除记录
    //    _dir_remove_record(&parent_i, this->name);
    _dir_remove_sub_dir(&parent_i, this->name);
  }
  // block_num一定要对，之后要检查  debug
  FSBlock block, fblk;  // fblk:first_level_indirect_index__block
  index_t fblk_id = fs::ILLEGAL;
  if (this->inode.block_num > fs::inode_direc_n) {
    fblk_id = this->inode.first_level_ptr;
    this->pfs->read_block(fblk, fblk_id);
  }
  //注意，第一个dir_block删除要小心，因为前两条记录为特殊记录
  //所以要特判
  for (int bi = this->inode.block_num - 1; bi >= 0; bi--) {
    index_t b_id;
    if (bi < fs::inode_direc_n) {
      b_id = this->inode.direct_ptr[bi];
      this->pfs->read_block(block, b_id);
    } else {
      b_id = fblk.index_block.block_id[bi - fs::inode_direc_n];
      this->pfs->read_block(block, b_id);
    }
    //第一个dir_block删除要小心，因为前两条记录为特殊记录
    int cur_first_norml_r = (bi == 0) ? 2 : 0;
    for (int ri = block.dir_block.used - 1; ri >= cur_first_norml_r; ri--) {
      FSFile fs_file(this->pfs, block.dir_block.records[ri],
                     FSFile::OpenMode::W);
      //      fs_file.file_rm();
      if (fs_file.inode.file_type == 'f')
        fs_file.file_rm(false);
      else if (fs_file.inode.file_type == 'd')
        fs_file.dir_rm(false);
      else
        throw "FSFile::dir_rm: " + QString("未知文件类型");
      block.dir_block.used--;
    }
    this->pfs->remove_block(b_id, true);
  }

  if (fblk_id != fs::ILLEGAL) {
    this->pfs->remove_block(fblk_id, true);
  }
  this->pfs->remove_inode(this->inode.inode_id, true);
}

// 9/12
void FSFile::_dir_add_sub_dir(FSInode* dir_inode, index_t inode_id,
                              QString sub_dir_name, int8 sub_tree_layer) {
  //  FSInode inode;
  //  this->pfs->read_inode(inode, inode_id);
  //  if (dir_inode->dir_layer + inode.tree_layer > fs::max_dir_layer) {
  //    throw "FSFile::_dir_add_sub_dir: " + QString("不能超过目录层级");
  //  }
  //  _dir_add_record(dir_inode, inode_id, sub_dir_name);
  //  int8 tmp_tree_layer = inode.tree_layer + 1;

  if (dir_inode->dir_layer + sub_tree_layer + 1 > fs::max_dir_layer) {
    throw "FSFile::_dir_add_sub_dir: " + QString("不能超过目录层级");
  }
  _dir_add_record(dir_inode, inode_id, sub_dir_name);
  int8 tmp_tree_layer = sub_tree_layer + 1;

  if (dir_inode->tree_layer >= tmp_tree_layer) {
    return;  //不用改变
  } else {
    dir_inode->tree_layer = tmp_tree_layer;
    this->pfs->write_inode(*dir_inode, dir_inode->inode_id);  //写inode
    tmp_tree_layer++;

    index_t p_id = dir_inode->inode_id;
    FSInode p_inode;  // parent_inode
    while ((p_id = get_parent_inode_id(p_id)) != fs::ILLEGAL) {
      this->pfs->read_inode(p_inode, p_id);
      if (p_inode.tree_layer >= tmp_tree_layer) {
        return;  //不用改变
      } else {
        p_inode.tree_layer = tmp_tree_layer;
        this->pfs->write_inode(p_inode, p_id);  //写inode
        tmp_tree_layer++;
      }
    }
  }
}

// 9/12
void FSFile::_dir_remove_sub_dir(FSInode* dir_inode, QString sub_dir_name) {
  int abandon_tree_inode_id = _dir_remove_record(dir_inode, sub_dir_name);
  //计算移除后，父文件夹的tree_layer，如果不变则不用动，变了，则要向上检查
  int8 tmp_tree_layer = _dir_calcu_tree_layer(dir_inode, abandon_tree_inode_id);
  if (dir_inode->tree_layer <= tmp_tree_layer) {
    //这里其实用==就行，用<=是以防万一,因为remove后，层级只会减少或不变
    return;  //不用改变
  }
  dir_inode->tree_layer = tmp_tree_layer;
  this->pfs->write_inode(*dir_inode, dir_inode->inode_id);
  /////
  index_t p_id = dir_inode->inode_id;
  FSInode p_inode;  // parent_inode
  while ((p_id = get_parent_inode_id(p_id)) != fs::ILLEGAL) {
    this->pfs->read_inode(p_inode, p_id);
    //检查新的tree_layer
    tmp_tree_layer = _dir_calcu_tree_layer(&p_inode, fs::ILLEGAL);
    if (p_inode.tree_layer <= tmp_tree_layer) {
      return;  //不用改变
    } else {
      p_inode.tree_layer = tmp_tree_layer;
      this->pfs->write_inode(p_inode, p_id);  //写inode
    }
  }
}

// 9/12
int8 FSFile::_dir_calcu_tree_layer(FSInode* parent_dir_inode,
                                   index_t abandon_tree_inode_id) {
  if (parent_dir_inode->file_type != 'd') {
    throw "FSFile::_dir_get_max_sub_tree_layer: " +
        QString("不是文件夹，层级不可计算`");
  }
  FSBlock block;
  FSInode inode;
  index_t block_id;
  int8 max_sub_tree_layer = -1;  //对没有子文件夹的或屏蔽唯一子文件夹的返回-1
  int start, end, r_inode_id;
  for (int bi = 0; bi < parent_dir_inode->block_num; bi++) {
    block_id = locate_bi(parent_dir_inode, bi);
    this->pfs->read_block(block, block_id);
    start = (bi == 0) ? 2 : 0;
    end = block.dir_block.used;
    for (int i = start; i < end; i++) {
      r_inode_id = block.dir_block.records[i].inode_id;
      if (r_inode_id == abandon_tree_inode_id)
        continue;  //屏蔽一个子文件夹，用于remove_dir
      this->pfs->read_inode(inode, r_inode_id);
      if (inode.file_type == 'd') {
        max_sub_tree_layer = std::max(max_sub_tree_layer, inode.tree_layer);
      }
    }
  }
  return max_sub_tree_layer + 1;
}

void FSFile::_dir_recurrently_change_dir_layer(FSInode* dir_inode,
                                               int8 former_base_dir_layer,
                                               int8 new_base_dir_layer) {
  dir_inode->dir_layer += new_base_dir_layer - former_base_dir_layer;
  this->pfs->write_inode(*dir_inode, dir_inode->inode_id);  //写inode
  if (dir_inode->file_type != 'd') {
    return;
  } else {
    FSBlock block;
    FSInode inode;
    index_t block_id;
    int start, end, r_inode_id;
    for (int bi = 0; bi < dir_inode->block_num; bi++) {
      block_id = locate_bi(dir_inode, bi);
      this->pfs->read_block(block, block_id);
      start = (bi == 0) ? 2 : 0;
      end = block.dir_block.used;
      for (int i = start; i < end; i++) {
        r_inode_id = block.dir_block.records[i].inode_id;
        this->pfs->read_inode(inode, r_inode_id);
        _dir_recurrently_change_dir_layer(&inode, former_base_dir_layer + 1,
                                          new_base_dir_layer + 1);
      }
    }
  }
}

//写了
void FSFile::_dir_add_record(FSInode* dir_inode, index_t inode_id,
                             QString name) {
  if (name == "." or name == "..")
    throw "FSFile::_dir_add_record: " + QString("不能添加'.'或'..");
  if (fs::ILLEGAL != find_file_in_dir(dir_inode, name))
    throw "FSFile::add_record: " + QString("文件已存在");
  //占用目录块的数量
  index_t bn = dir_inode->block_num;
  FSBlock block;
  for (index_t bi = 0; bi < bn; bi++) {
    index_t block_id = locate_bi(dir_inode, bi);
    this->pfs->read_block(block, block_id);
    if (block.dir_block.used >= fs::rn_in_b)
      continue;
    else {
      block.dir_block.records[block.dir_block.used].set(inode_id,
                                                        name.toUtf8().data());
      block.dir_block.used++;
      this->pfs->write_block(block, block_id);
      //(debug ok)之前忘了写下当前inode
      dir_inode->file_size++;
      this->pfs->write_inode(*dir_inode, dir_inode->inode_id);
      return;
    }
  }
  //未测试
  //新增目录块
  if (bn + 1 >= fs::dir_max_block_n) {
    throw "FSFile::add_record: " +
        QString("文件夹最多容纳 %1 个子文件或子文件夹")
            .arg(fs::dir_max_block_n * rn_in_b - 2);
  }
  add_block(dir_inode, false);
  // dir_inode->block_num已经增加
  index_t block_id = locate_bi(dir_inode, dir_inode->block_num - 1);
  block.dir_block.records[0].set(inode_id, name.toUtf8().data());
  block.dir_block.used = 1;
  this->pfs->write_block(block, block_id);
  // block_num之前加了
  dir_inode->file_size++;
  this->pfs->write_inode(*dir_inode, dir_inode->inode_id);
  return;
}

//写了
//删除记录，整理所在dir_block（数组形式，删除一条后，后面的记录要整体前移）
//如果dir_block清空了，删除dir_block，整理inode
//由于这些性质，record是不能根据record索引直接定位的（对文件夹也没有这个需求）
index_t FSFile::_dir_remove_record(FSInode* dir_inode, QString file_name) {
  FSIRoute r;
  index_t located_block_id;
  index_t bi;
  //获取了r和block_id
  index_t find_inode_id =
      find_file_in_dir(dir_inode, file_name, r, bi, located_block_id);
  if (find_inode_id == fs::ILLEGAL) {
    return fs::ILLEGAL;
  }
  FSBlock block;
  this->pfs->read_block(block, located_block_id);
  int pos;  //按理说一定能找到这个pos，不用初始化
  for (int i = 0; i < block.dir_block.used; i++) {
    if (block.dir_block.records[i].inode_id == find_inode_id) {
      pos = i;
      break;
    }
  }
  for (int i = pos; i < block.dir_block.used - 1; i++) {
    block.dir_block.records[i] = block.dir_block.records[i + 1];
  }
  block.dir_block.used--;
  //  if (file_name == "a2649") {  // debug
  //    qDebug() << 111;
  //  }
  if (block.dir_block.used <= 0) {
    _dir_remove_block(dir_inode, bi, located_block_id);
  } else {
    this->pfs->write_block(block, located_block_id);
  }
  dir_inode->file_size--;
  this->pfs->write_inode(*dir_inode, dir_inode->inode_id);
  return find_inode_id;
  //  return true;
  //////////
}

bool FSFile::_dir_remove_block(FSInode* dir_inode, index_t bi,
                               index_t located_block_id) {
  //  if (dir_inode->file_type != 'd')
  //    throw "FSFile::_dir_remove_block: " + QString("不是文件夹");

  //先把目录块删除，再删索引块
  index_t block_id;
  if (located_block_id != fs::ILLEGAL) {
    block_id = located_block_id;
  } else {
    FSIRoute r;
    if (bi >= dir_inode->block_num || bi < 0)
      throw "FSFile::_dir_remove_block: " + QString("无效索引");
    block_id = locate_bi(dir_inode, bi, r);
  }
  this->pfs->remove_block(block_id, true);

  if (bi < fs::inode_direc_n) {
    if (dir_inode->block_num < fs::inode_direc_n) {
      for (int i = bi; i < dir_inode->block_num - 1; i++) {
        dir_inode->direct_ptr[i] = dir_inode->direct_ptr[i + 1];
      }
    } else {
      FSBlock block;
      this->pfs->read_block(block, dir_inode->first_level_ptr);
      for (int i = bi; i < fs::inode_direc_n - 1; i++) {
        dir_inode->direct_ptr[i] = dir_inode->direct_ptr[i + 1];
      }
      dir_inode->direct_ptr[fs::inode_direc_n - 1] =
          block.index_block.block_id[0];
      //减少到可以全部放到直接索引区
      if (dir_inode->block_num - 1 == fs::inode_direc_n) {
        this->pfs->remove_block(dir_inode->first_level_ptr, true);
        dir_inode->first_level_ptr = fs::ILLEGAL;
      } else {
        index_t end = dir_inode->block_num - 1 - fs::inode_direc_n;
        for (int i = 0; i < end; i++) {
          block.index_block.block_id[i] = block.index_block.block_id[i + 1];
        }
        this->pfs->write_block(block, dir_inode->first_level_ptr);
      }
    }
  } else {
    if (dir_inode->block_num - 1 == fs::inode_direc_n) {
      this->pfs->remove_block(dir_inode->first_level_ptr, true);
      dir_inode->first_level_ptr = fs::ILLEGAL;
    } else {
      FSBlock block;
      this->pfs->read_block(
          block, dir_inode->first_level_ptr);  //(debug)之前忘了这一句
      index_t end = dir_inode->block_num - 1 - fs::inode_direc_n;
      index_t start = bi - fs::inode_direc_n;
      for (int i = start; i < end; i++) {
        block.index_block.block_id[i] = block.index_block.block_id[i + 1];
      }
      this->pfs->write_block(block, dir_inode->first_level_ptr);
    }
  }
  dir_inode->block_num--;
  //  this->pfs->write_inode(*dir_inode, dir_inode->inode_id);
  return true;
}

QStringList FSFile::_dir_list(FSInode* dir_inode) {
  QStringList list;
  index_t bn = dir_inode->block_num;
  FSBlock block;
  for (index_t bi = 0; bi < bn; bi++) {
    index_t block_id = locate_bi(dir_inode, bi);
    this->pfs->read_block(block, block_id);
    for (int i = 0; i < block.dir_block.used; i++) {
      if (block.dir_block.records[i].inode_id == dir_inode->inode_id) {
        list.append(".");
      } else {
        list.append(block.dir_block.records[i].file_name);
      }
    }
  }
  return list;
}

QList<QPair<FSFileAttr, QString>> FSFile::_dir_list_with_attr(
    FSInode* dir_inode) {
  QList<QPair<FSFileAttr, QString>> list;
  index_t bn = dir_inode->block_num;
  FSBlock block;
  for (index_t bi = 0; bi < bn; bi++) {
    index_t block_id = locate_bi(dir_inode, bi);
    this->pfs->read_block(block, block_id);
    for (int i = 0; i < block.dir_block.used; i++) {
      if (block.dir_block.records[i].inode_id == dir_inode->inode_id) {
        list.append(qMakePair(_get_attr(block.dir_block.records[i].inode_id),
                              QString(".")));
      } else {
        list.append(qMakePair(_get_attr(block.dir_block.records[i].inode_id),
                              QString(block.dir_block.records[i].file_name)));
      }
    }
  }
  return list;
}

void FSFile::_file_sync_di_to_i(bool dump) {
  //文件的data_inode和inode同步
  this->inode.file_size = this->data_inode.file_size;
  this->inode.block_num = this->data_inode.block_num;
  if (dump) {
    this->pfs->write_inode(this->inode, this->inode.inode_id);
    this->pfs->write_inode(this->data_inode, this->data_inode.inode_id);
  }
}

void FSFile::_file_truncate_by_bi(index_t cut_start_bi) {
  /////////
  _file_truncate_by_bi(&this->data_inode, cut_start_bi);
  _file_sync_di_to_i(true);
}

// inode没写入外存
void FSFile::_file_truncate_by_bi(FSInode* file_data_inode,
                                  index_t cut_start_bi) {
  //这个截断是以块为单位的
  if (cut_start_bi >= file_data_inode->block_num or cut_start_bi < 0) {
    //如果文件占用块为0且截断点为0，则也是正确的
    if (not(cut_start_bi == file_data_inode->block_num and
            file_data_inode->block_num == 0))
      throw "FSFile::_file_truncate_by_bi: " + QString("截断点必须在文件中间");
  }

  offset_t new_file_size = cut_start_bi * fs::block_size;
  index_t new_block_num = cut_start_bi;
  FSIRoute r;
  r = file_data_inode->locate_block(cut_start_bi);
  int level = r.level;
  switch (level) {
    case 1: {
      if (file_data_inode->block_num > fs::inode_fst_n) {
        remove_index_block_level_two(
            file_data_inode->second_level_ptr,
            file_data_inode->block_num - fs::inode_fst_n);
        remove_index_block_level_one(file_data_inode->first_level_ptr,
                                     fs::inode_first_n);
      } else if (file_data_inode->block_num > fs::inode_direc_n) {
        remove_index_block_level_one(
            file_data_inode->first_level_ptr,
            file_data_inode->block_num - fs::inode_direc_n);
      }
      int end = MIN(fs::inode_direc_n, file_data_inode->block_num);
      //(debug)起始为cut_start_bi，之前误写为0
      for (int i = cut_start_bi; i < end; i++) {
        this->pfs->remove_block(file_data_inode->direct_ptr[i], true);
        file_data_inode->direct_ptr[i] = fs::ILLEGAL;
      }
      file_data_inode->first_level_ptr = fs::ILLEGAL;
      file_data_inode->second_level_ptr = fs::ILLEGAL;
      // inode未存外存
    } break;
    case 2: {
      int cut_start = r.route[1];
      if (file_data_inode->block_num > fs::inode_fst_n) {
        remove_index_block_level_two(
            file_data_inode->second_level_ptr,
            file_data_inode->block_num - fs::inode_fst_n);
        truncate_index_block_level_one(file_data_inode->first_level_ptr,
                                       fs::inode_first_n, cut_start);
      } else {
        truncate_index_block_level_one(
            file_data_inode->first_level_ptr,
            file_data_inode->block_num - fs::inode_direc_n, cut_start);
      }
      if (cut_start == 0) file_data_inode->first_level_ptr = fs::ILLEGAL;
      file_data_inode->second_level_ptr = fs::ILLEGAL;
      // inode未存外存
    } break;
    case 3: {
      if (cut_start_bi == fs::inode_fst_n) {
        remove_index_block_level_two(
            file_data_inode->second_level_ptr,
            file_data_inode->block_num - fs::inode_fst_n);
        file_data_inode->second_level_ptr = fs::ILLEGAL;
      } else {
        FSBlock block;
        this->pfs->read_block(block, r.route[0]);
        int cut_start = r.route[2];
        index_t st = file_data_inode->parse_route_to_bi(
            {{r.route[0], r.route[1], 0}, level});
        index_t bi_cnt =
            MIN(fs::index_block_i_n, file_data_inode->block_num - st);
        truncate_index_block_level_one(block.index_block.block_id[r.route[1]],
                                       bi_cnt, cut_start);
        if (cut_start == 0)
          block.index_block.block_id[r.route[1]] = fs::ILLEGAL;
        st += fs::index_block_i_n;
        //      index_t st_ib = file_data_inode->locate_l1_in_l2(st);
        index_t st_ib = r.route[1] + 1;

        index_t end_ib = (file_data_inode->block_num - fs::inode_fst_n +
                          fs::index_block_i_n - 1) /
                         fs::index_block_i_n;
        for (index_t ib = st_ib; ib < end_ib; ib++) {
          bi_cnt = MIN(fs::index_block_i_n, file_data_inode->block_num - st);
          index_t index_block_id = block.index_block.block_id[ib];
          remove_index_block_level_one(index_block_id, bi_cnt);
          block.index_block.block_id[ib] = fs::ILLEGAL;
          st += fs::index_block_i_n;
        }
        this->pfs->write_block(block, r.route[0]);
      }
      // inode未存外存
      /////
    } break;
    default:
      break;
  }
  file_data_inode->file_size = new_file_size;
  file_data_inode->block_num = new_block_num;
}

void FSFile::remove_index_block_level_one(index_t index_block_id,
                                          index_t bi_cnt) {
  if (index_block_id == fs::ILLEGAL) return;
  FSBlock block;  //????效率
  remove_index_block_level_one(block, index_block_id, bi_cnt);
}

void FSFile::remove_index_block_level_one(FSBlock& block,
                                          index_t index_block_id,
                                          index_t bi_cnt) {
  if (index_block_id == fs::ILLEGAL) return;
  int end = MIN(bi_cnt, fs::index_block_i_n);
  this->pfs->read_block(block, index_block_id);
  for (int i = 0; i < end; i++) {
    this->pfs->remove_block(block.index_block.block_id[i], true);
  }
  this->pfs->remove_block(index_block_id, true);
}

void FSFile::remove_index_block_level_two(index_t index_block_id,
                                          index_t bi_cnt) {
  if (index_block_id == fs::ILLEGAL) return;
  FSBlock block;
  FSBlock sub_block;
  this->pfs->read_block(block, index_block_id);
  int end = (bi_cnt + fs::index_block_i_n - 1) / fs::index_block_i_n;
  index_t cur_b_id = fs::ILLEGAL, cur_bi_cnt = -1;
  for (int i = 0; i < end; i++) {
    cur_b_id = block.index_block.block_id[i];
    cur_bi_cnt = MIN(fs::index_block_i_n, (bi_cnt - i * fs::index_block_i_n));
    //(debug)之前误传入block
    remove_index_block_level_one(sub_block, cur_b_id, cur_bi_cnt);
  }
  this->pfs->remove_block(index_block_id, true);
}

void FSFile::truncate_index_block_level_one(index_t index_block_id,
                                            index_t bi_cnt, index_t cut_start) {
  if (cut_start == 0) {
    remove_index_block_level_one(index_block_id, bi_cnt);
    return;
  }
  FSBlock block;  //????效率
  int end = MIN(bi_cnt, fs::index_block_i_n);
  this->pfs->read_block(block, index_block_id);
  for (int i = cut_start; i < end; i++) {
    this->pfs->remove_block(block.index_block.block_id[i], true);
    block.index_block.block_id[i] = fs::ILLEGAL;
  }
  this->pfs->write_block(block, index_block_id);
}

index_t FSFile::add_block(FSInode* inode, bool dump) {
  index_t new_bn = inode->block_num + 1;
  //(debug)不应先加，之后add_block_link的时会混乱
  //  inode->block_num = new_bn;
  index_t new_block_id = this->pfs->add_new_block(true);
  FSIRoute route;
  // bool add_block_num设置为false，防止inode->block_num重复加两次
  add_block_link(inode, new_block_id, route, false);

  inode->block_num = new_bn;
  //把inode信息写入外存
  if (dump) {
    this->pfs->write_inode(*inode, inode->inode_id);
  }
  return new_block_id;
}

//文件夹则要检查是否只利用一级间接
void FSFile::add_block_link(FSInode* inode, index_t block_id, FSIRoute& route,
                            bool add_block_num) {
  //这个函数没有做重复性检查

  // (debug)如果之前inode->block_num增加过了，就不用增加了
  //之前在add_block里时出现了这样的bug:inode->block_num重复加了两次
  index_t new_bn = inode->block_num;
  if (add_block_num) {
    new_bn++;
  }
  FSIRoute r = inode->locate_block(new_bn);
  int level = r.level;
  if (this->inode.file_type != 'f' and level > 2) {
    throw "FSFile::add_block_link：" + QString("文件夹只能利用一级间接索引");
  }
  switch (level) {
    case 1:
      inode->direct_ptr[r.route[0]] = block_id;
      break;
    case 2:
      if (inode->check_route({{r.route[0], 0, 0}, level})) {
        FSBlock block;
        index_t index_block_id = inode->direct_ptr[r.route[0]];
        this->pfs->read_block(block, index_block_id);
        block.index_block.block_id[r.route[1]] = block_id;
        this->pfs->write_block(block, index_block_id);
      } else {
        //        inode->direct_ptr[r.route[0]] = add_index_block(block_id);
        //        //加index块
        inode->direct_ptr[r.route[0]] = add_index_block(block_id);
      }
      break;
    case 3: {
      if (inode->check_route({{r.route[0], r.route[1], 0}, level})) {
        FSBlock block;
        index_t index_block_id = inode->direct_ptr[r.route[0]];
        this->pfs->read_block(block, index_block_id);
        index_block_id = block.index_block.block_id[r.route[1]];
        this->pfs->read_block(block, index_block_id);
        block.index_block.block_id[r.route[2]] = block_id;
        this->pfs->write_block(block, index_block_id);
      } else if (inode->check_route({{r.route[0], 0, 0}, level})) {
        FSBlock block;
        index_t index_block_id = inode->direct_ptr[r.route[0]];
        this->pfs->read_block(block, index_block_id);
        //        block.index_block.block_id[r.route[1]] =
        //            add_index_block(block_id);  //加index块
        //        this->pfs->write_block(block, index_block_id);
        block.index_block.block_id[r.route[1]] =
            add_index_block(block_id);  //加index块
        this->pfs->write_block(block, index_block_id);
      } else {
        //        index_t tmp = add_index_block(block_id); //加index块
        //        inode->direct_ptr[r.route[0]] = add_index_block(tmp);
        //        //加index块
        index_t tmp = add_index_block(block_id);  //加index块
        inode->direct_ptr[r.route[0]] = add_index_block(tmp);
        //加index块
      }
    } break;
    default:
      throw "FSFile::add_block_link：" + QString("没有返回正确的inode_route");
      break;
  }
  route = r;
}

index_t FSFile::add_index_block(index_t first_block_id) {
  FSBlock block;
  block.index_block.block_id[0] = first_block_id;
  block.index_block.block_id[1] = fs::ILLEGAL;
  index_t new_id = this->pfs->add_new_block(true);
  this->pfs->write_block(block, new_id);
  return new_id;
}

}  // namespace fs
