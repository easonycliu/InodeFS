#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void disk::read_block(blockid_t id, char *buf) const
{
  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void disk::write_block(blockid_t id, const char *buf)
{
  memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------
bool block_manager::is_free(blockid_t id) const
{
  char buf[BLOCK_SIZE];
  read_block(id / BPB + 2, buf);
  return (buf[(id % BPB) / 8] & (0x1 << ((id % BPB) % 8))) == 0;
}

void block_manager::mark_bit(blockid_t id)
{
  char buf[BLOCK_SIZE];
  read_block(id / BPB + 2, buf);
  buf[(id % BPB) / 8] |= (0x1 << ((id % BPB) % 8));
  write_block(id / BPB + 2, buf);
}

void block_manager::unmark_bit(blockid_t id)
{
  char buf[BLOCK_SIZE];
  read_block(id / BPB + 2, buf);
  buf[(id % BPB) / 8] &= ~(0x1 << ((id % BPB) % 8));
  write_block(id / BPB + 2, buf);
}

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  for (blockid_t id = IBLOCK(sb.ninodes, sb.nblocks) + 1; id < sb.nblocks; ++id)
  {
    if (is_free(id))
    {
      mark_bit(id);
      return id;
    }
  }
  std::cout << "Cannot find free block" << std::endl;
  return 0;
}

void block_manager::free_block(uint32_t id)
{
  /*
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  if (id > IBLOCK(sb.ninodes, sb.nblocks) && !is_free(id))
  {
    unmark_bit(id);
    char buf[BLOCK_SIZE] = {0};
    write_block(id, buf);
  }
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;
}

void block_manager::read_block(uint32_t id, char *buf) const
{
  d->read_block(id, buf);
}

void block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1)
  {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /*
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  if (type != extent_protocol::T_DIR && type != extent_protocol::T_FILE && type != extent_protocol::T_LNK)
  {
    return 0;
  }

  uint32_t id = find_free_inode();
  if (id == 0)
  {
    throw std::bad_alloc();
  }
  struct inode * ino = (struct inode *)malloc(sizeof(struct inode));
  ino->size = 0;
  uint32_t curr_time = (uint32_t)time(NULL);
  ino->atime = curr_time;
  ino->ctime = curr_time;
  ino->mtime = curr_time;
  memset(ino->blocks, 0, sizeof(ino->blocks));
  ino->type = type;
  put_inode(id, ino);
  return id;
}

void inode_manager::free_inode(uint32_t inum)
{
  /*
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  struct inode *ino = get_inode(inum);
  if (ino->type != 0)
  {
    memset(ino, 0, sizeof(struct inode));
    put_inode(inum, ino);
  }
  free(ino);
  return;
}

/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode *
inode_manager::get_inode(uint32_t inum) const
{
  struct inode *ino = (struct inode *)malloc(sizeof(struct inode));
  /*
   * your code goes here.
   */
  char buf[BLOCK_SIZE];
  // printf("\tim: get_inode %d\n", inum);
  if (inum > INODE_NUM || inum < 1)
  {
    return NULL;
  }
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  *ino = *((struct inode *)buf + inum % IPB);
  return ino;
}

void inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL || inum > INODE_NUM || inum < 1)
  {
    return;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode *)buf + inum % IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

uint32_t inode_manager::find_free_inode() const
{
  uint32_t id = 1;
  while (id <= bm->sb.nblocks)
  {
    struct inode *ino = get_inode(id);
    if (ino->type == 0)
    {
      free(ino);
      return id;
    }
    free(ino);
    ++id;
  }
  return 0;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

std::string inode_manager::array_to_string(const char *buf, uint32_t size) const
{
  std::string str = "";
  str.resize(size);
  for (uint32_t i = 0; i < size; ++i)
  {
    str[i] = buf[i];
  }
  return str;
}

/* Get all the data of a file by inum.
 * Return alloced data, should be freed by caller. */
void inode_manager::read_file(uint32_t inum, char **buf_out, int *size) const
{
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
  std::cout << "inode_manager: start read " << std::endl;
  struct inode *ino = get_inode(inum);
  if (ino == NULL)
  {
    return;
  }

  std::vector<std::string> direct_bufs;
  int direct_size = 0;
  direct_bufs.reserve(MIN(ino->size, NDIRECT));
  for (uint32_t i = 0; i < MIN(ino->size, NDIRECT); ++i)
  {
    char buf[BLOCK_SIZE];
    bm->read_block(ino->blocks[i], buf);
    buf[BLOCK_SIZE] = '\0';
    std::string str = array_to_string(buf, BLOCK_SIZE);
    direct_bufs.push_back(str);
    direct_size += str.size();
  }
  std::vector<std::string> indirect_bufs;
  int indirect_size = 0;
  if (ino->size > NDIRECT)
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      char *buf = NULL;
      int one_size = 0;
      read_file(inum_buf[i], &buf, &one_size);
      indirect_bufs.push_back(array_to_string(buf, one_size));
      indirect_size += one_size;
      free(buf);
    }
  }
  *size = direct_size + indirect_size;
  *buf_out = (char *)malloc((*size) + 1);
  memset(*buf_out, 0, (*size) + 1);
  std::vector<std::string> bufs;
  bufs.insert(bufs.end(), direct_bufs.begin(), direct_bufs.end());
  bufs.insert(bufs.end(), indirect_bufs.begin(), indirect_bufs.end());
  int curr_size = 0;
  std::cout << "inode_manager: start merge " << std::endl;
  for (auto buf : bufs)
  {
    int len = buf.size();
    memcpy((*buf_out) + curr_size, buf.c_str(), len);
    curr_size += len;
  }
  int adj_size = *size;
  std::cout << "inode_manager: start adjust " << std::endl;
  while ((*buf_out)[adj_size - 1] == '\0' && adj_size > 0)
  {
    adj_size -= 1;
  }
  *size = adj_size;
  std::cout << "inode_manager: size " << adj_size << std::endl;
  free(ino);
  return;
}

/* alloc/free blocks if needed */
void inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf
   * is larger or smaller than the size of original inode
   */
  if (size < 0)
  {
    return;
  }

  struct inode *ino = get_inode(inum);
  if (ino == NULL)
  {
    return;
  }

  for (int i = 0; i < MIN(size / BLOCK_SIZE + (size % BLOCK_SIZE != 0), NDIRECT); ++i)
  {
    blockid_t id = bm->alloc_block();
    if (id > 0)
    {
      if (BLOCK_SIZE * (i + 1) > size)
      {
        char temp[BLOCK_SIZE];
        memset(temp, 0, BLOCK_SIZE);
        memcpy(temp, buf + BLOCK_SIZE * i, size - BLOCK_SIZE * i);
        bm->write_block(id, temp);
      }
      else
      {
        bm->write_block(id, buf + BLOCK_SIZE * i);
      }
      ino->blocks[i] = id;
    }
    else
    {
      throw std::bad_alloc();
    }
  }
  if (size > NDIRECT * BLOCK_SIZE)
  {
    blockid_t id = bm->alloc_block();
    if (id == 0)
    {
      throw std::bad_alloc();
    }

    uint32_t inodes[BLOCK_SIZE / sizeof(uint32_t)] = {0};
    uint32_t i = 0;
    do
    {
      uint32_t inode_num = alloc_inode(extent_protocol::T_FILE);
      inodes[i] = inode_num;
      write_file(inode_num, 
        buf + NDIRECT * BLOCK_SIZE * (i + 1), 
        MIN(size - NDIRECT * BLOCK_SIZE * (i + 1), NDIRECT * BLOCK_SIZE));
      ++i;
    } while((uint32_t)size > NDIRECT * BLOCK_SIZE * (i + 1) && i < BLOCK_SIZE / sizeof(uint32_t) - 1);
    if (i == BLOCK_SIZE / sizeof(uint32_t) - 1)
    {
      uint32_t inode_num = alloc_inode(extent_protocol::T_FILE);
      inodes[i] = inode_num;
      write_file(inode_num,
        buf + NDIRECT * BLOCK_SIZE * (i + 1),
        size - NDIRECT * BLOCK_SIZE * (i + 1));
    }
    bm->write_block(id, (char *)inodes);
    ino->blocks[NDIRECT] = id;
  }
  ino->size = MIN(size / BLOCK_SIZE + (size % BLOCK_SIZE != 0), NDIRECT + 1);
  ino->mtime = time(NULL);
  put_inode(inum, ino);
  free(ino);
  return;
}

void inode_manager::get_attr(uint32_t inum, extent_protocol::attr &a) const
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  struct inode *ino = get_inode(inum);
  a.atime = ino->atime;
  a.mtime = ino->mtime;
  a.ctime = ino->ctime;
  a.size = get_file_size(inum);
  a.type = ino->type;
  free(ino);
  return;
}

void inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  struct inode *ino = get_inode(inum);
  if (ino->type == 0)
  {
    free(ino);
    return;
  }

  for (uint32_t i = 0; i < NDIRECT; ++i)
  {
    if (ino->blocks[i] == 0)
    {
      break;
    }
    bm->free_block(ino->blocks[i]);
  }
  if (ino->size > NDIRECT)
  {
    uint32_t inodes[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inodes);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inodes[i] == 0)
      {
        break;
      }
      remove_file(inodes[i]);
    }
  }
  free_inode(inum);
  free(ino);
  return;
}

void inode_manager::read_dir(uint32_t inum, std::vector<std::pair<extent_protocol::extentid_t, std::string>> &bufs) const
{
  struct inode *ino = get_inode(inum);
  if (ino == NULL || ino->type != extent_protocol::T_DIR)
  {
    return;
  }

  std::vector<std::pair<extent_protocol::extentid_t, std::string>> direct_bufs;
  direct_bufs.reserve(MIN(ino->size, NDIRECT));
  for (uint32_t i = 0; i < MIN(ino->size, NDIRECT); ++i)
  {
    char buf[BLOCK_SIZE + 1];
    uint32_t dir_inum = 0;
    bm->read_block(ino->blocks[i], buf);
    
    buf[BLOCK_SIZE] = '\0';
    memcpy(&dir_inum, buf, sizeof(uint32_t));
    std::string str(buf + sizeof(uint32_t));
    direct_bufs.push_back({(extent_protocol::extentid_t)dir_inum, str});
  }
  std::vector<std::pair<extent_protocol::extentid_t, std::string>> indirect_bufs;
  if (ino->size > NDIRECT)
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      std::vector<std::pair<extent_protocol::extentid_t, std::string>> temp_buf;
      read_dir(inum_buf[i], temp_buf);
      indirect_bufs.insert(indirect_bufs.end(), temp_buf.begin(), temp_buf.end());
    }
  }
  bufs.insert(bufs.end(), direct_bufs.begin(), direct_bufs.end());
  bufs.insert(bufs.end(), indirect_bufs.begin(), indirect_bufs.end());
  free(ino);
  return;
}

void inode_manager::add_to_dir(uint32_t parent_inum, uint32_t inum, std::string name)
{
  struct inode *ino = get_inode(parent_inum);
  if (ino == NULL || ino->type != extent_protocol::T_DIR)
  {
    return;
  }

  if (ino->size < NDIRECT)
  {
    blockid_t id = bm->alloc_block();
    if (id == 0)
    {
      throw std::bad_alloc();
    }
    char buf[BLOCK_SIZE] = {0};
    memcpy(buf, &inum, sizeof(uint32_t));
    memcpy(buf + sizeof(uint32_t), name.c_str(), name.length());
    bm->write_block(id, buf);
    ino->blocks[ino->size] = id;
  }
  else
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        inum_buf[i] = alloc_inode(extent_protocol::T_DIR);
        add_to_dir(inum_buf[i], inum, name);
        bm->write_block(ino->blocks[NDIRECT], (char *)inum_buf);
        break;
      }
      if (get_ino_block_num(inum_buf[i]) < NDIRECT || i == BLOCK_SIZE / sizeof(uint32_t) - 1)
      {
        add_to_dir(inum_buf[i], inum, name);
        break;
      }
    }
  }
  ino->size = (ino->size < NDIRECT + 1) ? ino->size + 1 : ino->size;
  ino->mtime = time(NULL);
  ino->ctime = time(NULL);
  put_inode(parent_inum, ino);
  free(ino);
  return;
}

void inode_manager::remove_from_dir(uint32_t parent_inum, uint32_t inum)
{
  uint32_t index = 0;
  blockid_t last_block_id = 0;
  uint32_t last_block_index = 0;
  free_and_find_last(parent_inum, inum, index, last_block_id, last_block_index);
  set_ino_block_id(parent_inum, index, last_block_id);
  set_ino_block_id(parent_inum, last_block_index, 0);
  std::cout << "inode_manager : remove from dir index " << index << std::endl;
  std::cout << "inode_manager : remove from dir last_block_id " << last_block_id << std::endl;
  std::cout << "inode_manager : remove from dir last_block_index " << last_block_index << std::endl;

  struct inode *ino = get_inode(parent_inum);
  if (ino == NULL)
  {
    return;
  }

  ino->mtime = time(NULL);
  ino->ctime = time(NULL);
  put_inode(parent_inum, ino);
  free(ino);
}

uint32_t inode_manager::get_ino_block_num(uint32_t inum) const
{
  struct inode *ino = get_inode(inum);
  uint32_t size = ino->size;
  free(ino);
  return size;
}

void inode_manager::set_attr(uint32_t inum, size_t size)
{
  uint32_t attr_size = get_file_size(inum);
  if (attr_size < size)
  {
    truncate_file(inum, size);
  }
  else if (attr_size > size)
  {
    padding_file(inum, size);
  }
}

uint32_t inode_manager::get_file_block_num(uint32_t inum) const
{
  uint32_t attr_size = 0;
  struct inode *ino = get_inode(inum);
  uint32_t size = ino->size;
  if (size < NDIRECT + 1)
  {
    attr_size += size;
  }
  else
  {
    attr_size += NDIRECT;
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      attr_size += get_file_block_num(inum_buf[i]);
    }
  }
  free(ino);
  return attr_size;
}

uint32_t inode_manager::get_file_size(uint32_t inum) const
{
  uint32_t attr_size = 0;
  struct inode *ino = get_inode(inum);
  uint32_t size = ino->size;
  if (size == 0)
  {
    attr_size = 0;
  }
  else if (size < NDIRECT + 1)
  {
    attr_size = (size - 1) * BLOCK_SIZE + last_non_zero_element_index_in_block(ino->blocks[size - 1]) + 1;
  }
  else
  {
    attr_size += NDIRECT * BLOCK_SIZE;
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      attr_size += get_file_size(inum_buf[i]);
    }
  }
  free(ino);
  return attr_size;
}

int32_t inode_manager::last_non_zero_element_index_in_block(blockid_t id) const
{
  char block[BLOCK_SIZE] = {0};
  bm->read_block(id, block);
  for (int32_t i = BLOCK_SIZE - 1; i >= 0; --i)
  {
    if (block[i] != 0)
    {
      return i;
    }
  }
  return 0;
}

void inode_manager::truncate_file(uint32_t inum, size_t size)
{
  struct inode* ino = get_inode(inum);
  uint32_t remain_blocks = size / BLOCK_SIZE + (size % BLOCK_SIZE != 0);
  char zeros[BLOCK_SIZE] = {0};
  for (uint32_t i = 0; i < MIN(ino->size, NDIRECT); ++i)
  {
    if (i >= remain_blocks)
    {
      bm->write_block(ino->blocks[i], zeros);
      bm->free_block(ino->blocks[i]);
    }
  }
  
  if (ino->size > NDIRECT)
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      uint32_t adj_size = (size > (i + 1) * BLOCK_SIZE * NDIRECT) ? size - (i + 1) * BLOCK_SIZE * NDIRECT : 0;
      truncate_file(inum_buf[i], adj_size);
    }
  }
  if (size == 0)
  {
    free_inode(inum);
  }
  else
  {
    ino->size = MIN(remain_blocks, NDIRECT + 1);
    ino->mtime = time(NULL);
    put_inode(inum, ino);
  }
  free(ino);
}

void inode_manager::padding_file(uint32_t inum, size_t size)
{
  struct inode* ino = get_inode(inum);
  uint32_t final_blocks = size / BLOCK_SIZE + (size % BLOCK_SIZE != 0);
  char zeros[BLOCK_SIZE] = {0};
  for (uint32_t i = 0; i < MIN(final_blocks, NDIRECT); ++i)
  {
    if (i >= ino->size)
    {
      ino->blocks[i] = bm->alloc_block();
      bm->write_block(ino->blocks[i], zeros);
    }
  }
  if (final_blocks > NDIRECT)
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (final_blocks <= (i + 1) * NDIRECT)
      {
        break;
      }
      if (inum_buf[i] == 0)
      {
        inum_buf[i] = alloc_inode(ino->type);
      }
      uint32_t adj_size = ((final_blocks > (i + 2) * NDIRECT) ? NDIRECT : final_blocks - (i + 1) * NDIRECT) * BLOCK_SIZE;
      padding_file(inum_buf[i], adj_size);
    }
  }
  
  ino->size = MIN(final_blocks, NDIRECT + 1);
  ino->mtime = time(NULL);
  put_inode(inum, ino);
  free(ino);
}

void inode_manager::ino_next_block(uint32_t inum, uint32_t iter, blockid_t &id) const
{
  struct inode* ino = get_inode(inum);
  if (ino == NULL)
  {
    return;
  }

  if (iter < NDIRECT)
  {
    id = ino->blocks[iter];
    free(ino);
    return;
  }
  else
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    uint32_t calc_index = (iter - NDIRECT) / NDIRECT;
    uint32_t max_index = BLOCK_SIZE / sizeof(uint32_t) - 1;
    uint32_t inum_buf_index = (calc_index < max_index) ? calc_index : max_index;

    ino_next_block(inum_buf[inum_buf_index], iter - (inum_buf_index + 1) * NDIRECT, id);
  }
  free(ino);
}

void inode_manager::read_blockid(uint32_t inum, std::vector<blockid_t> &blocks) const
{
  struct inode* ino = get_inode(inum);
  if (ino == NULL)
  {
    return;
  }

  for (uint32_t i = 0; i < MIN(ino->size, NDIRECT); ++i)
  {
    blocks.push_back(ino->blocks[i]);
  }

  if (ino->size > NDIRECT)
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)];
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      std::vector<blockid_t> temp_blocks;
      read_blockid(inum_buf[i], temp_blocks);
      blocks.insert(blocks.end(), temp_blocks.begin(), temp_blocks.end());
    }
  }

  free(ino);
}

void inode_manager::free_and_find_last(uint32_t parent_inum, uint32_t inum, uint32_t &index, blockid_t &last_block_id, uint32_t &last_block_index)
{
  struct inode *ino = get_inode(parent_inum);
  if (ino == NULL)
  {
    return;
  }
  if (ino->type != extent_protocol::T_DIR)
  {
    free(ino);
    return;
  }

  std::vector<blockid_t> block_ids;
  read_blockid(parent_inum, block_ids);
  index = 0;
  for (auto block_id : block_ids)
  {
    char buf[BLOCK_SIZE] = {0};
    bm->read_block(block_id, buf);
    uint32_t read_inum = 0;
    memcpy(&read_inum, buf, sizeof(uint32_t));
    if (read_inum == inum)
    {
      bm->free_block(block_id);
      break;
    }
    index += 1;
  }

  last_block_id = block_ids.back();
  last_block_index = block_ids.size() - 1;
  free(ino);
}

bool inode_manager::set_ino_block_id(uint32_t &parent_inum, uint32_t index, blockid_t block_id, bool auto_free)
{
  struct inode *ino = get_inode(parent_inum);
  if (ino == NULL)
  {
    return false;
  }
  if (ino->type != extent_protocol::T_DIR)
  {
    free(ino);
    return false;
  }

  bool set_success = false;
  if (index < MIN(ino->size, NDIRECT))
  {
    ino->blocks[index] = block_id;
    if (block_id == 0)
    {
      ino->size -= 1;
    }
    if (ino->size == 0 && auto_free)
    {
      free_inode(parent_inum);
      parent_inum = 0;
    }
    else
    {
      put_inode(parent_inum, ino);
    }

    set_success = true;
  }
  else if (ino->size > NDIRECT && index > NDIRECT)
  {
    uint32_t inum_buf[BLOCK_SIZE / sizeof(uint32_t)] = {0};
    bm->read_block(ino->blocks[NDIRECT], (char *)inum_buf);
    for (uint32_t i = 0; i < BLOCK_SIZE / sizeof(uint32_t); ++i)
    {
      if (inum_buf[i] == 0)
      {
        break;
      }
      if (set_ino_block_id(inum_buf[i], index - (i + 1) * NDIRECT, block_id, true))
      {
        set_success = true;
        if (i == 0 && inum_buf[0] == 0)
        {
          bm->free_block(ino->blocks[ino->size - 1]);
          ino->blocks[ino->size - 1] = 0;
          ino->size = NDIRECT;
          put_inode(parent_inum, ino);
        }
        break;
      }
    }
  }
  free(ino);
  return set_success;
}