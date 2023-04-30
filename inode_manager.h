// inode layer interface.

#ifndef inode_h
#define inode_h

#include <stdint.h>
#include <time.h>

#include <exception>
#include "extent_protocol.h"

#define DISK_SIZE 1024 * 1024 * 16 * 64
#define BLOCK_SIZE 512
#define BLOCK_NUM (DISK_SIZE / BLOCK_SIZE)

typedef uint32_t blockid_t;

// disk layer -----------------------------------------

class disk
{
private:
  unsigned char blocks[BLOCK_NUM][BLOCK_SIZE];

public:
  disk();
  void read_block(uint32_t id, char *buf) const;
  void write_block(uint32_t id, const char *buf);
};

// block layer -----------------------------------------

typedef struct superblock
{
  uint32_t size;
  uint32_t nblocks;
  uint32_t ninodes;
} superblock_t;

class block_manager
{
private:
  disk *d;
  std::map<uint32_t, int> using_blocks;
  bool is_free(blockid_t id) const;
  void mark_bit(blockid_t id);
  void unmark_bit(blockid_t id);

public:
  block_manager();
  struct superblock sb;

  uint32_t alloc_block();
  void free_block(uint32_t id);
  void read_block(uint32_t id, char *buf) const;
  void write_block(uint32_t id, const char *buf);
};

// inode layer -----------------------------------------

#define INODE_NUM 1024

// Inodes per block.
#define IPB 1
//(BLOCK_SIZE / sizeof(struct inode))

// Block containing inode i
#define IBLOCK(i, nblocks) ((nblocks) / BPB + (i) / IPB + 3)

// Bitmap bits per block
#define BPB (BLOCK_SIZE * 8)

// Block containing bit for block b
#define BBLOCK(b) ((b) / BPB + 2)

#define NDIRECT 100
#define NINDIRECT (BLOCK_SIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

typedef struct inode
{
  short type;
  unsigned int size;
  unsigned int atime;
  unsigned int mtime;
  unsigned int ctime;
  blockid_t blocks[NDIRECT + 1]; // Data block addresses
} inode_t;

class inode_manager
{
private:
  block_manager *bm;
  struct inode *get_inode(uint32_t inum) const;
  void put_inode(uint32_t inum, struct inode *ino);
  uint32_t find_free_inode() const;
  uint32_t get_ino_block_num(uint32_t inum) const;
  uint32_t get_file_block_num(uint32_t inum) const;
  uint32_t get_file_size(uint32_t inum) const;
  int32_t last_non_zero_element_index_in_block(blockid_t id) const;
  void truncate_file(uint32_t inum, size_t size);
  void padding_file(uint32_t inum, size_t size);
  void ino_next_block(uint32_t inum, uint32_t iter, blockid_t &id) const;
  void read_blockid(uint32_t inum, std::vector<blockid_t> &blocks) const;
  void free_and_find_last(uint32_t parent_inum, uint32_t inum, uint32_t &index, blockid_t &last_block_id, uint32_t &last_block_index);
  bool set_ino_block_id(uint32_t &parent_inum, uint32_t index, blockid_t last_block_id, bool auto_free=false);
  std::string array_to_string(const char *buf, uint32_t size) const;

public:
  inode_manager();
  uint32_t alloc_inode(uint32_t type);
  void free_inode(uint32_t inum);
  void read_file(uint32_t inum, char **buf, int *size) const;
  void write_file(uint32_t inum, const char *buf, int size);
  void remove_file(uint32_t inum);
  void get_attr(uint32_t inum, extent_protocol::attr &a) const;
  void read_dir(uint32_t inum, std::vector<std::pair<extent_protocol::extentid_t, std::string>> &bufs) const;
  void add_to_dir(uint32_t parent_inum, uint32_t inum, std::string name);
  void remove_from_dir(uint32_t parent_inum, uint32_t inum);
  void set_attr(uint32_t inum, size_t size);
};

#endif