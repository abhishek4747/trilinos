/*
// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER
*/

#ifndef TPETRA_DETAILS_FIXEDHASHTABLE_DEF_HPP
#define TPETRA_DETAILS_FIXEDHASHTABLE_DEF_HPP

#include "Tpetra_Details_FixedHashTable_decl.hpp"
#ifdef TPETRA_USE_MURMUR_HASH
#  include <Kokkos_Functional.hpp> // hash function used by Kokkos::UnorderedMap
#endif // TPETRA_USE_MURMUR_HASH
#include <limits> // std::numeric_limits
#include <type_traits>

namespace { // (anonymous)

template<class KeyType,
         class ArrayLayout,
         class InputExecSpace,
         class OutputExecSpace,
         const bool mustDeepCopy =
           ! std::is_same<typename InputExecSpace::memory_space,
                          typename OutputExecSpace::memory_space>::value>
struct DeepCopyIfNeeded {};

template<class KeyType,
         class ArrayLayout,
         class InputExecSpace,
         class OutputExecSpace>
struct DeepCopyIfNeeded<KeyType, ArrayLayout, InputExecSpace, OutputExecSpace, true>
{
  typedef Kokkos::View<const KeyType*, ArrayLayout,
                       InputExecSpace, Kokkos::MemoryUnmanaged> input_view_type;
  // In this case, a deep copy IS needed.
  typedef Kokkos::View<const KeyType*, ArrayLayout, OutputExecSpace> output_view_type;

  static output_view_type copy (const input_view_type& src) {
    typedef typename output_view_type::non_const_type NC;

    NC dst (Kokkos::ViewAllocateWithoutInitializing (src.tracker ().label ()),
            src.dimension_0 ());
    Kokkos::deep_copy (dst, src);
    return output_view_type (dst);
  }
};

// Specialization if no need to deep-copy.
template<class KeyType,
         class ArrayLayout,
         class InputExecSpace,
         class OutputExecSpace>
struct DeepCopyIfNeeded<KeyType, ArrayLayout, InputExecSpace, OutputExecSpace, false> {
  typedef Kokkos::View<const KeyType*, ArrayLayout,
                       InputExecSpace, Kokkos::MemoryUnmanaged> input_view_type;
  typedef Kokkos::View<const KeyType*, ArrayLayout, OutputExecSpace,
                       Kokkos::MemoryUnmanaged> output_view_type;

  static output_view_type copy (const input_view_type& src) {
    return output_view_type (src);
  }
};

/// \brief Reduction result for CountBuckets functor below.
///
/// The reduction result finds the min and max keys.  The default
/// values (\c minKey_ set to max \c KeyType value, and \c maxKey_ set
/// to min \c KeyType value) ensure correct behavior even if there is
/// only one key.
///
/// \tparam KeyType Type of each input of the hash function.
///   It must be an integer type.
template<class KeyType>
struct CountBucketsValue {
  CountBucketsValue () :
    minKey_ (std::numeric_limits<KeyType>::max ()),
    // min() for a floating-point type returns the minimum _positive_
    // normalized value.  This is different than for integer types.
    // lowest() is new in C++11 and returns the least value, always
    // negative for signed finite types.
    maxKey_ (std::numeric_limits<KeyType>::lowest ())
  {
    static_assert (std::is_arithmetic<KeyType>::value, "CountBucketsValue: "
                   "KeyType must be some kind of number type.");
  }

  KeyType minKey_; //!< The current minimum key.
  KeyType maxKey_; //!< The current maximum key.
};

template<class OffsetsViewType,
         class KeysViewType,
         class ExecutionSpaceType = typename OffsetsViewType::execution_space,
         class MemorySpaceType = typename OffsetsViewType::memory_space,
         class SizeType = typename KeysViewType::size_type>
class CountBuckets {
public:
  typedef OffsetsViewType offsets_view_type;
  typedef KeysViewType keys_view_type;
  typedef typename OffsetsViewType::execution_space execution_space;
  typedef typename OffsetsViewType::memory_space memory_space;
  typedef typename KeysViewType::size_type size_type;

  typedef typename keys_view_type::non_const_value_type key_type;
  typedef CountBucketsValue<key_type> value_type;
  // mfh 21 May 2015: Having a device_type typedef in the functor
  // along with an execution_space typedef causes compilation issues.
  typedef Tpetra::Details::Hash<key_type, Kokkos::Device<execution_space, memory_space> > hash_type;

  CountBuckets (const offsets_view_type& ptr,
                const keys_view_type& keys,
                const size_type size) :
    ptr_ (ptr),
    keys_ (keys),
    size_ (size),
    initMinKey_ (std::numeric_limits<key_type>::max ()),
    initMaxKey_ (std::numeric_limits<key_type>::lowest ())
  {}

  CountBuckets (const offsets_view_type& ptr,
                const keys_view_type& keys,
                const size_type size,
                const key_type initMinKey,
                const key_type initMaxKey) :
    ptr_ (ptr),
    keys_ (keys),
    size_ (size),
    initMinKey_ (initMinKey),
    initMaxKey_ (initMaxKey)
  {}

  //! Set the initial value of the reduction result.
  KOKKOS_INLINE_FUNCTION void init (value_type& dst) const
  {
    dst.minKey_ = initMinKey_;
    dst.maxKey_ = initMaxKey_;
  }

  /// \brief Combine two intermediate reduction results.
  ///
  /// This sets both the min and max GID, if necessary.
  KOKKOS_INLINE_FUNCTION void
  join (volatile value_type& dst,
        const volatile value_type& src) const
  {
    if (src.maxKey_ > dst.maxKey_) {
      dst.maxKey_ = src.maxKey_;
    }
    if (src.minKey_ < dst.minKey_) {
      dst.minKey_ = src.minKey_;
    }
  }

  /// \brief Do this for every entry of \c keys_.
  ///
  /// Count the number of keys in \c keys_ that hash to the same
  /// value.  Update the min and max key seen thus far in entries.
  ///
  /// (We can't count duplicate keys yet, because we haven't stored
  /// keys in the hash table yet.)
  KOKKOS_INLINE_FUNCTION void
  operator () (const size_type& i, value_type& dst) const
  {
    typedef typename hash_type::result_type hash_value_type;

    const key_type key = keys_[i];
    const hash_value_type hashVal = hash_type::hashFunc (key, size_);
    // Shift over one, so that counts[j] = ptr[j+1].  See below.
    Kokkos::atomic_fetch_add (&ptr_[hashVal+1], 1);

    if (key > dst.maxKey_) {
      dst.maxKey_ = key;
    }
    if (key < dst.minKey_) {
      dst.minKey_ = key;
    }
  }

private:
  //! Offsets in the FixedHashTable to construct (output argument).
  offsets_view_type ptr_;
  //! Keys for the FixedHashTable to construct (input argument).
  keys_view_type keys_;
  //! Number of buckets plus 1 (or 0, if no buckets).
  size_type size_;
  //! Initial minimum key.
  key_type initMinKey_;
  //! Initial maximum key.
  key_type initMaxKey_;
};

// Is it worth actually using CountBuckets instead of just counting in
// a sequential loop?
//
// The CountBuckets kernel uses atomic update instructions to count
// the number of "buckets" per offsets array (ptr) entry.  Atomic
// updates incur overhead, even in the sequential case.  The Kokkos
// kernel is still correct in that case, but I would rather not incur
// overhead then.  It might make sense to set the minimum number of
// threads for invoking the parallel kernel to something greater than
// 1, but we would need experiments to find out.
template<class ExecSpace>
struct WorthCountingBucketsInParallel {
  typedef typename ExecSpace::execution_space execution_space;

  static bool isWorth () {
    // NOTE: Kokkos::Cuda does NOT have this method.  That's why we
    // need the partial specialization below.
    return execution_space::max_hardware_threads () > 1;
  }
};

#ifdef KOKKOS_HAVE_CUDA
template<>
struct WorthCountingBucketsInParallel<Kokkos::Cuda> {
  // There could be more complicated expressions for whether this is
  // actually worthwhile, but for now I'll just say that with Cuda, we
  // will ALWAYS count buckets in parallel (that is, run a Kokkos
  // parallel kernel).
  static bool isWorth () {
    return true;
  }
};
#endif // KOKKOS_HAVE_CUDA

template<class ExecSpace>
bool worthCountingBucketsInParallel () {
  return WorthCountingBucketsInParallel<ExecSpace>::isWorth ();
}

} // namespace (anonymous)



namespace Tpetra {
namespace Details {

template<class KeyType, class ValueType, class DeviceType>
void
FixedHashTable<KeyType, ValueType, DeviceType>::
check () const
{
  const char prefix[] = "Tpetra::Details::FixedHashTable: ";
  const char suffix[] = "  Please report this bug to the Tpetra developers.";

#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  TEUCHOS_TEST_FOR_EXCEPTION
    (ptr_.ptr_on_device () != rawPtr_, std::logic_error,
     prefix << "ptr_.ptr_on_device () != rawPtr_." << suffix);
  TEUCHOS_TEST_FOR_EXCEPTION
    (val_.ptr_on_device () != rawVal_, std::logic_error,
     prefix << "val_.ptr_on_device () != rawVal_." << suffix);
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)

  TEUCHOS_TEST_FOR_EXCEPTION
    (invalidValue_ != Teuchos::OrdinalTraits<ValueType>::invalid (),
     std::logic_error, prefix << "invalidValue_ == " << invalidValue_
     << " != Teuchos::OrdinalTraits<ValueType>::invalid() == "
     << Teuchos::OrdinalTraits<ValueType>::invalid () << "." << suffix);
}

template<class KeyType, class ValueType, class DeviceType>
FixedHashTable<KeyType, ValueType, DeviceType>::
FixedHashTable () :
#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  rawPtr_ (NULL),
  rawVal_ (NULL),
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  invalidValue_ (Teuchos::OrdinalTraits<ValueType>::invalid ()),
  hasDuplicateKeys_ (false) // trivially true
{
#ifdef HAVE_TPETRA_DEBUG
  check ();
#endif // HAVE_TPETRA_DEBUG
}

template<class KeyType, class ValueType, class DeviceType>
FixedHashTable<KeyType, ValueType, DeviceType>::
FixedHashTable (const Teuchos::ArrayView<const KeyType>& keys) :
#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  rawPtr_ (NULL),
  rawVal_ (NULL),
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  invalidValue_ (Teuchos::OrdinalTraits<ValueType>::invalid ()),
  hasDuplicateKeys_ (false) // to revise in init()
{
  // mfh 01 May 2015: I don't trust that
  // Teuchos::ArrayView::getRawPtr() returns NULL when the size is 0,
  // so I ensure this manually.
  const ValueType startingValue = static_cast<ValueType> (0);
  host_input_keys_type keys_k (keys.size () == 0 ? NULL : keys.getRawPtr (),
                               keys.size ());
  init (keys_k, startingValue);

#ifdef HAVE_TPETRA_DEBUG
  check ();
#endif // HAVE_TPETRA_DEBUG
}

template<class KeyType, class ValueType, class DeviceType>
FixedHashTable<KeyType, ValueType, DeviceType>::
FixedHashTable (const Teuchos::ArrayView<const KeyType>& keys,
                const ValueType startingValue) :
#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  rawPtr_ (NULL),
  rawVal_ (NULL),
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  invalidValue_ (Teuchos::OrdinalTraits<ValueType>::invalid ()),
  hasDuplicateKeys_ (false) // to revise in init()
{
  // mfh 01 May 2015: I don't trust that
  // Teuchos::ArrayView::getRawPtr() returns NULL when the size is 0,
  // so I ensure this manually.
  host_input_keys_type keys_k (keys.size () == 0 ? NULL : keys.getRawPtr (),
                               keys.size ());
  init (keys_k, startingValue);

#ifdef HAVE_TPETRA_DEBUG
  check ();
#endif // HAVE_TPETRA_DEBUG
}

template<class KeyType, class ValueType, class DeviceType>
FixedHashTable<KeyType, ValueType, DeviceType>::
FixedHashTable (const Teuchos::ArrayView<const KeyType>& keys,
                const Teuchos::ArrayView<const ValueType>& vals) :
#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  rawPtr_ (NULL),
  rawVal_ (NULL),
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  invalidValue_ (Teuchos::OrdinalTraits<ValueType>::invalid ()),
  hasDuplicateKeys_ (false) // to revise in init()
{
  // mfh 01 May 2015: I don't trust that
  // Teuchos::ArrayView::getRawPtr() returns NULL when the size is 0,
  // so I ensure this manually.
  host_input_keys_type keys_k (keys.size () == 0 ? NULL : keys.getRawPtr (),
                               keys.size ());
  host_input_vals_type vals_k (vals.size () == 0 ? NULL : vals.getRawPtr (),
                               vals.size ());
  init (keys_k, vals_k);

#ifdef HAVE_TPETRA_DEBUG
  check ();
#endif // HAVE_TPETRA_DEBUG
}

template<class KeyType, class ValueType, class DeviceType>
void
FixedHashTable<KeyType, ValueType, DeviceType>::
init (const host_input_keys_type& keys,
      const ValueType startingValue)
{
  const offset_type numKeys = static_cast<offset_type> (keys.dimension_0 ());
  TEUCHOS_TEST_FOR_EXCEPTION
    (numKeys > static_cast<offset_type> (INT_MAX), std::logic_error, "Tpetra::"
     "Details::FixedHashTable: This class currently only works when the number "
     "of keys is <= INT_MAX = " << INT_MAX << ".  If this is a problem for you"
     ", please talk to the Tpetra developers.");

  const offset_type size = hash_type::getRecommendedSize (numKeys);
#ifdef HAVE_TPETRA_DEBUG
  TEUCHOS_TEST_FOR_EXCEPTION(
    size == 0 && numKeys != 0, std::logic_error,
    "Tpetra::Details::FixedHashTable constructor: "
    "getRecommendedSize(" << numKeys << ") returned zero, "
    "even though the number of keys " << numKeys << " is nonzero.  "
    "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG

  // NOTE (mfh 14 May 2015) This method currently assumes UVM.  We
  // could change that by setting up ptr and val as Kokkos::DualView
  // instances.  If we do that, since we are filling on host for now,
  // we want to make sure that we only zero-fill ptr on host
  // initially, and that we don't fill val at all.  Once we finish
  // Kokkos-izing all the set-up kernels, we won't need DualView for
  // either ptr or val.

  // Kokkos::View fills with zeros by default.
  typename ptr_type::non_const_type ptr ("ptr", size + 1);

  // Allocate the array of key,value pairs.  Don't waste time filling
  // it with zeros, because we will fill it with actual data below.
  typename val_type::non_const_type val (Kokkos::ViewAllocateWithoutInitializing ("val"), numKeys);

  // Only make a device copy of the input array 'keys' if the input
  // array lives in a different memory space.  Remember that with UVM,
  // host code can access CUDA device memory, but not the other way
  // around.
  typedef DeepCopyIfNeeded<KeyType, typename host_input_keys_type::array_layout,
                           typename host_input_keys_type::execution_space,
                           execution_space> copier_type;
  typedef typename copier_type::output_view_type keys_type;
  keys_type keys_d = copier_type::copy (keys);

  //
  // Count the number of "buckets" per offsets array (ptr) entry.
  //

  // The Kokkos kernel uses atomic update instructions to count the
  // number of "buckets" per offsets array (ptr) entry.  Atomic
  // updates incur overhead, even in the sequential case.  The Kokkos
  // kernel is still correct in that case, but I would rather not
  // incur overhead then.
  if (worthCountingBucketsInParallel<execution_space> ()) {
    for (offset_type k = 0; k < numKeys; ++k) {
      const typename hash_type::result_type hashVal =
        hash_type::hashFunc (keys[k], size);
      // Shift over one, so that counts[j] = ptr[j+1].  See below.
      ++ptr[hashVal+1];

      if (ptr[hashVal+1] > 1) {
        // FIXME (mfh 19 May 2015) This is wrong!
        // Different keys could hash to the same hash value.
        hasDuplicateKeys_ = true;
      }
    }
  }
  else {
    CountBucketsValue<KeyType> result;
    CountBuckets<typename ptr_type::non_const_type,
                 keys_type> functor (ptr, keys_d, size);
    Kokkos::parallel_reduce (numKeys, functor, result);
  }

  // Compute row offsets via prefix sum:
  //
  // ptr[i+1] = \sum_{j=0}^{i} counts[j].
  //
  // Thus, ptr[i+1] - ptr[i] = counts[i], so that ptr[i+1] = ptr[i] +
  // counts[i].  If we stored counts[i] in ptr[i+1] on input, then the
  // formula is ptr[i+1] += ptr[i].
  for (offset_type i = 0; i < size; ++i) {
    ptr[i+1] += ptr[i];
  }
  //ptr[0] = 0; // We've already done this when initializing ptr above.

  // curRowStart[i] is the offset of the next element in row i.
  typename ptr_type::non_const_type curRowStart ("curRowStart", size);

  // Fill in the hash table.
  for (offset_type k = 0; k < numKeys; ++k) {
    const KeyType key = keys[k];
    const ValueType theVal = startingValue + static_cast<ValueType> (k);
    const typename hash_type::result_type hashVal =
      hash_type::hashFunc (key, size);

    const offset_type offset = curRowStart[hashVal];
    const offset_type curPos = ptr[hashVal] + offset;

    val[curPos].first = key;
    val[curPos].second = theVal;
    ++curRowStart[hashVal];
  }

  // "Commit" the computed arrays.
  ptr_ = ptr;
  val_ = val;
#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  rawPtr_ = ptr.ptr_on_device ();
  rawVal_ = val.ptr_on_device ();
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
}


template<class KeyType, class ValueType, class DeviceType>
void
FixedHashTable<KeyType, ValueType, DeviceType>::
init (const host_input_keys_type& keys,
      const host_input_vals_type& vals)
{
  const offset_type numKeys = static_cast<offset_type> (keys.dimension_0 ());
  TEUCHOS_TEST_FOR_EXCEPTION
    (numKeys > static_cast<offset_type> (INT_MAX), std::logic_error, "Tpetra::"
     "Details::FixedHashTable: This class currently only works when the number "
     "of keys is <= INT_MAX = " << INT_MAX << ".  If this is a problem for you"
     ", please talk to the Tpetra developers.");

  const offset_type size = hash_type::getRecommendedSize (numKeys);
#ifdef HAVE_TPETRA_DEBUG
  TEUCHOS_TEST_FOR_EXCEPTION(
    size == 0 && numKeys != 0, std::logic_error,
    "Tpetra::Details::FixedHashTable constructor: "
    "getRecommendedSize(" << numKeys << ") returned zero, "
    "even though the number of keys " << numKeys << " is nonzero.  "
    "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG

  // NOTE (mfh 14 May 2015) This method currently assumes UVM.  We
  // could change that by setting up ptr and val as Kokkos::DualView
  // instances.  If we do that, since we are filling on host for now,
  // we want to make sure that we only zero-fill ptr on host
  // initially, and that we don't fill val at all.  Once we finish
  // Kokkos-izing all the set-up kernels, we won't need DualView for
  // either ptr or val.

  typename ptr_type::non_const_type ptr ("ptr", size + 1);

  // Allocate the array of key,value pairs.  Don't waste time filling
  // it with zeros, because we will fill it with actual data below.
  typename val_type::non_const_type val (Kokkos::ViewAllocateWithoutInitializing ("val"), numKeys);

  // Compute number of entries in each hash table position.
  for (offset_type k = 0; k < numKeys; ++k) {
    const typename hash_type::result_type hashVal =
      hash_type::hashFunc (keys[k], size);
    // Shift over one, so that counts[j] = ptr[j+1].  See below.
    ++ptr[hashVal+1];

    if (ptr[hashVal+1] > 1) {
      hasDuplicateKeys_ = true;
    }
  }

  // Compute row offsets via prefix sum:
  //
  // ptr[i+1] = \sum_{j=0}^{i} counts[j].
  //
  // Thus, ptr[i+1] - ptr[i] = counts[i], so that ptr[i+1] = ptr[i] +
  // counts[i].  If we stored counts[i] in ptr[i+1] on input, then the
  // formula is ptr[i+1] += ptr[i].
  for (offset_type i = 0; i < size; ++i) {
    ptr[i+1] += ptr[i];
  }
  //ptr[0] = 0; // We've already done this when initializing ptr above.

  // curRowStart[i] is the offset of the next element in row i.
  typename ptr_type::non_const_type curRowStart ("curRowStart", size);

  // Fill in the hash table.
  for (offset_type k = 0; k < numKeys; ++k) {
    const KeyType key = keys[k];
    const ValueType theVal = vals[k];
    const typename hash_type::result_type hashVal =
      hash_type::hashFunc (key, size);

    const offset_type offset = curRowStart[hashVal];
    const offset_type curPos = ptr[hashVal] + offset;

    val[curPos].first = key;
    val[curPos].second = theVal;
    ++curRowStart[hashVal];
  }

  // "Commit" the computed arrays.
  ptr_ = ptr;
  val_ = val;
#if ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
  rawPtr_ = ptr.ptr_on_device ();
  rawVal_ = val.ptr_on_device ();
#endif // ! defined(TPETRA_HAVE_KOKKOS_REFACTOR)
}

template <class KeyType, class ValueType, class DeviceType>
std::string
FixedHashTable<KeyType, ValueType, DeviceType>::
description () const
{
  std::ostringstream oss;
  oss << "FixedHashTable<"
      << Teuchos::TypeNameTraits<KeyType>::name () << ","
      << Teuchos::TypeNameTraits<ValueType>::name () << ">: "
      << "{ numKeys: " << val_.dimension_0 ()
      << ", tableSize: " << ptr_.dimension_0 () << " }";
  return oss.str();
}

template <class KeyType, class ValueType, class DeviceType>
void
FixedHashTable<KeyType, ValueType, DeviceType>::
describe (Teuchos::FancyOStream& out,
          const Teuchos::EVerbosityLevel verbLevel) const
{
  using std::endl;
  using std::setw;
  using Teuchos::OSTab;
  using Teuchos::rcpFromRef;
  using Teuchos::TypeNameTraits;
  using Teuchos::VERB_DEFAULT;
  using Teuchos::VERB_NONE;
  using Teuchos::VERB_LOW;
  using Teuchos::VERB_EXTREME;

  // NOTE (mfh 14 May 2015) This method currently assumes UVM for
  // access to ptr_ and val_ from the host.

  Teuchos::EVerbosityLevel vl = verbLevel;
  if (vl == VERB_DEFAULT) vl = VERB_LOW;

  if (vl == VERB_NONE) {
    // do nothing
  }
  else if (vl == VERB_LOW) {
    out << this->description() << endl;
  }
  else {  // MEDIUM, HIGH or EXTREME
    out << "FixedHashTable:" << endl;
    {
      OSTab tab1 (rcpFromRef (out));

      const std::string label = this->getObjectLabel ();
      if (label != "") {
        out << "label: " << label << endl;
      }
      out << "Template parameters:" << endl;
      {
        OSTab tab2 (rcpFromRef (out));
        out << "KeyType: " << TypeNameTraits<KeyType>::name () << endl
            << "ValueType: " << TypeNameTraits<ValueType>::name () << endl;
      }

      const offset_type tableSize = this->getSize ();
      const offset_type numKeys = val_.dimension_0 ();

      out << "Table parameters:" << endl;
      {
        OSTab tab2 (rcpFromRef (out));
        out << "numKeys: " << numKeys << endl
            << "tableSize: " << tableSize << endl;
      }

      if (vl >= VERB_EXTREME) {
        out << "Contents: ";
        if (tableSize == 0 || numKeys == 0) {
          out << "[]" << endl;
        } else {
          out << "[ " << endl;
          {
            OSTab tab2 (rcpFromRef (out));
            for (offset_type i = 0; i < tableSize; ++i) {
              OSTab tab3 (rcpFromRef (out));
              out << "[";
              for (offset_type k = ptr_[i]; k < ptr_[i+1]; ++k) {
                out << "(" << val_[k].first << "," << val_[k].second << ")";
                if (k + 1 < ptr_[i+1]) {
                  out << ", ";
                }
              }
              out << "]" << endl;
            } // for each table position i
          }
          out << "]" << endl;
        } // The table contains entries
      } // vl >= VERB_EXTREME
    }
    out << endl;
  } // if vl > VERB_LOW
}

} // namespace Details
} // namespace Tpetra

// Macro that explicitly instantiates FixedHashTable for the given local
// ordinal (LO) and global ordinal (GO) types.  Note that FixedHashTable's
// template parameters occur in the opposite order of most Tpetra
// classes.  This is because FixedHashTable performs global-to-local
// lookup, and the convention in templated C++ lookup tables (such as
// std::map) is <KeyType, ValueType>.
//
// This macro must be explanded within the Tpetra::Details namespace.
#define TPETRA_DETAILS_FIXEDHASHTABLE_INSTANT_DEFAULTNODE(LO,GO) \
  template class FixedHashTable< GO , LO >;

// Macro that explicitly instantiates FixedHashTable for the given
// local ordinal (LO), global ordinal (GO), and Kokkos device (DEVICE)
// types.  Note that FixedHashTable's first two template parameters
// occur in the opposite order of most Tpetra classes.  This is
// because FixedHashTable performs global-to-local lookup, and the
// convention in templated C++ lookup tables (such as std::map) is
// <KeyType, ValueType>.
//
// This macro must be explanded within the Tpetra::Details namespace.
#define TPETRA_DETAILS_FIXEDHASHTABLE_INSTANT(LO, GO, DEVICE) \
  template class FixedHashTable< GO , LO , DEVICE >;

#endif // TPETRA_DETAILS_FIXEDHASHTABLE_DEF_HPP
