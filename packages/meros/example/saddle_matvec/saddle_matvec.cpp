// @HEADER
// ***********************************************************************
// 
//              Meros: Segregated Preconditioning Package
//                 Copyright (2004) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

/*
  1. compile in TSF/examples
*/

// TSF includes
#include "TSFMPI.h"
#include "TSFOut.h"
#include "TSFVectorType.h"
#include "TSFProductSpace.h"
#include "TSFBlockLinearOperator.h"
#include "PetraVectorType.h"
#include "PetraVectorSpace.h"
#include "PetraVector.h"
#include "PetraMatrix.h"
#include "GMRESSolver.h"
#include "AZTECSolver.h"
#include "TSFPreconditionerFactory.h"
#include "GenericRightPreconditioner.h"
#include "TSFPreconditioner.h"
#include "TSFMatrixOperator.h"

// Epetra includes
#include "Epetra_Vector.h"
#ifdef EPETRA_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

// our includes
#include "Vbr2Petra.h"
#include "ReadPetra.h"
using namespace TSF;

#include "TSFLinearOperator2EpetraRowMatrix.h"
#include "Aztec2TSF.h"

int main(int argc, void** argv)
{

  /**************************************************************/
  // Letting TSF do the MPI_Init for now.
  // Initialize TSFMPI
  TSFMPI::init(&argc, &argv);
  /**************************************************************/

  // consts for matrix reader
  int Nnz, Nblks;
  Nnz = 4228;
  Nblks = 256;
  int blk_size = 3;
  AZ_MATRIX *Amat;
  Epetra_Comm *comm;
#ifdef EPETRA_MPI
  Epetra_MpiComm mpicomm(MPI_COMM_WORLD);
  comm = (Epetra_Comm *) (&mpicomm);
#else
  comm = (Epetra_Comm *) new Epetra_SerialComm();
#endif

  Epetra_BlockMap  *VbrMap;
  Epetra_Map **subblock_maps;
  subblock_maps = (Epetra_Map **) malloc(sizeof(Epetra_Map *)*2);
  subblock_maps[0] = new Epetra_Map(Nblks*(blk_size-1), 0, *comm);
  subblock_maps[1] = new Epetra_Map(Nblks, 0, *comm);

  // Read Matrix from file into Aztec VBR matrix

  ReadAztecVbr(blk_size, Nnz, Nblks, &Amat, "../data/mac-vbr/K_reordered");

  Epetra_RowMatrix *saddleA_epet;
  Aztec2TSF( Amat, comm, VbrMap, saddleA_epet, 
	     NULL, NULL, NULL, NULL, subblock_maps);

 // Read in the 'x' 
 Epetra_Vector *x_epet;
 x_epet = new Epetra_Vector(*VbrMap);
 ReadPetraVector(x_epet, "../data/mac-vbr/guess_reordered");

 // Initialize 'rhs'
 Epetra_Vector *rhs_epet;
 rhs_epet = new Epetra_Vector(*VbrMap);

 // Do multiply

 saddleA_epet->Apply(*x_epet, *rhs_epet);

 // Print result
 double *dtemp;
 rhs_epet->ExtractView(&dtemp);
 for (int i = 0; i < rhs_epet->MyLength(); i++)
   printf("%d %e\n",i,dtemp[i]);


  delete saddleA_epet;
  delete x_epet;
  delete rhs_epet;
  delete subblock_maps[0];
  delete subblock_maps[1];
  delete VbrMap;

#ifndef EPETRA_MPI
  delete comm;
#endif

  free(Amat->data_org);
  free(Amat->bpntr);
  free(Amat->bindx);
  free(Amat->val);
  free(Amat->cpntr);
  free(Amat->rpntr);
  free(Amat->indx);
  free(subblock_maps);
  AZ_matrix_destroy(&Amat);

  TSFMPI::finalize(); 

}



