/*
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
*/

#ifndef _VBR2PETRA_H_
#define _VBR2PETRA_H_

#ifndef __cplusplus
#define __cplusplus
#endif

#include "az_aztec.h"

#ifdef AZTEC_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_VbrMatrix.h"
#include "Epetra_CrsMatrix.h"


/*! \file 
\brief Aztec2Petra:  A function that converts an Aztec linear problem to a Petra linear problem.

    Aztec2Petra takes the Aztec proc_config, Amat, az_x and az_b objects and converts them into
    corresponding Petra equivalents: comm, map (layout information), A, x, and b.  This function
    is used by AZOO_iterate, but can be used independently by someone making a transistion from 
    Aztec to Trilinos/AztecOO.
*/
/*! \fn int Aztec2Petra(int * proc_config,
          AZ_MATRIX * Amat, double * az_x,double * az_b,
          Epetra_Comm * &comm,
          Epetra_BlockMap * & map,
          Epetra_RowMatrix * &A,
          Epetra_Vector * & x,
          Epetra_Vector * & b)

\brief Converts from an Aztec linear problem to a Petra linear problem.

\param proc_config (In)
       Aztec array containing information about the parallel machine.
\param Amat (In)
       An Aztec AZ_MATRIX structure.  Must be an MSR or VBR matrix at this time.
\param az_x (In)
       The Aztec initial guess/solution vector.  Must be of adequate length on each processor
       for any ghost values (unnecessary in uniprocessor mode).
\param az_b (In)
       The Aztec right hand side vector.  .
\param comm (Out)
       A pointer to a Epetra_Comm object.  Must be deleted by the caller of this function.
\param map (Out)
       A pointer to a Epetra_BlockMap object.  Must be deleted by the caller of this function.  
       Note:  This object may actually be a Epetra_Map object, but Epetra_BlockMap is a base 
       clase for Epetra_Map.
\param A (Out)
       A pointer to a Epetra_RowMatrix object containing a \bf deep copy of the matrix in Amat.  
       Must be deleted by the caller of this function.  Note:  This pointer will actually point to a 
       Epetra_CrsMatrix or a Epetra_VbrMatrix.  We cast the pointer to a Epetra_RowMatrix
       because it is the abstract base class used by AztecOO.
\param x (Out)
       A pointer to a Epetra_Vector object containing a \bf shallow copy (view) of az_x.  
       Must be deleted by the caller of this function.
\param b (Out)
       A pointer to a Epetra_Vector object containing a \bf shallow copy (view) of az_b.  
       Must be deleted by the caller of this function.
*/

int VbrMatrix2PetraMatrix(int blk_size,
          AZ_MATRIX * Amat, 
          Epetra_Comm *comm,
		Epetra_CrsMatrix * &F,
	        Epetra_CrsMatrix * &B,
	        Epetra_CrsMatrix * &Bt,
	        Epetra_CrsMatrix * &C,
	        Epetra_Map  **VBR_map
			  , Epetra_Map **petra_maps
			  );



int VbrVector2BlockVector(Epetra_Vector *guess_vbr, 
			  Epetra_Vector *velocity, 
			  Epetra_Vector *pressure, int blk_size);

int BlockVector2VbrVector(Epetra_Vector *vbr, 
			  Epetra_Vector *velocity, 
			  Epetra_Vector *pressure, int blk_size);


#endif /* _VBR2PETRA_H_ */
