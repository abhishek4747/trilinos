#include "Simple_ModelEval.hpp"

#ifdef HAVE_MPI
#  include "Epetra_MpiComm.h"
#else
#  include "Epetra_SerialComm.h"
#endif // HAVE_MPI


Simple_ModelEval::Simple_ModelEval(const MPI_Comm appComm) 
{

#ifdef HAVE_MPI
    Comm = Teuchos::rcp(new Epetra_MpiComm(appComm));
cout << "XXX MPI" << endl;
#else
    Comm = Teuchos::rcp(new Epetra_SerialComm);
cout << "XXX NO MPI" << endl;
#endif

    //set up responses
    const int numResponses = 3;
    g_map = Teuchos::rcp(new Epetra_LocalMap(numResponses, 0, *Comm));

    //set up parameters
    const int numParameters= 3;
    p_map = Teuchos::rcp(new Epetra_LocalMap(numParameters, 0, *Comm));
    p_init = Teuchos::rcp(new Epetra_Vector(*p_map), false);
    for (int i=0; i<numParameters; i++)
      (*p_init)[i]= 1.0;

}

Simple_ModelEval::~Simple_ModelEval()
{
}

Teuchos::RCP<const Epetra_Map> Simple_ModelEval::get_x_map() const
{
  return Teuchos::null;
}

Teuchos::RCP<const Epetra_Map> Simple_ModelEval::get_f_map() const
{
  return Teuchos::null;
}

Teuchos::RCP<const Epetra_Map> Simple_ModelEval::get_p_map(int l) const
{
  TEST_FOR_EXCEPTION(l != 0, std::logic_error,
                     std::endl <<
                     "Error!  App::ModelEval::get_p_map() only " <<
                     " supports 1 parameter vector.  Supplied index l = " <<
                     l << std::endl);
  return p_map;
}

Teuchos::RCP<const Epetra_Map> Simple_ModelEval::get_g_map(int j) const
{
  TEST_FOR_EXCEPTION(j != 0, std::logic_error,
                     std::endl <<
                     "Error!  Simple_ModelEval::get_g_map() only " <<
                     " supports 1 parameter vector.  Supplied index l = " <<
                     j << std::endl);
  return g_map;
}

Teuchos::RCP<const Epetra_Vector> Simple_ModelEval::get_x_init() const
{
  return Teuchos::null;
}

Teuchos::RCP<const Epetra_Vector> Simple_ModelEval::get_p_init(int l) const
{
  TEST_FOR_EXCEPTION(l != 0, std::logic_error,
                     std::endl <<
                     "Error!  App::ModelEval::get_p_map() only " <<
                     " supports 1 parameter vector.  Supplied index l = " <<
                     l << std::endl);
  return p_init;
}

EpetraExt::ModelEvaluator::InArgs Simple_ModelEval::createInArgs() const
{
  EpetraExt::ModelEvaluator::InArgsSetup inArgs;
  inArgs.setModelEvalDescription(this->description());
  inArgs.set_Np(1);
  return inArgs;
}

EpetraExt::ModelEvaluator::OutArgs Simple_ModelEval::createOutArgs() const
{
  EpetraExt::ModelEvaluator::OutArgsSetup outArgs;
  outArgs.setModelEvalDescription(this->description());
  outArgs.set_Np_Ng(1, 1);
  outArgs.setSupports(OUT_ARG_DgDp, 0, 0, DerivativeSupport(DERIV_MV_BY_COL));

  return outArgs;
}

void Simple_ModelEval::evalModel( const InArgs& inArgs,
                              const OutArgs& outArgs ) const
{

  // Parse InArgs
  Teuchos::RCP<const Epetra_Vector> p_in = inArgs.get_p(0);
  if (!p_in.get()) cout << "ERROR: Simple_ModelEval requires p as inargs" << endl;
  int numParameters = p_in->GlobalLength();

  // Parse OutArgs

  Teuchos::RCP<Epetra_Vector> g_out = outArgs.get_g(0); 

  // Parse out-args for sensitivity calculation
  Teuchos::RCP<Epetra_MultiVector> dgdp_out;

  dgdp_out = outArgs.get_DgDp(0,0).getMultiVector();

  if (!is_null(g_out)) {
    (*g_out)[0] = 1.0 - (*p_in)[0];
    (*g_out)[1] = 1.2 - (*p_in)[1];
    (*g_out)[2] = 4.0 - (*p_in)[2];
  }

  if (dgdp_out != Teuchos::null) {
     for (int i=0; i<numParameters; i++) {
       (*dgdp_out)[i][i] = -1.0;
     }
   }

} 
