/*! \@HEADER */
/*
************************************************************************

                CTrilinos:  C interface to Trilinos
                Copyright (2009) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact M. Nicole Lemaster (mnlemas\@sandia.gov)

************************************************************************
*/
/*! \@HEADER */


#include "CTrilinos_config.h"


#ifdef HAVE_CTRILINOS_AMESOS

#include "CAmesos_BaseSolver.h"
#include "CEpetra_LinearProblem.h"
#include "CTeuchos_ParameterList.h"
#include "Amesos.h"
#include "CAmesos.h"
#include "CAmesos_Cpp.hpp"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_exceptions.hpp"
#include "CTrilinos_utils.hpp"

#include "CTrilinos_UnitTestHelpers.hpp"
#include "Teuchos_UnitTestHarness.hpp"


namespace {



/**********************************************************************
CT_Amesos_ID_t Amesos_Create (  );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Amesos , Create )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Amesos_ID_t selfID = Amesos_Create());

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(selfID.type, CT_Amesos_ID);
}

/**********************************************************************
void Amesos_Destroy ( CT_Amesos_ID_t * selfID );
 **********************************************************************/

/**********************************************************************
CT_Amesos_BaseSolver_ID_t Amesos_CreateSolver ( 
  CT_Amesos_ID_t selfID, const char * ClassType, 
  CT_Epetra_LinearProblem_ID_t LinearProblemID );
 **********************************************************************/

/**********************************************************************
boolean Amesos_Query ( 
  CT_Amesos_ID_t selfID, const char * ClassType );
 **********************************************************************/

/**********************************************************************
CT_Teuchos_ParameterList_ID_t Amesos_GetValidParameters (  );
 **********************************************************************/

/**********************************************************************/

//
// Template Instantiations
//


#ifdef TEUCHOS_DEBUG

#  define DEBUG_UNIT_TEST_GROUP( T ) \

#else

#  define DEBUG_UNIT_TEST_GROUP( T )

#endif


#define UNIT_TEST_GROUP( T ) \
  DEBUG_UNIT_TEST_GROUP( T )


} // namespace

#endif /* HAVE_CTRILINOS_AMESOS */

