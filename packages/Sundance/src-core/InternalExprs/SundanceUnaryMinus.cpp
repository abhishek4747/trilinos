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

#include "SundanceUnaryMinus.hpp"

using namespace SundanceCore;
using namespace SundanceUtils;

using namespace SundanceCore::Internal;
using namespace Teuchos;

UnaryMinus::UnaryMinus(const RefCountPtr<ScalarExpr>& arg)
  : UnaryExpr(arg)
{
  if (isEvaluatable(arg.get()))
    {
      for (int d=0; d<MultiIndex::maxDim(); d++) 
        {
          setOrderOfDependency(d, evaluatableArg()->orderOfSpatialDependency(d));
        }

      setFuncIDSet(evaluatableArg()->funcIDSet());
    }
}

ostream& UnaryMinus::toText(ostream& os, bool paren) const 
{
  if (paren) os << "(";
  os << "-" << arg().toString();
  if (paren) os << ")";
  return os;
}

ostream& UnaryMinus::toLatex(ostream& os, bool paren) const 
{
  return toText(os, paren);
}

XMLObject UnaryMinus::toXML() const
{
  XMLObject rtn("UnaryMinus");
  rtn.addChild(arg().toXML());
  return rtn;
}
