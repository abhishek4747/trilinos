/*--------------------------------------------------------------------*/
/*    Copyright 2003 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/
#ifndef PMMShapeImprover_hpp
#define PMMShapeImprover_hpp

#if !defined(__IBMCPP__)
#ifdef STK_BUILT_IN_SIERRA

#include <Mesquite.hpp>
#include <MsqError.hpp>
#include <MsqDebug.hpp>
#include <InstructionQueue.hpp>
#include <Settings.hpp>
#include <ShapeImprovementWrapper.hpp>
#include <UntangleWrapper.hpp>

#include <IdealWeightInverseMeanRatio.hpp> 
#include <QualityAssessor.hpp>
#include <TerminationCriterion.hpp>

#include <TQualityMetric.hpp>
#include <TShapeB1.hpp>
#include <IdealShapeTarget.hpp>

#include <MsqTimer.hpp>
#include <UntangleBetaQualityMetric.hpp>
#include <LPtoPTemplate.hpp>
#include <ConjugateGradient.hpp>
#include <FeasibleNewton.hpp>

#include <stk_percept/mesh/mod/mesquite-interface/PerceptMesquiteMesh.hpp>
#include <stk_percept/mesh/mod/mesquite-interface/PerceptMesquiteMeshDomain.hpp>

#undef USE_CALLGRIND
//#define USE_CALLGRIND
#ifdef USE_CALLGRIND
#include "/usr/netpub/valgrind-3.6.0/include/valgrind/callgrind.h"
#endif

namespace stk {
  namespace percept {

    using namespace Mesquite;

    const double DEF_UNT_BETA = 1e-8;
    const double DEF_SUC_EPS = 1e-4;

    class PMMShapeImprover
    {
    public:

      PMMShapeImprover() {}

      class PMMShapeImprovementWrapper : public Wrapper {
     
      public:  
        
        //Constructor sets the instructions in the queue.
        PMMShapeImprovementWrapper(MsqError& err,
                                   double cpu_time = 0.0, 
                                   double grad_norm =1.e-6,
                                   int parallel_iterations = 10)
          : maxTime(cpu_time), 
            gradNorm(grad_norm),
            untBeta(DEF_UNT_BETA),
            successiveEps(DEF_SUC_EPS),
            parallelIterations(parallel_iterations),
            m_do_untangle_only(false)
        {}

        //Constructor sets the instructions in the queue.
        PMMShapeImprovementWrapper(double cpu_time = 0.0, 
                                   double grad_norm =1.e-6,
                                   int parallel_iterations = 10)
          : maxTime(cpu_time), 
            gradNorm(grad_norm),
            untBeta(DEF_UNT_BETA),
            successiveEps(DEF_SUC_EPS),
            parallelIterations(parallel_iterations),
            m_do_untangle_only(false)
        {}


      protected:

        void run_wrapper( Mesh* mesh,
                          ParallelMesh* pmesh,
                          MeshDomain* domain,
                          Settings* settings,
                          QualityAssessor* qa,
                          MsqError& err )
        {
          // Define an untangler
          UntangleBetaQualityMetric untangle_metric( untBeta );
          LPtoPTemplate untangle_func( 2, &untangle_metric );
          ConjugateGradient untangle_solver( &untangle_func );
          TerminationCriterion untangle_inner, untangle_outer;
          untangle_solver.use_global_patch();
          untangle_inner.add_absolute_quality_improvement( 0.0 );
          untangle_inner.add_absolute_gradient_L2_norm( gradNorm );
          //untangle_inner.add_absolute_successive_improvement( successiveEps );
          untangle_inner.add_iteration_limit( 100 );
          untangle_inner.write_iterations("untangle.gpt", err);
          untangle_inner.add_untangled_mesh();
          untangle_outer.add_iteration_limit( 1 );
          untangle_solver.set_inner_termination_criterion( &untangle_inner );
          untangle_solver.set_outer_termination_criterion( &untangle_outer );

          // define shape improver
          IdealWeightInverseMeanRatio inverse_mean_ratio;
          inverse_mean_ratio.set_averaging_method( QualityMetric::LINEAR );
          LPtoPTemplate obj_func( 2, &inverse_mean_ratio );
          //FeasibleNewton shape_solver( &obj_func );
          ConjugateGradient shape_solver( &obj_func );
          TerminationCriterion term_inner, term_outer;
          shape_solver.use_global_patch();
          qa->add_quality_assessment( &inverse_mean_ratio );
          term_inner.add_absolute_gradient_L2_norm( gradNorm );
          //term_inner.add_relative_successive_improvement( successiveEps );
          term_inner.add_iteration_limit( 50 );
          term_inner.write_iterations("shape.gpt", err);
          term_outer.add_iteration_limit( pmesh ? parallelIterations : 1 );
          shape_solver.set_inner_termination_criterion( &term_inner );
          shape_solver.set_outer_termination_criterion( &term_outer );

          // Apply CPU time limit to untangler
          if (maxTime > 0.0)
            untangle_inner.add_cpu_time( maxTime );
  
          Timer totalTimer;

          // Run untangler
          std::cout << "\ntmp srk PMMShapeImprovementWrapper: running untangler...\n " << std::endl;
          bool use_untangle_wrapper = false;
          if (use_untangle_wrapper)
            {
              UntangleWrapper uw;
              //uw.set_untangle_metric(UntangleWrapper::BETA);
              uw.run_instructions(mesh, domain, err);
            }
          else
            {
              InstructionQueue q1;
              q1.set_master_quality_improver( &untangle_solver, err ); MSQ_ERRRTN(err);
              q1.add_quality_assessor( qa, err ); MSQ_ERRRTN(err);
              q1.run_common( mesh, pmesh, domain, settings, err ); 
            }
          std::cout << "\ntmp srk PMMShapeImprovementWrapper: running untangler... done\n " << std::endl;
          std::cout << "\ntmp srk PMMShapeImprovementWrapper: MsqError after untangler: " << err << std::endl;

          bool check_quality_after_untangler = true;
          if (check_quality_after_untangler)
            {
              int num_invalid = count_invalid_elements(*mesh, *domain);
              std::cout << "\ntmp srk PMMShapeImprover num_invalid after untangler= " << num_invalid << " " 
                        << (num_invalid ? " ERROR still have invalid elements after Mesquite untangle" : 
                            " SUCCESS: untangled invalid elements ")
                        << std::endl;
              if (num_invalid) return;
            }
          if (m_do_untangle_only) return;
          MSQ_ERRRTN(err);
  

          // If limited by CPU time, limit next step to remaning time
          if (maxTime > 0.0) {
            double remaining = maxTime - totalTimer.since_birth();
            if (remaining <= 0.0 ){
              MSQ_DBGOUT(2) << "Optimization is terminating without perfoming shape improvement." << std::endl;
              remaining = 0.0;
            }
            term_inner.add_cpu_time( remaining );
          }
  
          // Run shape improver
          InstructionQueue q2;
          std::cout << "\ntmp srk PMMShapeImprovementWrapper: running shape improver... \n" << std::endl;
          q2.add_quality_assessor( qa, err ); MSQ_ERRRTN(err);
          q2.set_master_quality_improver( &shape_solver, err ); MSQ_ERRRTN(err);
          q2.add_quality_assessor( qa, err ); MSQ_ERRRTN(err);
          q2.run_common( mesh, pmesh, domain, settings, err ); 
          std::cout << "\ntmp srk PMMShapeImprovementWrapper: running shape improver... done \n" << std::endl;
          MSQ_ERRRTN(err);
        }
      
      private:

        double maxTime, gradNorm;
        // constants
        const double untBeta;
        const double successiveEps;
        int parallelIterations;
      public:
        bool m_do_untangle_only;


      };

      static int count_invalid_elements(Mesh &mesh, MeshDomain &domain)
      {
        MsqError err;
        InstructionQueue q;
  
        // Set up barrier metric to see if mesh contains inverted elements
        TShapeB1 mu_b;
        IdealShapeTarget w_ideal;
        TQualityMetric barrier( &w_ideal, &mu_b );
  
        // Check for inverted elements in the mesh
        QualityAssessor inv_check( &barrier );
        //inv_check.disable_printing_results();
        q.add_quality_assessor( &inv_check, err );  MSQ_ERRZERO(err);
        Settings settings;
        q.run_common( &mesh, 0, &domain, &settings, err ); MSQ_ERRZERO(err);
        //q.remove_quality_assessor( 0, err ); MSQ_ERRZERO(err);
        const QualityAssessor::Assessor* inv_b = inv_check.get_results( &barrier );
        int num_invalid = inv_b->get_invalid_element_count();
        return num_invalid;
      }

      void run(PerceptMesquiteMesh &mesh, PerceptMesquiteMeshDomain &domain, bool always_smooth=true, int debug=0)
      {
#ifdef USE_CALLGRIND
  CALLGRIND_START_INSTRUMENTATION
  CALLGRIND_TOGGLE_COLLECT
#endif
        if (debug)
          {
            Mesquite::MsqDebug::enable(1);
            if (debug > 1) Mesquite::MsqDebug::enable(2);
            if (debug > 2) Mesquite::MsqDebug::enable(3);
          }
        Mesquite::MsqError mErr;
        int num_invalid = 0;
        bool check_quality=true;
        if (check_quality)
          {
            num_invalid = count_invalid_elements(mesh, domain);
            std::cout << "\ntmp srk PMMShapeImprover num_invalid before= " << num_invalid 
                      << (num_invalid ? " WARNING: invalid elements exist before Mesquite smoothing" : " ")
                      << std::endl;
          }

        if (num_invalid || always_smooth)
          {
            bool use_canned_wrapper = false;
            if (use_canned_wrapper)
              {
                Mesquite::ShapeImprovementWrapper siw(mErr);
                siw.run_instructions(&mesh, &domain, mErr);
              }
            else
              {
                bool do_untangle_only = false;
                PMMShapeImprovementWrapper siw(mErr);
                siw.m_do_untangle_only = do_untangle_only;
                siw.run_instructions(&mesh, &domain, mErr);
              }

            std::cout << "\ntmp srk PMMShapeImprover: MsqError after ShapeImprovementWrapper: " << mErr << std::endl;

            if (check_quality)
              {
                num_invalid = count_invalid_elements(mesh, domain);
                std::cout << "\ntmp srk PMMShapeImprover num_invalid after= " << num_invalid << " " 
                          << (num_invalid ? " ERROR still have invalid elements after Mesquite smoothing" : 
                              " SUCCESS: smoothed and removed invalid elements ")
                          << std::endl;
              }

            MSQ_ERRRTN(mErr);

          }
#ifdef USE_CALLGRIND
  CALLGRIND_TOGGLE_COLLECT
  CALLGRIND_STOP_INSTRUMENTATION
#endif
      }
    };

  }
}

#endif
#endif
#endif
