#include "Tpetra_CrsMatrixMultiplyOp.hpp"

#ifdef HAVE_TPETRA_EXPLICIT_INSTANTIATION

// #include "Tpetra_ExplicitInstantiationHelpers.hpp"

#include "Tpetra_CrsMatrixMultiplyOp_def.hpp"
// need this to instantiate CrsMatrix::multiply()
#include "Tpetra_CrsMatrix_def.hpp"

#include <Kokkos_SerialNode.hpp>
#if defined(HAVE_KOKKOS_TBB)
#  include <Kokkos_TBBNode.hpp>
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
#  include <Kokkos_TPINode.hpp>
#endif
#if defined(HAVE_KOKKOS_THRUST)
#  include <Kokkos_ThrustGPUNode.hpp>
#endif

namespace Tpetra {

  // instantiate all single-scalar implementations; these are needed internally by CrsMatrix

#if defined(HAVE_TPETRA_INST_FLOAT)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,float,int,int,Kokkos::SerialNode)
#if defined(HAVE_KOKKOS_TBB)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,float,int,int,Kokkos::TBBNode)
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,float,int,int,Kokkos::TPINode)
#endif
#if defined(HAVE_KOKKOS_THRUST) && defined(HAVE_KOKKOS_CUDA_FLOAT)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,float,int,int,Kokkos::ThrustGPUNode)
#endif
#endif

#if defined(HAVE_TPETRA_INST_DOUBLE)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,double,int,int,Kokkos::SerialNode)
#if defined(HAVE_KOKKOS_TBB)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,double,int,int,Kokkos::TBBNode)
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,double,int,int,Kokkos::TPINode)
#endif
#if defined(HAVE_KOKKOS_THRUST) && defined(HAVE_KOKKOS_CUDA_DOUBLE)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,double,int,int,Kokkos::ThrustGPUNode)
#endif
#endif

  // get all cross scalar applications

  // double x float
#if defined(HAVE_TPETRA_INST_DOUBLE) && defined(HAVE_TPETRA_INST_FLOAT)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,float,int,int,Kokkos::SerialNode)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,double,int,int,Kokkos::SerialNode)
#if defined(HAVE_KOKKOS_TBB)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,float,int,int,Kokkos::TBBNode)
  TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,double,int,int,Kokkos::TBBNode)
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,float,int,int,Kokkos::TPINode)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,double,int,int,Kokkos::TPINode)
#endif
#if defined(HAVE_KOKKOS_THRUST) && defined(HAVE_KOKKOS_CUDA_FLOAT) && defined(HAVE_KOKKOS_CUDA_DOUBLE)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(double,float,int,int,Kokkos::ThrustGPUNode)
    TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(float,double,int,int,Kokkos::ThrustGPUNode)
#endif
#endif

} // namespace Tpetra

#endif // HAVE_TPETRA_EXPLICIT_INSTANTIATION
