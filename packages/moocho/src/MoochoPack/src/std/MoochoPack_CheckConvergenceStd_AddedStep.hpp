// ////////////////////////////////////////////////////////////////////////////
// CheckConvergenceStd_AddedStep.h

#ifndef CHECK_CONVERGENCE_STD_ADDEDSTEP_H
#define CHECK_CONVERGENCE_STD_ADDEDSTEP_H

#include "../rSQPAlgo_Step.h"
#include "Misc/include/StandardMemberCompositionMacros.h"

namespace ReducedSpaceSQPPack {

///
/** Check for convergence.
  */
class CheckConvergenceStd_AddedStep : public rSQPAlgo_Step {
public:

	///
	enum EOptErrorCheck { OPT_ERROR_REDUCED_GRADIENT_LAGR, OPT_ERROR_GRADIENT_LAGR };

	///
	/** <<std member comp>> members for whether the optimality conditions
	  * should be scaled by the 
	  */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( EOptErrorCheck, opt_error_check )

	///
	enum EScaleKKTErrorBy { SCALE_BY_ONE, SCALE_BY_NORM_2_X, SCALE_BY_NORM_INF_X };

	///
	/** <<std member comp>> members for whether the optimality conditions
	  * should be scaled by the 
	  */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( EScaleKKTErrorBy, scale_kkt_error_by )

	///
	/** <<std member comp>> members for whether the optimality conditions
	  * should be scaled by the 
	  */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( bool, scale_opt_error_by_Gf )

	///
	CheckConvergenceStd_AddedStep(
		  EOptErrorCheck opt_error_check		= OPT_ERROR_REDUCED_GRADIENT_LAGR
		, EScaleKKTErrorBy scale_kkt_error_by	= SCALE_BY_ONE
		, bool scale_opt_error_by_Gf 			= true
		);

	// ////////////////////
	// Overridden

	///
	bool do_step(Algorithm& algo, poss_type step_poss, GeneralIterationPack::EDoStepType type
		, poss_type assoc_step_poss);

	///
	void print_step( const Algorithm& algo, poss_type step_poss, GeneralIterationPack::EDoStepType type
		, poss_type assoc_step_poss, std::ostream& out, const std::string& leading_str ) const;

};	// end class CheckConvergenceStd_AddedStep

}	// end namespace ReducedSpaceSQPPack 

#endif	// CHECK_CONVERGENCE_STD_ADDEDSTEP_H