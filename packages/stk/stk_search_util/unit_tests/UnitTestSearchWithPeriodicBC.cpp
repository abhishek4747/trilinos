#include <stk_util/unit_test_support/stk_utest_macros.hpp>
#include <stk_mesh/fixtures/HexFixture.hpp>
#include <stk_search_util/stk_mesh/PeriodicBoundarySearch.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>

typedef stk::mesh::fixtures::HexFixture::CoordFieldType CoordFieldType;

namespace {

template<typename SearchPairVector>
void check_gold( const SearchPairVector & search_results )
  // check search result
{
  typedef std::vector<std::pair<stk::mesh::EntityId,stk::mesh::EntityId> > GoldVector;
  GoldVector gold;
  gold.push_back(std::make_pair(1,4));
  gold.push_back(std::make_pair(5,8));
  gold.push_back(std::make_pair(9,12));
  gold.push_back(std::make_pair(13,16));

  gold.push_back(std::make_pair(17,20));
  gold.push_back(std::make_pair(21,24));
  gold.push_back(std::make_pair(25,28));
  gold.push_back(std::make_pair(29,32));

  gold.push_back(std::make_pair(33,36));
  gold.push_back(std::make_pair(37,40));
  gold.push_back(std::make_pair(41,44));
  gold.push_back(std::make_pair(45,48));

  gold.push_back(std::make_pair(49,52));
  gold.push_back(std::make_pair(53,56));
  gold.push_back(std::make_pair(57,60));
  gold.push_back(std::make_pair(61,64));

  //make sure search result shows up in gold
  for (size_t i=0, size=search_results.size(); i<size; ++i) {
    stk::mesh::EntityId domain_node = search_results[i].first.ident.id();
    stk::mesh::EntityId range_node = search_results[i].second.ident.id();

    //entry in search is found in gold
    EXPECT_TRUE((std::find(gold.begin(),gold.end(),std::make_pair(domain_node,range_node))) != gold.end());
  }
}

template<typename SearchPairVector>
void check_gold_multiperiodic( const SearchPairVector & search_results )
  // check search result
{
  typedef std::vector<std::pair<stk::mesh::EntityId,stk::mesh::EntityId> > GoldVector;
  GoldVector gold;
  gold.push_back(std::make_pair(1,4));
  gold.push_back(std::make_pair(5,8));
  gold.push_back(std::make_pair(9,12));
  gold.push_back(std::make_pair(13,16));

  gold.push_back(std::make_pair(17,20));
  gold.push_back(std::make_pair(21,24));
  gold.push_back(std::make_pair(25,28));
  gold.push_back(std::make_pair(29,32));

  gold.push_back(std::make_pair(33,36));
  gold.push_back(std::make_pair(37,40));
  gold.push_back(std::make_pair(41,44));
  gold.push_back(std::make_pair(45,48));

  gold.push_back(std::make_pair(49,52));
  gold.push_back(std::make_pair(53,56));
  gold.push_back(std::make_pair(57,60));
  gold.push_back(std::make_pair(61,64));

  //top and bottom
  gold.push_back(std::make_pair(1,13));
  gold.push_back(std::make_pair(2,14));
  gold.push_back(std::make_pair(3,15));
  gold.push_back(std::make_pair(4,16));

  gold.push_back(std::make_pair(17,29));
  gold.push_back(std::make_pair(18,30));
  gold.push_back(std::make_pair(19,31));
  gold.push_back(std::make_pair(20,32));

  gold.push_back(std::make_pair(33,45));
  gold.push_back(std::make_pair(34,46));
  gold.push_back(std::make_pair(35,47));
  gold.push_back(std::make_pair(36,48));

  gold.push_back(std::make_pair(49,61));
  gold.push_back(std::make_pair(50,62));
  gold.push_back(std::make_pair(51,63));
  gold.push_back(std::make_pair(52,64));

  //edge cases
  gold.push_back(std::make_pair(1,16));
  gold.push_back(std::make_pair(17,32));
  gold.push_back(std::make_pair(33,48));
  gold.push_back(std::make_pair(49,64));



  for (size_t i=0, size=search_results.size(); i<size; ++i) {
    stk::mesh::EntityId domain_node = search_results[i].first.ident.id();
    stk::mesh::EntityId range_node = search_results[i].second.ident.id();

    EXPECT_TRUE((std::find(gold.begin(), gold.end(), std::make_pair(domain_node,range_node) ) ) != gold.end());
  }
}


}// namespace

STKUNIT_UNIT_TEST(CoarseSearch, PeriodicBC)
{
  const unsigned x = 3, y = 3, z = 3;


  stk::mesh::fixtures::HexFixture fixture(MPI_COMM_WORLD, x, y, z);

  stk::mesh::BulkData & bulk_data = fixture.m_bulk_data;
  stk::mesh::MetaData & meta_data = fixture.m_meta;
  CoordFieldType & coords_field = fixture.m_coord_field;

  stk::mesh::Part & side_0 = meta_data.declare_part("side_0", stk::topology::NODE_RANK);
  stk::mesh::Part & side_3 = meta_data.declare_part("side_3", stk::topology::NODE_RANK);

  meta_data.commit();

  fixture.generate_mesh();

  // side 0 (master) is periodic with side 3 (slave)


  // add nodes to side 0 and 3

  stk::mesh::PartVector side_0_parts(1,&side_0);
  stk::mesh::PartVector side_3_parts(1,&side_3);

  bulk_data.modification_begin();
  for (unsigned i=0; i<y+1u; ++i) {
  for (unsigned j=0; j<z+1u; ++j) {
    stk::mesh::Entity node_0 = fixture.node(0,i,j);
    if (bulk_data.is_valid(node_0)  && bulk_data.bucket(node_0).owned()) {
      bulk_data.change_entity_parts( fixture.node(0,i,j), side_0_parts);
    }
    stk::mesh::Entity node_3 = fixture.node(3,i,j);
    if (bulk_data.is_valid(node_3)  && bulk_data.bucket(node_3).owned()) {
      bulk_data.change_entity_parts( fixture.node(3,i,j), side_3_parts);
    }
  }}
  bulk_data.modification_end();

  //do periodic search
  typedef stk::mesh::GetCoordinates<CoordFieldType> CoordinateFunctor;
  typedef stk::mesh::PeriodicBoundarySearch<CoordinateFunctor> PeriodicSearch;
  PeriodicSearch pbc_search(bulk_data, CoordinateFunctor(bulk_data, coords_field));

  pbc_search.add_periodic_pair(side_0 & meta_data.locally_owned_part(), side_3 & meta_data.locally_owned_part() );
  pbc_search.find_periodic_nodes(bulk_data.parallel());


  check_gold(pbc_search.get_pairs() );

  //also check the number
  const int local_search_count = pbc_search.get_pairs().size();
  int global_search_count=0;
  stk::all_reduce_sum(bulk_data.parallel(), &local_search_count, &global_search_count, 1);

  EXPECT_EQ(global_search_count, 16);

  bulk_data.modification_begin();
  pbc_search.create_ghosting("periodic_ghosts");
  bulk_data.modification_end();

  const stk::mesh::Ghosting & periodic_bc_ghosting = pbc_search.get_ghosting();


  std::vector< stk::mesh::FieldBase const * > ghosted_fields;
  ghosted_fields.push_back(&coords_field);
  stk::mesh::communicate_field_data( periodic_bc_ghosting, ghosted_fields);

  pbc_search.find_periodic_nodes(MPI_COMM_SELF);

  check_gold(pbc_search.get_pairs());
}



STKUNIT_UNIT_TEST(CoarseSearch, MultiPeriodicBC)
{
  const unsigned x = 3, y = 3, z = 3;


  stk::mesh::fixtures::HexFixture fixture(MPI_COMM_WORLD, x, y, z);

  stk::mesh::BulkData & bulk_data = fixture.m_bulk_data;
  stk::mesh::MetaData & meta_data = fixture.m_meta;
  CoordFieldType & coords_field = fixture.m_coord_field;

  stk::mesh::Part & side_0 = meta_data.declare_part("side_0", stk::topology::NODE_RANK);
  stk::mesh::Part & side_2 = meta_data.declare_part("side_2", stk::topology::NODE_RANK);

  stk::mesh::Part & side_1 = meta_data.declare_part("side_1", stk::topology::NODE_RANK);
  stk::mesh::Part & side_3 = meta_data.declare_part("side_3", stk::topology::NODE_RANK);

  meta_data.commit();

  fixture.generate_mesh();

  // side 0 (master) is periodic with side 3 (slave)


  // add nodes to side 0 and 3

  stk::mesh::PartVector side_0_parts(1,&side_0);
  stk::mesh::PartVector side_2_parts(1,&side_2);

  stk::mesh::PartVector side_1_parts(1,&side_1);
  stk::mesh::PartVector side_3_parts(1,&side_3);

  //add periodic pair for side 0 and side 2
  bulk_data.modification_begin();
  for (unsigned i=0; i<y+1u; ++i) {
  for (unsigned j=0; j<z+1u; ++j) {
    stk::mesh::Entity node_0 = fixture.node(0,i,j);
    if (bulk_data.is_valid(node_0)  && bulk_data.bucket(node_0).owned()) {
      bulk_data.change_entity_parts( fixture.node(0,i,j), side_0_parts);
    }
    stk::mesh::Entity node_2 = fixture.node(3,i,j);
    if (bulk_data.is_valid(node_2)  && bulk_data.bucket(node_2).owned()) {
      bulk_data.change_entity_parts( fixture.node(3,i,j), side_2_parts);
    }
  }}
  bulk_data.modification_end();

  //add periodic pair for side 1 and side 3
  bulk_data.modification_begin();
  for (unsigned i=0; i<y+1u; ++i) {
  for (unsigned j=0; j<z+1u; ++j) {
    stk::mesh::Entity node_1 = fixture.node(i,0,j);
    if (bulk_data.is_valid(node_1)  && bulk_data.bucket(node_1).owned()) {
      bulk_data.change_entity_parts( fixture.node(i,0,j), side_1_parts);
    }
    stk::mesh::Entity node_3 = fixture.node(i,3,j);
    if (bulk_data.is_valid(node_3)  && bulk_data.bucket(node_3).owned()) {
      bulk_data.change_entity_parts( fixture.node(i,3,j), side_3_parts);
    }
  }}
  bulk_data.modification_end();

  //do periodic search
  typedef stk::mesh::GetCoordinates<CoordFieldType> CoordinateFunctor;
  typedef stk::mesh::PeriodicBoundarySearch<CoordinateFunctor> PeriodicSearch;
  PeriodicSearch pbc_search(bulk_data, CoordinateFunctor(bulk_data, coords_field));

  pbc_search.add_periodic_pair(side_0 & meta_data.locally_owned_part(), side_2 & meta_data.locally_owned_part() );
  pbc_search.add_periodic_pair(side_1 & meta_data.locally_owned_part(), side_3 & meta_data.locally_owned_part() );
  pbc_search.find_periodic_nodes(bulk_data.parallel());

  check_gold_multiperiodic(pbc_search.get_pairs());

  //also check the number
  const int local_search_count = pbc_search.get_pairs().size();
  int global_search_count=0;
  stk::all_reduce_sum(bulk_data.parallel(), &local_search_count, &global_search_count, 1);

  //has 32 "normal" and then 4 edge (corner to corner)
  EXPECT_EQ(global_search_count, 36);

  //now we ghost everything to do a local search
  bulk_data.modification_begin();
  pbc_search.create_ghosting("periodic_ghosts");
  bulk_data.modification_end();

  const stk::mesh::Ghosting & periodic_bc_ghosting = pbc_search.get_ghosting();

  std::vector< stk::mesh::FieldBase const * > ghosted_fields;
  ghosted_fields.push_back(&coords_field);
  stk::mesh::communicate_field_data( periodic_bc_ghosting, ghosted_fields);

  //do a local search only to verify ghosting
  pbc_search.find_periodic_nodes(MPI_COMM_SELF);

  check_gold_multiperiodic(pbc_search.get_pairs());
}
