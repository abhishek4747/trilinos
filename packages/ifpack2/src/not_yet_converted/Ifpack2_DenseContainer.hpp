/*@HEADER
// ***********************************************************************
//
//       Ifpack2: Tempated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//@HEADER
*/

#ifndef IFPACK2_DENSECONTAINER_HPP
#define IFPACK2_DENSECONTAINER_HPP

#include "Ifpack2_ConfigDefs.hpp"
#include "Ifpack2_Container.hpp"
#include "Tpetra_SerialDenseMatrix.hpp"
#include "Tpetra_SerialDenseSolver.hpp"
#include "Tpetra_IntSerialDenseVector.hpp"
class Tpetra_RowMatrix;

//! Ifpack2_DenseContainer: a class to define containers for dense matrices.
/*!

<P>To understand what an TIFPACK container is, please refer to the documentation 
of the pure virtual class Ifpack2_Container. Currently, containers are
used by class Ifpack2_BlockRelaxation.

<P>Using block methods, one needs to store all diagonal blocks and
to be also to apply the inverse of each diagonal block. Using
class Ifpack2_DenseContainer, one can store the blocks as dense
matrices, which can be advantageous when the 
blocks are small. Otherwise,
class Ifpack2_SparseContainer is probably more appropriate.

<P>A typical use of a container is as follows:
\code
#include "Ifpack2_DenseContainer.hpp"
...

// local matrix of (5,5), with two vectors for solution and rhs.
Ifpack2_Container* Container = new
  Ifpack2_DenseContainer(5,5);
  
// assign local rows 1, 5, 12, 13, 16 to this container
Container(0) = 1;
Container(1) = 5;
Container(2) = 12;
Container(3) = 13;
Container(4) = 16;

// Now extract the submatrix corresponding to rows and columns:
// 1. initialize the container.
Container.Initialize();
// 2. extract matrix values from an Tpetra_RowMatrix A,
// and compute LU factors of the submatrix identified by rows
// and columns 1, 5, 12, 13 and 16 using LAPACK
Container.Compute(A);

// We can set the RHS as follows:
Container.RHS(0) = 1.0;
Container.RHS(1) = 2.0;
Container.RHS(2) = 3.0;
Container.RHS(3) = 4.0;
Container.RHS(4) = 5.0;

// The linear system with the submatrix is solved as follows:
Container.ApplyInverse().
\endcode

A call to Compute() computes the LU factorization of the
linear system matrix, using LAPACK (more precisely, by calling
the corresponding routines in Tpetra_SerialDenseSolver). 
The default behavior is 
to store the matrix factors by overwriting the linear system matrix
itself. This way, method Apply() fails, as the original matrix
does no longer exists. An alternative is to call
\c KeepNonFactoredMatrix(true), which forces Ifpack2_DenseContainer to
maintain in memory a copy of the non-factored matrix.

\author Michael Heroux, SNL 9214.

\date Last update Nov-04.
*/

class Ifpack2_DenseContainer : public Ifpack2_Container {

public:

  //@{ Constructors/Destructors

  //! Default constructor
  Ifpack2_DenseContainer(const int NumRows_in, const int NumVectors_in = 1) :
    NumRows_(NumRows_in),
    NumVectors_(NumVectors_in),
    KeepNonFactoredMatrix_(false),
    IsInitialized_(false),
    IsComputed_(false),
    ComputeFlops_(0.0),
    ApplyFlops_(0.0),
    ApplyInverseFlops_(0.0)
  {}

  //! Copy constructor
  Ifpack2_DenseContainer(const Ifpack2_DenseContainer& rhs) :
    NumRows_(rhs.NumRows()),
    NumVectors_(rhs.NumVectors()),
    KeepNonFactoredMatrix_(rhs.KeepNonFactoredMatrix()),
    IsInitialized_(rhs.IsInitialized()),
    IsComputed_(rhs.IsComputed())
  {
    Matrix_ = rhs.Matrix();
    if (KeepNonFactoredMatrix_)
      NonFactoredMatrix_ = rhs.NonFactoredMatrix();
    LHS_ = rhs.LHS();
    RHS_ = rhs.RHS();
    ID_ = rhs.ID();
  }
    
  //! Destructor.
  virtual ~Ifpack2_DenseContainer()
  {}
  //@}

  //@{ Overloaded operators.

  //! Operator=
  Ifpack2_DenseContainer& operator=(const Ifpack2_DenseContainer& rhs)
  {
    if (&rhs == this)
      return(*this);

    NumRows_ = rhs.NumRows();
    NumVectors_ = rhs.NumVectors();
    IsComputed_ = rhs.IsComputed();
    KeepNonFactoredMatrix_ = rhs.KeepNonFactoredMatrix();
    Matrix_ = rhs.Matrix();
    if (KeepNonFactoredMatrix_)
      NonFactoredMatrix_ = rhs.NonFactoredMatrix();
    LHS_ = rhs.LHS();
    RHS_ = rhs.RHS();
    ID_ = rhs.ID();

    return(*this);
  }

  //@}

  //@{ Get/Set methods.

  //! Returns the number of rows of the matrix and LHS/RHS.
  virtual int NumRows() const;

  //! Returns the number of vectors in LHS/RHS.
  virtual int NumVectors() const
  {
    return(NumVectors_);
  }

  //! Sets the number of vectors for LHS/RHS.
  virtual int SetNumVectors(const int NumVectors_in)
  {
    if (NumVectors_ == NumVectors_in) 
      return(0);

    NumVectors_ = NumVectors_in;
    IFPACK2_CHK_ERR(RHS_.Reshape(NumRows_,NumVectors_));
    IFPACK2_CHK_ERR(LHS_.Reshape(NumRows_,NumVectors_));
    // zero out vector elements
    for (int i = 0 ; i < NumRows_ ; ++i)
      for (int j = 0 ; j < NumVectors_ ; ++j) {
	LHS_(i,j) = 0.0;
	RHS_(i,j) = 0.0;
      }

    return(0);
  }

  //! Returns the i-th component of the vector Vector of LHS.
  virtual double& LHS(const int i, const int Vector = 0);
  
  //! Returns the i-th component of the vector Vector of RHS.
  virtual double& RHS(const int i, const int Vector = 0);

  //! Returns the ID associated to local row i. 
  /*!
   * The set of (local) rows assigned to this container is defined
   * by calling ID(i) = j, where i (from 0 to NumRows()) indicates
   * the container-row, and j indicates the local row in the calling
   * process.
   *
   * This is usually used to recorder the local row ID (on calling process)
   * of the i-th row in the container.
   */
  virtual int& ID(const int i);

  //! Set the matrix element (row,col) to \c value.
  virtual int SetMatrixElement(const int row, const int col,
			       const double value);

  //! Sets all necessary parameters.
  virtual int SetParameters(Teuchos::ParameterList& List)
  {
    return(0);
  }

  //! Returns \c true is the container has been successfully initialized.
  virtual bool IsInitialized() const
  {
    return(IsInitialized_);
  }

  //! Returns \c true is the container has been successfully computed.
  virtual bool IsComputed() const
  {
    return(IsComputed_);
  }

  //! Returns the label of \e this container.
  virtual const char* Label() const
  {
    return(Label_.c_str());
  }

  //! If \c flag is \c true, keeps a copy of the non-factored matrix.
  virtual int SetKeepNonFactoredMatrix(const bool flag)
  {
    KeepNonFactoredMatrix_ = flag;
    return(0);
  }

  //! Returns KeepNonFactoredMatrix_.
  virtual bool KeepNonFactoredMatrix() const
  {
    return(KeepNonFactoredMatrix_);
  }

  //! Returns the dense vector containing the LHS.
  virtual const Tpetra_SerialDenseMatrix& LHS() const
  {
    return(LHS_);
  }

  //! Returns the dense vector containing the RHS.
  virtual const Tpetra_SerialDenseMatrix& RHS() const
  {
    return(RHS_);
  }

  //! Returns the dense matrix or its factors.
  virtual const Tpetra_SerialDenseMatrix& Matrix() const
  {
    return(Matrix_);
  }

  //! Returns the non-factored dense matrix (only if stored).
  virtual const Tpetra_SerialDenseMatrix& NonFactoredMatrix() const
  {
    return(NonFactoredMatrix_);
  }

  //! Returns the integer dense vector of IDs.
  virtual const Tpetra_IntSerialDenseVector& ID() const
  {
    return(ID_);
  }

  //@}

  //@{ Mathematical methods.
  //! Initialize the container.
  virtual int Initialize();

  //! Finalizes the linear system matrix and prepares for the application of the inverse.
  virtual int Compute(const Tpetra_RowMatrix& Matrix_in);

  //! Apply the matrix to RHS, results are stored in LHS.
  virtual int Apply();

  //! Apply the inverse of the matrix to RHS, results are stored in LHS.
  virtual int ApplyInverse();

  //@}

  virtual double InitializeFlops() const
  {
    return(0.0);
  }

  virtual double ComputeFlops() const
  {
    return(ComputeFlops_);
  }

  virtual double ApplyFlops() const
  {
    return(ApplyFlops_);
  }

  virtual double ApplyInverseFlops() const
  {
    return(ApplyInverseFlops_);
  }

  //! Prints basic information on iostream. This function is used by operator<<.
  virtual ostream& Print(std::ostream& os) const;

private:
  
  //! Extract the submatrices identified by the ID set int ID().
  virtual int Extract(const Tpetra_RowMatrix& Matrix_in);

  //! Number of rows in the container.
  int NumRows_; 
  //! Number of vectors in the container.
  int NumVectors_;
  //! Dense matrix, that contains the non-factored matrix.
  Tpetra_SerialDenseMatrix NonFactoredMatrix_;
  //! Dense matrix.
  Tpetra_SerialDenseMatrix Matrix_;
  //! Dense vector representing the LHS.
  Tpetra_SerialDenseMatrix LHS_;
  //! Dense vector representing the RHS.
  Tpetra_SerialDenseMatrix RHS_;
  //! Dense solver (solution will be get using LAPACK).
  Tpetra_SerialDenseSolver Solver_;
  //! Sets of local rows.
  Tpetra_IntSerialDenseVector ID_;
  //! If \c true, keeps a copy of the non-factored matrix.
  bool KeepNonFactoredMatrix_;
  //! If \c true, the container has been successfully initialized.
  bool IsInitialized_;
  //! If \c true, the container has been successfully computed.
  bool IsComputed_;
  //! Label for \c this object
  string Label_;

  //! Flops in Compute().
  double ComputeFlops_;
  //! Flops in Apply().
  double ApplyFlops_;
  //! Flops in ApplyInverse().
  double ApplyInverseFlops_;
};

#endif
