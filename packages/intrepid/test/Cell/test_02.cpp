// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Pavel Bochev (pbboche@sandia.gov) or
//                    Denis Ridzal (dridzal@sandia.gov).
//                    Kara Peterson (kjpeter@sandia.gov)
//
// ************************************************************************
// @HEADER


/** \file
    \brief  Test of the CellTools class.
    \author Created by P. Bochev, D. Ridzal and K. Peterson
*/
#include "Intrepid_CellTools.hpp"
#include "Intrepid_FieldContainer.hpp"
#include "Intrepid_DefaultCubatureFactory.hpp"
#include "Shards_CellTopology.hpp"

#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_ScalarTraits.hpp"

using namespace std;
using namespace Intrepid;
using namespace shards;
  

int main(int argc, char *argv[]) {
 
  typedef CellTools<double>       CellTools;
  typedef shards::CellTopology    CellTopology;
  
  // This little trick lets us print to std::cout only if a (dummy) command-line argument is provided.
  int iprint     = argc - 1;
  
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);
  
  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);
  
  *outStream \
    << "===============================================================================\n" \
    << "|                                                                             |\n" \
    << "|                              Unit Test CellTools                            |\n" \
    << "|                                                                             |\n" \
    << "|     1) Mapping to and from reference cells with base topologies             |\n" \
    << "|     2) Mapping to and from reference cells with extended topologies         |\n" \
    << "|                                                                             |\n" \
    << "|  Questions? Contact  Pavel Bochev (pbboche@sandia.gov)                      |\n" \
    << "|                      Denis Ridzal (dridzal@sandia.gov), or                  |\n" \
    << "|                      Kara Peterson (kjpeter@sandia.gov)                     |\n" \
    << "|                                                                             |\n" \
    << "|  Intrepid's website: http://trilinos.sandia.gov/packages/intrepid           |\n" \
    << "|  Trilinos website:   http://trilinos.sandia.gov                             |\n" \
    << "|                                                                             |\n" \
    << "===============================================================================\n";
  
  int errorFlag  = 0;
  try{

    *outStream \
    << "\n"
    << "===============================================================================\n"\
    << "| Test 1: mapping from reference to physical frame and back to reference      |\n"\
    << "===============================================================================\n\n";
    /*
     *  Test summary:
     *
     *    A reference point set is mapped to physical frame and then back to reference frame.
     *    Test passes if the final set of points matches the first set of points. The cell workset
     *    is generated by perturbing randomly the nodes of a reference cell with the specified 
     *    cell topology. 
     *
     */
    
    // Collect all supported cell topologies
    std::vector<shards::CellTopology> supportedTopologies;
    supportedTopologies.push_back(shards::getCellTopologyData<Triangle<3> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Triangle<6> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Quadrilateral<4> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Quadrilateral<9> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Tetrahedron<4> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Tetrahedron<10> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Hexahedron<8> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Hexahedron<27> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Wedge<6> >() );
    supportedTopologies.push_back(shards::getCellTopologyData<Wedge<18> >() );
    
    // Declare iterator to loop over the cell topologies
    std::vector<shards::CellTopology>::iterator topo_iterator;


    // Declare arrays for cell workset and point sets. We will have 10 cells in the wset. and 10 pts per pt. set
    FieldContainer<double> cellWorkset;                 // physical cell workset
    FieldContainer<double> refPoints;                   // reference point set(s) 
    FieldContainer<double> physPoints;                  // physical point set(s)
    FieldContainer<double> controlPoints;               // preimages: physical points mapped back to ref. frame
    
    // We will use cubature factory to get some points on the reference cells. Declare necessary arrays
    DefaultCubatureFactory<double>  cubFactory;   
    FieldContainer<double> cubPoints;
    FieldContainer<double> cubWeights;

    // Initialize number of cells in the cell workset
    int numCells  = 10;
    

    
    // Loop over cell topologies, make cell workset for each one by perturbing the nodes & test methods
    for(topo_iterator = supportedTopologies.begin(); topo_iterator != supportedTopologies.end(); ++topo_iterator){
      
      // 1.   Define a single reference point set using cubature factory with order 6 cubature
      Teuchos::RCP<Cubature<double> > cellCubature = cubFactory.create( (*topo_iterator), 4); 
      int cubDim = cellCubature -> getDimension();
      int numPts = cellCubature -> getNumPoints();
      cubPoints.resize(numPts, cubDim);
      cubWeights.resize(numPts);
      cellCubature -> getCubature(cubPoints, cubWeights);
             
      // 2.   Define a cell workset by perturbing the nodes of the reference cell with the specified topology
      // 2.1  Resize dimensions of the rank-3 (C,N,D) cell workset array for the current topology
      int numNodes = (*topo_iterator).getNodeCount();
      int cellDim  = (*topo_iterator).getDimension();
      cellWorkset.resize(numCells, numNodes, cellDim);
      
      // 2.2  Copy nodes of the reference cell with the same topology to temp rank-2 (N,D) array
      FieldContainer<double> refCellNodes(numNodes, cellDim );
      CellTools::getReferenceSubcellNodes(refCellNodes, cellDim, 0, (*topo_iterator) );
      
      
      // 2.3  Create randomly perturbed version of the reference cell and save in the cell workset array
      for(int cellOrd = 0; cellOrd < numCells; cellOrd++){
        
        // Move vertices +/-0.125 along their axes. Gives nondegenerate cells for base and extended topologies 
        for(int nodeOrd = 0; nodeOrd < numNodes; nodeOrd++){
          for(int d = 0; d < cellDim; d++){
            double delta = Teuchos::ScalarTraits<double>::random()/16.0;
            cellWorkset(cellOrd, nodeOrd, d) = refCellNodes(nodeOrd, d) + delta;
          } // d
        }// nodeOrd           
      }// cellOrd
      
      
      /* 
       * 3.1 Test 1: single point set to single physical cell: map ref. point set in rank-2 (P,D) array
       *      to a physical point set in rank-2 (P,D) array for a specified cell ordinal. Use the cub.
       *      points array for this test. Resize physPoints and controlPoints to appropriate rank.
       */
      physPoints.resize(numPts, cubDim);
      controlPoints.resize(numPts, cubDim);
      
      *outStream 
        << " Mapping a set of " << numPts << " points to one cell in a workset of " << numCells << " " 
        << (*topo_iterator).getName() << " cells. \n"; 
      
      for(int cellOrd = 0; cellOrd < numCells; cellOrd++){
        
        // Forward map:: requires cell ordinal
        CellTools::mapToPhysicalFrame(physPoints, cubPoints, cellWorkset, (*topo_iterator), cellOrd);
        // Inverse map: requires cell ordinal
        CellTools::mapToReferenceFrame(controlPoints, physPoints, cellWorkset, (*topo_iterator), cellOrd);

        // Points in controlPoints should match the originals in cubPoints up to a tolerance
        for(int pt = 0; pt < numPts; pt++){
          for(int d = 0; d < cellDim; d++){
            
            if( abs( controlPoints(pt, d) - cubPoints(pt, d) ) > 100.0*INTREPID_TOL ){
              errorFlag++;
              *outStream
                << std::setw(70) << "^^^^----FAILURE!" << "\n"
                << " Single point set mappings to single physical cell in a workset failed for: \n"
                << "                    Cell Topology = " << (*topo_iterator).getName() << "\n"
                << " Physical cell ordinal in workset = " << cellOrd << "\n"
                << "          Reference point ordinal = " << setprecision(12) << pt << "\n"
                << "    At reference point coordinate = " << setprecision(12) << d << "\n"
                << "                   Original value = " << cubPoints(pt, d) << "\n"
                << "                     F^{-1}F(P_d) = " << controlPoints(pt, d) <<"\n";
            }
          }// d
        }// pt
      }// cellOrd
      
  
      /* 
       * 3.2  Test 2: single point set to multiple physical cells: map ref. point set in rank-2 (P,D) array
       *      to a physical point set in rank-3 (C, P,D) array for all cell ordinals. Use the cub.
       *      points array for this test. Resize physPoints and controlPoints to appropriate rank.
       */
      physPoints.clear(); 
      controlPoints.clear();
      physPoints.resize(numCells, numPts, cubDim);
      controlPoints.resize(numCells, numPts, cubDim);
      
      *outStream 
        << " Mapping a set of " << numPts << " points to all cells in workset of " << numCells << " " 
        << (*topo_iterator).getName() << " cells. \n"; 
      
      // Forward map: do not specify cell ordinal
      CellTools::mapToPhysicalFrame(physPoints, cubPoints, cellWorkset, (*topo_iterator));
      // Inverse map: do not specify cell ordinal
      CellTools::mapToReferenceFrame(controlPoints, physPoints, cellWorkset, (*topo_iterator));
      
      // Check: points in controlPoints should match the originals in cubPoints up to a tolerance
      for(int cellOrd = 0; cellOrd < numCells; cellOrd++){
        for(int pt = 0; pt < numPts; pt++){
          for(int d = 0; d < cellDim; d++){
            
            if( abs( controlPoints(cellOrd, pt, d) - cubPoints(pt, d) ) > 100.0*INTREPID_TOL ){
              errorFlag++;
              *outStream
                << std::setw(70) << "^^^^----FAILURE!" << "\n"
                << " Mapping a single point set to all physical cells in a workset failed for: \n"
                << "                    Cell Topology = " << (*topo_iterator).getName() << "\n"
                << " Physical cell ordinal in workset = " << cellOrd << "\n"
                << "          Reference point ordinal = " << setprecision(12) << pt << "\n"
                << "    At reference point coordinate = " << setprecision(12) << d << "\n"
                << "                   Original value = " << cubPoints(pt, d) << "\n"
                << "                     F^{-1}F(P_d) = " << controlPoints(cellOrd, pt, d) <<"\n";
            }
          }// d
        }// pt
      }// cellOrd
      
      /* 
       * 3.3 Test 3: multiple point sets to multiple physical cells: map ref. point sets in rank-3 (C,P,D) array
       *     to physical point sets in rank-3 (C, P,D) array for all cell ordinals. The (C,P,D) array
       *     with reference point sets is obtained by cloning the cubature point array.
       */
      physPoints.clear(); 
      controlPoints.clear();
      refPoints.resize(numCells, numPts, cubDim);
      physPoints.resize(numCells, numPts, cubDim);
      controlPoints.resize(numCells, numPts, cubDim);
      
      // Clone cubature points in refPoints:
      for(int c = 0; c < numCells; c++){
        for(int pt = 0; pt < numPts; pt++){
          for(int d = 0; d < cellDim; d++){
            refPoints(c, pt, d) = cubPoints(pt, d);
            
          }// d
        }// pt
      }// c
      
      *outStream 
        << " Mapping " << numCells << " sets of " << numPts << " points to corresponding cells in workset of " << numCells << " " 
        << (*topo_iterator).getName() << " cells. \n"; 
      
      // Forward map: do not specify cell ordinal
      CellTools::mapToPhysicalFrame(physPoints, refPoints, cellWorkset, (*topo_iterator));
      // Inverse map: do not specify cell ordinal
      CellTools::mapToReferenceFrame(controlPoints, physPoints, cellWorkset, (*topo_iterator));
      
      // Check: points in controlPoints should match the originals in cubPoints up to a tolerance
      for(int cellOrd = 0; cellOrd < numCells; cellOrd++){
        for(int pt = 0; pt < numPts; pt++){
          for(int d = 0; d < cellDim; d++){
            
            if( abs( controlPoints(cellOrd, pt, d) - cubPoints(pt, d) ) > 100.0*INTREPID_TOL ){
              errorFlag++;
              *outStream
                << std::setw(70) << "^^^^----FAILURE!" << "\n"
                << " Mapping multiple point sets to corresponding physical cells in a workset failed for: \n"
                << "                    Cell Topology = " << (*topo_iterator).getName() << "\n"
                << " Physical cell ordinal in workset = " << cellOrd << "\n"
                << "          Reference point ordinal = " << setprecision(12) << pt << "\n"
                << "    At reference point coordinate = " << setprecision(12) << d << "\n"
                << "                   Original value = " << refPoints(cellOrd, pt, d) << "\n"
                << "                     F^{-1}F(P_d) = " << controlPoints(cellOrd, pt, d) <<"\n";
            }
          }// d
        }// pt
      }// cellOrd
      
    }// topo_iterator

    
    
    
    
  }// try
  
  //============================================================================================//
  // Wrap up test: check if the test broke down unexpectedly due to an exception                //
  //============================================================================================//

    catch (std::logic_error err) {
    *outStream << err.what() << "\n";
    errorFlag = -1000;
  };
  
  
  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";
  
  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);
  
  return errorFlag;
}
  






