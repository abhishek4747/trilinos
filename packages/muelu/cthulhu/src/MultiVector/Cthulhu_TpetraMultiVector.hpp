#ifndef CTHULHU_TPETRAMULTIVECTOR_HPP
#define CTHULHU_TPETRAMULTIVECTOR_HPP

/* this file is automatically generated - do not edit (see script/tpetra.py) */

#include "Cthulhu_TpetraConfigDefs.hpp"

#include "Cthulhu_MultiVector.hpp"

#include "Cthulhu_TpetraMap.hpp" //TMP
#include "Cthulhu_CombineMode.hpp"
#include "Cthulhu_TpetraImport.hpp"
#include "Cthulhu_TpetraExport.hpp"

#include "Tpetra_MultiVector.hpp"

namespace Cthulhu {

  // TODO: move that elsewhere
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  const Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node> & toTpetra(const MultiVector< Scalar,LocalOrdinal, GlobalOrdinal, Node> &map);

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node> & toTpetra(MultiVector< Scalar,LocalOrdinal, GlobalOrdinal, Node> &map);
  //

#ifndef DOXYGEN_SHOULD_SKIP_THIS
  // forward declaration of TpetraVector, needed to prevent circular inclusions
  template<class S, class LO, class GO, class N> class TpetraVector;
#endif

  template <class Scalar, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType>
  class TpetraMultiVector
    : public virtual MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >
  {

    // The following typedef are used by the CTHULHU_DYNAMIC_CAST() macro.
    typedef TpetraMultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Node> TpetraMultiVectorClass;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Basic MultiVector constuctor.
    TpetraMultiVector(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map, size_t NumVectors, bool zeroOut=true)
      : vec_(Teuchos::rcp(new Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(toTpetra(map), NumVectors, zeroOut))) { }

    //! MultiVector copy constructor.
    TpetraMultiVector(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &source)
      : vec_(Teuchos::rcp(new Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(toTpetra(source)))) { }

    //! Set multi-vector values from two-dimensional array using Teuchos memory management classes. (copy).
    TpetraMultiVector(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map, const Teuchos::ArrayView< const Scalar > &A, size_t LDA, size_t NumVectors)
      : vec_(Teuchos::rcp(new Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(toTpetra(map), A, LDA, NumVectors))) { }

    //! Set multi-vector values from array of pointers using Teuchos memory management classes. (copy).
    TpetraMultiVector(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map, const Teuchos::ArrayView< const Teuchos::ArrayView< const Scalar > > &ArrayOfPtrs, size_t NumVectors)
      : vec_(Teuchos::rcp(new Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >(toTpetra(map), ArrayOfPtrs, NumVectors))) { }

    //! MultiVector destructor.
    virtual ~TpetraMultiVector() { }

    //@}

    //! @name Post-construction modification routines
    //@{

    //! Replace current value at the specified (globalRow, vectorIndex) location with specified value.
    void replaceGlobalValue(GlobalOrdinal globalRow, size_t vectorIndex, const Scalar &value){ vec_->replaceGlobalValue(globalRow, vectorIndex, value); }

    //! Adds specified value to existing value at the specified (globalRow, vectorIndex) location.
    void sumIntoGlobalValue(GlobalOrdinal globalRow, size_t vectorIndex, const Scalar &value){ vec_->sumIntoGlobalValue(globalRow, vectorIndex, value); }

    //! Replace current value at the specified (myRow, vectorIndex) location with specified value.
    void replaceLocalValue(LocalOrdinal myRow, size_t vectorIndex, const Scalar &value){ vec_->replaceLocalValue(myRow, vectorIndex, value); }

    //! Adds specified value to existing value at the specified (myRow, vectorIndex) location.
    void sumIntoLocalValue(LocalOrdinal myRow, size_t vectorIndex, const Scalar &value){ vec_->sumIntoLocalValue(myRow, vectorIndex, value); }

    //! Initialize all values in a multi-vector with specified value.
    void putScalar(const Scalar &value){ vec_->putScalar(value); }

    //! Set multi-vector values to random numbers.
    void randomize(){ vec_->randomize(); }

    //! Instruct a local (non-distributed) MultiVector to sum values across all nodes.
    void reduce(){ vec_->reduce(); }

    //@}

    //! @name Data Copy and View get methods
    //@{

    //! 
    Teuchos::ArrayRCP< const Scalar > getData(size_t j) const { return vec_->getData(j); }

    //! 
    Teuchos::ArrayRCP< Scalar > getDataNonConst(size_t j){ return vec_->getDataNonConst(j); }

    //! Return multi-vector values in user-provided two-dimensional array (using Teuchos memory management classes).
    void get1dCopy(Teuchos::ArrayView< Scalar > A, size_t LDA) const { vec_->get1dCopy(A, LDA); }

    //! Return multi-vector values in user-provided array of pointers (using Teuchos memory management classes).
    void get2dCopy(Teuchos::ArrayView< const Teuchos::ArrayView< Scalar > > ArrayOfPtrs) const { vec_->get2dCopy(ArrayOfPtrs); }

    //! Return const persisting view of values in a one-dimensional array. Throws std::runtime_error if the underlying data is non-contiguous.
    Teuchos::ArrayRCP< const Scalar > get1dView() const { return vec_->get1dView(); }

    //! Return const persisting pointers to values.
    Teuchos::ArrayRCP< Teuchos::ArrayRCP< const Scalar > > get2dView() const { return vec_->get2dView(); }

    //! Return non-const persisting view of values in a one-dimensional array. Throws std::runtime_error if the underlying data is non-contiguous. Teuchos::ArrayRCP<Scalar> get1dViewNonConst();.
    Teuchos::ArrayRCP< Scalar > get1dViewNonConst(){ return vec_->get1dViewNonConst(); }

    //! Return non-const persisting pointers to values.
    Teuchos::ArrayRCP< Teuchos::ArrayRCP< Scalar > > get2dViewNonConst(){ return vec_->get2dViewNonConst(); }

    //@}

    //! @name Mathematical methods
    //@{

    //! Computes dot product of each corresponding pair of vectors, dots[i] = this[i].dot(A[i]).
    void dot(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const Teuchos::ArrayView< Scalar > &dots) const { vec_->dot(toTpetra(A), dots); }

    //! Puts element-wise absolute values of input Multi-vector in target: A = abs(this).
    void abs(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A){ vec_->abs(toTpetra(A)); }

    //! Puts element-wise reciprocal values of input Multi-vector in target, this(i,j) = 1/A(i,j).
    void reciprocal(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A){ vec_->reciprocal(toTpetra(A)); }

    //! Scale the current values of a multi-vector, this = alpha*this.
    void scale(const Scalar &alpha){ vec_->scale(alpha); }

    //! Scale the current values of a multi-vector, this[j] = alpha[j]*this[j].
    void scale(Teuchos::ArrayView< const Scalar > alpha){ vec_->scale(alpha); }

    //! Replace multi-vector values with scaled values of A, this = alpha*A.
    void scale(const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A){ vec_->scale(alpha, toTpetra(A)); }

    //! Update multi-vector values with scaled values of A, this = beta*this + alpha*A.
    void update(const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const Scalar &beta){ vec_->update(alpha, toTpetra(A), beta); }

    //! Update multi-vector with scaled values of A and B, this = gamma*this + alpha*A + beta*B.
    void update(const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const Scalar &beta, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &B, const Scalar &gamma){ vec_->update(alpha, toTpetra(A), beta, toTpetra(B), gamma); }

    //! Compute 1-norm of each vector in multi-vector.
    void norm1(const Teuchos::ArrayView< typename Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const { vec_->norm1(norms); }

    //! Compute 2-norm of each vector in multi-vector.
    void norm2(const Teuchos::ArrayView< typename Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const { vec_->norm2(norms); }

    //! Compute Inf-norm of each vector in multi-vector.
    void normInf(const Teuchos::ArrayView< typename Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const { vec_->normInf(norms); }

    //! Compute Weighted 2-norm (RMS Norm) of each vector in multi-vector.
    void normWeighted(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &weights, const Teuchos::ArrayView< typename Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const { vec_->normWeighted(toTpetra(weights), norms); }

    //! Compute mean (average) value of each vector in multi-vector.
    void meanValue(const Teuchos::ArrayView< Scalar > &means) const { vec_->meanValue(means); }

    //! Matrix-Matrix multiplication, this = beta*this + alpha*op(A)*op(B).
    void multiply(Teuchos::ETransp transA, Teuchos::ETransp transB, const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &B, const Scalar &beta){ vec_->multiply(transA, transB, alpha, toTpetra(A), toTpetra(B), beta); }

    //! Element-wise multiply of a Vector A with a MultiVector B.
    void elementWiseMultiply(Scalar scalarAB, const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &B, Scalar scalarThis){ vec_->elementWiseMultiply(scalarAB, toTpetra(A), toTpetra(B), scalarThis); }

    //@}

    //! @name Attribute access functions
    //@{

    //! Returns the number of vectors in the multi-vector.
    size_t getNumVectors() const { return vec_->getNumVectors(); }

    //! Returns the local vector length on the calling processor of vectors in the multi-vector.
    size_t getLocalLength() const { return vec_->getLocalLength(); }

    //! Returns the global vector length of vectors in the multi-vector.
    global_size_t getGlobalLength() const { return vec_->getGlobalLength(); }

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const { return vec_->description(); }

    //! Print the object with some verbosity level to an FancyOStream object.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const { vec_->describe(out, verbLevel); }

    //@}

    //{@
    // Implements DistObject interface
    
    const Teuchos::RCP< const Map<LocalOrdinal,GlobalOrdinal,Node> > getMap() const { return toCthulhu(vec_->getMap()); }
    
    void doImport(const DistObject< Scalar, LocalOrdinal,GlobalOrdinal,Node> &source, const Import<LocalOrdinal,GlobalOrdinal,Node> &importer, CombineMode CM) { 
      
      CTHULHU_DYNAMIC_CAST(const TpetraMultiVectorClass, source, tSource, "Cthulhu::TpetraMultiVector::doImport only accept Cthulhu::TpetraMultiVector as input arguments."); //TODO: remove and use toTpetra()
      RCP< const Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal,Node> > v = tSource.getTpetra_MultiVector();
      this->getTpetra_MultiVector()->doImport(*v, toTpetra(importer), toTpetra(CM));
    }

    void doExport(const DistObject< Scalar, LocalOrdinal, GlobalOrdinal, Node > &dest, const Import<LocalOrdinal,GlobalOrdinal,Node>& importer, CombineMode CM) {
            
      CTHULHU_DYNAMIC_CAST(const TpetraMultiVectorClass, dest, tDest, "Cthulhu::TpetraMultiVector::doImport only accept Cthulhu::TpetraMultiVector as input arguments."); //TODO: remove and use toTpetra()
      RCP< const Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal,Node> > v = tDest.getTpetra_MultiVector();
      this->getTpetra_MultiVector()->doExport(*v, toTpetra(importer), toTpetra(CM)); 

    }

    void doImport(const DistObject< Scalar, LocalOrdinal, GlobalOrdinal, Node > &source, const Export<LocalOrdinal,GlobalOrdinal,Node>& exporter, CombineMode CM) {

      CTHULHU_DYNAMIC_CAST(const TpetraMultiVectorClass, source, tSource, "Cthulhu::TpetraMultiVector::doImport only accept Cthulhu::TpetraMultiVector as input arguments."); //TODO: remove and use toTpetra()
      RCP< const Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal,Node> > v = tSource.getTpetra_MultiVector();
      this->getTpetra_MultiVector()->doImport(*v, toTpetra(exporter), toTpetra(CM));

    }

    void doExport(const DistObject< Scalar, LocalOrdinal, GlobalOrdinal, Node > &dest, const Export<LocalOrdinal,GlobalOrdinal,Node>& exporter, CombineMode CM) {
      
      CTHULHU_DYNAMIC_CAST(const TpetraMultiVectorClass, dest, tDest, "Cthulhu::TpetraMultiVector::doImport only accept Cthulhu::TpetraMultiVector as input arguments."); //TODO: remove and use toTpetra()
      RCP< const Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal,Node> > v = tDest.getTpetra_MultiVector();
      this->getTpetra_MultiVector()->doExport(*v, toTpetra(exporter), toTpetra(CM)); 

    }

    //@}

    //! @name Cthulhu specific
    //@{

    //! TpetraMultiVector constructor to wrap a Tpetra::MultiVector object
    TpetraMultiVector(const Teuchos::RCP<Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node> > &vec) : vec_(vec) { } //TODO removed const

    //! Get the underlying Tpetra multivector
    RCP< Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node> > getTpetra_MultiVector() const { return vec_; }

    //! Set seed for Random function.
    void setSeed(unsigned int seed) { Teuchos::ScalarTraits< Scalar >::seedrandom(seed); }
 
    //@}
    
  private:

    RCP< Tpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node> > vec_;
    
  }; // TpetraMultiVector class

  // TODO: move that elsewhere
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  const Tpetra::MultiVector< Scalar,LocalOrdinal, GlobalOrdinal, Node> & toTpetra(const MultiVector< Scalar,LocalOrdinal, GlobalOrdinal, Node> &x) {
    typedef TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > TpetraMultiVectorClass;
      CTHULHU_DYNAMIC_CAST(const TpetraMultiVectorClass, x, tX, "toTpetra");
      return *tX.getTpetra_MultiVector();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node>
  Tpetra::MultiVector< Scalar,LocalOrdinal, GlobalOrdinal, Node> & toTpetra(MultiVector< Scalar,LocalOrdinal, GlobalOrdinal, Node> &x) {
    typedef TpetraMultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > TpetraMultiVectorClass;
      CTHULHU_DYNAMIC_CAST(      TpetraMultiVectorClass, x, tX, "toTpetra");
      return *tX.getTpetra_MultiVector();
  }
  //

} // Cthulhu namespace

#define CTHULHU_TPETRAMULTIVECTOR_SHORT
#endif // CTHULHU_TPETRAMULTIVECTOR_HPP
