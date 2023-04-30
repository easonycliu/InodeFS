// extent client interface.

#ifndef extent_client_h
#define extent_client_h

#include <string>
#include <list>
#include "extent_protocol.h"
#include "extent_server.h"

class extent_client {
 private:
  extent_server *es;

 public:
  extent_client();

  extent_protocol::status create(uint32_t type, extent_protocol::extentid_t &eid);
  extent_protocol::status get(extent_protocol::extentid_t eid, 
			                        std::string &buf);
  extent_protocol::status getattr(extent_protocol::extentid_t eid, 
				                          extent_protocol::attr &a);
  extent_protocol::status put(extent_protocol::extentid_t eid, std::string buf);
  extent_protocol::status remove(extent_protocol::extentid_t eid);
  extent_protocol::status read_dir(extent_protocol::extentid_t eid, std::vector<std::pair<extent_protocol::extentid_t, std::string>> &bufs);
  extent_protocol::status add_to_dir(extent_protocol::extentid_t parent_id, extent_protocol::extentid_t eid, std::string name);
  extent_protocol::status remove_from_dir(extent_protocol::extentid_t eid, extent_protocol::extentid_t id);
  extent_protocol::status set_attr(extent_protocol::extentid_t eid, size_t size);
};

#endif 

