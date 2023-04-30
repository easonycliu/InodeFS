// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() 
{
  im = new inode_manager();
}

int extent_server::create(uint32_t type, extent_protocol::extentid_t &id)
{
  // alloc a new inode and return inum
  printf("extent_server: create inode\n");
  id = im->alloc_inode(type);

  return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  id &= 0x7fffffff;
  
  const char * cbuf = buf.c_str();
  int size = buf.size();
  im->write_file(id, cbuf, size);
  
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }

  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->get_attr(id, attr);
  a = attr;

  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  printf("extent_server: remove %lld\n", id);

  id &= 0x7fffffff;
  im->remove_file(id);
 
  return extent_protocol::OK;
}

int extent_server::read_dir(extent_protocol::extentid_t id, std::vector<std::pair<extent_protocol::extentid_t, std::string>> &bufs)
{
  printf("extent_server: read dir %lld\n", id);
  im->read_dir(id, bufs);
  printf("extent_server: have found dir %ld\n", bufs.size());
  return extent_protocol::OK;
}

int extent_server::add_to_dir(extent_protocol::extentid_t parent_id, extent_protocol::extentid_t id, std::string name)
{
  printf("extent_server: add %lld to %lld\n", id, parent_id);
  im->add_to_dir(parent_id, id, name);
  return extent_protocol::OK;
}

int extent_server::remove_from_dir(extent_protocol::extentid_t parent_id, extent_protocol::extentid_t id)
{
  printf("extent_server: remove %lld from %lld\n", id, parent_id);
  im->remove_from_dir(parent_id, id);
  return extent_protocol::OK;
}

int extent_server::set_attr(extent_protocol::extentid_t id, size_t size)
{
  printf("extent_server: set %lld attr size to %ld\n", id, size);
  im->set_attr(id, size);
  return extent_protocol::OK;
}
