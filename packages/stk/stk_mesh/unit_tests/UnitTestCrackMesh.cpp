/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <stdexcept>

#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_mesh/fixtures/QuadFixture.hpp>
#include <stk_mesh/fixtures/HexFixture.hpp>

//----------------------------------------------------------------------------

STKUNIT_UNIT_TEST ( UnitTestCrackMesh , VerifyDestroy2D )
{
  // In 2D, build a fresh 3x3 mesh each loop iteration, destroying a different
  // single element each time.

  stk::ParallelMachine pm = MPI_COMM_WORLD ;
  const unsigned p_rank = stk::parallel_machine_rank( pm );

  const unsigned nx = 3 , ny = 3 ;

  for ( unsigned iy = 0 ; iy < ny ; ++iy ) {
  for ( unsigned ix = 0 ; ix < nx ; ++ix ) {
    stk::mesh::fixtures::QuadFixture fixture( pm , nx , ny );
    fixture.m_fem_meta.commit();
    fixture.generate_mesh();

    fixture.m_bulk_data.modification_begin();

    stk::mesh::Entity elem = fixture.elem( ix , iy );

    if ( elem.is_valid() && p_rank == elem.owner_rank() ) {
      stk::mesh::Entity tmp = elem ;
      fixture.m_bulk_data.destroy_entity( tmp );
    }

    fixture.m_bulk_data.modification_end();
  }
  }
}

STKUNIT_UNIT_TEST ( UnitTestCrackMesh , VerifyDestroy3D )
{
  // In 3D, build a 3x3x3 mesh each loop iteration, destroying a different
  // single element each time.

  stk::ParallelMachine pm = MPI_COMM_WORLD ;
  const unsigned p_rank = stk::parallel_machine_rank( pm );

  const unsigned nx = 3 , ny = 3 , nz = 3 ;

  for ( unsigned iz = 0 ; iz < nz ; ++iz ) {
  for ( unsigned iy = 0 ; iy < ny ; ++iy ) {
  for ( unsigned ix = 0 ; ix < nx ; ++ix ) {
    stk::mesh::fixtures::HexFixture fixture( pm , nx , ny , nz );
    fixture.m_fem_meta.commit();
    fixture.generate_mesh();

    fixture.m_bulk_data.modification_begin();

    stk::mesh::Entity elem = fixture.elem( ix , iy , iz );

    if ( elem.is_valid() && p_rank == elem.owner_rank() ) {
      stk::mesh::Entity tmp = elem ;
      fixture.m_bulk_data.destroy_entity( tmp );
    }

    fixture.m_bulk_data.modification_end();
  }
  }
  }
}

//----------------------------------------------------------------------------

STKUNIT_UNIT_TEST ( UnitTestCrackMesh , verifyBoxGhosting )
{
  // Start with a normal hex fixture, then crack it, and check to see
  // if all (incl ghosted) copies get updated.

  // Make the hex fixture
  stk::mesh::fixtures::HexFixture fixture( MPI_COMM_WORLD, 2,2,2 );
  fixture.m_fem_meta.commit();
  fixture.generate_mesh();

  stk::mesh::BulkData & mesh = fixture.m_bulk_data;

  // Hardwire which entities are being modified. Note that not every
  // process will know about these entities

  stk::mesh::Entity old_node = fixture.node(0,1,1);

  stk::mesh::Entity right_element = fixture.elem(0,0,1);
  const stk::mesh::EntityKey right_element_key = right_element.key();

  unsigned right_ordinal = 0;
  const unsigned new_node_id = 28;

  // If this process knows about both entities, compute the ordinal
  // of the relation from right_element to old_node

  if ( old_node.is_valid() && right_element.is_valid() ) {
    stk::mesh::PairIterRelation rel = old_node.relations();

    for (; rel.first != rel.second; ++rel) {
      if ( right_element == rel.first->entity() ) {
        right_ordinal = rel.first->identifier();
      }
    }
  }

  // Crack the mesh

  mesh.modification_begin();

  //only crack the mesh if I own the element
  if ( right_element.is_valid() &&
       right_element.owner_rank() == mesh.parallel_rank() ) {

    const stk::mesh::PartVector no_parts;

    // create a new node
    stk::mesh::Entity new_node = mesh.declare_entity(stk::mesh::MetaData::NODE_RANK, new_node_id, no_parts);

    // destroy right_element's relation to old_node, replace with a
    // relation to new node
    mesh.destroy_relation(right_element, old_node, right_ordinal);
    mesh.declare_relation(right_element, new_node, right_ordinal);
  }

  mesh.modification_end();

  // Refesh handle. If ghosting, it would have been deleted and recreated, so the unlying
  // pointer may have changed.
  right_element = mesh.get_entity(right_element_key);

  // Now that modification_end has been called, all processes that know
  // about right_element should know about the crack.

  if ( right_element.is_valid() ) {
    stk::mesh::PairIterRelation rel = right_element.relations();
    STKUNIT_ASSERT_EQ(rel.size(), 8u);
    stk::mesh::Entity new_node      = rel.first[right_ordinal].entity();
    STKUNIT_ASSERT(new_node.is_valid());

    STKUNIT_EXPECT_EQ ( new_node.identifier(), new_node_id );
  }
}
