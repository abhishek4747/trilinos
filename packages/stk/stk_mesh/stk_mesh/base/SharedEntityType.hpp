#ifndef STK_SHAREDENTITYTYPE_HPP
#define STK_SHAREDENTITYTYPE_HPP

#include <stk_mesh/base/EntityKey.hpp>

namespace stk {
namespace mesh {

struct shared_entity_type
{
  stk::topology::topology_t topology;
  std::vector<EntityKey>    nodes;
  stk::mesh::EntityKey      local_key;
  stk::mesh::EntityKey      global_key;
  std::vector<int>          sharing_procs;

  friend inline bool operator < (shared_entity_type const& l, shared_entity_type const& r)
  {
    if (l.topology < r.topology)   return true;
    if (l.topology > r.topology)   return false;
    return l.nodes < r.nodes;
  }

  friend inline bool operator == (shared_entity_type const& l, shared_entity_type const& r)
  {

    bool sameTopologyAndNodes =  (l.topology == r.topology) && (l.nodes==r.nodes);
    return sameTopologyAndNodes;
  }
};

}
}

#endif
