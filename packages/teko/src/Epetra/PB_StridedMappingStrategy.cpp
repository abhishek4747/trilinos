#include "Epetra/PB_StridedMappingStrategy.hpp"
#include "Epetra/PB_InterlacedEpetra.hpp"
#include "Epetra/PB_EpetraHelpers.hpp"

#include "Thyra_EpetraThyraWrappers.hpp"
#include "Thyra_EpetraLinearOp.hpp"
#include "Thyra_DefaultProductMultiVector.hpp"
#include "Thyra_DefaultProductVectorSpace.hpp"
#include "Thyra_DefaultSpmdMultiVector.hpp"
#include "Thyra_DefaultBlockedLinearOp.hpp"
#include "Thyra_get_Epetra_Operator.hpp"

using Teuchos::RCP;
using Teuchos::rcp;
using Teuchos::rcp_dynamic_cast;

namespace PB {
namespace Epetra {

// Creates a strided mapping strategy. This class is useful
// for breaking up nodally ordered matrices (i.e. the unknowns
// in a FEM problem are ordered [u0,v0,p0,u1,v1,p1,...]). Current
// implimentation only supports a fixed number of variables
//
//    arguments: 
//       vars - Number of different variables 
//       map  - original Epetra_Map to be broken up
//       comm - Epetra_Comm object related to the map
//
StridedMappingStrategy::StridedMappingStrategy(const std::vector<int> & vars,const RCP<const Epetra_Map> & map,
                                               const Epetra_Comm & comm)
{
   rangeMap_ = map;
   domainMap_ = map;
   buildBlockTransferData(vars, rangeMap_,comm);
}

// Virtual function defined in MappingStrategy.  This copies
// an Epetra_MultiVector into a Thyra::MultiVectorBase with
// blocking handled by the strides defined in the constructor.
//
//   arguments:
//      X       - source Epetra_MultiVector
//      thyra_X - destination Thyra::MultiVectorBase
//      eow     - Operator that defines the transition...this may
//                be removed in the future
//
void StridedMappingStrategy::copyEpetraIntoThyra(const Epetra_MultiVector& X,
                                                 const Teuchos::Ptr<Thyra::MultiVectorBase<double> > & thyra_X,
                                                 const PB::Epetra::EpetraOperatorWrapper & eow) const
{
   int count = X.NumVectors(); 

   std::vector<RCP<Epetra_MultiVector> > subX;

   // allocate vectors to copy into
   PB::Epetra::buildSubVectors(blockMaps_,subX,count);

   // copy source vector to X vector
   PB::Epetra::one2many(subX,X,blockImport_);

   // convert subX to an array of multi vectors
   Teuchos::Array<RCP<Thyra::MultiVectorBase<double> > > thyra_subX;
   Teuchos::Ptr<Thyra::ProductMultiVectorBase<double> > prod_X
         = Teuchos::ptr_dynamic_cast<Thyra::ProductMultiVectorBase<double> >(thyra_X);
   for(int i=0;i<blockMaps_.size();i++) {
      RCP<Thyra::DefaultSpmdMultiVector<double> > vec 
            = rcp_dynamic_cast<Thyra::DefaultSpmdMultiVector<double> >(prod_X->getNonconstMultiVectorBlock(i)); 
      fillDefaultSpmdMultiVector(vec,subX[i]);
   }
}

// Virtual function defined in MappingStrategy.  This copies
// an Epetra_MultiVector into a Thyra::MultiVectorBase with
// blocking handled by the strides defined in the constructor.
//
//   arguments:
//      thyra_Y - source Thyra::MultiVectorBase
//      Y       - destination Epetra_MultiVector
//      eow     - Operator that defines the transition...this may
//                be removed in the future
//
void StridedMappingStrategy::copyThyraIntoEpetra(const RCP<const Thyra::MultiVectorBase<double> > & thyra_Y,
                                                 Epetra_MultiVector& Y,
                                                 const PB::Epetra::EpetraOperatorWrapper & eow) const
{
   std::vector<RCP<const Epetra_MultiVector> > subY;
   RCP<const Thyra::DefaultProductMultiVector<double> > prod_Y 
         = rcp_dynamic_cast<const Thyra::DefaultProductMultiVector<double> >(thyra_Y);

   int numBlocks = prod_Y->productSpace()->numBlocks();

   // convert thyra product vector to subY
   for(int i=0;i<blockMaps_.size();i++)
      subY.push_back(Thyra::get_Epetra_MultiVector(*blockMaps_[i].second,prod_Y->getMultiVectorBlock(i)));

   // endow the subVectors with required information about the maps
   PB::Epetra::associateSubVectors(blockMaps_,subY);

   // copy solution vectors to Y vector
   PB::Epetra::many2one(Y,subY,blockExport_);
}

// this is the core routine that builds the maps
// and importers/exporters neccessary for all the
// transfers. Currently it simply calls out to the
// interlaced epetra functions. (Comment: this
// routine should probably be private or protected
// ... it is basically the meat of the constructor)
//
//    arguments:
//       vars - Vector describing the blocking of variables
//       baseMap - basic map to use in the transfers
//       comm    - Epetra_Comm object
//
void StridedMappingStrategy::buildBlockTransferData(const std::vector<int> & vars,const Teuchos::RCP<const Epetra_Map> & baseMap, const Epetra_Comm & comm)
{
   int numGlobals = baseMap->NumGlobalElements();

   // build maps and exporters/importers
   PB::Epetra::buildSubMaps(numGlobals,vars,comm,blockMaps_);
   PB::Epetra::buildExportImport(*baseMap, blockMaps_, blockExport_,blockImport_);
}

// Builds a blocked Thyra operator that uses the strided
// mapping strategy to define sub blocks.
//
//    arguments:
//       mat - Epetra_CrsMatrix with FillComplete called, this
//             matrix is assumed to be square, with the same
//             range and domain maps
//    returns: Blocked Thyra linear operator with sub blocks
//             defined by this mapping strategy
//
const Teuchos::RCP<Thyra::BlockedLinearOpBase<double> > 
StridedMappingStrategy::buildBlockedThyraOp(const RCP<const Epetra_CrsMatrix> & crsContent,const std::string & label) const
{
   int dim = blockMaps_.size();

   RCP<Thyra::DefaultBlockedLinearOp<double> > A = Thyra::defaultBlockedLinearOp<double>();

   A->beginBlockFill(dim,dim);
   for(int i=0;i<dim;i++) {
      for(int j=0;j<dim;j++) {
         // label block correctly
         std::stringstream ss;
         ss << label << "_" << i << "," << j;

         // build the blocks and place it the right location
         A->setNonconstBlock(i,j,Thyra::nonconstEpetraLinearOp(PB::Epetra::buildSubBlock(i,j,*crsContent,blockMaps_),ss.str()));
      }
   } // end for i
   A->endBlockFill();

   return A;
}

// Rebuilds a blocked Thyra operator that uses the strided
// mapping strategy to define sub blocks.
//
//    arguments:
//       crsContent - Epetra_CrsMatrix with FillComplete called, this
//                    matrix is assumed to be square, with the same
//                    range and domain maps
//       A - Destination block linear op composed of blocks of
//           Epetra_CrsMatrix at all relevant locations
//
void StridedMappingStrategy::rebuildBlockedThyraOp(const RCP<const Epetra_CrsMatrix> & crsContent,
                                                   const RCP<Thyra::BlockedLinearOpBase<double> > & A) const
{
   int dim = blockMaps_.size();

   for(int i=0;i<dim;i++) {
      for(int j=0;j<dim;j++) {
         // get Epetra version of desired block
         RCP<Thyra::LinearOpBase<double> > Aij = A->getNonconstBlock(i,j);
         RCP<Epetra_CrsMatrix> eAij = rcp_dynamic_cast<Epetra_CrsMatrix>(Thyra::get_Epetra_Operator(*Aij),true);

         // rebuild the blocks and place it the right location
         PB::Epetra::rebuildSubBlock(i,j,*crsContent,blockMaps_,*eAij);
      }
   } // end for i
}

} // end namespace Epetra
} // end namespace PB
