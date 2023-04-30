// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include <list>
#include "extent_protocol.h"
#include "inode_manager.h"

class extent_server {
 protected:
#if 0
  typedef struct extent {
    std::string data;
    struct extent_protocol::attr attr;
  } extent_t;
  std::map <extent_protocol::extentid_t, extent_t> extents;
#endif
  inode_manager *im;

 public:
  extent_server();

  int create(uint32_t type, extent_protocol::extentid_t &id);
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
  int read_dir(extent_protocol::extentid_t id, std::vector<std::pair<extent_protocol::extentid_t, std::string>> &bufs);
  int add_to_dir(extent_protocol::extentid_t parent_id, extent_protocol::extentid_t id, std::string name);
  int remove_from_dir(extent_protocol::extentid_t parent_id, extent_protocol::extentid_t id);
  int set_attr(extent_protocol::extentid_t id, size_t size);
};

#endif 







