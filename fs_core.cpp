#include "fs_core.h"

namespace fs {

MiniFSGroup MiniFSGroup::instance;

MiniFSGroup::~MiniFSGroup() {
  for (int i = 0; i < 3; i++)
    if (this->ptr_fs_array[i] != nullptr) {
      delete ptr_fs_array[i];
    }
}

FSBase* MiniFSGroup::mount(QString outer_path, QString name) {
  name = name.split(" ")[0];
  {
    if (name == "") {
      throw "MiniFSGroup::mount: " + QString("name不能为空");
    }
    QStringList list = outer_path.split(".");
    if (list.length() != 2 or list[0] == "" or list[1] == "")
      throw "MiniFSGroup::mount: " + QString("outer_path 应形如 'test.minifs'");
    if (list[1] != "minifs") {
      throw "MiniFSGroup::mount: " + QString("outer_path 后缀名应该为'minifs'");
    }
    QFileInfo fi(outer_path);
    if (!fi.exists()) {
      throw "MiniFSGroup::mount: " + outer_path + " " + QString(TR("不存在"));
    }
    int64 fs_size = fi.size();
    if (not(fs_size % fs::EIGHT_FK == 0 and fs_size >= fs::min_size and
            fs_size <= fs::max_size)) {
      throw "MiniFSGroup::mount: " +
          QString(TR("大小应该为32KB的整数倍,且大于等于 %1 "
                     "MB,小于 %2 GB"))
              .arg(int(fs::min_size / fs::ONE_MB))
              .arg(int(fs::max_size / fs::ONE_GB));
    }
    FSGroupHeader header;
    QFile f(outer_path);
    f.open(QIODevice::ReadOnly);
    f.seek(0);
    f.read((char*)(&header), sizeof(FSGroupHeader));
    {  //检查头
      if (header.version != parse_version(fs::version)) {
        qDebug() << header.version;
        throw "MiniFSGroup::mount: " + QString("版本号应该为 ") + fs::version;
        //暂时检查不充分
      }
    }
  }
  {
    if (this->map_name_to_id.find(name) != this->map_name_to_id.end()) {
      throw "MiniFSGroup::mount: " + QString("不能重名");
    }
    //    minifs_name_array
  }
  if (name.trimmed() == "") throw QString("名字不能为空");
  if (this->cnt + 1 >= 3) throw QString("最多挂载3个MINIFS");
  for (int i = 0; i < 3; i++) {
    if (this->ptr_fs_array[i] == nullptr) {
      this->ptr_fs_array[i] = new FSBase(outer_path);
      this->minifs_name_array[i] = name;
      this->map_name_to_id[name] = i;
      this->cnt++;
      return this->ptr_fs_array[i];
    }
  }
  return nullptr;
}
FSBase* MiniFSGroup::get_fs(int fs_index) {
  return this->ptr_fs_array[fs_index];
}

FSBase* MiniFSGroup::get_fs(QString name) {
  return this->ptr_fs_array[this->map_name_to_id[name]];
}
QString MiniFSGroup::get_name(int fs_index) {
  return this->minifs_name_array[fs_index];
}

int MiniFSGroup::get_fs_id(QString name) { return this->map_name_to_id[name]; }

QList<QPair<int, QString>> MiniFSGroup::list_fs() {
  QList<QPair<int, QString>> list;
  for (int i = 0; i < 3; i++) {
    if (this->ptr_fs_array[i] != nullptr) {
      list.append(qMakePair(i, this->minifs_name_array[i]));
    }
  }
  return list;
}

void MiniFSGroup::unmount(int fs_index) {
  if (this->ptr_fs_array[fs_index] == nullptr) throw QString("该MINIFS不存在");
  delete this->ptr_fs_array[fs_index];
  ptr_fs_array[fs_index] = nullptr;
  this->map_name_to_id.remove(this->minifs_name_array[fs_index]);
  this->minifs_name_array[fs_index] = "";
  this->cnt--;
}

void MiniFSGroup::unmount(QString name) {
  if (map_name_to_id.find(name) == map_name_to_id.end()) {
    throw QString("该MINIFS不存在");
  }
  unmount(this->map_name_to_id[name]);
}

QString parse_version(int32 version) {
  QString str("");
  int a = 1024;
  for (int i = 0; i < 3; i++) {
    if (i == 0) {
      str = QString::number(version % a, 10) + str;
    } else {
      str = QString::number(version % a, 10) + "." + str;
    }
    version /= a;
  }
  return str;
}

int32 parse_version(int v1, int v2, int v3) {
  int a = 1024;
  if (v1 >= 1024 || v1 < 0 || v2 >= 1024 || v2 < 0 || v3 >= 1024 || v3 < 0) {
    //    APP_ERROR(QString(TR("格式不正确")))
    //      return -1;
    throw "fs::parse_version: " + QString(TR("格式不正确"));
  }
  int32 version = v1 * a * a + v2 * a + v3;
  return version;
}

int32 parse_version(QString version) {
  int v[3] = {-1, -1, -1};
  QStringList vlist = version.split(".", QString::SkipEmptyParts);
  for (int i = 0; i < 3; i++) {
    v[i] = vlist.at(i).toInt();
  }
  return parse_version(v[0], v[1], v[2]);
}

void create_minifs(QString outer_path, int64 fs_size) {
  QStringList list = outer_path.split(".");
  if (list.length() != 2 or list[0] == "" or list[1] == "")
    throw "fs::create_minifs " + QString("outer_path 应形如 'test.minifs'");
  if (list[1] != "minifs") {
    throw "fs::create_minifs " + QString("outer_path 后缀名应该为'minifs'");
  }
  QFileInfo fi(outer_path);
  if (fi.exists()) {
    throw "fs::create_minifs: " + outer_path + " " + QString(TR("已经存在"));
  }
  if (fs_size % fs::EIGHT_FK == 0 and fs_size >= fs::min_size and
      fs_size <= fs::max_size) {
    QFile file(outer_path);
    file.open(QIODevice::ReadWrite);
    file.resize(fs_size);
    //    file.open(QIODevice::WriteOnly);
    return format_minifs(&file);
  } else {
    throw "fs::create_minifs: " +
        QString(TR("大小应该为32KB的整数倍,且大于等于 %1 "
                   "MB,小于 %2 GB"))
            .arg(int(fs::min_size / fs::ONE_MB))
            .arg(int(fs::max_size / fs::ONE_GB));
  }
}

void format_minifs(QString outer_path) {
  QStringList list = outer_path.split(".");
  if (list.length() != 2 or list[0] == "" or list[1] == "")
    throw "fs::create_minifs " + QString("outer_path 应形如 'test.minifs'");
  if (list[1] != "minifs") {
    throw "fs::create_minifs " + QString("outer_path 后缀名应该为'minifs'");
  }
  QFileInfo fi(outer_path);
  if (!fi.exists()) {
    throw "fs::format_minifs: " + outer_path + " " + QString(TR("不存在"));

  } else if (fi.isDir()) {
    throw "fs::format_minifs: " + outer_path + " " + QString(TR("是文件夹"));

  } else {
    QFile file(outer_path);
    file.open(QIODevice::ReadWrite);
    return format_minifs(&file);
  }
}

void format_minifs(QFile* outer_file) {
  offset_t total_size = outer_file->size();
  if (!(total_size % fs::EIGHT_FK == 0 and total_size >= fs::min_size and
        total_size <= fs::max_size)) {
    throw "fs::format_minifs: " +
        QString(TR("大小应该为32KB的整数倍,且大于等于 %1 MB,小于 %2 GB"))
            .arg(int(fs::min_size / fs::ONE_MB))
            .arg(int(fs::max_size / fs::ONE_GB));
  }
  FSGroupHeader header;
  offset_t last_group_size = total_size % fs::ONE_GB;
  index_t group_num = (total_size + fs::ONE_GB - 1) / fs::ONE_GB;

  if (last_group_size == 0) {
    last_group_size = fs::ONE_GB;
  } else if (group_num >= 2 &&
             last_group_size < fs::GROUP_UPBOUND - fs::ONE_GB) {
    last_group_size += fs::ONE_GB;
    group_num--;
  }

  index_t total_block_num =
      (group_num - 1) * fs::default_block_num +
      (last_group_size - fs::default_blocks_offset) / fs::B;

  header.version = parse_version(fs::version);
  header.group_num = group_num;
  header.total_block_num = total_block_num;
  header.cur_group_id = 0;
  for (int i = 0; i < header.group_num; i++) {
    header.group_offset_array[i] = offset_t(i) * fs::ONE_GB;
  }
  for (int i = 0; i < group_num; i++) {
    if (i != group_num - 1) {
      header.cur_group_id = i;
      header.cur_group_block_num = fs::default_block_num;
      header.cur_group_size = fs::ONE_GB;
    } else {
      header.cur_group_id = i;
      header.cur_group_size = last_group_size;
      header.cur_group_block_num =
          (last_group_size - fs::default_blocks_offset) / fs::B;
    }

    offset_t offset = header.group_offset_array[i];
    outer_file->seek(offset);
    outer_file->write((char*)(&header), sizeof(fs::FSGroupHeader));

    //    FSGroupHeader hh;
    //    outer_file->seek(offset);
    //    outer_file->read((char*)(&hh), sizeof(fs::FSGroupHeader));

    offset_t ibm_offset = offset + fs::default_ibm_offset;
    for (int i = 0; i < fs::ibm_B_num; i++) {
      offset_t off = ibm_offset + i * fs::B;
      outer_file->seek(off);
      outer_file->write((char*)(fs::ZERO_BLOCK), fs::B);
    }

    offset_t bbm_offset = offset + fs::default_bbm_offset;
    for (int i = 0; i < fs::bbm_B_num; i++) {
      offset_t off = bbm_offset + i * fs::B;
      outer_file->seek(off);
      outer_file->write((char*)(fs::ZERO_BLOCK), fs::B);
    }
  }
  // root
  offset_t root_inode_offset = 0 + fs::default_inodes_offset;
  offset_t root_block_offset = 0 + fs::default_blocks_offset;
  FSInode root_i;
  {
    root_i.file_type = 'd';
    root_i.dir_layer = 0;
    root_i.tree_layer = 0;  //
    root_i.auth.set_default();
    root_i.inode_id = 0;
    root_i.file_size = 2;
    root_i.block_num = 1;
    root_i.create_time = QDateTime::currentSecsSinceEpoch();
    root_i.change_time = QDateTime::currentSecsSinceEpoch();
    memset(root_i.direct_ptr, -1,
           fs::inode_direc_n * sizeof(root_i.direct_ptr[0]));
    root_i.direct_ptr[0] = 0;
    root_i.first_level_ptr = -1;
    root_i.second_level_ptr = -1;
  }
  outer_file->seek(root_inode_offset);
  outer_file->write((char*)(&root_i), sizeof(FSInode));
  FSBlock root_b;
  {
    root_b.dir_block.records[0].set(0, "/");
    root_b.dir_block.records[1].set(fs::ILLEGAL, "..");
    root_b.dir_block.used = 2;
  }
  outer_file->seek(root_block_offset);
  outer_file->write((char*)(&root_b), sizeof(FSBlock));
  //位图置1
  uint8 one = uint8(1);
  outer_file->seek(fs::default_ibm_offset);
  outer_file->write((char*)(&one), sizeof(uint8));
  outer_file->seek(fs::default_bbm_offset);
  outer_file->write((char*)(&one), sizeof(uint8));
}

void change_current_dir(FSBase* pfs, QString path) {
  path = path.trimmed();
  if (not FSFile::check_path_name(path)) {
    throw "fs::change_current_dir: " + QString("路径名不规范");
  }
  FSFile cur_dir(pfs);
  FSInode new_cur_inode;
  bool flag = cur_dir.parse_path(path, new_cur_inode);
  if (!flag) {
    throw "fs::change_current_dir: " + QString("没找到路径");
  }
  if (new_cur_inode.file_type != 'd') {
    throw "fs::change_current_dir: " + QString("该路径所表示的不是文件夹");
  }
  pfs->set_cur_inode_id(new_cur_inode.inode_id);
  pfs->calcu_cur_route();
}

QString get_current_dir(FSBase* pfs) { return pfs->get_cur_dir(); }

QStringList list_dir(FSBase* pfs, QString dir_path) {
  dir_path = dir_path.trimmed();
  FSFile dir(pfs, dir_path);
  return dir._dir_list(&dir.inode);
}

QList<QPair<FSFileAttr, QString>> list_dir_with_attr(FSBase* pfs,
                                                     QString dir_path) {
  dir_path = dir_path.trimmed();
  FSFile dir(pfs, dir_path);
  return dir._dir_list_with_attr(&dir.inode);
}

void _create_dir(FSBase* pfs, FSFile& parent, QString name,
                 bool force = false) {
  name = name.trimmed();
  if (name == ".." or name == "." or name == "/") {
    throw "fs::create_dir: " + QString("新建文件夹名不能为'/'或'.'或'..'");
  }
  if (not FSFile::check_folder_name(name)) {
    throw "fs::create_dir: " + QString("文件夹名不规范");
  }
  if (parent.inode.file_size + 1 > fs::max_sub_file_n) {
    throw "fs::create_dir: " + QString("当前文件夹已经满了");
  }
  if (parent.inode.dir_layer + 1 > fs::max_dir_layer) {
    throw "fs::create_dir: " +
        QString("目录层级最多为 %1").arg(fs::max_dir_layer);
  }
  if (parent.inode.file_type != 'd') {
    throw "fs::create_dir: " + QString("parent 不是文件夹");
  }
  if (not force) {
    if (fs::ILLEGAL != parent.find_file_in_dir(&parent.inode, name)) {
      // 因为这里比较特殊，传了指针（其实我也不清楚。。。。），随便throw导致程序中断，会出现野指针
      // 所以QT会报错（弱的IDE不会）
      // 这个可以说是一个设计上的bug
      //    throw "fs::create_dir: " + QString("重名");
      qDebug() << "fs::create_dir: " + QString("重名");
      return;
    }
  }
  FSInode inode;
  {
    inode.file_type = 'd';
    inode.dir_layer = parent.inode.dir_layer + 1;
    inode.tree_layer = 0;
    inode.auth.set_default();
    inode.inode_id = pfs->add_new_inode(true);
    inode.file_size = 2;
    inode.block_num = 1;
    inode.create_time = QDateTime::currentSecsSinceEpoch();
    inode.change_time = QDateTime::currentSecsSinceEpoch();
    memset(inode.direct_ptr, -1,
           fs::inode_direc_n * sizeof(inode.direct_ptr[0]));
    inode.direct_ptr[0] = pfs->add_new_block(true);
    inode.first_level_ptr = -1;
    inode.second_level_ptr = -1;
  }

  FSBlock block;
  {
    block.dir_block.records[0].set(inode.inode_id, name.toUtf8().data());
    block.dir_block.records[1].set(parent.inode.inode_id, "..");
    block.dir_block.used = 2;
  }
  parent._dir_add_sub_dir(&parent.inode, inode.inode_id, name.toUtf8().data(),
                          inode.tree_layer);

  pfs->write_inode(inode, inode.inode_id);

  pfs->write_block(block, inode.direct_ptr[0]);
}

void create_dir(FSBase* pfs, QString path, bool last_force) {
  FSFile cur_dir(pfs);
  FSFile *cur, *tmp;
  if (not FSFile::check_dir_name(path))
    throw "fs::create_dir: " + QString("路径不合法");
  int layer = cur_dir.parse_path_layer(path);
  if (layer <= 0 or layer > fs::max_dir_layer) {
    throw "fs::create_dir: " + QString("目录层级不合法");
  }
  if (path == "/") throw "fs::create_dir: " + QString("不可新建根目录");
  try {
    QStringList p_list = path.split("/");
    int length = p_list.length();
    // name or name/
    if (length == 1 or (length == 2 and p_list[1] == "")) {
      cur = open(pfs, ".");
      QString name;
      if (length == 1)
        name = path;
      else
        name = p_list[0];
      if (name == "." or name == "..") {
        //        throw "fs::create_dir: " + QString("不可新'.'或'..'");
        return;
      }
      if (fs::ILLEGAL == cur->find_file_in_dir(&cur->inode, name)) {
        _create_dir(pfs, *cur, name, last_force);
      }
      //      _create_dir(pfs, cur_dir, name, last_force);
      //      //新建文件时有bug，所以要先检测
      close(cur);
      return;
    }

    int start = 0, end = length - 1;

    QString name = "", route = "";
    if (p_list[0] == "") {
      cur = open(pfs, "/");
      start++;
    } else {
      route += ".";
      cur = open(pfs, ".");
    }
    if (p_list[end] == "") {
      end--;
    }

    for (int i = start; i <= end; i++) {
      route += "/" + p_list[i];
      tmp = cur;
      name = path.section('/', i, i);

      if (fs::ILLEGAL == cur->find_file_in_dir(&cur->inode, name)) {
        if (i != end) {
          _create_dir(pfs, *cur, name, true);
        } else {
          _create_dir(pfs, *cur, name, last_force);
        }
      }

      //      else {
      //        if (last_force == false) {
      //          qDebug() << "fs::create_dir: " + QString("重名");
      //        }
      //      }

      //      //这个if内部实现了处理.和..;
      //      //_create_dir里也检测了，如果不force则会做一次多余的检测
      //      if (fs::ILLEGAL == cur->find_file_in_dir(&cur->inode, name)) {
      //        if (i != end) {
      //          _create_dir(pfs, *cur, name, true);
      //        } else {
      //          _create_dir(pfs, *cur, name, last_force);
      //        }
      //      }
      cur = open(pfs, route);
      close(tmp);
    }
    close(cur);

  } catch (QString error_msg) {
    close(cur);
    close(tmp);
    throw error_msg;
  }
}

void _create_file(FSBase* pfs, FSFile& parent, QString name) {
  name = name.trimmed();
  if (not FSFile::check_file_name(name)) {
    throw "fs::create_file: " + QString("文件名不规范");
  }
  if (parent.inode.file_size + 1 > fs::max_sub_file_n) {
    throw "fs::create_file: " + QString("当前文件夹已经满了");
  }
  //  if (parent.inode.dir_layer + 1 > fs::max_dir_layer) {
  //    throw "fs::create_dir: " +
  //        QString("目录层级最多为 %1").arg(fs::max_dir_layer);
  //  }
  if (parent.inode.file_type != 'd') {
    throw "fs::create_file: " + QString("parent 不是文件夹");
  }
  if (fs::ILLEGAL != parent.find_file_in_dir(&parent.inode, name)) {
    //    throw "fs::create_file: " + QString("重名");
    qDebug() << "fs::create_file: " + QString("重名");
    return;
  }

  FSInode inode, data_inode;
  index_t data_inode_id = fs::ILLEGAL;
  FSBlock block;
  {
    inode.file_type = 'f';
    inode.dir_layer = parent.inode.dir_layer + 1;  //层级应该对文件无限制吧
    inode.tree_layer = 0;  //层级应该对文件无限制吧
    inode.auth.set_default();
    inode.inode_id = pfs->add_new_inode(true);  // bug
    inode.file_size = 0;
    inode.block_num = 0;
    inode.create_time = QDateTime::currentSecsSinceEpoch();
    inode.change_time = QDateTime::currentSecsSinceEpoch();

    memset(inode.direct_ptr, -1,
           fs::inode_direc_n * sizeof(inode.direct_ptr[0]));
    inode.direct_ptr[0] = pfs->add_new_block(true);  // bug
    inode.first_level_ptr = -1;
    inode.second_level_ptr = -1;
  }
  {
    data_inode_id = pfs->add_new_inode(true);
    block.dir_block.records[0].set(data_inode_id, name.toUtf8().data());
    block.dir_block.records[1].set(parent.inode.inode_id, "..");
    block.dir_block.used = 2;
  }
  {
    data_inode.file_type = 'b';
    data_inode.dir_layer = parent.inode.dir_layer + 2;  //层级应该对文件无限制吧
    data_inode.tree_layer = 0;  //层级应该对文件无限制吧
    data_inode.auth.set_default();
    data_inode.inode_id = data_inode_id;
    data_inode.file_size = 0;
    data_inode.block_num = 0;
    data_inode.create_time = QDateTime::currentSecsSinceEpoch();
    data_inode.change_time = QDateTime::currentSecsSinceEpoch();
    memset(data_inode.direct_ptr, -1,
           fs::inode_direc_n * sizeof(data_inode.direct_ptr[0]));
    data_inode.first_level_ptr = -1;
    data_inode.second_level_ptr = -1;
  }
  //写了
  parent._dir_add_record(&parent.inode, inode.inode_id, name.toUtf8().data());

  pfs->write_inode(inode, inode.inode_id);
  pfs->write_block(block, inode.direct_ptr[0]);
  pfs->write_inode(data_inode, data_inode_id);
}

void create_file(FSBase* pfs, QString path) {
  FSFile cur_dir(pfs);
  FSFile* cur;
  if (not FSFile::check_path_name(path))
    throw "fs::create_file: " + QString("路径不合法");
  QStringList p_list = path.split('/');
  int length = p_list.length();
  if (not FSFile::check_file_name(p_list[length - 1]))
    throw "fs::create_file: " + QString("文件名不合法");

  try {
    if (length == 1) {
      _create_file(pfs, cur_dir, path);
      return;
    }

    QString file_name = path.section('/', length - 1, length - 1);
    QString parent_path = path.section('/', 0, length - 2);

    if (parent_path == "/") {
      cur = open(pfs, "/");
      _create_file(pfs, *cur, file_name);
      close(cur);
    } else {
      create_dir(pfs, parent_path, true);
      cur = open(pfs, parent_path);
      _create_file(pfs, *cur, file_name);
      close(cur);
    }
  } catch (QString error_msg) {
    close(cur);
    throw error_msg;
  }
}

void change_auth_mod(FSBase* pfs, QString path, FileAuth auth) {
  path = path.trimmed();
  FSFile file(pfs, path);
  change_auth_mod(&file, auth);
}
void change_auth_mod(FSFile* file, FileAuth auth) {
  file->change_auth_mod(auth);
}

int8 get_dir_layer(FSBase* pfs, QString path) {
  path = path.trimmed();
  FSFile dir(pfs, path);
  if (dir.inode.file_type != 'd')
    return -1;
  else
    return dir.inode.dir_layer;
}

int8 get_tree_layer(FSBase* pfs, QString path) {
  path = path.trimmed();
  FSFile dir(pfs, path);
  if (dir.inode.file_type != 'd')
    return -1;
  else
    return dir.inode.tree_layer;
}

void move(FSBase* pfs, QString former_path, QString new_path) {
  former_path = former_path.trimmed();
  FSFile file(pfs, former_path);
  move(&file, new_path);
}
void move(FSFile* file, QString new_path) {
  new_path = new_path.trimmed();
  if (new_path.at(new_path.length() - 1) == '/') new_path.chop(1);
  if (not FSFile::check_path_name(new_path)) {
    throw "fs::move: " + QString("路径不合法");
  }
  if (new_path == "")
    throw "fs::move: " + QString("新路径不能为空");  //已经检查过，可以去掉？？
  if (new_path == "/") throw "fs::move: " + QString("新路径不可为根");
  QStringList list = new_path.split("/");
  QString new_parent, new_name;
  int length = list.length();
  if (length == 1) {
    new_parent = ".";
    new_name = list[0];
  } else {
    new_name = new_path.section("/", length - 1);
    new_parent = new_path.section("/", 0, length - 2);
    if (new_parent == "") {
      new_parent = "/";
    }
  }
  if (file->inode.file_type == 'd') {
    file->mv_dir_to(new_parent, new_name);
  } else {
    file->mv_file_to(new_parent, new_name);
  }
}

FSFileAttr get_file_attr(FSBase* pfs, QString path) {
  path = path.trimmed();
  FSFile file(pfs, path);
  return get_file_attr(&file);
}
FSFileAttr get_file_attr(FSFile* file) { return file->get_attr(); }

FSFile* open(FSBase* pfs, QString path, FSFile::OpenMode mode) {
  path = path.trimmed();
  FSFile* file = new FSFile(pfs, path, mode);
  return file;
}

FSFile* open(FSBase* pfs, QString path) {
  path = path.trimmed();
  FSFile* file = new FSFile(pfs, path);
  return file;
}

void close(FSFile*& file) {
  if (file == nullptr) return;
  delete file;
  file = nullptr;
}

int64 read(FSFile* file, char* data, int64 offset_in_file, int64 max_size) {
  return file->read(data, offset_in_file, max_size);
}

void write(FSFile* file, char* data, int64 offset_in_file, int64 max_size) {
  file->write(data, offset_in_file, max_size);

  //  {  // debug
  //    FSBlock block;
  //    FSInode data_inode;
  //    file->pfs->read_inode(data_inode, file->data_inode.inode_id);
  //    file->pfs->read_block(block, data_inode.direct_ptr[0]);
  //    char a[100] = "";
  //    int i;
  //    for (i = 0; i < max_size; i++) {
  //      a[i] = block.data_block.data_byte[i];
  //    }
  //    a[i] = '\0';
  //    qDebug() << a;
  //  }
}

void add_content(FSFile* file, char* data, int64 max_size) {
  file->add_content(data, max_size);
}

void clear(FSFile* file) { file->clear(); }

void remove(FSBase* pfs, QString name) {
  FSFile* file = new FSFile(pfs, name);
  remove(file);
}

void remove(FSFile* file) {
  file->remove();
  close(file);
}

index_t get_rest_inode_num(FSBase* pfs) {
  return pfs->inode_bitmap->get_rest_free_num();
}

index_t get_rest_block_num(FSBase* pfs) {
  return pfs->block_bitmap->get_rest_free_num();
}

QList<index_t> get_block_id_list(FSBase* pfs, QString path) {
  FSFile* f = open(pfs, path);
  QList<index_t> ans = f->get_block_id_list();
  close(f);
  return ans;
}

}  // namespace fs
