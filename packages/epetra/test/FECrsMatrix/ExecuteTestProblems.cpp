//@HEADER
// ************************************************************************
// 
//               Epetra: Linear Algebra Services Package 
//                 Copyright (2001) Sandia Corporation
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
// ************************************************************************
//@HEADER

#include "Epetra_BLAS.h"
#include "ExecuteTestProblems.h"
#include "Epetra_Comm.h"
#include "Epetra_Vector.h"
#include "Epetra_FEVector.h"
#include "Epetra_IntSerialDenseVector.h"
#include "Epetra_SerialDenseMatrix.h"
#include <../src/Epetra_matrix_data.h>
#include <../src/Epetra_test_functions.h>


int Drumm1(const Epetra_Map& map, bool verbose)
{
  //Simple 2-element problem (element as in "finite-element") from
  //Clif Drumm. Two triangular elements, one per processor, as shown
  //here:
  //
  //   *----*
  //  3|\  2|
  //   | \  |
  //   | 0\1|
  //   |   \|
  //   *----*
  //  0    1
  //
  //Element 0 on processor 0, element 1 on processor 1.
  //Processor 0 will own nodes 0,1 and processor 1 will own nodes 2,3.
  //Each processor will pass a 3x3 element-matrix to Epetra_FECrsMatrix.
  //After GlobalAssemble(), the matrix should be as follows:
  //
  //         row 0: 2  1  0  1
  //proc 0   row 1: 1  4  1  2
  //----------------------------------
  //         row 2: 0  1  2  1
  //proc 1   row 3: 1  2  1  4
  //

  int numProcs = map.Comm().NumProc();
  int localProc = map.Comm().MyPID();

  if (numProcs != 2) return(0);

  //so first we'll set up a epetra_test::matrix_data object with
  //contents that match the above-described matrix. (but the
  //matrix_data object will have all 4 rows on each processor)

  int i;
  int* rowlengths = new int[4];
  rowlengths[0] = 3;
  rowlengths[1] = 4;
  rowlengths[2] = 3;
  rowlengths[3] = 4;

  epetra_test::matrix_data matdata(4, rowlengths);
  for(i=0; i<4; ++i) {
    for(int j=0; j<matdata.rowlengths()[i]; ++j) {
      matdata.colindices()[i][j] = j;
    }
  }

  matdata.colindices()[0][2] = 3;

  matdata.colindices()[2][0] = 1;
  matdata.colindices()[2][1] = 2;
  matdata.colindices()[2][2] = 3;

  double** coefs = matdata.coefs();
  coefs[0][0] = 2.0; coefs[0][1] = 1.0;                    coefs[0][2] = 1.0;
  coefs[1][0] = 1.0; coefs[1][1] = 4.0; coefs[1][2] = 1.0; coefs[1][3] = 2.0;
                     coefs[2][0] = 1.0; coefs[2][1] = 2.0; coefs[2][2] = 1.0;
  coefs[3][0] = 1.0; coefs[3][1] = 2.0; coefs[3][2] = 1.0; coefs[3][3] = 4.0;

  //now we'll load a Epetra_FECrsMatrix with data that matches the
  //above-described finite-element problem.

  int indexBase = 0, ierr = 0;
  int* myNodes = new int[4];
  double* values = new double[9];
  values[0] = 2.0;
  values[1] = 1.0;
  values[2] = 1.0;
  values[3] = 1.0;
  values[4] = 2.0;
  values[5] = 1.0;
  values[6] = 1.0;
  values[7] = 1.0;
  values[8] = 2.0;

  int numMyNodes = 2;

  if (localProc == 0) {
    myNodes[0] = 0;
    myNodes[1] = 1;
  }
  else {
    myNodes[0] = 2;
    myNodes[1] = 3;
  }

  Epetra_Map Map(-1, numMyNodes, myNodes, indexBase, map.Comm());

  numMyNodes = 3;

  if (localProc == 0) {
    myNodes[0] = 0;
    myNodes[1] = 1;
    myNodes[2] = 3;
  }
  else {
    myNodes[0] = 1;
    myNodes[1] = 2;
    myNodes[2] = 3;
  }

  int rowLengths = 3;
  Epetra_FECrsMatrix A(Copy, Map, rowLengths);

  EPETRA_TEST_ERR( A.InsertGlobalValues(numMyNodes, myNodes,
                                        numMyNodes, myNodes, values,
                                        Epetra_FECrsMatrix::ROW_MAJOR),ierr);
  delete [] myNodes;
  delete [] values;

  EPETRA_TEST_ERR( A.GlobalAssemble(), ierr );

  //now the test is to check whether the FECrsMatrix data matches the
  //epetra_test::matrix_data object...

  bool the_same = matdata.compare_local_data(A);

  if (!the_same) {
    return(-1);
  }

  return(0);
}

int Drumm2(const Epetra_Map& map, bool verbose)
{
  //Simple 2-element problem (element as in "finite-element") from
  //Clif Drumm. Two triangular elements, one per processor, as shown
  //here:
  //
  //   *----*
  //  3|\  2|
  //   | \  |
  //   | 0\1|
  //   |   \|
  //   *----*
  //  0    1
  //
  //Element 0 on processor 0, element 1 on processor 1.
  //Processor 0 will own nodes 0,1,3 and processor 1 will own node 2.
  //Each processor will pass a 3x3 element-matrix to Epetra_FECrsMatrix.
  //After GlobalAssemble(), the matrix should be as follows:
  //
  //         row 0: 2  1  0  1
  //proc 0   row 1: 1  4  1  2
  //         row 2: 0  1  2  1
  //----------------------------------
  //proc 1   row 3: 1  2  1  4
  //

  int numProcs = map.Comm().NumProc();
  int localProc = map.Comm().MyPID();

  if (numProcs != 2) return(0);

  int indexBase = 0, ierr = 0;

  double* values = new double[9];
  values[0] = 2.0;
  values[1] = 1.0;
  values[2] = 1.0;
  values[3] = 1.0;
  values[4] = 2.0;
  values[5] = 1.0;
  values[6] = 1.0;
  values[7] = 1.0;
  values[8] = 2.0;

  if (localProc == 0) {
    int numMyNodes = 3;
    int* myNodes = new int[numMyNodes];
    myNodes[0] = 0;
    myNodes[1] = 1;
    myNodes[2] = 3;

    Epetra_Map Map(-1, numMyNodes, myNodes, indexBase, map.Comm());

    int rowLengths = 3;
    Epetra_FECrsMatrix A(Copy, Map, rowLengths);

    EPETRA_TEST_ERR( A.InsertGlobalValues(numMyNodes, myNodes,
			  numMyNodes, myNodes,
			  values, Epetra_FECrsMatrix::ROW_MAJOR),ierr);

    EPETRA_TEST_ERR( A.GlobalAssemble(), ierr );

    if (verbose) {
    A.Print(cout);
    }

    //now let's make sure we can do a matvec with this matrix.
    Epetra_Vector x(Map), y(Map);
    x.PutScalar(1.0);
    EPETRA_TEST_ERR( A.Multiply(false, x, y), ierr);

    if (verbose&&localProc==0) {
      cout << "y = A*x, x=1.0's"<<endl;
    }

    if (verbose) {
    y.Print(cout);
    }

    delete [] myNodes;
    delete [] values;
  }
  else {
    int numMyNodes = 1;
    int* myNodes = new int[numMyNodes];
    myNodes[0] = 2;

    Epetra_Map Map(-1, numMyNodes, myNodes, indexBase, map.Comm());

    int rowLengths = 3;
    Epetra_FECrsMatrix A(Copy, Map, rowLengths);

    delete [] myNodes;
    numMyNodes = 3;
    myNodes = new int[numMyNodes];
    myNodes[0] = 1;
    myNodes[1] = 2;
    myNodes[2] = 3;

    EPETRA_TEST_ERR( A.InsertGlobalValues(numMyNodes, myNodes,
			  numMyNodes, myNodes,
			  values, Epetra_FECrsMatrix::ROW_MAJOR),ierr);

    EPETRA_TEST_ERR( A.GlobalAssemble(), ierr );

    if (verbose) {
    A.Print(cout);
    }

    //now let's make sure we can do a matvec with this matrix.
    Epetra_Vector x(Map), y(Map);
    x.PutScalar(1.0);
    EPETRA_TEST_ERR( A.Multiply(false, x, y), ierr);

    if (verbose) {
    y.Print(cout);
    }

    delete [] myNodes;
    delete [] values;
  }

  return(0);
}

int Drumm3(const Epetra_Map& map, bool verbose)
{
  const Epetra_Comm & Comm = map.Comm();

  /* get number of processors and the name of this processor */

  int Numprocs = Comm.NumProc();
  int MyPID   = Comm.MyPID();

  if (Numprocs != 2) return(0);

  int NumGlobalRows = 4;
  int IndexBase = 0;
  Epetra_Map Map(NumGlobalRows, IndexBase, Comm);

  // Construct FECrsMatrix

  int NumEntriesPerRow = 3;

  Epetra_FECrsMatrix A(Copy, Map, NumEntriesPerRow);

  double ElementArea = 0.5;
  
  int NumCols = 3;
  int* Indices = new int[NumCols];

  if(MyPID==0)  // indices corresponding to element 0 on processor 0
  {
    Indices[0] = 0;
    Indices[1] = 1;
    Indices[2] = 3;
  }
  else if(MyPID==1)  // indices corresponding to element 1 on processor 1
  {
    Indices[0] = 1;
    Indices[1] = 2;
    Indices[2] = 3;
  }

  double* Values = new double[NumCols*NumCols];

// removal term
  Values[0] = 2*ElementArea/12.;
  Values[1] = 1*ElementArea/12.;
  Values[2] = 1*ElementArea/12.;
  Values[3] = 1*ElementArea/12.;
  Values[4] = 2*ElementArea/12.;
  Values[5] = 1*ElementArea/12.;
  Values[6] = 1*ElementArea/12.;
  Values[7] = 1*ElementArea/12.;
  Values[8] = 2*ElementArea/12.;

  A.InsertGlobalValues(NumCols, Indices,
                        Values,
                        Epetra_FECrsMatrix::ROW_MAJOR);

  A.GlobalAssemble();

//  A.Print(cout);

// Create vectors for CG algorithm

  Epetra_FEVector* bptr = new Epetra_FEVector(A.RowMap());
  Epetra_FEVector* x0ptr = new Epetra_FEVector(A.RowMap());

  Epetra_FEVector& b = *bptr;
  Epetra_FEVector& x0 = *x0ptr;

  // source terms
  NumCols = 2;

  if(MyPID==0)  // indices corresponding to element 0 on processor 0
  {
    Indices[0] = 0;
    Indices[1] = 3;

    Values[0] = 1./2.;
    Values[1] = 1./2.;

   }
   else
   {
    Indices[0] = 1;
    Indices[1] = 2;

    Values[0] = 0;
    Values[1] = 0;
   }

  b.SumIntoGlobalValues(NumCols, Indices, Values);

  b.GlobalAssemble();

  if (verbose&&MyPID==0) cout << "b:" << endl;
  if (verbose) {
  b.Print(cout);
  }

  x0 = b;

  if (verbose&&MyPID==0) {
  cout << "x:"<<endl;
  }

  if (verbose) {
  x0.Print(cout);
  }

  delete [] Values;
  delete [] Indices;

  delete bptr;
  delete x0ptr;

  return(0);
}

int four_quads(const Epetra_Comm& Comm, bool preconstruct_graph, bool verbose)
{
  if (verbose) {
    cout << "******************* four_quads ***********************"<<endl;
  }

  //This function assembles a matrix representing a finite-element mesh
  //of four 2-D quad elements. There are 9 nodes in the problem. The
  //same problem is assembled no matter how many processors are being used
  //(within reason). It may not work if more than 9 processors are used.
  //
  //  *------*------*
  // 6|     7|     8|
  //  | E2   | E3   |
  //  *------*------*
  // 3|     4|     5|
  //  | E0   | E1   |
  //  *------*------*
  // 0      1      2
  //
  //Nodes are denoted by * with node-numbers below and left of each node.
  //E0, E1 and so on are element-numbers.
  //
  //Each processor will contribute a sub-matrix of size 4x4, filled with 1's,
  //for each element. Thus, the coefficient value at position 0,0 should end up
  //being 1.0*numProcs, the value at position 4,4 should be 1.0*4*numProcs, etc.
  //
  //Depending on the number of processors being used, the locations of the
  //specific matrix positions (in terms of which processor owns them) will vary.
  //

  int numProcs = Comm.NumProc();

  int numNodes = 9;
  int numElems = 4;
  int numNodesPerElem = 4;

  int indexBase = 0;

  //Create a map using epetra-defined linear distribution.
  Epetra_Map map(numNodes, indexBase, Comm);

  Epetra_CrsGraph* graph = NULL;

  int* nodes = new int[numNodesPerElem];
  int i, j, err = 0;

  if (preconstruct_graph) {
    graph = new Epetra_CrsGraph(Copy, map, 1);

    //we're going to fill the graph with indices, but remember it will only
    //accept indices in rows for which map.MyGID(row) is true.

    for(i=0; i<numElems; ++i) {
      switch(i) {
      case 0:
	nodes[0] = 0; nodes[1] = 1; nodes[2] = 4; nodes[3] = 3;
	break;
      case 1:
	nodes[0] = 1; nodes[1] = 2; nodes[2] = 5; nodes[3] = 4;
	break;
      case 2:
	nodes[0] = 3; nodes[1] = 4; nodes[2] = 7; nodes[3] = 6;
	break;
      case 3:
	nodes[0] = 4; nodes[1] = 5; nodes[2] = 8; nodes[3] = 7;
	break;
      }

      for(j=0; j<numNodesPerElem; ++j) {
	if (map.MyGID(nodes[j])) {
	  err = graph->InsertGlobalIndices(nodes[j], numNodesPerElem,
					   nodes);
	  if (err<0) return(err);
	}
      }
    }

    EPETRA_CHK_ERR( graph->FillComplete() );
  }

  Epetra_FECrsMatrix* A = NULL;

  if (preconstruct_graph) {
    A = new Epetra_FECrsMatrix(Copy, *graph);
  }
  else {
    A = new Epetra_FECrsMatrix(Copy, map, 1);
  }

  EPETRA_CHK_ERR( A->PutScalar(0.0) );

  double* values_1d = new double[numNodesPerElem*numNodesPerElem];
  double** values_2d = new double*[numNodesPerElem];

  for(i=0; i<numNodesPerElem*numNodesPerElem; ++i) values_1d[i] = 1.0;

  int offset = 0;
  for(i=0; i<numNodesPerElem; ++i) {
    values_2d[i] = &(values_1d[offset]);
    offset += numNodesPerElem;
  }

  int format = Epetra_FECrsMatrix::ROW_MAJOR;
  Epetra_IntSerialDenseVector epetra_nodes(View, nodes, numNodesPerElem);
  Epetra_SerialDenseMatrix epetra_values(View, values_1d, numNodesPerElem,
					 numNodesPerElem, numNodesPerElem);

  for(i=0; i<numElems; ++i) {
    switch(i) {
    case 0:
      nodes[0] = 0; nodes[1] = 1; nodes[2] = 4; nodes[3] = 3;
      if (preconstruct_graph) {
	err = A->SumIntoGlobalValues(epetra_nodes,
				     epetra_values, format);
	if (err<0) return(err);
      }
      else {
	err = A->InsertGlobalValues(epetra_nodes,
				    epetra_values, format);
	if (err<0) return(err);
      }
      break;

    case 1:
      nodes[0] = 1; nodes[1] = 2; nodes[2] = 5; nodes[3] = 4;
      if (preconstruct_graph) {
	err = A->SumIntoGlobalValues(numNodesPerElem, nodes,
				     values_2d, format);
	if (err<0) return(err);
      }
      else {
	err = A->InsertGlobalValues(numNodesPerElem, nodes,
				    values_2d, format);
	if (err<0) return(err);
      }
      break;

    case 2:
      nodes[0] = 3; nodes[1] = 4; nodes[2] = 7; nodes[3] = 6;
      if (preconstruct_graph) {
	err = A->SumIntoGlobalValues(numNodesPerElem, nodes,
				     numNodesPerElem, nodes,
				     values_1d, format);
	if (err<0) return(err);
      }
      else {
	err = A->InsertGlobalValues(numNodesPerElem, nodes,
				    numNodesPerElem, nodes,
				    values_1d, format);
	if (err<0) return(err);
      }
      break;

     case 3:
      nodes[0] = 4; nodes[1] = 5; nodes[2] = 8; nodes[3] = 7;
      if (preconstruct_graph) {
	err = A->SumIntoGlobalValues(numNodesPerElem, nodes,
				     numNodesPerElem, nodes,
				     values_2d, format);
	if (err<0) return(err);
      }
      else {
	err = A->InsertGlobalValues(numNodesPerElem, nodes,
				    numNodesPerElem, nodes,
				    values_2d, format);
	if (err<0) return(err);
      }
      break;
    }
  }

  err = A->GlobalAssemble();
  if (err < 0) {
    return(err);
  }

  Epetra_Vector x(A->RowMap()), y(A->RowMap());

  x.PutScalar(1.0); y.PutScalar(0.0);

  Epetra_FECrsMatrix Acopy(*A);

  err = Acopy.GlobalAssemble();
  if (err < 0) {
    return(err);
  }

  bool the_same = epetra_test::compare_matrices(*A, Acopy);
  if (!the_same) {
    return(-1);
  }

  Epetra_FECrsMatrix Acopy2(Copy, A->RowMap(), A->ColMap(), 1);

  Acopy2 = Acopy;

  the_same = epetra_test::compare_matrices(*A, Acopy);
  if (!the_same) {
    return(-1);
  }

  int len = 20;
  int* indices = new int[len];
  double* values = new double[len];
  int numIndices;

  if (map.MyGID(0)) {
    EPETRA_CHK_ERR( A->ExtractGlobalRowCopy(0, len, numIndices,
					    values, indices) );
    if (numIndices != 4) {
      return(-1);
    }
    if (indices[0] != 0) {
      return(-2);
    }

    if (values[0] != 1.0*numProcs) {
      cout << "ERROR: values[0] ("<<values[0]<<") should be "<<numProcs<<endl;
      return(-3);
    }
  }

  if (map.MyGID(4)) {
    EPETRA_CHK_ERR( A->ExtractGlobalRowCopy(4, len, numIndices,
					    values, indices) );

    if (numIndices != 9) {
      return(-4);
    }
    int lcid = A->LCID(4);
    if (lcid<0) {
      return(-5);
    }
    if (values[lcid] != 4.0*numProcs) {
      cout << "ERROR: values["<<lcid<<"] ("<<values[lcid]<<") should be "
	   <<4*numProcs<<endl;
      return(-6);
    }
  }

  delete [] values_2d;
  delete [] values_1d;
  delete [] nodes;
  delete [] indices;
  delete [] values;

  delete A;
  delete graph;

  return(0);
}

int submatrix_formats(const Epetra_Comm& Comm, bool verbose)
{
  //
  //This function simply verifies that the ROW_MAJOR/COLUMN_MAJOR switch works.
  //
  int numProcs = Comm.NumProc();
  int myPID = Comm.MyPID();

  int numLocalElements = 3;
  int numGlobalElements = numLocalElements*numProcs;
  int indexBase = 0;

  Epetra_Map map(numGlobalElements, numLocalElements, indexBase, Comm);

  Epetra_FECrsMatrix A(Copy, map, numLocalElements);

  Epetra_IntSerialDenseVector epetra_indices(numLocalElements);

  int firstGlobalElement = numLocalElements*myPID;

  int i, j;
  for(i=0; i<numLocalElements; ++i) {
    epetra_indices[i] = firstGlobalElement+i;
  }

  Epetra_SerialDenseMatrix submatrix(numLocalElements, numLocalElements);

  for(i=0; i<numLocalElements; ++i) {
    for(j=0; j<numLocalElements; ++j) {
      submatrix(i,j) = 1.0*(firstGlobalElement+i);
    }
  }

  EPETRA_CHK_ERR( A.InsertGlobalValues(epetra_indices, submatrix) );

  EPETRA_CHK_ERR( A.GlobalAssemble() );

  int len = 20;
  int numIndices;
  int* indices = new int[len];
  double* coefs = new double[len];

  for(i=0; i<numLocalElements; ++i) {
    int row = firstGlobalElement+i;

    EPETRA_CHK_ERR( A.ExtractGlobalRowCopy(row, len, numIndices,
					   coefs, indices) );

    for(j=0; j<numIndices; ++j) {
      if (coefs[j] != 1.0*row) {
	return(-2);
      }
    }
  }

  //now reset submatrix (transposing the i,j indices)

  for(i=0; i<numLocalElements; ++i) {
    for(j=0; j<numLocalElements; ++j) {
      submatrix(j,i) = 1.0*(firstGlobalElement+i);
    }
  }

  //sum these values into the matrix using the ROW_MAJOR switch, which should
  //result in doubling what's already there from the above Insert operation.

  EPETRA_CHK_ERR( A.SumIntoGlobalValues(epetra_indices, submatrix,
					Epetra_FECrsMatrix::ROW_MAJOR) );

  EPETRA_CHK_ERR( A.GlobalAssemble() );

  for(i=0; i<numLocalElements; ++i) {
    int row = firstGlobalElement+i;

    EPETRA_CHK_ERR( A.ExtractGlobalRowCopy(row, len, numIndices,
					   coefs, indices) );

    for(j=0; j<numIndices; ++j) {
      if (coefs[j] != 2.0*row) {
	return(-3);
      }
    }
  }

  delete [] indices;
  delete [] coefs;

  return(0);
}

int rectangular(const Epetra_Comm& Comm, bool verbose)
{
  int numprocs = Comm.NumProc();
  int localproc = Comm.MyPID();
  int numMyRows = 2;
  int numGlobalRows = numprocs*numMyRows;
  int* myrows = new int[numMyRows];

  int myFirstRow = localproc*numMyRows;
  int i;
  for(i=0; i<numMyRows; ++i) {
    myrows[i] = myFirstRow+i;
  }

  Epetra_Map map(numGlobalRows, numMyRows, myrows, 0, Comm);

  Epetra_FECrsMatrix A(Copy, map, 30);

  int numcols = 20;
  int* cols = new int[numcols];
  for(i=0; i<numcols; ++i) {
    cols[i] = i;
  }

  double* coefs = new double[numGlobalRows*numcols];
  int offset = 0;
  for(int j=0; j<numcols; ++j) {
    for(i=0; i<numGlobalRows; ++i) {
      coefs[offset++] = 1.0*i;
    }
  }

  int* globalRows = new int[numGlobalRows];
  for(i=0; i<numGlobalRows; ++i) globalRows[i] = i;

  EPETRA_CHK_ERR( A.InsertGlobalValues(numGlobalRows, globalRows,
                                       numcols, cols, coefs));

  //Since the matrix is rectangular, we need to call GlobalAssemble with
  //a domain-map and a range-map. Otherwise, GlobalAssemble has no way of
  //knowing what the domain-map and range-map should be.
  //We'll use a linear distribution of the columns for a domain-map, and
  //our original row-map for the range-map.
  int numMyCols = numcols/numprocs;
  int rem = numcols%numprocs;
  if (localproc<rem) ++numMyCols;
  Epetra_Map domainmap(numcols, numMyCols, 0, Comm);

  EPETRA_CHK_ERR( A.GlobalAssemble(domainmap, map) );

  int numGlobalCols = A.NumGlobalCols();
  int numGlobalNNZ = A.NumGlobalNonzeros();

  if (numGlobalCols != numcols ||
      numGlobalNNZ != numGlobalRows*numcols) {
    return(-1);
  }

  delete [] cols;
  delete [] myrows;
  return(0);
}

