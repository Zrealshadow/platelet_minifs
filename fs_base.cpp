#include "fs_base.h"

namespace fs {

FSBase::FSBase(QString outer_path) {
  //  {
  //    QStringList list = outer_path.split(".");
  //    if (list.length() != 2 or list[0] == "" or list[1] != "minifs")
  //      throw "FSBase::FSBase: " + QString("outer_path 应形如 'test.minfs'");
  //  }
  this->ptr_fs = new QFile(outer_path);
  this->ptr_fs->open(QIODevice::ReadWrite);
  FSGroupHeader first_group_header, last_group_header;
  load(0, &first_group_header, sizeof(FSGroupHeader));
  this->version = first_group_header.version;
  this->group_num = first_group_header.group_num;
  this->total_block_num = first_group_header.total_block_num;
  this->normal_group_block_num = fs::default_block_num;
  this->normal_group_size = fs::ONE_GB;
  for (index_t i = 0; i < this->group_num; i++) {
    this->group_offset_array[i] = first_group_header.group_offset_array[i];
  }
  load(this->group_offset_array[this->group_num - 1], &last_group_header,
       sizeof(FSGroupHeader));
  this->last_group_block_num = last_group_header.cur_group_block_num;
  this->last_group_size = last_group_header.cur_group_size;

  offset_t ibm_offsets[100], bbm_offsets[100];
  uint8 *ibm = new uint8[this->total_block_num / 8],
        *bbm = new uint8[this->total_block_num / 8];
  for (index_t i = 0; i < this->group_num; i++) {
    ibm_offsets[i] = this->group_offset_array[i] + fs::default_ibm_offset;
    bbm_offsets[i] = this->group_offset_array[i] + fs::default_bbm_offset;
  }
  index_t cur;
  for (index_t i = 0; i < this->group_num - 1; i++) {
    cur = i * fs::default_block_num;
    load(ibm_offsets[i], &ibm[cur], fs::default_block_num * sizeof(uint8) / 8);
    load(bbm_offsets[i], &bbm[cur], fs::default_block_num * sizeof(uint8) / 8);
  }
  cur = ((this->group_num - 1) * fs::default_block_num);
  load(ibm_offsets[group_num - 1], &ibm[cur],
       this->last_group_block_num * sizeof(uint8) / 8);
  load(bbm_offsets[group_num - 1], &bbm[cur],
       this->last_group_block_num * sizeof(uint8) / 8);
  this->inode_bitmap = new _fs::FSBitmap(
      this->total_block_num, ibm, this->ptr_fs, ibm_offsets, this->group_num);
  this->block_bitmap = new _fs::FSBitmap(
      this->total_block_num, bbm, this->ptr_fs, bbm_offsets, this->group_num);
  delete[] ibm;
  delete[] bbm;

  this->inode_hash = new _fs::FSHash<FSInode, fs::inode_hash_size>();
  this->block_hash = new _fs::FSHash<FSBlock, fs::block_hash_size>();
  //根目录
  this->cur_inode_id = 0;

  this->cur_dir_path = new QStringList();
  this->cur_route = new QList<index_t>();
  this->cur_dir_path->append("/");
  this->cur_route->append(0);

  //似乎无法正确释放QStringList？？
  //  this->cur_dir_path.append("/");
  //  this->cur_route.append(0);

  //这里似乎会导致内存泄露？？？？？？？
  // QStack<FSDirRecord>的错？？
  //  calcu_cur_route();  // bug 可能出现死循环
}

FSBase::~FSBase() {
  this->ptr_fs->close();
  delete this->ptr_fs;
  delete this->inode_bitmap;
  delete this->block_bitmap;
  delete this->inode_hash;
  delete this->block_hash;
  this->cur_dir_path->clear();
  delete this->cur_dir_path;
  this->cur_route->clear();
  delete this->cur_route;
}

void FSBase::read_inode(FSInode& data, index_t inode_id) {
  FSInode* p_i = this->inode_hash->get(inode_id);
  if (p_i == nullptr)
    load_inode(&data, inode_id);
  else
    memcpy(&data, p_i, sizeof(FSInode));
}
void FSBase::read_block(FSBlock& data, index_t block_id) {
  FSBlock* p_b = this->block_hash->get(block_id);
  if (p_b == nullptr)
    load_block(&data, block_id);
  else
    memcpy(&data, p_b, sizeof(FSBlock));
}

//写inode时修改change_time
void FSBase::write_inode(FSInode& data, index_t inode_id) {
  //时间_______________________________
  data.change_time = QDateTime::currentSecsSinceEpoch();
  dump_inode(&data, inode_id);
  if (this->inode_hash->get(inode_id) != nullptr) {
    this->inode_hash->set(inode_id, &data);
  }
}

void FSBase::write_block(FSBlock& data, index_t block_id) {
  dump_block(&data, block_id);
  if (this->block_hash->get(block_id) != nullptr) {
    this->block_hash->set(block_id, &data);
  }
}

void FSBase::calcu_cur_route() {
  //  FSInode tmp_inode = FSInode(new_cur_inode);
  FSInode tmp_inode;
  this->read_inode(tmp_inode, this->cur_inode_id);
  FSBlock tmp_block;
  index_t p_id;
  QStack<FSDirRecord> stack;
  while (true) {  //如果根目录崩坏会死循环
    this->read_block(tmp_block, tmp_inode.direct_ptr[0]);
    p_id = tmp_block.dir_block.records[1].inode_id;
    stack.push(tmp_block.dir_block.records[0]);
    if (p_id == fs::ILLEGAL)  //回溯到根目录了
      break;
    this->read_inode(tmp_inode, p_id);
  }
  FSDirRecord r;
  this->cur_dir_path->clear();
  this->cur_route->clear();
  while (not stack.isEmpty()) {
    r = stack.pop();
    this->cur_dir_path->append(r.file_name);
    this->cur_route->append(r.inode_id);
  }
}

index_t FSBase::get_cur_inode_id() { return this->cur_inode_id; }

void FSBase::set_cur_inode_id(index_t new_cur_inode_id) {
  if (new_cur_inode_id < 0 || new_cur_inode_id > this->total_block_num)
    throw QString("not a valid inode_id");
  this->cur_inode_id = new_cur_inode_id;
}

QString FSBase::get_cur_dir() {
  QString str = "";
  QStringList::iterator it = this->cur_dir_path->begin();
  QStringList::iterator it_end = this->cur_dir_path->end();
  for (; it != it_end; it++) {
    if (*it == "/")
      str += "/";
    else
      str += *it + "/";
  }
  return str;
}

bool FSBase::check_in_cur_route(index_t inode_id) {
  for (int i = 0; i < this->cur_route->length(); i++) {
    if (inode_id == this->cur_route->at(i)) return true;
  }
  return false;
}

index_t FSBase::add_new_block(bool dump) {
  index_t new_block_id = this->block_bitmap->get_first_free_id_and_set_one();
  if (new_block_id == fs::ILLEGAL)
    throw "FSBase::add_new_block: " + QString("no free block");
  if (dump) dump_block_bitmap_by_id(new_block_id);
  return new_block_id;
}
index_t FSBase::add_new_inode(bool dump) {
  index_t new_inode_id = this->inode_bitmap->get_first_free_id_and_set_one();
  if (new_inode_id == fs::ILLEGAL)
    throw "FSBase::add_new_inode: " + QString("no free inode");
  if (dump) dump_inode_bitmap_by_id(new_inode_id);
  return new_inode_id;
}

void FSBase::remove_block(index_t block_id, bool dump) {
  this->block_bitmap->set_by_index(block_id, false, dump);
}

void FSBase::remove_inode(index_t inode_id, bool dump) {
  this->inode_bitmap->set_by_index(inode_id, false, dump);
}

void FSBase::dump_block_bitmap_by_id(index_t block_id) {
  this->block_bitmap->dump_to_outer_device_by_id(block_id);
}
void FSBase::dump_inode_bitmap_by_id(index_t inode_id) {
  this->inode_bitmap->dump_to_outer_device_by_id(inode_id);
}

// private

void FSBase::load(offset_t offset, void* data, offset_t size) {
  this->ptr_fs->seek(offset);
  this->ptr_fs->read((char*)(data), size);
}

void FSBase::dump(offset_t offset, const void* data, offset_t size) {
  this->ptr_fs->seek(offset);
  this->ptr_fs->write((const char*)(data), size);
}

offset_t FSBase::seek_inode(index_t inode_id) {
  //如果最后一组大于1G
  if (inode_id / fs::default_block_num >= this->group_num)
    return this->group_offset_array[inode_id / fs::default_block_num - 1] +
           fs::default_inodes_offset + fs::default_block_num +
           (inode_id % fs::default_block_num) *
               fs::inode_size;  //(debug)之前忘记乘inode_size了
  //如果最后一组小于1G或1G
  else
    return this->group_offset_array[inode_id / fs::default_block_num] +
           fs::default_inodes_offset +
           (inode_id % fs::default_block_num) * fs::inode_size;
}

offset_t FSBase::seek_block(index_t block_id) {
  if (block_id / fs::default_block_num >= this->group_num)
    return this->group_offset_array[block_id / fs::default_block_num - 1] +
           fs::default_blocks_offset + fs::default_block_num +
           (block_id % fs::default_block_num) * fs::block_size;
  else
    return this->group_offset_array[block_id / fs::default_block_num] +
           fs::default_blocks_offset +
           (block_id % fs::default_block_num) * fs::block_size;
}

void FSBase::load_inode(FSInode* data, index_t inode_id) {
  offset_t offset = seek_inode(inode_id);
  load(offset, data, sizeof(FSInode));
}

void FSBase::dump_inode(FSInode* data, index_t inode_id) {
  offset_t offset = seek_inode(inode_id);
  dump(offset, data, sizeof(FSInode));
}

void FSBase::load_block(FSBlock* data, index_t block_id) {
  offset_t offset = seek_block(block_id);
  load(offset, data, sizeof(FSBlock));
}

void FSBase::dump_block(FSBlock* data, index_t block_id) {
  offset_t offset = seek_block(block_id);
  dump(offset, data, sizeof(FSBlock));
}

////仅用于快速写文件
void FSBase::read_block_fast(void* block_data, offset_t offset_in_block,
                             offset_t size, index_t block_id) {
  FSBlock* p_b = this->block_hash->get(block_id);
  if (p_b != nullptr) {
    memcpy(block_data, &p_b->data_block.data_byte[offset_in_block], size);
  } else {
    offset_t offset = seek_block(block_id);
    load(offset + offset_in_block, block_data, size);
  }
}
void FSBase::write_block_fast(void* block_data, offset_t offset_in_block,
                              offset_t size, index_t block_id) {
  offset_t offset = seek_block(block_id);
  dump(offset + offset_in_block, block_data, size);
  if (this->block_hash->get(block_id) != nullptr) {
    this->block_hash->set(block_id, (char*)block_data, offset_in_block, size);
  }
}

}  // namespace fs
