#ifndef TSFVECTORBASE_H
#define TSFVECTORBASE_H

#include "TSFConfig.h"
#include "TSFVectorSpace.h"

#if HAVE_RTOP
#include "RTOp.h"
#endif

namespace TSF
{
	class TSFVector;
	class TSFRandomNumberGenerator;

	/** \ingroup VectorSubtypes
	 * Base class for abstract vector implementations. 
	 *
	 */

	class TSFVectorBase
		{
		public: 
			/** \name Constructor and Destructors */
			//@{
			/** construct with a given space */
			TSFVectorBase(const TSFVectorSpace& space);

			/** the usual virtual dtor */
			virtual ~TSFVectorBase();
			//@}

			/** \name Vector space access */
			//@{
			/** Return the vector space in which this vector lives. */
			virtual const TSFVectorSpace& space() const {return space_;}
			//@}

			/** \name mathematical methods */
			//@{
			/** x = a*x + y */
			virtual void axpy(const TSFReal& a, const TSFVector& y) = 0 ;

			/** multiply by a scalar */
			virtual void scalarMult(const TSFReal& a) = 0 ;

			/** dot product */
			virtual TSFReal dot(const TSFVector& other) const = 0 ;

			/** infinity-norm */
			virtual TSFReal normInf() const = 0 ;

			/** one-norm */
			virtual TSFReal norm1() const = 0 ;

			/** sum all elements */
			virtual TSFReal sumElements() const = 0 ;

			/** set all elements to a scalar value */
			virtual void setScalar(const TSFReal& a) = 0 ;
			//@}

#if HAVE_RTOP
			/** \name General reduction and transformation operators */
			//@{
			/** apply a transformation operator */
			virtual void apply(RTOp op) = 0 ;
			//@}
#endif
					
			/** \name access to elements */
			//@{
			/** read-only access to a single element */
			virtual const TSFReal& getElement(int globalIndex) const = 0 ;

			/** read-write access to a single element */
			virtual TSFReal& setElement(int globalIndex) = 0 ;

			/** set a block of elements */
			virtual void setElements(int n, const int* globalIndices, 
															 const TSFReal* values) = 0 ;

			/** get a block of elements */
			virtual void getElements(int n, const int* globalIndices, 
															 TSFReal* values) const = 0 ;
			
			/** add to a block of elements */
			virtual void addToElements(int n, const int* globalIndices, 
																 const TSFReal* values) = 0 ;
			//@}


			/** \name Block access */
			//@{
			/** return the number of subvector blocks */
			virtual int numBlocks() const ;

			/** return the i-th subvector */
			virtual void getBlock(int i, const TSFVector& self, 
														TSFVector& sub) const ;

			/** set the i-th subvector */
			virtual void setBlock(int i, const TSFVector& sub);
			
			//@}

			/** \name generating random vectors */
			//@{
			/** Fill a vector with random elements */
			virtual void randomize(const TSFRandomNumberGenerator& r) = 0 ;
			//@}

			/** \name maintenance methods */
			//@{
			/** virtual copy ctor */
			virtual TSFVectorBase* deepCopy() const = 0 ;

			/** copy the data from another vector into this vector */
			virtual void acceptCopyOf(const TSFVector& x) = 0 ;

			/** write to stream */
			virtual ostream& print(ostream& os) const = 0 ;
			//@}

			/** \name Hooks for parallel support */
			//@{
			/** gather valid ghost values from other procs. Default is a no-op */
			virtual void synchronizeGhostValues() const {;}

			/** mark ghost values as invalid, meaning that they need to be
			 * synchronized. Default is a no-op.  */
			virtual void invalidateGhostValues() {;}
			//@}
			
		protected:
			TSFVectorSpace space_;
			static double dummyElement_;
		private:
		};
};

#endif
