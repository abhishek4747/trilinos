/*------------------------------------------------------------------------*/
/*                 Copyright 2013 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#ifndef  STK_GEOMETRICTRANSFER_HPP
#define  STK_GEOMETRICTRANSFER_HPP


#include <boost/smart_ptr/shared_ptr.hpp>

#include <stk_util/util/StaticAssert.hpp>
#include <stk_util/environment/ReportHandler.hpp>

#include <stk_search/CoarseSearch.hpp>
#include <stk_transfer/TransferBase.hpp>

namespace stk {
namespace transfer {

template <class INTERPOLATE> class GeometricTransfer : public TransferBase {

public :
  
  typedef INTERPOLATE                                     InterpolateClass;
  typedef typename InterpolateClass::MeshA                MeshA;
  typedef typename InterpolateClass::MeshB                MeshB;
  typedef typename MeshA::EntityKey                       EntityKeyA;
  typedef typename MeshB::EntityKey                       EntityKeyB;
  typedef typename std::multimap<EntityKeyB, EntityKeyA>  EntityKeyMap;
  typedef typename std::set     <EntityKeyA>              EntityKeySetA;
  typedef typename std::set     <EntityKeyB>              EntityKeySetB;
  typedef typename MeshA::BoundingBox                     BoundingBoxA;
  typedef typename MeshB::BoundingBox                     BoundingBoxB;
  
  typedef typename MeshA::EntityProc                      EntityProcA;
  typedef typename MeshB::EntityProc                      EntityProcB;

  typedef std::pair<EntityProcB, EntityProcA>             EntityProcRelation;
  typedef std::vector<EntityProcRelation>                 EntityProcRelationVec;
  
  enum {Dimension = MeshA::Dimension};
  
  
  GeometricTransfer(MeshA &mesha, 
                    MeshB &meshb,
                    const double radius,
                    const double expansion_factor,
                    const std::string &name);
  virtual void initialize() {
    coarse_search();
    communication();
    local_search();
  }
  virtual void apply();
  
  
  virtual void coarse_search();
  virtual void communication();
  virtual void local_search();
private :
  
  MeshA                &m_mesha;
  MeshB                &m_meshb;
  const double          m_radius;
  const double          m_expansion_factor;
  const std::string     m_name;
  
  EntityProcRelationVec m_global_range_to_domain;
  EntityKeyMap          m_local_range_to_domain;
  
  
  template <class MESH>
  void boundingboxes(std::vector<typename MESH::BoundingBox> &vector,
                     const typename MESH::EntityKeySet       &keys_to_match,
                     const MESH                              &mesh,
                     const double                             radius) const ;
  

  template <bool B, typename T = void> struct Enable_If          { typedef T type; };
  template <        typename T       > struct Enable_If<false, T>{                 };

  template <typename T> struct optional_functions {
  private : 
    template <typename X, X> class check {};
    template <typename X> static long copy_ent(...);
    template <typename X> static char copy_ent(
      check<
        void (X::*)(const typename MeshA::EntityProcVec&, const std::string&), 
        &X::copy_entities
      >*);
  public :
    static const bool copy_entities = (sizeof(copy_ent<T>(0)) == sizeof(char));
  };
  
  template <typename T> 
  typename Enable_If<optional_functions<T>::copy_entities>::type
       copy_entities(T                                   &mesh,
                     const typename MeshA::EntityProcVec &entities_to_copy,
                     const std::string                   &transfer_name) const 
  {
    mesh.copy_entities(entities_to_copy, transfer_name);
  }

  template <typename T> 
  typename Enable_If<!optional_functions<T>::copy_entities>::type
       copy_entities(T                                   &mesh,
                     const typename MeshA::EntityProcVec &entities_to_copy,
                     const std::string                   &transfer_name) const {
    ThrowErrorMsg(__FILE__<<":"<<__LINE__<<" Error: copy_entities undefinded in this class.");
  }

  EntityKeyMap copy_domain_to_range_processors(MeshA                            &mesh,
                                               const EntityProcRelationVec      &RangeToDomain,
                                               const std::string                &transfer_name) const;
  
  void coarse_search(EntityProcRelationVec   &RangeToDomain,
                     const MeshA             &mesha,
                     const MeshB             &meshb,
                     const double             radius,          
                     const double             expansion_factor) const ;

  struct compare {
    bool operator()(const BoundingBoxB &a, const EntityProcB  &b) const;
    bool operator()(const EntityProcB  &a, const BoundingBoxB &b) const;
  };

  void delete_range_points_found(std::vector<BoundingBoxB>            &range_vector,
                                 const EntityProcRelationVec          &del) const ;
  
  enum { dim_eq = StaticAssert<static_cast<unsigned>(MeshB::Dimension)==static_cast<unsigned>(MeshA::Dimension)>::OK };
  
};



template <class INTERPOLATE> GeometricTransfer<INTERPOLATE>::GeometricTransfer (MeshA            &mesha,
                                                                                MeshB            &meshb,
                                                                                const double      radius,
                                                                                const double      expansion_factor,
                                                                                const std::string &name) :
  m_mesha(mesha),
  m_meshb(meshb),
  m_radius(radius),
  m_expansion_factor(expansion_factor),
  m_name (name) {}


template <class INTERPOLATE> void GeometricTransfer<INTERPOLATE>::coarse_search() {

  m_global_range_to_domain.clear();
  coarse_search(m_global_range_to_domain, 
                m_mesha, 
                m_meshb, 
                m_radius,
                m_expansion_factor);
}
template <class INTERPOLATE> void GeometricTransfer<INTERPOLATE>::communication() {
  m_local_range_to_domain.clear();

  ParallelMachine comm = m_mesha.comm();
  const unsigned p_size = parallel_machine_size(comm);
  if (1==p_size) {
    const typename EntityProcRelationVec::const_iterator end=m_global_range_to_domain.end();
    for (typename EntityProcRelationVec::const_iterator i=m_global_range_to_domain.begin(); i!=end; ++i) {
      const EntityKeyB range_entity    = i->first.ident;
      const EntityKeyA domain_entity   = i->second.ident;
      std::pair<EntityKeyB,EntityKeyA> key_map(range_entity, domain_entity);
      m_local_range_to_domain.insert(key_map);
    }
  } else if (optional_functions<MeshA>::copy_entities) {
    m_local_range_to_domain = copy_domain_to_range_processors(m_mesha, m_global_range_to_domain, m_name);
  } else {
    ThrowRequireMsg (optional_functions<MeshA>::copy_entities,
      __FILE__<<":"<<__LINE__<<" Still working on communicaiton capabilities.");
  }
}

template <class INTERPOLATE> void GeometricTransfer<INTERPOLATE>::local_search() {
  INTERPOLATE::filter_to_nearest(m_local_range_to_domain, m_mesha, m_meshb); 
}


template <class INTERPOLATE> void GeometricTransfer<INTERPOLATE>::apply(){
  m_mesha.update_values();
  INTERPOLATE::apply(m_meshb, m_mesha, m_local_range_to_domain);
}

template <class INTERPOLATE> template <class MESH> void GeometricTransfer<INTERPOLATE>::boundingboxes(
  std::vector<typename MESH::BoundingBox>        &vector,
  const typename MESH::EntityKeySet              &keys_to_match,
  const MESH                                     &mesh,
  const double                                    radius) const {

  typedef typename MESH::BoundingBox        BoundingBox;
  typedef typename MESH::BoundingBox::Data  Data;
  typedef typename MESH::EntityKey          EntityKey;
  typedef typename MESH::EntityKeySet       EntityKeySet;
  vector.clear();
  vector.reserve(keys_to_match.size());
  const Data r = radius;
  for (typename EntityKeySet::const_iterator k=keys_to_match.begin(); k!=keys_to_match.end(); ++k) {
    const EntityKey  Id = *k;
    const BoundingBox B = mesh.boundingbox(Id, r);
    vector.push_back(B);
  }
}

template <class INTERPOLATE>
typename GeometricTransfer<INTERPOLATE>::EntityKeyMap GeometricTransfer<INTERPOLATE>::copy_domain_to_range_processors(
  MeshA                               &mesh,
  const EntityProcRelationVec         &range_to_domain,
  const std::string                   &transfer_name)  const {
  
  ParallelMachine comm = mesh.comm();
  const unsigned my_rank = parallel_machine_rank(comm);
  
  typename MeshA::EntityProcVec entities_to_copy ;

  const typename EntityProcRelationVec::const_iterator end=range_to_domain.end();
  for (typename EntityProcRelationVec::const_iterator i=range_to_domain.begin(); i!=end; ++i) {
    const unsigned            domain_owning_rank = i->second.proc;
    const unsigned             range_owning_rank = i->first.proc;
    if (domain_owning_rank == my_rank && range_owning_rank != my_rank) {
      const EntityKeyA entity = i->second.ident;
      const typename MeshA::EntityProc ep(entity, range_owning_rank);
      entities_to_copy.push_back(ep);
    }   
  }
  {
    std::sort(entities_to_copy.begin(), entities_to_copy.end());
    typename MeshA::EntityProcVec::iterator del = std::unique(entities_to_copy.begin(), entities_to_copy.end());
    entities_to_copy.resize(std::distance(entities_to_copy.begin(), del));
  }
 
  copy_entities(mesh, entities_to_copy, transfer_name);

  EntityKeyMap entity_key_map;
  for (typename EntityProcRelationVec::const_iterator i=range_to_domain.begin(); i!=end; ++i) {
    const unsigned range_owning_rank = i->first.proc;
    if (range_owning_rank == my_rank) {
      const EntityKeyB range_entity  = i->first.ident;
      const EntityKeyA domain_entity = i->second.ident;
      std::pair<EntityKeyB,EntityKeyA> key_map(range_entity, domain_entity);
      entity_key_map.insert(key_map);
    }   
  }   
  return entity_key_map;
}

template <class INTERPOLATE> bool GeometricTransfer<INTERPOLATE>::compare::operator()(const BoundingBoxB &a, const EntityProcB &b) const {
  return a.key < b;
}
template <class INTERPOLATE> bool GeometricTransfer<INTERPOLATE>::compare::operator()(const EntityProcB &a, const BoundingBoxB &b) const {
  return a < b.key;
}

template <class INTERPOLATE> void GeometricTransfer<INTERPOLATE>::delete_range_points_found(
                               std::vector<BoundingBoxB>            &range_vector,
                               const EntityProcRelationVec          &del) const {

  std::vector<EntityProcB> range_entities_found;
  range_entities_found.reserve(del.size());
  for (typename EntityProcRelationVec::const_iterator i=del.begin(); i!=del.end(); ++i) {
    range_entities_found.push_back(i->first);
  }
  {
    std::sort(range_entities_found.begin(), range_entities_found.end());
    const typename std::vector<EntityProcB>::iterator it = std::unique(range_entities_found.begin(), range_entities_found.end());
    range_entities_found.resize(it-range_entities_found.begin());
  }
  
  std::vector<BoundingBoxB> difference(range_vector.size());
  {
    const typename std::vector<BoundingBoxB>::iterator it = 
      std::set_difference(
        range_vector.        begin(), range_vector.        end(),
        range_entities_found.begin(), range_entities_found.end(),
        difference.begin(), compare());
    difference.resize(it-difference.begin());
  }
  swap(difference, range_vector);  
}

template <class INTERPOLATE>  void GeometricTransfer<INTERPOLATE>::coarse_search
(EntityProcRelationVec   &range_to_domain,
 const MeshA             &mesha,
 const MeshB             &meshb,
 const double            radius,
 const double            expansion_factor) const {
  
  EntityKeySetA keys_to_matcha;
  EntityKeySetB keys_to_matchb;

  mesha.keys(keys_to_matcha);;
  meshb.keys(keys_to_matchb);;

  double r = radius;
  std::vector<BoundingBoxB> range_vector;
  std::vector<BoundingBoxA> domain_vector;

  boundingboxes(domain_vector, keys_to_matcha, mesha, radius);
  boundingboxes(range_vector,  keys_to_matchb, meshb, radius);

  search::FactoryOrder order;
  order.m_communicator = mesha.comm();

  while (!range_vector.empty()) { // Keep going until all range points are processed.
    // Slightly confusing: coarse_search documentation has domain->range
    // relations sorted by domain key.  We want range->domain type relations
    // sorted on range key. It might appear we have the arguments revered
    // in coarse_search call, but really, this is what we want.  
    EntityProcRelationVec rng_to_dom;
    search::coarse_search(rng_to_dom, domain_vector, range_vector, order);

    INTERPOLATE::post_coarse_search_filter(rng_to_dom, mesha, meshb); 

    range_to_domain.insert(range_to_domain.end(), rng_to_dom.begin(), rng_to_dom.end());

    delete_range_points_found(range_vector, rng_to_dom); 

    r *= expansion_factor; // If points were missed, increase search radius.
    for (typename std::vector<BoundingBoxB>::iterator i=range_vector.begin(); i!=range_vector.end(); ++i) {
      i->expand(r);
    }
  } 
  sort (range_to_domain.begin(), range_to_domain.end());
}

}
}


#endif

