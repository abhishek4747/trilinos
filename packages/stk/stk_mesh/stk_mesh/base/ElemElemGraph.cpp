#include "ElemElemGraph.hpp"
#include "ElemElemGraphImpl.hpp"

#include <vector>
#include <algorithm>

#include <stk_topology/topology.hpp>
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/Comm.hpp>
#include <stk_mesh/base/FEMHelpers.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/baseImpl/MeshImplUtils.hpp>

#include <stk_util/parallel/CommSparse.hpp>
#include <stk_util/environment/ReportHandler.hpp>

namespace stk { namespace mesh {


ElemElemGraph::ElemElemGraph(stk::mesh::BulkData& bulkData) : m_bulk_data(bulkData)
{
    int numElems = size_data_members();
    if (numElems > 0)
    {
        impl::fill_local_ids_and_fill_element_entities_and_topologies(m_bulk_data, m_local_id_to_element_entity, m_entity_to_local_id, m_element_topologies);
        fill_graph();

        impl::ElemSideToProcAndFaceId elem_side_comm = impl::get_element_side_ids_to_communicate(bulkData);
        size_t num_face_ids_needed = elem_side_comm.size();
        for(size_t i=0;i<m_via_sides.size();++i)
        {
            m_num_edges += m_via_sides[i].size();
        }
        num_face_ids_needed += m_num_edges;

        bulkData.generate_new_ids(stk::topology::FACE_RANK, num_face_ids_needed, m_suggested_face_ids);

        fill_parallel_graph(elem_side_comm);

        m_num_parallel_edges = m_parallel_graph_info.size();
        m_num_edges += m_num_parallel_edges;
    }
}

const std::vector<stk::mesh::EntityId>& ElemElemGraph::get_suggested_face_ids() const
{
    return m_suggested_face_ids;
}

void ElemElemGraph::set_num_face_ids_used(size_t num_used)
{
    m_suggested_face_ids.erase(m_suggested_face_ids.begin(), m_suggested_face_ids.begin()+num_used);
}

ElemElemGraph::~ElemElemGraph() {}

size_t ElemElemGraph::get_num_connected_elems(stk::mesh::Entity local_element) const
{
    impl::LocalId local_id = get_local_element_id(local_element);
    return m_elem_graph[local_id].size();
}

bool ElemElemGraph::is_connected_elem_locally_owned(stk::mesh::Entity local_element, size_t index_conn_elem) const
{
    impl::LocalId local_id = get_local_element_id(local_element);
    return m_elem_graph[local_id][index_conn_elem] >= 0;
}

int ElemElemGraph::get_side_id_to_connected_element(stk::mesh::Entity local_element, size_t index_conn_elem) const
{
    impl::LocalId local_id = get_local_element_id(local_element);
    return m_via_sides[local_id][index_conn_elem];
}

stk::mesh::Entity ElemElemGraph::get_connected_element(stk::mesh::Entity local_element, size_t index_conn_elem) const
{
    impl::LocalId local_id = get_local_element_id(local_element);
    impl::LocalId other_element_id = m_elem_graph[local_id][index_conn_elem];
    return m_local_id_to_element_entity[other_element_id];
}

stk::mesh::EntityId ElemElemGraph::get_entity_id_of_remote_element(stk::mesh::Entity local_element, size_t index_conn_elem) const
{
    ThrowRequireMsg(!is_connected_elem_locally_owned(local_element, index_conn_elem) , "Program error. Contact sierra-help@sandia.gov for support.");
    impl::LocalId local_id = get_local_element_id(local_element);
    stk::mesh::EntityId id = -m_elem_graph[local_id][index_conn_elem];
    return id;
}

int ElemElemGraph::get_owning_proc_id_of_remote_element(stk::mesh::Entity local_element, stk::mesh::EntityId other_element_id) const
{
    impl::LocalId local_id = get_local_element_id(local_element);
    impl::ParallelGraphInfo::const_iterator iter = m_parallel_graph_info.find(std::make_pair(local_id, other_element_id));
    ThrowRequireMsg( iter != m_parallel_graph_info.end(), "Program error. Contact sierra-help@sandia.gov for support.");
    int other_proc = iter->second.m_other_proc;
    return other_proc;
}

int ElemElemGraph::get_side_from_element1_to_remote_element2(stk::mesh::Entity local_element, stk::mesh::EntityId other_element_id) const
{
    impl::LocalId remote_element_local_id = -other_element_id;
    impl::LocalId element1_local_id = get_local_element_id(local_element);

    int side = -1;
    const std::vector<impl::LocalId>& conn_elements = m_elem_graph[element1_local_id];

    std::vector<impl::LocalId>::const_iterator iter = std::find(conn_elements.begin(), conn_elements.end(), remote_element_local_id);
    if ( iter != conn_elements.end() )
    {
        int64_t index = iter - conn_elements.begin();
        side = m_via_sides[element1_local_id][index];
    }
    return side;
}

int ElemElemGraph::get_side_from_element1_to_locally_owned_element2(stk::mesh::Entity local_element, stk::mesh::Entity other_element) const
{
    impl::LocalId other_element_id = get_local_element_id(other_element);
    impl::LocalId element1_local_id = get_local_element_id(local_element);

    int side = -1;
    const std::vector<impl::LocalId>& conn_elements = m_elem_graph[element1_local_id];

    std::vector<impl::LocalId>::const_iterator iter = std::find(conn_elements.begin(), conn_elements.end(), other_element_id);
    if ( iter != conn_elements.end() )
    {
        int64_t index = iter - conn_elements.begin();
        side = m_via_sides[element1_local_id][index];
    }
    return side;
}

size_t ElemElemGraph::num_edges() const
{
    return m_num_edges;
}

impl::parallel_info& ElemElemGraph::get_parallel_edge_info(stk::mesh::Entity element, stk::mesh::EntityId remote_id)
{
    impl::LocalId this_elem_local_id = get_local_element_id(element);

    impl::ParallelGraphInfo::iterator iter = m_parallel_graph_info.find(std::make_pair(this_elem_local_id, remote_id));
    ThrowRequireMsg( iter != m_parallel_graph_info.end(), "Program error. Contact sierra-help@sandia.gov for support.");
    return iter->second;
}

impl::LocalId ElemElemGraph::get_local_element_id(stk::mesh::Entity local_element) const
{
    ThrowRequireMsg(m_entity_to_local_id.size() > local_element.local_offset(),"Program error. Contact sierra-help@sandia.gov for support.");
    impl::LocalId local_id = m_entity_to_local_id[local_element.local_offset()];
    ThrowRequireMsg(local_id != std::numeric_limits<impl::LocalId>::max(), "Program error. Contact sierra-help@sandia.gov for support.");
    return local_id;
}

int ElemElemGraph::size_data_members()
{
    std::vector<unsigned> counts(stk::topology::NUM_RANKS,0);
    stk::mesh::count_entities(m_bulk_data.mesh_meta_data().locally_owned_part(), m_bulk_data, counts);
    int numElems = counts[stk::topology::ELEM_RANK];

    m_elem_graph.resize(numElems);
    m_via_sides.resize(numElems);
    m_local_id_to_element_entity.resize(numElems, Entity());
    m_entity_to_local_id.resize(m_bulk_data.m_entity_keys.size(), ElemGraphLocalId::INVALID_ID);
    m_element_topologies.resize(numElems);
    m_num_edges = 0;
    m_num_parallel_edges = 0;
    m_local_id_in_pool.resize(numElems, false);
    return numElems;
}

void ElemElemGraph::ensure_space_in_entity_to_local_id(size_t max_index)
{
    size_t needed = max_index + 1;

    if (m_elem_graph.size() < needed)
    {
        m_entity_to_local_id.resize(needed, ElemGraphLocalId::INVALID_ID);
    }
}

void ElemElemGraph::get_element_side_pairs(const stk::mesh::MeshIndex &meshIndex, impl::LocalId local_elem_id, std::vector<impl::ElementSidePair> &elem_side_pairs) const
{
    stk::mesh::EntityVector side_nodes;
    stk::mesh::EntityVector connected_elements;
    stk::mesh::Entity element = (*meshIndex.bucket)[meshIndex.bucket_ordinal];
    stk::topology topology = meshIndex.bucket->topology();
    int num_sides = topology.num_sides();
    const stk::mesh::Entity* elem_nodes = meshIndex.bucket->begin_nodes(meshIndex.bucket_ordinal);
    elem_side_pairs.clear();
    for(int side_index=0; side_index<num_sides; ++side_index)
    {
        unsigned num_side_nodes = topology.side_topology(side_index).num_nodes();
        side_nodes.resize(num_side_nodes);
        topology.side_nodes(elem_nodes, side_index, side_nodes.begin());
        connected_elements.clear();
        impl::ConnectedElementDataVector connectedElementDataVector;
        impl::find_locally_owned_elements_these_nodes_have_in_common(m_bulk_data, num_side_nodes, side_nodes.data(), connected_elements);
        add_local_elements_to_connected_list(connected_elements, side_nodes, connectedElementDataVector);
        impl::filter_for_candidate_elements_to_connect(m_bulk_data, element, side_index, connectedElementDataVector);

        for (const impl::ConnectedElementData & elemData: connectedElementDataVector)
        {
            if (local_elem_id != elemData.m_elementId)
            {
                elem_side_pairs.push_back(std::make_pair(elemData.m_elementId,side_index));
            }
        }
    }
    std::sort(elem_side_pairs.begin(), elem_side_pairs.end());
    std::vector<impl::ElementSidePair>::iterator new_end = std::unique(elem_side_pairs.begin(), elem_side_pairs.end());
    elem_side_pairs.resize(new_end - elem_side_pairs.begin());
}

void ElemElemGraph::fill_graph()
{
    const stk::mesh::BucketVector& elemBuckets = m_bulk_data.get_buckets(stk::topology::ELEM_RANK, m_bulk_data.mesh_meta_data().locally_owned_part());
    for(size_t i=0; i<elemBuckets.size(); ++i)
    {
        const stk::mesh::Bucket& bucket = *elemBuckets[i];
        std::vector<impl::ElementSidePair> elem_side_pairs;

        for(size_t j=0; j<bucket.size(); ++j)
        {
            impl::LocalId local_elem_id = get_local_element_id(bucket[j]);
            get_element_side_pairs(stk::mesh::MeshIndex(elemBuckets[i],j), local_elem_id, elem_side_pairs);
            for(size_t index=0; index<elem_side_pairs.size(); ++index)
            {
                m_elem_graph[local_elem_id].push_back(elem_side_pairs[index].first);
                m_via_sides[local_elem_id].push_back(elem_side_pairs[index].second);
            }
        }
    }
}

void ElemElemGraph::fill_parallel_graph(impl::ElemSideToProcAndFaceId& elem_side_comm)
{
    stk::CommSparse comm(m_bulk_data.parallel());

    pack_shared_side_nodes_of_elements(comm, m_bulk_data, elem_side_comm, m_suggested_face_ids);
    comm.allocate_buffers();

    pack_shared_side_nodes_of_elements(comm, m_bulk_data, elem_side_comm, m_suggested_face_ids);
    comm.communicate();

    stk::mesh::impl::ConnectedElementDataVector communicatedElementDataVector;
    std::map<EntityVector, stk::mesh::impl::ConnectedElementDataVector> communicatedElementDataMap;

    for(int proc_id=0; proc_id<m_bulk_data.parallel_size(); ++proc_id)
    {
        if (proc_id != m_bulk_data.parallel_rank())
        {
            while(comm.recv_buffer(proc_id).remaining())
            {
                stk::mesh::impl::ConnectedElementData elementData;
                elementData.m_procId = proc_id;
                comm.recv_buffer(proc_id).unpack<impl::LocalId>(elementData.m_elementId);
                comm.recv_buffer(proc_id).unpack<stk::topology>(elementData.m_elementTopology);
                comm.recv_buffer(proc_id).unpack<unsigned>(elementData.m_sideIndex);
                comm.recv_buffer(proc_id).unpack<stk::mesh::EntityId>(elementData.m_suggestedFaceId);

                unsigned num_side_nodes = 0;
                comm.recv_buffer(proc_id).unpack<unsigned>(num_side_nodes);
                elementData.m_sideNodes.resize(num_side_nodes);
                for(unsigned i=0; i<num_side_nodes; ++i)
                {
                    stk::mesh::EntityKey key;
                    comm.recv_buffer(proc_id).unpack<stk::mesh::EntityKey>(key);
                    elementData.m_sideNodes[i] = m_bulk_data.get_entity(key);
                }

                stk::mesh::EntityVector sortedSideNodes = elementData.m_sideNodes;
                std::sort(sortedSideNodes.begin(), sortedSideNodes.end());
                communicatedElementDataMap[sortedSideNodes].push_back(elementData);
            }
        }
    }

    for (auto & elementData: communicatedElementDataMap) {
        add_possibly_connected_elements_to_graph_using_side_nodes(elem_side_comm, elementData.second);
    }
}

void ElemElemGraph::add_possibly_connected_elements_to_graph_using_side_nodes( const stk::mesh::impl::ElemSideToProcAndFaceId& elemSideComm,
                                                                               stk::mesh::impl::ConnectedElementDataVector & communicatedElementDataVector)
{
    if (communicatedElementDataVector.empty()) {
        return;  // Nothing to connect to
    }

    const stk::mesh::EntityVector sideNodes = communicatedElementDataVector[0].m_sideNodes;  // Pick a convenient set of side nodes (same for all elems)
    unsigned num_side_nodes = sideNodes.size();
    std::set<EntityId> localElementsConnectedToRemoteShell;
    stk::mesh::EntityVector localElements;
    impl::find_locally_owned_elements_these_nodes_have_in_common(m_bulk_data, num_side_nodes, sideNodes.data(), localElements);
    add_local_elements_to_connected_list(localElements, sideNodes, communicatedElementDataVector);

    for (stk::mesh::Entity localElem: localElements) {
        stk::topology localElemTopology = m_bulk_data.bucket(localElem).topology();

        const stk::mesh::Entity* localElemNodes = m_bulk_data.begin_nodes(localElem);
        int localElemNumSides = localElemTopology.num_sides();
        for(int side_index=0; side_index<localElemNumSides; ++side_index)
        {
            unsigned num_nodes_this_side = localElemTopology.side_topology(side_index).num_nodes();
            if (num_nodes_this_side == num_side_nodes)
            {
                stk::mesh::EntityVector localElemSideNodes(num_nodes_this_side);
                localElemTopology.side_nodes(localElemNodes, side_index, localElemSideNodes.begin());

                impl::ConnectedElementDataVector filteredCommunicatedElementData = communicatedElementDataVector;  // Modify copy for this local element
                impl::filter_for_candidate_elements_to_connect(m_bulk_data, localElem, side_index, filteredCommunicatedElementData);

                for (const impl::ConnectedElementData & elemData : filteredCommunicatedElementData) {
                    const bool isRemoteElement = (elemData.m_procId != m_bulk_data.parallel_rank());
                    if (isRemoteElement)
                    {
                        if (elemData.m_elementTopology.is_shell()) {
                            localElementsConnectedToRemoteShell.insert(get_local_element_id(localElem));  // Store connection to error-check later
                        }
                        impl::LocalId local_elem_id = get_local_element_id(localElem);
                        m_elem_graph[local_elem_id].push_back(-1*elemData.m_elementId);
                        m_via_sides[local_elem_id].push_back(side_index);

                        stk::mesh::EntityId chosen_face_id = 0;
                        if(m_bulk_data.identifier(localElem) < static_cast<stk::mesh::EntityId>(elemData.m_elementId))
                        {
                            const auto iterRange = elemSideComm.equal_range(impl::EntitySidePair(localElem, side_index));
                            bool found = false;
                            for (impl::ElemSideToProcAndFaceId::const_iterator iter = iterRange.first; iter != iterRange.second; ++iter) {
                                if ((iter->second.proc != elemData.m_procId) && (iter->first.side_id != elemData.m_sideIndex)) {
                                    continue;
                                }
                                found = true;
                                chosen_face_id = iter->second.face_id;
                                break;
                            }
                            ThrowRequireMsg(found, "Program error. Please contact sierra-help@sandia.gov for support.");
                        }
                        else
                        {
                            chosen_face_id = elemData.m_suggestedFaceId;
                        }

                        std::pair<bool,unsigned> result = localElemTopology.side_topology(side_index).equivalent(localElemSideNodes, elemData.m_sideNodes);
                        m_parallel_graph_info.insert(std::make_pair(std::make_pair(local_elem_id, elemData.m_elementId),
                                                                    impl::parallel_info(elemData.m_procId, elemData.m_sideIndex, result.second, chosen_face_id)));
                    }
                }
            }
        }
    }

    impl::fix_conflicting_shell_connections(localElementsConnectedToRemoteShell, m_elem_graph, m_via_sides);
}

void perform_element_death(stk::mesh::BulkData& bulkData, ElemElemGraph& elementGraph, const stk::mesh::EntityVector& killedElements, stk::mesh::Part& active,
        const stk::mesh::PartVector& boundary_mesh_parts)
{
    const std::vector<stk::mesh::EntityId>& requestedIds = elementGraph.get_suggested_face_ids();
    size_t id_counter = 0;

    std::vector<stk::mesh::sharing_info> shared_modified;
    stk::mesh::EntityVector deletedEntities;

    std::vector<impl::graphEdgeProc> elements_to_comm = impl::get_elements_to_communicate(bulkData, killedElements, elementGraph);
    std::vector<std::pair<stk::mesh::EntityId, stk::mesh::EntityId> > remote_edges;

    impl::communicate_killed_entities(bulkData, elements_to_comm, remote_edges);

    stk::mesh::Part& faces_created_during_death = bulkData.mesh_meta_data().declare_part("faces_created_during_death",
            stk::topology::FACE_RANK, true);

    stk::mesh::PartVector add_parts = boundary_mesh_parts;
    add_parts.push_back(&active);

    bulkData.modification_begin();

    for(size_t re = 0; re < remote_edges.size(); ++re)
    {
        stk::mesh::EntityId local_id = remote_edges[re].first;
        stk::mesh::EntityId remote_id = remote_edges[re].second;

        stk::mesh::Entity element = bulkData.get_entity(stk::topology::ELEM_RANK, local_id);
        bool create_face = true;
        if(!bulkData.bucket(element).member(active))
        {
            create_face = false;
        }

        impl::parallel_info &parallel_edge_info = elementGraph.get_parallel_edge_info(element, remote_id);
        impl::create_or_delete_shared_face(bulkData, parallel_edge_info, elementGraph, element, remote_id, create_face, add_parts,
                shared_modified, deletedEntities, id_counter, requestedIds[id_counter], faces_created_during_death);

        parallel_edge_info.m_in_part = false;
    }

    std::vector<impl::ElementSidePair> element_side_pairs;
    element_side_pairs.reserve(impl::get_element_face_multiplier() * killedElements.size());

    for(size_t k = 0; k < killedElements.size(); ++k)
    {
        stk::mesh::Entity this_elem_entity = killedElements[k];

        for(size_t j = 0; j < elementGraph.get_num_connected_elems(this_elem_entity); ++j)
        {
            if(elementGraph.is_connected_elem_locally_owned(this_elem_entity, j))
            {
                stk::mesh::Entity other_element = elementGraph.get_connected_element(this_elem_entity, j);
                int side_id = elementGraph.get_side_id_to_connected_element(this_elem_entity, j);
                ThrowRequireMsg(side_id != -1, "Program error. Please contact sierra-help@sandia.gov for support.");

                bool is_other_element_alive = bulkData.bucket(other_element).member(active);
                if(is_other_element_alive)
                {
                    // create or delete a face with a particular id

                    stk::topology side_top = bulkData.bucket(this_elem_entity).topology().side_topology(side_id);
                    stk::mesh::PartVector parts = add_parts;
                    parts.push_back(&bulkData.mesh_meta_data().get_topology_root_part(side_top));

                    std::string msg = "Program error. Please contact sierra-help@sandia.gov for support.";

                    stk::mesh::Entity face = stk::mesh::impl::get_face_for_element_side(bulkData, this_elem_entity, side_id);
                    if(!bulkData.is_valid(face))
                    {
                        parts.push_back(&faces_created_during_death);
                        stk::mesh::EntityId face_global_id = requestedIds[id_counter];
                        ++id_counter;
                        ThrowRequireMsg(!impl::is_id_already_in_use_locally(bulkData, bulkData.mesh_meta_data().side_rank(), face_global_id), msg);
                        parts.push_back(&faces_created_during_death);
                        face = stk::mesh::declare_element_side(bulkData, face_global_id, this_elem_entity, side_id, parts);
                    }
                    else
                    {
                        if(bulkData.bucket(face).owned())
                        {
                            bulkData.change_entity_parts(face, parts, stk::mesh::PartVector());
                        }
                    }

                    const stk::mesh::Entity* side_nodes = bulkData.begin_nodes(face);
                    unsigned num_side_nodes = bulkData.num_nodes(face);
                    stk::mesh::EntityVector side_nodes_vec(side_nodes, side_nodes + num_side_nodes);

                    std::pair<stk::mesh::ConnectivityOrdinal, stk::mesh::Permutation> ord_and_perm =
                            stk::mesh::get_ordinal_and_permutation(bulkData, other_element, stk::topology::FACE_RANK, side_nodes_vec);

                    if(ord_and_perm.first == stk::mesh::INVALID_CONNECTIVITY_ORDINAL)
                    {
                        std::ostringstream yo;
                        yo << "Proc: " << bulkData.parallel_rank() << std::endl;
                        yo << "this element: " << bulkData.identifier(this_elem_entity) << std::endl;
                        yo << "other element: " << bulkData.identifier(other_element) << std::endl;
                        yo << "Nodes: ";

                        for(stk::mesh::Entity side_node : side_nodes_vec)
                        {
                            yo << bulkData.identifier(side_node) << " "; // nodes of elem 960 (other_elem: 1, this_elem: 401)
                        }

                        yo << std::endl;
                        std::cerr << yo.str();
                    }

                    ThrowRequireMsg(ord_and_perm.first != stk::mesh::INVALID_CONNECTIVITY_ORDINAL, "yikes!");
                    ThrowRequireMsg(ord_and_perm.second != stk::mesh::INVALID_PERMUTATION, "yikes!");

                    bulkData.declare_relation(other_element, face, ord_and_perm.first, ord_and_perm.second);
                }
                else
                {
                    stk::mesh::Entity face = stk::mesh::impl::get_face_for_element_side(bulkData, this_elem_entity, side_id);
                    if(bulkData.is_valid(face) && bulkData.bucket(face).member(faces_created_during_death))
                    {
                        deletedEntities.push_back(face);
                    }
                }
            }
            else
            {
                // create or delete a face with a particular id

                // stk::mesh::EntityId local_id = bulkData.identifier(this_elem_entity);
                stk::mesh::EntityId remote_id = elementGraph.get_entity_id_of_remote_element(this_elem_entity, j);

                impl::parallel_info &parallel_edge_info = elementGraph.get_parallel_edge_info(this_elem_entity, remote_id);
                bool other_element_active = parallel_edge_info.m_in_part;
                bool create_face = false;
                if(other_element_active)
                {
                    create_face = true;
                }

                impl::create_or_delete_shared_face(bulkData, parallel_edge_info, elementGraph, this_elem_entity, remote_id, create_face, add_parts,
                        shared_modified, deletedEntities, id_counter, requestedIds[id_counter], faces_created_during_death);
            }
        }
    }

    ThrowRequireMsg(id_counter<requestedIds.size(), "Program error. Please contact sierra-help@sandia.gov for support.");
    elementGraph.set_num_face_ids_used(id_counter);
    stk::mesh::impl::delete_entities_and_upward_relations(bulkData, deletedEntities);
    bulkData.modification_end_for_face_creation_and_deletion(shared_modified, deletedEntities);
}

void ElemElemGraph::add_local_elements_to_connected_list(const stk::mesh::EntityVector & connected_elements,
                                                         const stk::mesh::EntityVector & sideNodes,
                                                         impl::ConnectedElementDataVector & connectedElementDataVector) const
{
    for (const stk::mesh::Entity & connectedElem: connected_elements) {
        impl::ConnectedElementData elemData;
        stk::mesh::EntityVector connectedSideNodes;
        stk::mesh::OrdinalAndPermutation connectedOrdAndPerm = stk::mesh::get_ordinal_and_permutation(m_bulk_data, connectedElem, stk::topology::FACE_RANK, sideNodes);
        const stk::mesh::Bucket & connectedBucket = m_bulk_data.bucket(connectedElem);
        const stk::mesh::Entity* connectedElemNodes = m_bulk_data.begin_nodes(connectedElem);

        elemData.m_procId = m_bulk_data.parallel_rank();
        elemData.m_elementId = get_local_element_id(connectedElem);
        if (elemData.m_elementId == ElemGraphLocalId::INVALID_ID)
        {
            continue;
        }
        elemData.m_elementTopology = connectedBucket.topology();
        elemData.m_sideIndex = connectedOrdAndPerm.first;
        elemData.m_suggestedFaceId = 0;
        elemData.m_sideNodes.resize(sideNodes.size());
        elemData.m_elementTopology.side_nodes(connectedElemNodes, elemData.m_sideIndex, elemData.m_sideNodes.begin());
        connectedElementDataVector.push_back(elemData);

        if (elemData.m_elementTopology.is_shell()) {
            // Also add the back-side face if this is a shell
            elemData.m_sideIndex = (elemData.m_sideIndex == 0) ? 1 : 0;
            elemData.m_elementTopology.side_nodes(connectedElemNodes, elemData.m_sideIndex, elemData.m_sideNodes.begin());
            connectedElementDataVector.push_back(elemData);
        }
    }
}

struct moved_parallel_graph_info {

    moved_parallel_graph_info(int in_proc_to_tell, stk::mesh::EntityId in_elem_id, stk::mesh::EntityId in_moved_elem_id, int in_destination_proc)
     : proc_to_tell(in_proc_to_tell), elem_id(in_elem_id), moved_elem_id(in_moved_elem_id), destination_proc(in_destination_proc) {};

    int proc_to_tell;
    stk::mesh::EntityId elem_id;
    stk::mesh::EntityId moved_elem_id;
    int destination_proc;
};

void ElemElemGraph::change_entity_owner(const stk::mesh::EntityProcVec &elem_proc_pairs_to_move, impl::ParallelGraphInfo &new_parallel_graph_entries)
{
    std::vector <moved_parallel_graph_info> moved_graph_info_vector;

    stk::CommSparse comm(m_bulk_data.parallel());
    for(int phase=0; phase <2; ++phase) {
        for (size_t i=0; i<elem_proc_pairs_to_move.size(); i++)
        {
            stk::mesh::Entity elem_to_send = elem_proc_pairs_to_move[i].first;
            int destination_proc = elem_proc_pairs_to_move[i].second;
            stk::CommBuffer &buff = comm.send_buffer(destination_proc);
            stk::mesh::EntityId elem_global_id = m_bulk_data.identifier(elem_to_send);

            buff.pack<stk::mesh::EntityId>(elem_global_id);
            impl::LocalId elem_local_id = m_entity_to_local_id[elem_to_send.local_offset()];
            std::vector <impl::LocalId> &connected_elements = m_elem_graph[elem_local_id];
            size_t num_connected_elements = connected_elements.size();
            buff.pack<size_t>(num_connected_elements);
            for (size_t k=0; k<num_connected_elements; k++)
            {
                impl::LocalId local_id = connected_elements[k];
                stk::mesh::EntityId connected_global_id;
                bool local_connection = local_id >= 0;
                buff.pack<bool>(local_connection);
                if (!local_connection)
                {
                    connected_global_id = -local_id;
                }
                else
                {
                    stk::mesh::Entity connected_element = m_local_id_to_element_entity[local_id];
                    connected_global_id = m_bulk_data.identifier(connected_element);
                }
                buff.pack<stk::mesh::EntityId>(connected_global_id);
                buff.pack<int>(m_via_sides[elem_local_id][k]);
                if (!local_connection)
                {
                    std::pair<impl::LocalId, stk::mesh::EntityId> key(elem_local_id, connected_global_id);
                    auto iter = m_parallel_graph_info.find(key);
                    ThrowRequire(iter != m_parallel_graph_info.end());
                    impl::parallel_info &p_info = iter->second;
                    buff.pack<int>(p_info.m_other_proc);
                    buff.pack<int>(p_info.m_other_side_ord);
                    buff.pack<int>(p_info.m_permutation);
                    buff.pack<bool>(p_info.m_in_part);
                    buff.pack<stk::mesh::EntityId>(p_info.m_chosen_face_id);

                    if (phase == 0 && p_info.m_other_proc != destination_proc)
                    {
                        moved_graph_info_vector.push_back(moved_parallel_graph_info(p_info.m_other_proc, connected_global_id, elem_global_id, destination_proc));
                    }

                    if (phase == 1)
                    {
                        m_parallel_graph_info.erase(iter);
                    }
                }
                else
                {
                    stk::mesh::Entity connected_element = m_local_id_to_element_entity[local_id];
                    int side_id = get_side_from_element1_to_locally_owned_element2(connected_element, elem_to_send);
                    buff.pack<int>(m_bulk_data.parallel_rank());
                    buff.pack<int>(side_id);
                    stk::topology elem_topology = m_bulk_data.bucket(connected_element).topology();
                    stk::topology side_topology = elem_topology.side_topology(side_id);
                    std::vector<stk::mesh::Entity> side_nodes(side_topology.num_nodes());
                    const stk::mesh::Entity *elem_nodes = m_bulk_data.begin_nodes(connected_element);
                    stk::mesh::EntityRank side_rank = m_bulk_data.mesh_meta_data().side_rank();
                    elem_topology.side_nodes(elem_nodes, side_id, side_nodes.begin());
                    stk::mesh::OrdinalAndPermutation ordperm = get_ordinal_and_permutation(m_bulk_data, connected_element, side_rank, side_nodes);

                    buff.pack<int>(ordperm.second);
                    buff.pack<bool>(true);//fake data, need to check actual part membership
                    buff.pack<stk::mesh::EntityId>(999); //fake data, need to get actual face id
                }
            }

            if (phase == 1)
            {
                for (size_t j=0; j<m_elem_graph.size(); j++)
                {
                    if (j == static_cast <size_t> (elem_local_id))
                    {
                        continue;
                    }
                    std::vector <impl::LocalId> &connected_elements = m_elem_graph[j];
                    auto iter = std::find(connected_elements.begin(), connected_elements.end(), elem_local_id);
                    if (iter != connected_elements.end())
                    {
                        int index = iter - connected_elements.begin();
                        connected_elements[index] = -elem_global_id;
                    }
                }
                m_elem_graph[elem_local_id].clear();
                m_deleted_element_local_id_pool.push_back(elem_local_id);
                m_local_id_in_pool[elem_local_id] = true;
            }
        }

        if (phase == 0)
        {
            comm.allocate_buffers();
        }
        else
        {
            comm.communicate();
        }
    }

    for(int p = 0; p < m_bulk_data.parallel_size(); ++p)
    {
        stk::CommBuffer & buf = comm.recv_buffer(p);
        while(buf.remaining())
        {
            stk::mesh::EntityId recvd_elem_global_id;
            buf.unpack<stk::mesh::EntityId>(recvd_elem_global_id);
            stk::mesh::Entity recvd_elem = m_bulk_data.get_entity(stk::topology::ELEM_RANK, recvd_elem_global_id);
            impl::LocalId recvd_elem_local_id = m_elem_graph.size();
            if (m_deleted_element_local_id_pool.size() > 0)
            {
                recvd_elem_local_id = m_deleted_element_local_id_pool.back();
                m_local_id_in_pool[recvd_elem_local_id] = false;
                m_deleted_element_local_id_pool.pop_back();
            }
            else
            {
                if (m_local_id_to_element_entity.size() <= static_cast<size_t> (recvd_elem_local_id))
                {
                    m_local_id_to_element_entity.resize(recvd_elem_local_id+1);
                }
                if (m_entity_to_local_id.size() <= recvd_elem.local_offset())
                {
                    m_entity_to_local_id.resize(recvd_elem.local_offset()+1);
                }
                m_elem_graph.push_back(std::vector<impl::LocalId>());
                m_via_sides.push_back(std::vector<int>());
            }
            m_local_id_to_element_entity[recvd_elem_local_id] = recvd_elem;
            m_entity_to_local_id[recvd_elem.local_offset()] = recvd_elem_local_id;
            size_t num_connected_elements;
            buf.unpack<size_t>(num_connected_elements);
            for (size_t k=0; k<num_connected_elements; k++)
            {
                bool local_connection_on_source_proc;
                buf.unpack<bool>(local_connection_on_source_proc);
                stk::mesh::EntityId connected_elem_global_id;
                buf.unpack<stk::mesh::EntityId>(connected_elem_global_id);
                int side_from_recvd_elem_to_connected_elem;
                buf.unpack<int>(side_from_recvd_elem_to_connected_elem);
                stk::mesh::Entity connected_elem = m_bulk_data.get_entity(stk::topology::ELEM_RANK, connected_elem_global_id);
                int other_proc, side_from_connected_elem_to_recvd_elem, other_permutation;
                bool in_part;
                stk::mesh::EntityId chosen_face_id;
                buf.unpack<int>(other_proc);
                buf.unpack<int>(side_from_connected_elem_to_recvd_elem);
                buf.unpack<int>(other_permutation);
                buf.unpack<bool>(in_part);
                buf.unpack<stk::mesh::EntityId>(chosen_face_id);
                impl::parallel_info p_info(other_proc, side_from_connected_elem_to_recvd_elem, other_permutation, chosen_face_id);
                p_info.m_in_part = in_part;
                if (p_info.m_other_proc == m_bulk_data.parallel_rank())
                {
                    impl::LocalId connected_elem_local_id = get_local_element_id(connected_elem);
                    m_elem_graph[recvd_elem_local_id].push_back(connected_elem_local_id);
                    std::pair<impl::LocalId, stk::mesh::EntityId> key(connected_elem_local_id, recvd_elem_global_id);
                    auto iter = m_parallel_graph_info.find(key);
                    if (iter != m_parallel_graph_info.end())
                    {
                        m_parallel_graph_info.erase(iter);
                    }
                }
                else
                {
                    m_elem_graph[recvd_elem_local_id].push_back(-connected_elem_global_id);
                    std::pair<impl::LocalId, stk::mesh::EntityId> recvd_elem_key(recvd_elem_local_id, connected_elem_global_id);
                    m_parallel_graph_info.insert(std::make_pair(recvd_elem_key, p_info));
                }
                m_via_sides[recvd_elem_local_id].push_back(side_from_recvd_elem_to_connected_elem);
            }
        }
    }
    auto iter_begin = new_parallel_graph_entries.begin();
    auto iter_end   = new_parallel_graph_entries.end();
    for (auto iter=iter_begin; iter!=iter_end; ++iter)
    {
        m_parallel_graph_info.insert(std::make_pair(iter->first, iter->second));
    }

    stk::CommSparse comm2(m_bulk_data.parallel());
    for(int phase=0; phase <2; ++phase) {
        for (size_t i=0; i<moved_graph_info_vector.size(); ++i)
        {
            moved_parallel_graph_info &info = moved_graph_info_vector[i];
            stk::CommBuffer &buf = comm2.send_buffer(info.proc_to_tell);

            buf.pack<stk::mesh::EntityId>(info.elem_id);
            buf.pack<stk::mesh::EntityId>(info.moved_elem_id);
            buf.pack<int>(info.destination_proc);
        }
        if (phase == 0)
        {
            comm2.allocate_buffers();
        }
        else
        {
            comm2.communicate();
        }
    }

    for(int p = 0; p < m_bulk_data.parallel_size(); ++p)
    {
        stk::CommBuffer & buf = comm2.recv_buffer(p);
        while(buf.remaining())
        {
            stk::mesh::EntityId elem_id, moved_elem_id;
            int destination_proc;
            buf.unpack<stk::mesh::EntityId>(elem_id);
            buf.unpack<stk::mesh::EntityId>(moved_elem_id);
            buf.unpack<int>(destination_proc);

            stk::mesh::Entity local_elem = m_bulk_data.get_entity(stk::topology::ELEM_RANK, elem_id);
            impl::LocalId elem_local_id = get_local_element_id(local_elem);
            std::pair<impl::LocalId, stk::mesh::EntityId> key(elem_local_id, moved_elem_id);
            auto iter = m_parallel_graph_info.find(key);
            if (iter != m_parallel_graph_info.end())
            {
                iter->second.m_other_proc = destination_proc;
            }
        }
    }
}

impl::LocalId ElemElemGraph::get_new_local_element_id_from_pool()
{
    impl::LocalId new_local_id;
    if (!m_deleted_element_local_id_pool.empty())
    {
        new_local_id = m_deleted_element_local_id_pool.back();
        m_deleted_element_local_id_pool.pop_back();
        m_local_id_in_pool[new_local_id] = false;
    }
    else
    {
        new_local_id = m_elem_graph.size();
        std::vector<impl::LocalId> new_element_connectivity;
        std::vector<int> new_element_via_sides;
        m_elem_graph.push_back(new_element_connectivity);
        m_via_sides.push_back(new_element_via_sides);
        m_local_id_in_pool.push_back(false);
        m_local_id_to_element_entity.push_back(Entity());
        m_element_topologies.push_back(stk::topology::INVALID_TOPOLOGY);
    }
    return new_local_id;
}

bool ElemElemGraph::is_valid_graph_element(stk::mesh::Entity local_element)
{
    impl::LocalId max_elem_id = static_cast<impl::LocalId>(m_elem_graph.size());
    impl::LocalId elem_id = get_local_element_id(local_element);
    return (elem_id >= 0 && elem_id < max_elem_id && !m_local_id_in_pool[elem_id]);
}

void ElemElemGraph::delete_elements_from_graph(std::vector<stk::mesh::Entity> &elements_to_delete)
{
    for (size_t elem_index = 0; elem_index < elements_to_delete.size(); ++elem_index)
    {
        stk::mesh::Entity elem_to_delete = elements_to_delete[elem_index];
        ThrowRequireMsg(m_bulk_data.is_valid(elem_to_delete), "Program error. Not valid mesh element. Contact sierra-help@sandia.gov for support.");

        if (m_bulk_data.bucket(elem_to_delete).owned())
        {
            impl::LocalId elem_to_delete_id = get_local_element_id(elem_to_delete);
            ThrowRequireMsg(is_valid_graph_element(elem_to_delete), "Program error. Not valid graph element. Contact sierra-help@sandia.gov for support.");
            m_deleted_element_local_id_pool.push_back(elem_to_delete_id);
            m_local_id_in_pool[elem_to_delete_id] = true;

            size_t num_connected_elems = get_num_connected_elems(elem_to_delete);
            for (int conn_elem_index = num_connected_elems-1; conn_elem_index >= 0; --conn_elem_index)
            {
                impl::LocalId connected_elem_id = m_elem_graph[elem_to_delete_id][conn_elem_index];

                std::vector <impl::LocalId>::iterator pos_of_elem2_in_elem1 = std::find(m_elem_graph[elem_to_delete_id].begin(), m_elem_graph[elem_to_delete_id].end(), connected_elem_id);
                ThrowRequire(pos_of_elem2_in_elem1 != m_elem_graph[elem_to_delete_id].end());
                int index_of_elem2_in_elem1 = pos_of_elem2_in_elem1 - m_elem_graph[elem_to_delete_id].begin();
                m_via_sides[elem_to_delete_id].erase(m_via_sides[elem_to_delete_id].begin() + index_of_elem2_in_elem1);

                std::vector <impl::LocalId>::iterator pos_of_elem1_in_elem2 = std::find(m_elem_graph[connected_elem_id].begin(), m_elem_graph[connected_elem_id].end(), elem_to_delete_id);
                ThrowRequire(pos_of_elem1_in_elem2 != m_elem_graph[connected_elem_id].end());
                int index_of_elem1_in_elem2 = pos_of_elem1_in_elem2 - m_elem_graph[connected_elem_id].begin();
                m_via_sides[connected_elem_id].erase(m_via_sides[connected_elem_id].begin() + index_of_elem1_in_elem2);
            }

            ThrowRequire(m_via_sides[elem_to_delete_id].empty());

            for (size_t id = 0; id < size(); ++id)
            {
                if (id != static_cast<size_t>(elem_to_delete_id))
                {
                    std::vector <impl::LocalId>::iterator pos_of_deleted_elem_in_current_id = std::find(m_elem_graph[id].begin(), m_elem_graph[id].end(), elem_to_delete_id);
                    if (pos_of_deleted_elem_in_current_id != m_elem_graph[id].end())
                    {
                        int index_of_deleted_elem_in_current_id = pos_of_deleted_elem_in_current_id - m_elem_graph[id].begin();
                        m_elem_graph[id].erase(m_elem_graph[id].begin() + index_of_deleted_elem_in_current_id);
                    }
                }
            }

            m_elem_graph[elem_to_delete_id].clear();
            m_via_sides[elem_to_delete_id].clear();
        }
        else
        {
           stk::mesh::EntityId elem_to_delete_entity_id = m_bulk_data.identifier(elem_to_delete);
           for (size_t id = 0; id < size(); ++id)
           {
               size_t num_conn_elem = get_num_connected_elems(elem_to_delete);
               for (size_t conn_elem_index = 0; conn_elem_index < num_conn_elem; ++conn_elem_index)
               {
                   if (m_elem_graph[id][conn_elem_index] == static_cast<int64_t>(-elem_to_delete_entity_id))
                   {
                       m_elem_graph[id].erase(m_elem_graph[id].begin() + conn_elem_index);
                       m_via_sides[id].erase(m_via_sides[id].begin() + conn_elem_index);
                   }
               }
           }
        }
    }
}

stk::mesh::ConnectivityOrdinal ElemElemGraph::get_neighboring_side_ordinal(const stk::mesh::BulkData &mesh,
                                                                           stk::mesh::Entity currentElem,
                                                                           stk::mesh::ConnectivityOrdinal currentOrdinal,
                                                                           stk::mesh::Entity neighborElem)
{
    stk::topology currentElemTopology = mesh.bucket(currentElem).topology();
    stk::topology currentFaceTopology = currentElemTopology.face_topology(currentOrdinal);
    const stk::mesh::Entity* currentElemNodes = mesh.begin_nodes(currentElem);
    stk::mesh::EntityVector currentElemSideNodes(currentFaceTopology.num_nodes());
    currentElemTopology.side_nodes(currentElemNodes, currentOrdinal, currentElemSideNodes.begin());

    stk::topology neighborTopology = mesh.bucket(neighborElem).topology();
    stk::mesh::EntityVector neighborSideNodes;

    bool foundNeighborOrdinal = false;
    unsigned neighborOrdinal = 0;
    for (; neighborOrdinal < neighborTopology.num_faces(); ++neighborOrdinal)
    {
        stk::topology neighborFaceTopology = neighborTopology.face_topology(neighborOrdinal);
        neighborSideNodes.resize(neighborFaceTopology.num_nodes());
        const stk::mesh::Entity* neighborNodes = mesh.begin_nodes(neighborElem);
        neighborTopology.side_nodes(neighborNodes, neighborOrdinal, neighborSideNodes.begin());
        std::pair<bool,unsigned> result = neighborFaceTopology.equivalent(currentElemSideNodes, neighborSideNodes);

        if (result.first && result.second >= neighborFaceTopology.num_positive_permutations())
        {
            foundNeighborOrdinal = true;
            break;
        }
    }
    ThrowRequireMsg(foundNeighborOrdinal, "Error: neighborElem is not a true neighbor of currentElem.");
    return static_cast<stk::mesh::ConnectivityOrdinal>(neighborOrdinal);
}

size_t ElemElemGraph::find_max_local_offset_in_neighborhood(stk::mesh::Entity element)
{
    stk::mesh::EntityVector side_nodes;
    stk::mesh::EntityVector connected_elements;
    stk::mesh::Bucket &elem_bucket = m_bulk_data.bucket(element);
    stk::topology topology = elem_bucket.topology();
    size_t num_sides = topology.num_sides();
    const stk::mesh::Entity* elem_nodes = elem_bucket.begin_nodes(m_bulk_data.bucket_ordinal(element));
    size_t max_local_offset = 0;
    size_t current_offset = element.local_offset();
    if (current_offset > max_local_offset)
    {
        max_local_offset = current_offset;
    }
    for (size_t side_index = 0; side_index < num_sides; ++side_index)
    {
        unsigned num_side_nodes = topology.side_topology(side_index).num_nodes();
        side_nodes.resize(num_side_nodes);
        topology.side_nodes(elem_nodes, side_index, side_nodes.begin());
        connected_elements.clear();
        impl::ConnectedElementDataVector connectedElementDataVector;
        impl::find_locally_owned_elements_these_nodes_have_in_common(m_bulk_data, num_side_nodes, side_nodes.data(), connected_elements);

        for (stk::mesh::Entity & connected_element : connected_elements)
        {
            current_offset = connected_element.local_offset();
            if (current_offset > max_local_offset)
            {
                max_local_offset = current_offset;
            }
        }
    }
    return max_local_offset;
}

void ElemElemGraph::add_elements_to_graph(std::vector <stk::mesh::Entity> &elements_to_add)
{
    size_t max_offset = 0;
    for (stk::mesh::Entity & element_to_add : elements_to_add)
    {
        size_t local_max = find_max_local_offset_in_neighborhood(element_to_add);
        if (local_max > max_offset)
        {
            max_offset = local_max;
        }
    }
    ensure_space_in_entity_to_local_id(max_offset);

    std::vector<impl::ElementSidePair> elem_side_pairs;

    for(unsigned i=0; i<elements_to_add.size(); ++i)
    {
        stk::mesh::Entity elem_to_add = elements_to_add[i];
        if (!m_bulk_data.bucket(elem_to_add).owned())
        {
            continue;
        }
        ThrowRequire(!is_valid_graph_element(elem_to_add));
        impl::LocalId new_elem_id = get_new_local_element_id_from_pool();
        m_local_id_to_element_entity[new_elem_id] = elem_to_add;
        m_entity_to_local_id[elem_to_add.local_offset()] = new_elem_id;
        stk::topology elem_topology = m_bulk_data.bucket(elem_to_add).topology();
        m_element_topologies[new_elem_id] = elem_topology;
        get_element_side_pairs(m_bulk_data.mesh_index(elem_to_add), new_elem_id, elem_side_pairs);
        for(size_t index=0; index<elem_side_pairs.size(); ++index)
        {
            stk::mesh::Entity neighbor = m_local_id_to_element_entity[elem_side_pairs[index].first];
            if (is_valid_graph_element(neighbor))
            {
                m_elem_graph[new_elem_id].push_back(elem_side_pairs[index].first);
                m_via_sides[new_elem_id].push_back(elem_side_pairs[index].second);
                impl::LocalId neighbor_id = m_entity_to_local_id[neighbor.local_offset()];
                stk::mesh::ConnectivityOrdinal currentOrdinal = static_cast<stk::mesh::ConnectivityOrdinal>(elem_side_pairs[index].second);
                stk::mesh::ConnectivityOrdinal neighborOrdinal = get_neighboring_side_ordinal(m_bulk_data, elem_to_add, currentOrdinal, neighbor);
                m_elem_graph[neighbor_id].push_back(new_elem_id);
                m_via_sides[neighbor_id].push_back(neighborOrdinal);
                m_num_edges+=2;
            }
        }
    }
}

}} // end namespaces stk mesh

