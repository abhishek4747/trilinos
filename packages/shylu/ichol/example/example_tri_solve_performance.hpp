#pragma once
#ifndef __EXAMPLE_TRI_SOLVE_PERFORMANCE_HPP__
#define __EXAMPLE_TRI_SOLVE_PERFORMANCE_HPP__

#include <Kokkos_Core.hpp>
#include <impl/Kokkos_Timer.hpp>

#include "util.hpp"

#include "crs_matrix_base.hpp"
#include "crs_matrix_view.hpp"
#include "crs_row_view.hpp"

#include "dense_matrix_base.hpp"
#include "dense_matrix_view.hpp"

#include "graph_helper_scotch.hpp"
#include "crs_matrix_helper.hpp"
#include "dense_matrix_helper.hpp"

#include "team_view.hpp"
#include "task_view.hpp"

#include "parallel_for.hpp"

#include "team_factory.hpp"
#include "task_factory.hpp"
#include "task_team_factory.hpp"

#include "tri_solve.hpp"

namespace Example {

  using namespace std;

#define __INIT_DENSE_MATRIX__(Mat, Val)                 \
  {                                                     \
    for (ordinal_type j=0;j<Mat.NumCols();++j)          \
      for (ordinal_type i=0;i<Mat.NumRows();++i)        \
        Mat.Value(i, j) = Val;                          \
  }
  
  template<typename ValueType,
           typename OrdinalType,
           typename SizeType = OrdinalType,
           typename SpaceType = void,
           typename MemoryTraits = void>
  KOKKOS_INLINE_FUNCTION
  int exampleTriSolvePerformance(const string file_input,
                                 const OrdinalType nrhs,
                                 const OrdinalType nb,
                                 const int niter,
                                 const int nthreads,
                                 const int max_task_dependence,
                                 const int team_size, 
                                 const bool team_interface,
                                 const bool verbose) {
    typedef ValueType   value_type;
    typedef OrdinalType ordinal_type;
    typedef SizeType    size_type;

    typedef TaskTeamFactory<Kokkos::Experimental::TaskPolicy<SpaceType>,
      Kokkos::Experimental::Future<int,SpaceType>,
      Kokkos::Impl::TeamThreadRangeBoundariesStruct> TaskFactoryType;
    typedef ParallelFor ForType;

    typedef CrsMatrixBase<value_type,ordinal_type,size_type,SpaceType,MemoryTraits> CrsMatrixBaseType;
    typedef GraphHelper_Scotch<CrsMatrixBaseType> GraphHelperType;

    typedef CrsMatrixView<CrsMatrixBaseType> CrsMatrixViewType;
    typedef TaskView<CrsMatrixViewType,TaskFactoryType> CrsTaskViewType;
    
    typedef CrsMatrixBase<CrsTaskViewType,ordinal_type,size_type,SpaceType,MemoryTraits> CrsHierMatrixBaseType;

    typedef CrsMatrixView<CrsHierMatrixBaseType> CrsHierMatrixViewType;
    typedef TaskView<CrsHierMatrixViewType,TaskFactoryType> CrsHierTaskViewType;

    typedef DenseMatrixBase<value_type,ordinal_type,size_type,SpaceType,MemoryTraits> DenseMatrixBaseType;

    typedef DenseMatrixView<DenseMatrixBaseType> DenseMatrixViewType;
    typedef TaskView<DenseMatrixViewType,TaskFactoryType> DenseTaskViewType;

    typedef DenseMatrixBase<DenseTaskViewType,ordinal_type,size_type,SpaceType,MemoryTraits> DenseHierMatrixBaseType;

    typedef DenseMatrixView<DenseHierMatrixBaseType> DenseHierMatrixViewType;
    typedef TaskView<DenseHierMatrixViewType,TaskFactoryType> DenseHierTaskViewType;

    int r_val = 0;

    Kokkos::Impl::Timer timer;
    double 
      t_import = 0.0,
      t_reorder = 0.0,
      t_solve_seq = 0.0,
      //t_solve_team = 0.0,
      t_solve_task = 0.0;
    const int start = -2;

    cout << "TriSolvePerformance:: import input file = " << file_input << endl;
    CrsMatrixBaseType AA("AA");
    {
      timer.reset();

      ifstream in;
      in.open(file_input);
      if (!in.good()) {
        cout << "Failed in open the file: " << file_input << endl;
        return ++r_val;
      }
      AA.importMatrixMarket(in);

      t_import = timer.seconds();

      if (verbose)
        cout << AA << endl;
    }
    cout << "TriSolvePerformance:: import input file::time = " << t_import << endl;

    CrsMatrixBaseType   UU("UU");
    DenseMatrixBaseType BB("BB",  AA.NumRows(), nrhs);

    cout << "TriSolvePerformance:: reorder the matrix and partition right hand side, nb = " << nb << endl;
    CrsHierMatrixBaseType   HU("HU");
    DenseHierMatrixBaseType HB("HB");
    {
      timer.reset();

      GraphHelperType S(AA);
      S.computeOrdering();

      CrsMatrixBaseType PA("Permuted AA");
      PA.copy(S.PermVector(), S.InvPermVector(), AA);

      UU.copy(Uplo::Upper, PA);

      CrsMatrixHelper::flat2hier(Uplo::Upper, UU, HU,
                                 S.NumBlocks(),
                                 S.RangeVector(),
                                 S.TreeVector());

      DenseMatrixHelper::flat2hier(BB, HB,
                                   S.NumBlocks(),
                                   S.RangeVector(),
                                   nb);

      t_reorder = timer.seconds();

      if (verbose)
        cout << UU << endl;
    }
    cout << "TriSolvePerformance:: reorder the matrix and partition right hand side::time = " << t_reorder << endl;

    {
      __INIT_DENSE_MATRIX__(BB, 1.0);
#ifdef __USE_FIXED_TEAM_SIZE__
      typename TaskFactoryType::policy_type policy(max_task_dependence);
#else
      typename TaskFactoryType::policy_type policy(max_task_dependence, 1);
#endif
      TaskFactoryType::setUseTeamInterface(team_interface);
      TaskFactoryType::setMaxTaskDependence(max_task_dependence);
      TaskFactoryType::setPolicy(&policy);
      
      CrsTaskViewType U(&UU);
      DenseTaskViewType B(&BB);
      U.fillRowViewArray();

      cout << "TriSolvePerformance:: Serial forward and backward solve of the matrix" << endl;
      {

        for (int i=start;i<niter;++i) {
          timer.reset();
          // {
          //   auto future = TaskFactoryType::Policy().create_team(TriSolve<Uplo::Upper,Trans::ConjTranspose,AlgoTriSolve::Unblocked>
          //                                                       ::TaskFunctor<ForType,CrsTaskViewType,DenseTaskViewType>
          //                                                       (Diag::NonUnit, U, B), 0);
            
          //   TaskFactoryType::Policy().spawn(future);
          //   Kokkos::Experimental::wait(TaskFactoryType::Policy());
          // }
          {
            TriSolve<Uplo::Upper,Trans::ConjTranspose,AlgoTriSolve::Unblocked>
              ::invoke<ForType>(TaskFactoryType::Policy(),
                                TaskFactoryType::Policy().member_single(),
                                Diag::NonUnit, U, B);
            
          }
          // {
          //   auto future = TaskFactoryType::Policy().create_team(TriSolve<Uplo::Upper,Trans::NoTranspose,AlgoTriSolve::Unblocked>
          //                                                       ::TaskFunctor<ForType,CrsTaskViewType,DenseTaskViewType>
          //                                                       (Diag::NonUnit, U, B), 0);
            
          //   TaskFactoryType::Policy().spawn(future);
          //   Kokkos::Experimental::wait(TaskFactoryType::Policy());
          // }
          {
            TriSolve<Uplo::Upper,Trans::NoTranspose,AlgoTriSolve::Unblocked>
              ::invoke<ForType>(TaskFactoryType::Policy(),
                                TaskFactoryType::Policy().member_single(),
                                Diag::NonUnit, U, B);
          }
          t_solve_seq += timer.seconds() * (i>=0);
        }
        t_solve_seq /= niter;
        
        if (verbose)
          cout << BB << endl;
      }
      cout << "TriSolvePerformance:: Serial forward and backward solve of the matrix::time = " << t_solve_seq << endl;
    }
    

//     {
//       __INIT_DENSE_MATRIX__(BB, 1.0);
// #ifdef __USE_FIXED_TEAM_SIZE__
//       typename TaskFactoryType::policy_type policy(max_task_dependence);
// #else
//       typename TaskFactoryType::policy_type policy(max_task_dependence, nthreads);
// #endif
//       TaskFactoryType::setPolicy(&policy);
      
//       CrsTaskViewType U(&UU);
//       DenseTaskViewType B(&BB);
//       U.fillRowViewArray();
      
//       {
//         timer.reset();
        
//         {
//           auto future = TaskFactoryType::Policy().create_team(TriSolve<Uplo::Upper,Trans::ConjTranspose,AlgoTriSolve::Unblocked>
//                                                               ::TaskFunctor<ForType,CrsTaskViewType,DenseTaskViewType>
//                                                               (Diag::NonUnit, U, B), 0);

//           TaskFactoryType::Policy().spawn(future);
//           Kokkos::Experimental::wait(TaskFactoryType::Policy());
//         }
//         {
//           auto future = TaskFactoryType::Policy().create_team(TriSolve<Uplo::Upper,Trans::NoTranspose,AlgoTriSolve::Unblocked>
//                                                               ::TaskFunctor<ForType,CrsTaskViewType,DenseTaskViewType>
//                                                               (Diag::NonUnit, U, B), 0);

//           TaskFactoryType::Policy().spawn(future);
//           Kokkos::Experimental::wait(TaskFactoryType::Policy());
//         }

//         t_solve_team = timer.seconds();

//         if (verbose)
//           cout << BB << endl;
//       }
//       cout << "TriSolvePerformance:: Team forward and backward solve of the matrix::time = " << t_solve_team << endl;
//     }
    
    {
      __INIT_DENSE_MATRIX__(BB, 1.0);
#ifdef __USE_FIXED_TEAM_SIZE__
      typename TaskFactoryType::policy_type policy(max_task_dependence);
#else
      typename TaskFactoryType::policy_type policy(max_task_dependence, team_size);
#endif
      TaskFactoryType::setUseTeamInterface(team_interface);
      TaskFactoryType::setMaxTaskDependence(max_task_dependence);
      TaskFactoryType::setPolicy(&policy);

      // wrap the hierarchically partitioned matrix with task handler
      CrsHierTaskViewType TU(&HU);
      for (ordinal_type k=0;k<HU.NumNonZeros();++k)
        HU.Value(k).fillRowViewArray();
      
      DenseHierTaskViewType TB(&HB);
      
      cout << "TriSolvePerformance:: ByBlocks forward and backward solve of the matrix" << endl;
      {
        for (int i=start;i<niter;++i) {
          timer.reset(); 
          {
            auto future_forward_solve = TaskFactoryType::Policy().create_team
              (TriSolve<Uplo::Upper,Trans::ConjTranspose,AlgoTriSolve::ByBlocks>
               ::TaskFunctor<ForType,CrsHierTaskViewType,DenseHierTaskViewType>
               (Diag::NonUnit, TU, TB), 0);
            
            TaskFactoryType::Policy().spawn(future_forward_solve);
            
            auto future_backward_solve = TaskFactoryType::Policy().create_team
              (TriSolve<Uplo::Upper,Trans::NoTranspose,AlgoTriSolve::ByBlocks>
               ::TaskFunctor<ForType,CrsHierTaskViewType,DenseHierTaskViewType>
               (Diag::NonUnit, TU, TB), 1);
            
            TaskFactoryType::Policy().add_dependence(future_backward_solve, future_forward_solve);
            TaskFactoryType::Policy().spawn(future_backward_solve);
            
            Kokkos::Experimental::wait(TaskFactoryType::Policy());
          }
          t_solve_task += timer.seconds() * (i>=0);
        }
        t_solve_task /= niter;

        if (verbose)
          cout << BB << endl;
      }

      cout << "TriSolvePerformance:: ByBlocks forward and backward solve of the matrix::time = " << t_solve_task << endl;
    }

    cout << "TriSolvePerformance:: task scale [seq/task] = " << t_solve_seq/t_solve_task << endl;
    //cout << "TriSolvePerformance:: team scale [seq/team] = " << t_solve_seq/t_solve_team << endl;

    return r_val;
  }
}

#endif
