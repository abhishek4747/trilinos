// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-
//
//   SUMMARY: 
//     USAGE:
//
//    AUTHOR: Thomas Leurent <tleurent@mcs.anl.gov>
//       ORG: Argonne National Laboratory
//    E-MAIL: tleurent@mcs.anl.gov
//
// ORIG-DATE: 14-Jan-02 at 08:05:56
//  LAST-MOD: 22-Jan-03 at 12:59:29 by Lori Freitag
//
// DESCRIPTION:
// ============
/*! \file AomdVtkTest.cpp

Unit testing of the uploading of VTK format into AOMD.

 */
// DESCRIP-END.
//



#include "Mesquite.hpp"
#include "MeshSet.hpp"
#include "PatchData.hpp"

#include "TSTT_Base.h"

#include "cppunit/extensions/HelperMacros.h"
#include "cppunit/SignalException.h"

#include <list>
#include <iterator>

using namespace Mesquite;

class AomdVtkTest : public CppUnit::TestFixture
{
private:
   CPPUNIT_TEST_SUITE(AomdVtkTest);
   CPPUNIT_TEST (test_elements);
   CPPUNIT_TEST_SUITE_END();
   
private:
   
   TSTT::Mesh_Handle tri10;
   MsqVertex *vtx_array;
   MsqMeshEntity *element_array;
   int num_elements;
   int num_vertices;

   //   int tri_check_validity( );
   //   int tet_check_validity( );

public:
   /* Automatically called by CppUnit before each test function. */
  void setUp()
  {
     MsqError err;
     
     char file_name[128];
     TSTT::MeshError tstt_err;
     
     /* Reads a TSTT Mesh file -- 10 triangles, 2 free vertices */
     TSTT::Mesh_Create(&tri10, &tstt_err); assert(!tstt_err);
     strcpy(file_name, "../../meshFiles/2D/VTK/equil_tri2.vtk");
     std::cout << file_name << std::endl; // dbg
     TSTT::Mesh_Load(tri10, file_name, &tstt_err);
     std::cout << tstt_err << std::endl; // dbg
     assert(!tstt_err);
     
  }

   /* Automatically called by CppUnit after each test function. */
  void tearDown()
  {
  }
  
public:
  AomdVtkTest()
   {}

   /*  */
   void test_elements()
   {
      MsqError err;
     /* Adds TSTT mesh to a MeshSet. */
      MeshSet mesh_set;
      mesh_set.add_mesh(tri10, err); MSQ_CHKERR(err);

      /* Retrieves a global patch */
      PatchData pd;
      PatchDataParameters pd_params;
      pd_params.set_patch_type(PatchData::ELEMENTS_ON_VERTEX_PATCH, err, 1, 0);
      pd_params.add_culling_method(PatchData::NO_BOUNDARY_VTX);
      
      mesh_set.get_next_patch(pd, pd_params, err); MSQ_CHKERR(err);

      int free_vtx = pd.num_free_vertices(err); MSQ_CHKERR(err);
      std::cout << "nb of free vertices: " << free_vtx << std::endl;
      CPPUNIT_ASSERT( free_vtx == 1 );
      
      element_array =  pd.get_element_array(err); MSQ_CHKERR(err);
      num_elements = pd.num_elements();
      CPPUNIT_ASSERT( num_elements == 6 );

      //      for (int i=0; i<num_elements; ++i) {
      //         std::cout << element_array[i];
      //      }
      
      vtx_array = pd.get_vertex_array(err); MSQ_CHKERR(err);
      num_vertices = pd.num_vertices();
      CPPUNIT_ASSERT( num_vertices == 7 );

      //      for (int i=0; i<num_vertices; ++i) {
      //         std::cout << vtx_array[i];
      //      }

      CPPUNIT_ASSERT( tri_check_validity() == 1 );
      
      mesh_set.get_next_patch(pd, pd_params, err); MSQ_CHKERR(err);

      element_array =  pd.get_element_array(err); MSQ_CHKERR(err);
      num_elements = pd.num_elements();
      CPPUNIT_ASSERT( num_elements == 6 );

      //      for (int i=0; i<num_elements; ++i) {
      //         std::cout << element_array[i];
      //      }
      
      vtx_array = pd.get_vertex_array(err); MSQ_CHKERR(err);
      num_vertices = pd.num_vertices();
      CPPUNIT_ASSERT( num_vertices == 7 );

      //      for (int i=0; i<num_vertices; ++i) {
      //         std::cout << vtx_array[i];
      //      }

      CPPUNIT_ASSERT( tri_check_validity() == 1 );
   }

   int tri_check_validity()
   {
  
    /* check that the simplicial mesh is still valid, 
       based on right handedness. Returns a 1 or a 0 */
    int valid = 1;
    double dEps = 1.e-13;

    double x1, x2, x3, y1, y2, y3;// z1, z2, z3;
    std::vector<size_t> vertex_indices;

    for (int i=0;i<num_elements;i++)
    {
        element_array[i].get_vertex_indices(vertex_indices);

        x1 = vtx_array[vertex_indices[0]][0];
        y1 = vtx_array[vertex_indices[0]][1];
        x2 = vtx_array[vertex_indices[1]][0];
        y2 = vtx_array[vertex_indices[1]][1];
        x3 = vtx_array[vertex_indices[2]][0];
        y3 = vtx_array[vertex_indices[2]][1];

        double a = x2*y3 - x3*y2;
        double b = y2 - y3;
        double c = x3 - x2;
      
        double area = .5*(a+b*x1+c*y1);
        if (area < dEps) {
           //          printf("x1 y1 = %f %f\n",x1,y1);
           //          printf("x2 y3 = %f %f\n",x2,y2);
           //          printf("x3 y3 = %f %f\n",x3,y3);
           //          printf("area = %f\n",area);
          valid=0;
        }
     }

     return(valid);
   }

   int tet_validity_check()
   {
      int valid = 1;
      double dEps = 1.e-13;
      double x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4;
      std::vector<size_t> vertex_indices;

      for (int i=0;i<num_elements;i++)
      {
        element_array[i].get_vertex_indices(vertex_indices);

        x1=vtx_array[vertex_indices[0]][0];
        y1=vtx_array[vertex_indices[0]][1];
        z1=vtx_array[vertex_indices[0]][2];

        x2=vtx_array[vertex_indices[1]][0];
        y2=vtx_array[vertex_indices[1]][1];
        z2=vtx_array[vertex_indices[1]][2];

        x3=vtx_array[vertex_indices[2]][0];
        y3=vtx_array[vertex_indices[2]][1];
        z3=vtx_array[vertex_indices[2]][2];

        x4=vtx_array[vertex_indices[3]][0];
        y4=vtx_array[vertex_indices[3]][1];
        z4=vtx_array[vertex_indices[3]][2];
            
        double dDX2 = x2 - x1;
        double dDX3 = x3 - x1;
        double dDX4 = x4 - x1;
        
        double dDY2 = y2 - y1;
        double dDY3 = y3 - y1;
        double dDY4 = y4 - y1;

        double dDZ2 = z2 - z1;
        double dDZ3 = z3 - z1;
        double dDZ4 = z4 - z1;
      
        /* dDet is proportional to the cell volume */
        double dDet = dDX2*dDY3*dDZ4 + dDX3*dDY4*dDZ2 + dDX4*dDY2*dDZ3
          - dDZ2*dDY3*dDX4 - dDZ3*dDY4*dDX2 - dDZ4*dDY2*dDX3 ;

        /* Compute a length scale based on edge lengths. */
        double dScale = ( sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) +
                               (z1-z2)*(z1-z2)) +
                          sqrt((x1-x3)*(x1-x3) + (y1-y3)*(y1-y3) +
                               (z1-z3)*(z1-z3)) +
                          sqrt((x1-x4)*(x1-x4) + (y1-y4)*(y1-y4) +
                               (z1-z4)*(z1-z4)) +
                          sqrt((x2-x3)*(x2-x3) + (y2-y3)*(y2-y3) +
                               (z2-z3)*(z2-z3)) +
                          sqrt((x2-x4)*(x2-x4) + (y2-y4)*(y2-y4) +
                               (z2-z4)*(z2-z4)) +
                          sqrt((x3-x4)*(x3-x4) + (y3-y4)*(y3-y4) +
                               (z3-z4)*(z3-z4)) ) / 6.;
      
          /* Use the length scale to get a better idea if the tet is flat or
             just really small. */
        if (fabs(dScale) < dEps)
        {
          return(valid = 0);
        }
        else
        {
          dDet /= (dScale*dScale*dScale);
        }
      
        if (dDet > dEps)
        {
          valid = 1;
        }
        else if (dDet < -dEps)
        {
          valid = -1;
        }
        else
        {
          valid = 0;
        }
     }  // end for i=1,numElements
  
     return(valid);
  }

   
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AomdVtkTest, "AomdVtkTest");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AomdVtkTest, "Unit");
