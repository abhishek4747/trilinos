/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */


#ifndef SUNDANCE_BOUNDARYCELLPREDICATE_H
#define SUNDANCE_BOUNDARYCELLPREDICATE_H

#ifndef DOXYGEN_DEVELOPER_ONLY

#include "SundanceDefs.hpp"
#include "SundanceCellPredicateBase.hpp"

namespace SundanceStdFwk
{
  using namespace SundanceUtils;
  using namespace SundanceStdMesh;
  using namespace SundanceStdMesh::Internal;
  using namespace Teuchos;

  
  namespace Internal
  {
    /** 
     * BoundaryCellPredicate tests whether a cell is on the 
     * boundary of the domain
     */
    class BoundaryCellPredicate : public CellPredicateBase 
    {
    public:
      /** Empty ctor */
      BoundaryCellPredicate() {;}

      /** virtual dtor */
      virtual ~BoundaryCellPredicate(){;}
      
      /** Test whether the cell with the given LID satisfies the condition */
      virtual bool test(int cellLID) const ;

      /** Write to XML */
      virtual XMLObject toXML() const ;

      /** comparison */
      virtual bool lessThan(const CellPredicateBase* other) const ;

      /* */
      GET_RCP(CellPredicateBase);

    private:
    };
  }
}
#endif  /* DOXYGEN_DEVELOPER_ONLY */

#endif
