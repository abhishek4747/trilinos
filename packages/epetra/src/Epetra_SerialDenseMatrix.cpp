
/* Copyright (2001) Sandia Corportation. Under the terms of Contract 
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this 
 * work by or on behalf of the U.S. Government.  Export of this program
 * may require a license from the United States Government. */


/* NOTICE:  The United States Government is granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in ths data to reproduce, prepare derivative works, and
 * perform publicly and display publicly.  Beginning five (5) years from
 * July 25, 2001, the United States Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable
 * worldwide license in this data to reproduce, prepare derivative works,
 * distribute copies to the public, perform publicly and display
 * publicly, and to permit others to do so.
 * 
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
 * OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
 * ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
 * RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
 * INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
 * THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS. */


#include "Epetra_SerialDenseMatrix.h"
#include "Epetra_SerialSymDenseMatrix.h"
#include "Epetra_SerialSymDenseMatrix.h"
//=============================================================================
Epetra_SerialDenseMatrix::Epetra_SerialDenseMatrix(void)
  : Epetra_CompObject(),
    M_(0),
    N_(0),
    LDA_(0),
    A_Copied_(false),
    A_(0)
{
}

//=============================================================================
Epetra_SerialDenseMatrix::Epetra_SerialDenseMatrix(Epetra_DataAccess CV, double *A, int LDA, 
					     int NumRows, int NumCols)
  : Epetra_CompObject(),
    M_(NumRows),
    N_(NumCols),
    LDA_(LDA),
    A_Copied_(false),    
    A_(A)

{
  if (CV==Copy) {
    LDA_ = M_;
    A_ = new double[LDA_*N_];
    CopyMat(A, LDA, M_, N_, A_, LDA_);
    A_Copied_ = true;
  }

}
//=============================================================================
Epetra_SerialDenseMatrix::Epetra_SerialDenseMatrix(const Epetra_SerialDenseMatrix& Source)
  : Epetra_CompObject(Source),  
    M_(Source.M_),
    N_(Source.N_),
    LDA_(Source.LDA_),
    A_Copied_(true),
    A_(Source.A_)

{

  LDA_ = M_;
  A_ = new double[LDA_*N_];
  CopyMat(Source.A_, Source.LDA_, M_, N_, A_, LDA_);
}

//=============================================================================
int Epetra_SerialDenseMatrix::Reshape(int NumRows, int NumCols) {

  // Allocate space for new matrix
  double * A_tmp = new double[NumRows*NumCols];
  for (int k = 0; k < NumRows*NumCols; k++) A_tmp[k] = 0.0; // Zero out values
  int M_tmp = EPETRA_MIN(M_, NumRows);
  int N_tmp = EPETRA_MIN(N_, NumCols);
  if (A_ != 0) CopyMat(A_, LDA_, M_tmp, N_tmp, A_tmp, NumRows); // Copy principal submatrix of A to new A
  
  DeleteArrays(); // Get rid of anything that might be already allocated  
  M_ = NumRows;
  N_ = NumCols;
  LDA_ = M_;
  A_ = A_tmp; // Set pointer to new A
  A_Copied_ = true;

  return(0);
}
int Epetra_SerialDenseMatrix::Shape(int NumRows, int NumCols) {
  DeleteArrays(); // Get rid of anything that might be already allocated
  M_ = NumRows;
  N_ = NumCols;
  LDA_ = M_;
  A_ = new double[LDA_*N_];
  for (int k = 0; k < LDA_*N_; k++) A_[k] = 0.0; // Zero out values
  A_Copied_ = true;

  return(0);
}
//=============================================================================
Epetra_SerialDenseMatrix::~Epetra_SerialDenseMatrix()
{
  DeleteArrays();
}
//=============================================================================
void Epetra_SerialDenseMatrix::DeleteArrays(void)
{
  if (A_Copied_)   {
    delete [] A_;
    A_ = 0;
    A_Copied_ = false;
  }
}
//=============================================================================
Epetra_SerialDenseMatrix & Epetra_SerialDenseMatrix::operator = ( const Epetra_SerialDenseMatrix & Source) {
  if (this==&Source) return(*this); // Special case of source same as target

  if (M()!=Source.M()) throw ReportError("Row dimension of source = " + toString(Source.M()) +
					  " is different than  row dimension of target = " + toString(LDA()), -1);
  if (N()!=Source.N()) throw ReportError("Column dimension of source = " + toString(Source.N()) +
					  " is different than column dimension of target = " + toString(N()), -2);

  CopyMat(Source.A(), Source.LDA(), Source.M(), Source.N(), A(), LDA());
  return(*this);
}
//=============================================================================
Epetra_SerialDenseMatrix & Epetra_SerialDenseMatrix::operator+= ( const Epetra_SerialDenseMatrix & Source) {

  if (M()!=Source.M()) throw ReportError("Row dimension of source = " + toString(Source.M()) +
					  " is different than  row dimension of target = " + toString(LDA()), -1);
  if (N()!=Source.N()) throw ReportError("Column dimension of source = " + toString(Source.N()) +
					  " is different than column dimension of target = " + toString(N()), -2);

  CopyMat(Source.A(), Source.LDA(), Source.M(), Source.N(), A(), LDA(), true);
  return(*this);
}
//=============================================================================
void Epetra_SerialDenseMatrix::CopyMat(double * A,
				       int LDA,
				       int NumRows,
				       int NumCols, 
				       double * B,
				       int LDB,
				       bool add)
{
  int i, j;
  double * ptr1 = B;
  double * ptr2;
  for (j=0; j<NumCols; j++) {
    ptr1 = B + j*LDB;
    ptr2 = A + j*LDA;
    if (add) {
      for (i=0; i<NumRows; i++) *ptr1++ += *ptr2++;
    }
    else {
      for (i=0; i<NumRows; i++) *ptr1++ = *ptr2++;
    }
  }
  return;
}
//=============================================================================
double Epetra_SerialDenseMatrix::OneNorm(void) {

  int i, j;

    double anorm = 0.0;
    double * ptr;
    for (j=0; j<N_; j++) {
      double sum=0.0;
      ptr = A_ + j*LDA_;
      for (i=0; i<M_; i++) sum += fabs(*ptr++);
      anorm = EPETRA_MAX(anorm, sum);
    }
    UpdateFlops(N_*N_);
    return(anorm);
}
//=============================================================================
double Epetra_SerialDenseMatrix::InfNorm(void) {

  int i, j;

    double anorm = 0.0;
    double * ptr;

    // Loop across columns in inner loop.  Most expensive memory access, but 
    // requires no extra storage.
    for (i=0; i<M_; i++) {
      double sum=0.0;
      ptr = A_ + i;
      for (j=0; j<N_; j++) {
	sum += fabs(*ptr);
	ptr += LDA_;
      }
      anorm = EPETRA_MAX(anorm, sum);
    }
    UpdateFlops(N_*N_);
    return(anorm);
}
//=========================================================================
double& Epetra_SerialDenseMatrix::operator () (int RowIndex, int ColIndex)  {

  if (RowIndex>=M_) throw ReportError("Row index = " +toString(RowIndex) + 
				      " Out of Range 0 - " + toString(M_-1),-1);
  if (ColIndex>=N_) throw ReportError("Column index = " +toString(ColIndex) + 
				      " Out of Range 0 - " + toString(N_-1),-2);

  return(A_[ColIndex*LDA_ + RowIndex]);
}

//=========================================================================
const double& Epetra_SerialDenseMatrix::operator () (int RowIndex, int ColIndex) const  {

  if (RowIndex>=M_) throw ReportError("Row index = " +toString(RowIndex) + 
				      " Out of Range 0 - " + toString(M_-1),-1);
  if (ColIndex>=N_) throw ReportError("Column index = " +toString(ColIndex) + 
				      " Out of Range 0 - " + toString(N_-1),-2);

   return(A_[ColIndex*LDA_ + RowIndex]);
}
//=========================================================================
const double* Epetra_SerialDenseMatrix::operator [] (int ColIndex) const  {

  if (ColIndex>=N_) throw ReportError("Column index = " +toString(ColIndex) + 
				      " Out of Range 0 - " + toString(N_-1),-2);
  return(A_ + ColIndex*LDA_);
}
//=========================================================================
double* Epetra_SerialDenseMatrix::operator [] (int ColIndex)  {

  if (ColIndex>=N_) throw ReportError("Column index = " +toString(ColIndex) + 
				      " Out of Range 0 - " + toString(N_-1),-2);
  return(A_+ ColIndex*LDA_);
}
//=========================================================================
int  Epetra_SerialDenseMatrix::Multiply (char TransA, char TransB, double ScalarAB, 
				      const Epetra_SerialDenseMatrix& A, 
				      const Epetra_SerialDenseMatrix& B,
				      double ScalarThis ) {
  // Check for compatible dimensions

  if (TransA!='T' && TransA!='N') EPETRA_CHK_ERR(-2); // Return error
  if (TransB!='T' && TransB!='N') EPETRA_CHK_ERR(-3);
  
  int A_nrows = (TransA=='T') ? A.N() : A.M();
  int A_ncols = (TransA=='T') ? A.M() : A.N();
  int B_nrows = (TransB=='T') ? B.N() : B.M();
  int B_ncols = (TransB=='T') ? B.M() : B.N();
  
  if (M_        != A_nrows     ||
      A_ncols   != B_nrows     ||
      N_        != B_ncols  ) EPETRA_CHK_ERR(-1); // Return error

    
  // Call GEMM function
  GEMM(TransA, TransB, M_, N_, A_ncols, ScalarAB, A.A(), A.LDA(), 
       B.A(), B.LDA(), ScalarThis, A_, LDA_);
  long int nflops = 2*M_;
  nflops *= N_;
  nflops *= A_ncols;
  if (ScalarAB != 1.0) nflops += M_*N_;
  if (ScalarThis != 0.0) nflops += M_*N_;
  UpdateFlops(nflops);

  return(0);
}
//=========================================================================
int  Epetra_SerialDenseMatrix::Multiply (char SideA, double ScalarAB, 
				      const Epetra_SerialSymDenseMatrix& A, 
				      const Epetra_SerialDenseMatrix& B,
				      double ScalarThis ) {
  // Check for compatible dimensions
  
  if (SideA=='R') {
    if (M_ != B.M() || 
	N_ != A.N() ||
	B.N() != A.M() ) EPETRA_CHK_ERR(-1); // Return error
  }
  else if (SideA=='L') {
    if (M_ != A.M() || 
	N_ != B.N() ||
	A.N() != B.M() ) EPETRA_CHK_ERR(-1); // Return error
  }
  else {
    EPETRA_CHK_ERR(-2); // Return error, incorrect value for SideA
  }
  
  // Call SYMM function
  SYMM(SideA, A.UPLO(), M_, N_, ScalarAB, A.A(), A.LDA(), 
       B.A(), B.LDA(), ScalarThis, A_, LDA_);
  long int nflops = 2*M_;
  nflops *= N_;
  nflops *= A.N();
  if (ScalarAB != 1.0) nflops += M_*N_;
  if (ScalarThis != 0.0) nflops += M_*N_;
  UpdateFlops(nflops);

  return(0);
}
void Epetra_SerialDenseMatrix::Print(ostream& os) const {

  os << endl;
  for (int i=0; i<M_; i++) {
    for (int j=0; j<N_; j++){
      os << (*this)(i,j) << " ";
    }
    os << endl;
  }
  return;
}
