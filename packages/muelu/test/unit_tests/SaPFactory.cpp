#include "Teuchos_UnitTestHarness.hpp"
#include "MueLu_TestHelpers.hpp"
#include "MueLu_Version.hpp"

#include "MueLu_SaPFactory.hpp"
#include "MueLu_TrilinosSmoother.hpp"

#include "MueLu_UseDefaultTypes.hpp"
#include "MueLu_UseShortNames.hpp"

namespace MueLuTests {

  TEUCHOS_UNIT_TEST(SaPFactory, Test0)
  {
    out << "version: " << MueLu::Version() << std::endl;

    RCP<SaPFactory> sapFactory = rcp(new SaPFactory);
    TEST_EQUALITY(sapFactory != Teuchos::null, true);

    out << *sapFactory << std::endl;

  }

  TEUCHOS_UNIT_TEST(SaPFactory, GetSetMethods)
  {
    out << "version: " << MueLu::Version() << std::endl;

    RCP<SaPFactory> sapFactory = rcp(new SaPFactory);
    sapFactory->SetDampingFactor( (Scalar)4/3 );
    TEST_EQUALITY(((Scalar)4/3) == sapFactory->GetDampingFactor(), true);
    sapFactory->SetDiagonalView("roomWithAView");
    TEST_EQUALITY( sapFactory->GetDiagonalView(), "roomWithAView");

  } //GetSetMethods

#if defined(HAVE_MUELU_EPETRA) && defined(HAVE_MUELU_TPETRA)
  TEUCHOS_UNIT_TEST(SaPFactory, SaPFactory_EpetraVsTpetra)
  {
    out << "version: " << MueLu::Version() << std::endl;
    out << "Compare results of Epetra and Tpetra" << std::endl;
    out << "for 3 level AMG solver using smoothed aggregation with" << std::endl;
    out << "one SGS sweep on each multigrid level as pre- and postsmoother" << std::endl;

    RCP<const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();

    Teuchos::Array<ST::magnitudeType> results(2);

    // run test only on 1 proc
    if(comm->getSize() == 1)
    {
        Xpetra::UnderlyingLib lib = Xpetra::UseEpetra;

        // run Epetra and Tpetra test
        for (int run = 0; run < 2; run++) //TODO: create a subfunction instead or Tuple of UnderlyingLib
        {
            if (run == 0) lib = Xpetra::UseEpetra;
            else lib = Xpetra::UseTpetra;

            // generate problem
            LO maxLevels = 3;
            LO its=10;
            LO nEle = 63;
            const RCP<const Map> map = MapFactory::Build(lib, nEle, 0, comm);
            Teuchos::ParameterList matrixParameters;
            matrixParameters.set("nx",nEle);
            RCP<Operator> Op = MueLu::Gallery::CreateCrsMatrix<SC, LO, GO, Map, CrsOperator>("Laplace1D", map, matrixParameters);

            // build nullspace
            RCP<MultiVector> nullSpace = MultiVectorFactory::Build(map,1);
            nullSpace->putScalar( (SC) 1.0);
            Teuchos::Array<ST::magnitudeType> norms(1);
            nullSpace->norm1(norms);
            if (comm->getRank() == 0)
              out << "||NS|| = " << norms[0] << std::endl;

            // fill hierarchy
            RCP<Hierarchy> H = rcp( new Hierarchy() );
            H->setDefaultVerbLevel(Teuchos::VERB_HIGH);

            RCP<Level> Finest = H->GetLevel();
            Finest->setDefaultVerbLevel(Teuchos::VERB_HIGH);
            Finest->Set("A",Op);                      // set fine level matrix
            Finest->Set("Nullspace",nullSpace);       // set null space information for finest level

            // define transfer operators
            RCP<UCAggregationFactory> UCAggFact = rcp(new UCAggregationFactory());
            UCAggFact->SetMinNodesPerAggregate(3);
            UCAggFact->SetMaxNeighAlreadySelected(0);
            UCAggFact->SetOrdering(MueLu::AggOptions::NATURAL);
            UCAggFact->SetPhase3AggCreation(0.5);

            RCP<TentativePFactory> Ptentfact = rcp(new TentativePFactory(UCAggFact));
            RCP<SaPFactory>         Pfact = rcp( new SaPFactory(Ptentfact));
            RCP<RFactory>           Rfact = rcp( new TransPFactory() );
            RCP<RAPFactory>        Acfact = rcp( new RAPFactory() );
            H->SetMaxCoarseSize(1);

            // setup smoothers
            Teuchos::ParameterList smootherParamList;
            smootherParamList.set("relaxation: type", "Symmetric Gauss-Seidel");
            smootherParamList.set("relaxation: sweeps", (LO) 1);
            smootherParamList.set("relaxation: damping factor", (SC) 1.0);
            RCP<SmootherPrototype> smooProto = rcp( new TrilinosSmoother(lib, "RELAXATION", smootherParamList) );
            RCP<SmootherFactory> SmooFact = rcp( new SmootherFactory(smooProto) );
            Acfact->setVerbLevel(Teuchos::VERB_HIGH);

            Teuchos::ParameterList status;
            status = H->FullPopulate(*Pfact,*Rfact, *Acfact,*SmooFact,0,maxLevels);

            SmootherFactory coarseSolveFact(smooProto);
            H->SetCoarsestSolver(coarseSolveFact,MueLu::PRE);

            // test some basic multigrid data
            RCP<Level> coarseLevel = H->GetLevel(1);
            TEST_EQUALITY(coarseLevel->IsRequested("A",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsRequested("P",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("PreSmoother",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("PostSmoother",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("R",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("A",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsAvailable("P",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsAvailable("PreSmoother",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsAvailable("PostSmoother",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsAvailable("R",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsKept("A",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsKept("P",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsKept("PreSmoother",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsKept("PostSmoother",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsKept("R",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel->IsRequested("P",Pfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("P",Ptentfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("PreSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("PostSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("R",Rfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsRequested("A",Acfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("P",Pfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("P",Ptentfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("PreSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("PostSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("R",Rfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsAvailable("A",Acfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsKept("P",Pfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsKept("P",Ptentfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsKept("PreSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel->IsKept("PostSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel->IsKept("R",Rfact.get()), false);
            TEST_EQUALITY(coarseLevel->IsKept("A",Acfact.get()), false);
            RCP<Operator> P1 = coarseLevel->Get< RCP<Operator> >("P");
            RCP<Operator> R1 = coarseLevel->Get< RCP<Operator> >("R");
            TEST_EQUALITY(P1->getGlobalNumRows(), 63);
            TEST_EQUALITY(P1->getGlobalNumCols(), 21);
            TEST_EQUALITY(R1->getGlobalNumRows(), 21);
            TEST_EQUALITY(R1->getGlobalNumCols(), 63);
            RCP<Level> coarseLevel2 = H->GetLevel(2);
            TEST_EQUALITY(coarseLevel2->IsRequested("A",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsRequested("P",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsRequested("R",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsRequested("PreSmoother",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsRequested("PostSmoother",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("A",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsAvailable("P",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsAvailable("PreSmoother",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsAvailable("PostSmoother",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("R",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsKept("A",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsKept("P",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsKept("PreSmoother",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsKept("PostSmoother",MueLu::NoFactory::get()), false);
            TEST_EQUALITY(coarseLevel2->IsKept("R",MueLu::NoFactory::get()), true);
            TEST_EQUALITY(coarseLevel2->IsRequested("P",Pfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsRequested("P",Ptentfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsRequested("R",Rfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("P",Pfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("P",Ptentfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("PreSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("PostSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsAvailable("R",Rfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsKept("P",Pfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsKept("P",Ptentfact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsKept("PreSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsKept("PostSmoother",SmooFact.get()), false);
            TEST_EQUALITY(coarseLevel2->IsKept("R",Rfact.get()), false);
            RCP<Operator> P2 = coarseLevel2->Get< RCP<Operator> >("P");
            RCP<Operator> R2 = coarseLevel2->Get< RCP<Operator> >("R");
            TEST_EQUALITY(P2->getGlobalNumRows(), 21);
            TEST_EQUALITY(P2->getGlobalNumCols(), 7);
            TEST_EQUALITY(R2->getGlobalNumRows(), 7);
            TEST_EQUALITY(R2->getGlobalNumCols(), 21);

            Teuchos::RCP<Xpetra::Operator<Scalar,LO,GO> > PtentTPtent = MueLu::Utils<Scalar,LO,GO>::TwoMatrixMultiply(P1,true,P1,false);
            TEST_EQUALITY(PtentTPtent->getGlobalMaxNumRowEntries()-3<1e-12, true);
            TEST_EQUALITY(P1->getGlobalMaxNumRowEntries()-2<1e-12, true);
            TEST_EQUALITY(P2->getGlobalMaxNumRowEntries()-2<1e-12, true);

            // Define RHS
            RCP<MultiVector> X = MultiVectorFactory::Build(map,1);
            RCP<MultiVector> RHS = MultiVectorFactory::Build(map,1);

            X->putScalar(1.0);
            X->norm2(norms);
            if (comm->getRank() == 0)
              out << "||X_true|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << norms[0] << std::endl;

            Op->apply(*X,*RHS,Teuchos::NO_TRANS,(SC)1.0,(SC)0.0);

            // Use AMG directly as an iterative method
            {
              X->putScalar( (SC) 0.0);

              H->Iterate(*RHS,its,*X);

              X->norm2(norms);
              if (comm->getRank() == 0)
                out << "||X_" << std::setprecision(2) << its << "|| = " << std::setiosflags(std::ios::fixed) << std::setprecision(10) << norms[0] << std::endl;
              results[run] = norms[0];
            }
        }

        TEST_EQUALITY(results[0] - results[1] < 1e-10, true); // check results of EPETRA vs TPETRA
    } // comm->getSize == 1

  } //SaPFactory_EpetraVsTpetra
#endif

}//namespace MueLuTests

