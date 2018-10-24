
#include "fs_macro.h"

namespace fs {

//看r表示的是否为有效的
bool FSInode::check_route(FSIRoute r) {
  if (!r.check()) return false;
  index_t bi = parse_route_to_bi(r);
  if (bi == fs::ILLEGAL or bi >= this->block_num) return false;
  return true;
}

FSIRoute FSInode::next_route(FSIRoute r) {
  index_t bi = fs::ILLEGAL;
  bi = parse_route_to_bi(r);
  bi++;
  if (bi >= this->block_num) return {{-1, -1, -1}, 0};
  return locate_block(bi);
}

index_t FSInode::parse_route_to_bi(FSIRoute r) {
  index_t bi = fs::ILLEGAL;
  switch (r.level) {
    case 1:
      bi = r.route[0];
      break;
    case 2:
      bi = 12 + r.route[1];
      break;
    case 3:
      bi = 12 + fs::index_block_i_n + r.route[1] * fs::index_block_i_n +
           r.route[2];
      break;
    default:
      break;
  }
  return bi;
}

FSIRoute FSInode::locate_block(index_t bi) {
  FSIRoute r;
  const index_t n = fs::index_block_i_n;
  const index_t direc = 12, first = 12 + n, second = 12 + n + n * n;
  if (bi < 0) {
    throw QString("fs::FSInode: parameter should larger than 0");
  } else if (bi < direc) {
    r.level = 1;
    r.route[0] = bi;
  } else if (bi < first) {
    r.level = 2;
    r.route[0] = 12;
    r.route[1] = bi - direc;
  } else if (bi < second) {
    r.level = 3;
    r.route[0] = 13;
    r.route[1] = (bi - first) / n;
    r.route[2] = (bi - first) % n;
  } else {
    throw QString("fs::FSInode: parameter is too large");
  }
  return r;
}

index_t FSInode::locate_l1_in_l2(index_t bi) {
  if (bi < fs::inode_direc_n + fs::inode_first_n)
    return fs::ILLEGAL;
  else {
    //    return (bi - fs::inode_direc_n - fs::inode_first_n) %
    //    fs::index_block_i_n;
    return (bi - 12 - 1024) / 1024;
  }
}
index_t FSInode::locate_direc_in_l2(index_t bi) {
  if (bi < fs::inode_direc_n + fs::inode_first_n)
    return fs::ILLEGAL;
  else {
    return (bi - 12 - 1024) % 1024;
  }
}
index_t FSInode::locate_direc_in_l1(index_t bi) {
  if (bi < fs::inode_direc_n) return fs::ILLEGAL;
  return (bi - 12) % 1024;
}

}  // namespace fs
