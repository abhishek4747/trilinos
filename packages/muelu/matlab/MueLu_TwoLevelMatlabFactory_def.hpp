// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_TWOLEVELMATLABFACTORY_DEF_HPP
#define MUELU_TWOLEVELMATLABFACTORY_DEF_HPP


#include <sstream>
#include <cstring>

#include <Xpetra_Matrix.hpp>
#include <Xpetra_MatrixFactory.hpp>
#include <Xpetra_Vector.hpp>
#include <Xpetra_VectorFactory.hpp>

#include "MueLu_MasterList.hpp"
#include "MueLu_Monitor.hpp"
#include "MueLu_PerfUtils.hpp"
#include "MueLu_TwoLevelMatlabFactory_decl.hpp"
#include "MueLu_Utilities.hpp"
#include "muemexTypes_decl.hpp"

#ifdef HAVE_MUELU_MATLAB
#include "mex.h"

namespace MueLu {

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  TwoLevelMatlabFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::TwoLevelMatlabFactory()
    : hasDeclaredInput_(false) { }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  RCP<const ParameterList> TwoLevelMatlabFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::GetValidParameterList() const {
    RCP<ParameterList> validParamList = rcp(new ParameterList());
    
    validParamList->set<std::string>("Provides"     , "" ,"A comma-separated list of objects provided on the coarse level by the TwoLevelMatlabFactory");
    validParamList->set<std::string>("Needs Fine"   , "", "A comma-separated list of objects needed on the fine level by the TwoLevelMatlabFactory");
    validParamList->set<std::string>("Needs Coarse" , "", "A comma-separated list of objects needed on the coarse level by the TwoLevelMatlabFactory");
    validParamList->set<std::string>("Function"     , "" , "The name of the Matlab MEX function to call for Build()");

    return validParamList;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  void TwoLevelMatlabFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::DeclareInput(Level &fineLevel, Level &coarseLevel) const {
    const Teuchos::ParameterList& pL = GetParameterList();

    // Get needs strings
    const std::string str_nf = pL.get<std::string>("Needs Fine");
    const std::string str_nc = pL.get<std::string>("Needs Coarse");

    // Tokenize the strings
    TokenizeStringAndStripWhiteSpace(str_nf,needsFine_);
    TokenizeStringAndStripWhiteSpace(str_nc,needsCoarse_);

    // Declare inputs
    for(size_t i=0; i<needsFine_.size(); i++)
      Input(fineLevel,needsFine_[i]);

    for(size_t i=0; i<needsCoarse_.size(); i++)
      Input(coarseLevel,needsCoarse_[i]);

    hasDeclaredInput_=true;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  void TwoLevelMatlabFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node>::Build(Level& fineLevel, Level& coarseLevel) const {
    const Teuchos::ParameterList& pL = GetParameterList();

    /* NOTE: We need types to call Get functions.  For the moment, I'm just going to infer types from names.
       We'll need to replace this wby modifying the needs list with strings that define types and adding some kind of lookup function instead */

    // NOTE: mexOutput[0] is the "Provides."  Might want to modify to allow for additional outputs
    vector<RCP<MuemexArg> > InputArgs;

    // Fine needs
    for(size_t i=0; needsFine_.size(); i++) {
      if(needsFine_[i] == "A" || needsFine_[i] == "P" || needsFine_[i] == "R" || needsFine_[i]=="Ptent") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<Matrix> >(fineLevel,name))));
      }

      if(needsFine_[i] == "Nullspace" || needsFine_[i] == "Coordinates") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<MultiVector> >(fineLevel,name))));
      }

      if(needsFine_[i] == "Aggregates") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<Aggregates> >(fineLevel,name))));
      }

      if(needsFine_[i] == "UnAmalgamationInfo") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<AmalgamationInfo> >(fineLevel,name))));
      }
    }

    // Coarser needs
    for(size_t i=0; needsCoarse_.size(); i++) {
      if(needsCoarse_[i] == "A" || needsCoarse_[i] == "P" || needsCoarse_[i] == "R" || needsCoarse_[i]=="Ptent") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<Matrix> >(fineLevel,name))));
      }

      if(needsCoarse_[i] == "Nullspace" || needsCoarse_[i] == "Coordinates") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<MultiVector> >(fineLevel,name))));
      }

      if(needsCoarse_[i] == "Aggregates") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<Aggregates> >(fineLevel,name))));
      }

      if(needsCoarse_[i] == "UnAmalgamationInfo") {
	InputArgs.push_back(rcp(new MueMexData(Get<RCP<AmalgamationInfo> >(fineLevel,name))));
      }
    }

    // Determine output
    const std::string str_prov = pL.get<std::string>("Provides");
    std::vector<std::string> provides;
    TokenizeStringAndStripWhiteSpace(str_prov,provides);

   
    // Call mex function
    std::string matlabFunction = pL.get<string>("Function");
    if(!matlabFunction.length()) throw std::runtime_error("Invalid matlab function name");
    std::vector<Teuchos::RCP<MuemexArg> > mexOutput = Muemexcallback::callMatlab(matlabFunction,provides.length(),InputArgs);


    // Set output
    if(mexOutput.length()!=provides.length()) throw std::runtime_error("Invalid matlab output");
    for(size_t i=0; provides.size(); i++)  {
      if(provides[i] == "A" || provides[i] == "P" || provides[i] == "R" || provides[i]=="Ptent") {
	coarseLevel.Set(provides[i],mexOutput[i].getData<RCP<Matrix> >();
      }

      if(provides[i] == "Nullspace" || provides[i] == "Coordinates") {
	coarseLevel.Set(provides[i],mexOutput[i].getData<RCP<MultiVector> >();
      }

      if(provides[i] == "Aggregates") {
	coarseLevel.Set(provides[i],mexOutput[i].getData<RCP<Aggregates> >();
      }

      if(provides[i] == "UnAmalgamationInfo") {
	coarseLevel.Set(provides[i],mexOutput[i].getData<RCP<AmalgamationInfo> >();
      }
    }



  }

} //namespace MueLu

#define MUELU_TWOLEVELMATLABFACTORY_SHORT
#endif // HAVE_MUELU_MATLAB

#endif // MUELU_TWOLEVELMATLABFACTORY_DEF_HPP

