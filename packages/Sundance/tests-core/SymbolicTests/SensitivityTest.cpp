#include "SundanceExpr.hpp"
#include "SundanceStdMathOps.hpp"
#include "SundanceDerivative.hpp"
#include "SundanceUnknownFunctionStub.hpp"
#include "SundanceUnknownFuncElement.hpp"
#include "SundanceUnknownParameter.hpp"
#include "SundanceTestFunctionStub.hpp"
#include "SundanceDiscreteFunctionStub.hpp"
#include "SundanceCoordExpr.hpp"
#include "SundanceZeroExpr.hpp"
#include "SundanceSymbolicTransformation.hpp"
#include "SundanceDeriv.hpp"
#include "SundanceParameter.hpp"
#include "SundanceOut.hpp"
#include "Teuchos_Time.hpp"
#include "Teuchos_GlobalMPISession.hpp"
#include "Teuchos_TimeMonitor.hpp"
#include "SundanceDerivSet.hpp"
#include "SundanceRegionQuadCombo.hpp"
#include "SundanceEvalManager.hpp"
#include "SundanceEvalVector.hpp"
#include "SundanceSymbPreprocessor.hpp"
#include "SundanceStringEvalMediator.hpp"

using namespace SundanceUtils;
using namespace SundanceCore;
using namespace SundanceCore;
using namespace Teuchos;

using SundanceCore::List;

using SundanceCore::UnknownFuncElement;

static Time& totalTimer() 
{
  static RefCountPtr<Time> rtn 
    = TimeMonitor::getNewTimer("total"); 
  return *rtn;
}

static Time& doitTimer() 
{
  static RefCountPtr<Time> rtn 
    = TimeMonitor::getNewTimer("doit"); 
  return *rtn;
}



void doit(const Expr& e, 
          const Expr& tests,
          const Expr& unks,
          const Expr& u0, 
          const Expr& unkParams,
          const Expr& paramVals, 
          const EvalContext& region)
{
  TimeMonitor t0(doitTimer());
  EvalManager mgr;
  mgr.setRegion(region);
  mgr.setVerbosity(5);

  static RefCountPtr<AbstractEvalMediator> mediator 
    = rcp(new StringEvalMediator());

  mgr.setMediator(mediator);

  const EvaluatableExpr* ev 
    = dynamic_cast<const EvaluatableExpr*>(e[0].ptr().get());

  Expr fixed;
  Expr fixed0;

  Out::os() << "params = " << unkParams << endl;
  Out::os() << "param vals = " << paramVals << endl;

  DerivSet d = SymbPreprocessor::setupSensitivities(e[0], 
                                                    tests,
                                                    unks,
                                                    u0,
                                                    unkParams,
                                                    paramVals,
                                                    fixed,
                                                    fixed0,
                                                    fixed0,
                                                    fixed0,
    region,
    Sensitivities);

  Tabs tab;
  Out::os() << tab << "done setup" << endl;
  Out::os() << tab << *ev->sparsitySuperset(region) << endl;
  //  ev->showSparsity(Out::os(), region);

  // RefCountPtr<EvalVectorArray> results;

  Array<double> constantResults;
  Array<RefCountPtr<EvalVector> > vectorResults;

  Out::os() << tab << "starting eval" << endl;
  ev->evaluate(mgr, constantResults, vectorResults);

  ev->sparsitySuperset(region)->print(Out::os(), vectorResults, constantResults);

  
  // results->print(Out::os(), ev->sparsitySuperset(region).get());
}



void testExpr(const Expr& e,  
              const Expr& tests,
              const Expr& unks,
              const Expr& u0, 
          const Expr& unkParams,
          const Expr& paramVals, 
              const EvalContext& region)
{
  Out::os() << endl 
       << "------------------------------------------------------------- " << endl;
  Out::os()  << "-------- testing " << e.toString() << " -------- " << endl;
  Out::os() << endl 
       << "------------------------------------------------------------- " << endl;

  try
    {
      doit(e, tests, unks, u0, unkParams, paramVals, region);
    }
  catch(exception& ex)
    {
      Out::os() << "EXCEPTION DETECTED!" << endl;
      Out::os() << ex.what() << endl;
      // Out::os() << "repeating with increased verbosity..." << endl;
      //       Out::os() << "-------- testing " << e.toString() << " -------- " << endl;
      //       Evaluator::verbosity() = 2;
      //       EvalVector::verbosity() = 2;
      //       EvaluatableExpr::verbosity() = 2;
      //       Expr::showAllParens() = true;
      //       doit(e, region);
      exit(1);
    }
}

int main(int argc, char** argv)
{
  
  try
		{
      GlobalMPISession session(&argc, &argv);


//      verbosity<SymbolicTransformation>() = 0;
      verbosity<Evaluator>() = 6;
//      verbosity<EvalVector>() = 6;
      verbosity<EvaluatableExpr>() = 6;
      Expr::showAllParens() = true;

      EvalVector::shadowOps() = true;

      Expr dx = new Derivative(0);
      Expr dy = new Derivative(1);

			Expr u = new UnknownFunctionStub("u");
			Expr alpha = new UnknownParameter("alpha");
			Expr alpha0 = new Parameter(3.14, "alpha0");
			Expr beta = new UnknownParameter("beta");
			Expr beta0 = new Parameter(2.72, "beta0");
			Expr v = new TestFunctionStub("v");

      Out::os() << "u=" << u << endl;
      Out::os() << "v=" << v << endl;
      Out::os() << "alpha=" << alpha << endl;

      Expr x = new CoordExpr(0);
      Expr y = new CoordExpr(1);

      Expr u0 = new DiscreteFunctionStub("u0");
      Expr zero = new ZeroExpr();

      Array<Expr> tests;

      

      tests.append(v*dx*(u*u - x*u) + dx*(v*alpha*u) + v*sin(alpha*u) + 2.0*v + v*exp(u*beta));
//      tests.append(v*sin(alpha*u));
      //tests.append(v*alpha*u);


      for (int i=0; i<tests.length(); i++)
        {
          RegionQuadCombo rqc(rcp(new CellFilterStub()), 
                              rcp(new QuadratureFamilyStub(1)));
          EvalContext context(rqc, makeSet(2), EvalContext::nextID());
          context.setSetupVerbosity(5);
          testExpr(tests[i], 
            SundanceCore::List(v),
            SundanceCore::List(u),
            SundanceCore::List(u0),
            SundanceCore::List(alpha, beta),
            SundanceCore::List(alpha0, beta0),
            context);
        }

      
    }
	catch(exception& e)
		{
			Out::println(e.what());
		}
  
}
