#include <Teuchos_TabularOutputter.hpp>

#include "MueLu_Level.hpp"

#include "MueLu_FactoryManagerBase.hpp"

namespace MueLu {

  Level::Level() : levelID_(-1) { }

  Level::Level(RCP<FactoryManagerBase> & factoryManager) : levelID_(-1), factoryManager_(factoryManager) { }

  RCP<Level> Level::Build() {
    RCP<Level> newLevel = rcp( new Level() );

    // Copy 'keep' status of variables
    // TODO: this only concerns needs_. so a function in Needs class should be provided to do that!
    // TODO: how can i move this to Needs? maybe we need a new constructor for Level which gets a
    // Needs object...
    typedef std::vector<std::string> KeyList;

    std::vector<const MueLu::FactoryBase*> ehandles = needs_.RequestedFactories();
    for (std::vector<const MueLu::FactoryBase*>::iterator kt = ehandles.begin(); kt != ehandles.end(); kt++) {
      std::vector<std::string> enames = needs_.RequestedKeys(*kt);
      for (std::vector<std::string>::iterator it = enames.begin(); it != enames.end(); it++) {
        const std::string & ename = *it;
        const MueLu::FactoryBase* fac = *kt;
        if (IsKept(ename, fac, MueLu::Keep)) { // MueLu::Keep is the only flag propagated
          if (fac == NULL) // TODO: Is this possible?? Throw exception. Not supposed to use the FactoryManager here.
            newLevel->Keep(ename);
          else
            newLevel->Keep(ename, fac);
        }
      }
    }
      
    return newLevel;
  }

  Level::~Level() {}

  int Level::GetLevelID() const { return levelID_; }

  void Level::SetLevelID(int levelID) {
    if (levelID_ != -1 && levelID_ != levelID)
      GetOStream(Warnings1, 0) << "Warning: Level::SetLevelID(): Changing an already defined LevelID (previousID=" << levelID_ << ", newID=" << levelID << ")" << std::endl;

    levelID_ = levelID;
  }

  RCP<Level> & Level::GetPreviousLevel() { return previousLevel_; }

  void Level::SetPreviousLevel(const RCP<Level> & previousLevel) {
    if (previousLevel_ != Teuchos::null && previousLevel_ != previousLevel)
      GetOStream(Warnings1, 0) << "Warning: Level::SetPreviousLevel(): PreviousLevel was already defined" << std::endl;
    
    previousLevel_ = previousLevel;
  }
 
  void Level::SetFactoryManager(const RCP<const FactoryManagerBase> & factoryManager) {
    factoryManager_ = factoryManager;
  }

  void Level::AddKeepFlag(const std::string& ename, const FactoryBase* factory, KeepType keepType) {
    needs_.AddKeepFlag(ename, GetFactory(ename, factory), keepType);
  }

  void Level::RemoveKeepFlag(const std::string& ename, const FactoryBase* factory, KeepType keepType) {
    needs_.RemoveKeepFlag(ename, GetFactory(ename, factory), keepType);
  }

  KeepType Level::GetKeepFlag(const std::string& ename, const FactoryBase* factory) const {
    return needs_.GetKeepFlag(ename, GetFactory(ename, factory));
  }
  
  void Level::Request(const FactoryBase& factory) {
    RequestMode prev = requestMode_;
    requestMode_ = REQUEST;
    factory.CallDeclareInput(*this);
    requestMode_ = prev;
  }

  void Level::Release(const FactoryBase& factory) {
    RequestMode prev = requestMode_;
    requestMode_ = RELEASE;
    factory.CallDeclareInput(*this);
    requestMode_ = prev;
  }

  void Level::DeclareInput(const std::string& ename, const FactoryBase* factory, const FactoryBase* requestedBy) {
    if (requestMode_ == REQUEST) {
      Request(ename, factory, requestedBy);
    }
    else if (requestMode_ == RELEASE) {
      Release(ename, factory, requestedBy);
    }
    else
      TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "MueLu::Level::DeclareInput(): requestMode_ undefined.");
  }

  void Level::DeclareDependencies(const FactoryBase* factory, bool bRequestOnly, bool bReleaseOnly) { //TODO: replace bReleaseOnly, bReleaseOnly by one RequestMode enum
    if (bRequestOnly && bReleaseOnly)
      TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "MueLu::Level::DeclareDependencies(): Both bRequestOnly and bReleaseOnly set to true makes no sense.");

    if (requestMode_ == REQUEST) {

      if (bReleaseOnly == false) Request(*factory);

    } else if (requestMode_ == RELEASE) {

      if (bRequestOnly == false) Release(*factory);

    } else TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "MueLu::Level::DeclareDependencies(): requestMode_ undefined.");
  }

  void Level::Request(const std::string& ename, const FactoryBase* factory, const FactoryBase* requestedBy) {
    const FactoryBase* fac = GetFactory(ename, factory);

    // check if generating factory is not the requesting factory and avoid self-recursive calls of Request
    // Even though it's very special, a factory can generate data, that it requests itself.
    if(fac != requestedBy) {
      // call Request for factory fac only if
      // the factory has not been requested before and no data has
      // been generated by fact (independent of ename)
      if ((needs_.IsAvailableFactory(fac) == false && needs_.IsRequestedFactory(fac) == false)) {
        Request(*fac);
      }
    }

    needs_.Request(ename, fac, requestedBy);
  }

  //TODO: finish this
// #define MUELU_LEVEL_ERROR_MESSAGE(function, message)    
//   "MueLu::Level[" << levelID_ << "]::" << function << "(" << ename << ", " << factory << " << ): " << message << std::endl 
//                                                                                                                  << ((factory == Teuchos::null && factoryManager_ != Teuchos::null) ? (*GetFactory(ename, factory)) : *factory)  << "Generating factory:" << *fac << " NoFactory=" << NoFactory::get()
//   //

  void Level::Release(const std::string& ename, const FactoryBase* factory, const FactoryBase* requestedBy) {
    const FactoryBase* fac = GetFactory(ename, factory);

    //FIXME FIXME FIXME FIXME
    //TMP    TEUCHOS_TEST_FOR_EXCEPTION(IsRequested(ename,fac) == false, Exceptions::RuntimeError, "MueLu::Level[" << levelID_ << "]::Release(): This method cannot be called on non requested data. Called on " << ename << ", " << fac << std::endl << "Generating factory:" << *fac << " NoFactory="<<NoFactory::get() ); //TODO: add print() of variable info to complete the error msg

    if(needs_.IsRequestedBy(fac, ename, requestedBy)) {
      
      // data has been requested but never built
      // can we release the dependencies of fac safely?
      int cnt = needs_.CountRequestedFactory(fac);
      if(/*fac != requestedBy &&*/ cnt == 1) {
        // factory is only generating factory of current variable (ename,factory)
        // Release(fac) can be called safely
        Release(*fac);
      }
      needs_.Release(ename,fac,requestedBy);
    }
  }

  bool Level::IsKey(const std::string & ename, const FactoryBase* factory) const {
    return needs_.IsKey(ename, GetFactory(ename, factory));
  }

  bool Level::IsAvailable(const std::string & ename, const FactoryBase* factory) {
    return needs_.IsAvailable(ename, GetFactory(ename, factory));
  }

  bool Level::IsRequested(const std::string & ename, const FactoryBase* factory) {
    return needs_.IsRequested(ename, GetFactory(ename, factory));
  }

  std::string Level::description() const {
    std::ostringstream out;
    out << BaseClass::description();
    out << "{ levelID = " << levelID_ << "}";
    return out.str();
  }

  void Level::print(Teuchos::FancyOStream &out, const VerbLevel verbLevel) const {
    MUELU_DESCRIBE; 
    out0 << ""; // remove warning

    Teuchos::TabularOutputter outputter(out);
    outputter.pushFieldSpec("name",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 32);
    outputter.pushFieldSpec("gen. factory addr.", Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 18);
    outputter.pushFieldSpec("req",                Teuchos::TabularOutputter::INT,    Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 3);
    outputter.pushFieldSpec("keep",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 5);
    outputter.pushFieldSpec("type",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 10);
    outputter.pushFieldSpec("data",               Teuchos::TabularOutputter::STRING, Teuchos::TabularOutputter::LEFT, Teuchos::TabularOutputter::GENERAL, 20);
    outputter.outputHeader();

    std::vector<const MueLu::FactoryBase*> ehandles = needs_.RequestedFactories();
    for (std::vector<const MueLu::FactoryBase*>::iterator kt = ehandles.begin(); kt != ehandles.end(); kt++) {
      std::vector<std::string> enames = needs_.RequestedKeys(*kt);
      for (std::vector<std::string>::iterator it = enames.begin(); it != enames.end(); it++) {
        outputter.outputField(*it);   // variable name

        // factory ptr
        if (*kt == NoFactory::get())
          outputter.outputField("NoFactory");
        else
          outputter.outputField(*kt);
        
        int reqcount = needs_.NumRequests(*it, *kt); // request counter
        outputter.outputField(reqcount);

        KeepType keepType = needs_.GetKeepFlag(*it, *kt);
        if (keepType != 0) { 
          std::stringstream ss;
          if (keepType & MueLu::UserData) { ss << "User";  }
          if (keepType & MueLu::Keep)     { ss << "Keep";  }
          if (keepType & MueLu::Final)    { ss << "Final"; }
          outputter.outputField(ss.str()); 
        } else { 
          outputter.outputField("No"); 
        }

        if (needs_.IsAvailable(*it, *kt)) {
          std::string strType = needs_.GetType(*it, *kt); // Variable type
          if (strType.find("Xpetra::Operator") != std::string::npos) {
            outputter.outputField("Operator" );
            outputter.outputField("available");
          } else if (strType.find("Xpetra::MultiVector") != std::string::npos) {
            outputter.outputField("Vector");
            outputter.outputField("available");
          } else if (strType.find("MueLu::SmootherBase") != std::string::npos) {
            outputter.outputField("SmootherBase");
            outputter.outputField("available");
          } else if (strType == "int") {
            outputter.outputField(strType);
            int data = needs_.Get<int>(*it, *kt);
            outputter.outputField(data);
          } else if (strType == "double") {
            outputter.outputField(strType);
            double data = needs_.Get<double>(*it, *kt);
            outputter.outputField(data);
          } else if (strType == "string") {
            outputter.outputField(strType);
            std::string data = needs_.Get<std::string>(*it, *kt);
            outputter.outputField(data);
          } else {
            outputter.outputField(strType);
            outputter.outputField("available");
          }
        } else {
          outputter.outputField("unknown");
          outputter.outputField("not available");
        }

        outputter.nextRow();
      }
    }

  }

  Level::Level(const Level& source) { }

  const FactoryBase* Level::GetFactory(const std::string& varname, const FactoryBase* factory) const {
    if (factory == NULL) {
      TEUCHOS_TEST_FOR_EXCEPTION(factoryManager_ == null, Exceptions::RuntimeError, "MueLu::Level::GetFactory(): no FactoryManager");
      const FactoryBase* fac = factoryManager_->GetFactory(varname).get();
      TEUCHOS_TEST_FOR_EXCEPTION(fac == NULL, Exceptions::RuntimeError, "MueLu::Level::GetFactory(): Default factory cannot be NULL");
      return fac;
    } else {
      return factory;
    }
  }

  Level::RequestMode Level::requestMode_ = UNDEF;

} //namespace MueLu

//TODO: Caps should not matter
