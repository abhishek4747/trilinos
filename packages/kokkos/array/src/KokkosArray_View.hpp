/*
//@HEADER
// ************************************************************************
//
//   KokkosArray: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
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
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOSARRAY_VIEW_HPP
#define KOKKOSARRAY_VIEW_HPP

#include <KokkosArray_Macros.hpp>
#include <KokkosArray_MemoryManagement.hpp>

#include <impl/KokkosArray_StaticAssert.hpp>
#include <impl/KokkosArray_ArrayTraits.hpp>
#include <impl/KokkosArray_Shape.hpp>
#include <impl/KokkosArray_AnalyzeShape.hpp>

namespace KokkosArray {
namespace Impl {

template< class MemorySpace , class ValueType , class Shape >
class ViewOper ;

template< class DstScalar , class DstMemory , class DstManagement ,
          class SrcScalar , class SrcMemory , class SrcManagement >
struct ViewAssignable ;

template< class Scalar , class Management >
struct ViewCreatable ;

template< class >
struct ViewInitialize ;

}

template< class DataType ,
          class LayoutType ,
          class DeviceType = LayoutType ,
          class ManagementType = DeviceType >
class View
  : public Impl::ViewOper<
      typename DeviceType::memory_space ,
      typename Impl::AnalyzeShape< DataType , typename LayoutType::array_layout >
                   ::value_type ,
      typename Impl::AnalyzeShape< DataType , typename LayoutType::array_layout >
                   ::shape >
{
private:

  typedef Impl::AnalyzeShape<DataType,
                             typename LayoutType::array_layout >  analysis ;
  typedef typename analysis::const_type      const_data_type ;
  typedef typename analysis::non_const_type  non_const_data_type ;

public:

  typedef DataType        data_type ;
  typedef LayoutType      layout_type ;
  typedef DeviceType      device_type ;
  typedef ManagementType  management_type;

  typedef View<       data_type, layout_type, device_type, management_type > type ;
  typedef View< const_data_type, layout_type, device_type, management_type > const_type ;

  typedef View< data_type , layout_type , Host, MemoryManaged >  HostMirror ;

  typedef typename analysis::scalar_type               scalar_type ;
  typedef typename analysis::value_type                value_type ;
  typedef typename LayoutType::array_layout            array_layout ;
  typedef typename device_type::memory_space           memory_space ;
  typedef typename management_type::memory_management  memory_management ;
  typedef typename device_type::size_type              size_type ;
  typedef typename analysis::shape                     shape_type ;

private:

  typedef Impl::ViewOper< memory_space ,
                          value_type ,
                          shape_type > oper_type ;

  template< class , class , class , class> friend class View ;

public:

  /*------------------------------------------------------------------*/

  static const unsigned Rank = shape_type::rank ;

  KOKKOSARRAY_INLINE_FUNCTION
  size_type rank() const { return shape_type::rank ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_0() const { return oper_type::m_shape.N0 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_1() const { return oper_type::m_shape.N1 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_2() const { return oper_type::m_shape.N2 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_3() const { return oper_type::m_shape.N3 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_4() const { return oper_type::m_shape.N4 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_5() const { return oper_type::m_shape.N5 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_6() const { return oper_type::m_shape.N6 ; }

  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension_7() const { return oper_type::m_shape.N7 ; }

  template< typename iType >
  KOKKOSARRAY_INLINE_FUNCTION
  size_type dimension( const iType & r ) const
    {
      return iType(0) == r ? oper_type::m_shape.N0 : (
             iType(1) == r ? oper_type::m_shape.N1 : (
             iType(2) == r ? oper_type::m_shape.N2 : (
             iType(3) == r ? oper_type::m_shape.N3 : (
             iType(4) == r ? oper_type::m_shape.N4 : (
             iType(5) == r ? oper_type::m_shape.N5 : (
             iType(6) == r ? oper_type::m_shape.N6 : (
             iType(7) == r ? oper_type::m_shape.N7 : 0 )))))));
    }

  /*------------------------------------------------------------------*/
  KOKKOSARRAY_INLINE_FUNCTION
  scalar_type * ptr_on_device() const { return oper_type::m_ptr_on_device ; }

  /*------------------------------------------------------------------*/
  /** \brief Query shape */
  KOKKOSARRAY_INLINE_FUNCTION
  shape_type shape() const { return oper_type::m_shape ; }

  /*------------------------------------------------------------------*/
  /** \brief  Query if NULL view */
  KOKKOSARRAY_INLINE_FUNCTION
  operator bool () const
  { return 0 != oper_type::m_ptr_on_device ; }

  /** \brief  Query if view to same memory */
  KOKKOSARRAY_INLINE_FUNCTION
  bool operator == ( const View & rhs ) const
  {
    return oper_type::m_ptr_on_device == rhs.m_ptr_on_device &&
           oper_type::m_shape == rhs.m_shape ;
  }

  /** \brief  Query if not view to same memory */
  KOKKOSARRAY_INLINE_FUNCTION
  bool operator != ( const View & rhs ) const
  {
    return oper_type::m_ptr_on_device != rhs.m_ptr_on_device ||
           oper_type::m_shape != rhs.m_shape ;
  }

  /** \brief  Query if view to same memory */
  template< class rhsDataType ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement >
  KOKKOSARRAY_INLINE_FUNCTION
  bool operator ==
    ( const View<rhsDataType,rhsLayout,rhsMemory,rhsManagement> & rhs ) const
  {
    return oper_type::m_ptr_on_device == rhs.m_ptr_on_device &&
           oper_type::m_shape == rhs.m_shape ;
  }

  /** \brief  Query if not view to same memory */
  template< class rhsDataType ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement >
  KOKKOSARRAY_INLINE_FUNCTION
  bool operator !=
    ( const View<rhsDataType,rhsLayout,rhsMemory,rhsManagement> & rhs ) const
  {
    return oper_type::m_ptr_on_device != rhs.m_ptr_on_device ||
           oper_type::m_shape != rhs.m_shape ;
  }

  /*------------------------------------------------------------------*/

private:

  KOKKOSARRAY_INLINE_FUNCTION
  void internal_private_assign( const shape_type & shape , scalar_type * ptr )
  {
    oper_type::m_shape          = shape ;
    oper_type::m_ptr_on_device  = ptr ;

#if defined( KOKKOSARRAY_EXECUTION_SPACE_IS_HOST )
    if ( Impl::is_same< memory_management , MemoryManaged >::value ) {
      memory_space::increment( oper_type::m_ptr_on_device );
    }
#endif
  }

  KOKKOSARRAY_INLINE_FUNCTION
  void internal_private_clear()
  {
#if defined( KOKKOSARRAY_EXECUTION_SPACE_IS_HOST )
    if ( Impl::is_same< memory_management , MemoryManaged >::value ) {
      memory_space::decrement( oper_type::m_ptr_on_device );
    }
#endif
    oper_type::m_ptr_on_device = 0 ;
  }


  void internal_private_create( const std::string & label ,
                                const shape_type shape )
  {
#if defined( KOKKOSARRAY_EXECUTION_SPACE_IS_HOST )
    if ( Impl::is_same< memory_management , MemoryManaged >::value ) {
      const size_t count =
        Impl::ShapeMap<shape_type>::allocation_count( shape );

      oper_type::m_shape = shape ;
      oper_type::m_ptr_on_device = (scalar_type *)
        memory_space::allocate( label ,
                                typeid(scalar_type) ,
                                sizeof(scalar_type) ,
                                count );

      Impl::ViewInitialize< View >::apply( *this );
    }
    else {
      oper_type::m_ptr_on_device = 0 ;
      oper_type::m_shape = shape_type();
    }
#else
    oper_type::m_ptr_on_device = 0 ;
    oper_type::m_shape = shape_type();
#endif
  }

public:

  /** \brief  Construct a NULL view */
  KOKKOSARRAY_INLINE_FUNCTION
  View()
    {
      oper_type::m_ptr_on_device = 0 ;
      oper_type::m_shape = shape_type();
    }

  /** \brief  Construct a view of the array */
  KOKKOSARRAY_INLINE_FUNCTION
  View( const View & rhs )
    {
      internal_private_assign( rhs.m_shape , rhs.m_ptr_on_device );
    }

  /**  \brief  Destroy this view of the array.
   *           If the last view then allocated memory is deallocated.
   */
  KOKKOSARRAY_INLINE_FUNCTION
  ~View()
  {
    internal_private_clear();
  }

  /** \brief  Assign to a view of the rhs array.
   *          If the old view is the last view
   *          then allocated memory is deallocated.
   */
  KOKKOSARRAY_INLINE_FUNCTION
  View & operator = ( const View & rhs )
  {
    internal_private_clear();
    internal_private_assign( rhs.m_shape , rhs.m_ptr_on_device );
    return *this ;
  }

  /*------------------------------------------------------------------*/
  // Construct a compatible view:

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement >
  View( const View< rhsData , rhsLayout , rhsMemory, rhsManagement > & rhs )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape() );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  // Assign a compatible view:

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement >
  View & operator =
    ( const View< rhsData , rhsLayout , rhsMemory, rhsManagement > & rhs )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape() );

      internal_private_clear();
      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );

      return *this ;
    }

  /*------------------------------------------------------------------*/
  /** \brief  Construct a subview */

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement,
            class ArgType0 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement > & rhs ,
        const ArgType0 & arg0 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape() , arg0 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement> & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape() , arg0 , arg1 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 , class ArgType2 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement > & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 ,
        const ArgType2 & arg2 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape() , arg0 , arg1 , arg2 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 , class ArgType2 ,
            class ArgType3 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement> & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 ,
        const ArgType2 & arg2 ,
        const ArgType3 & arg3 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape() , arg0 , arg1 , arg2 , arg3 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 , class ArgType2 ,
            class ArgType3 , class ArgType4 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement> & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 ,
        const ArgType2 & arg2 ,
        const ArgType3 & arg3 ,
        const ArgType4 & arg4 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type data( rhs.shape(), arg0, arg1, arg2, arg3, arg4 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 , class ArgType2 ,
            class ArgType3 , class ArgType4 , class ArgType5 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement> & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 ,
        const ArgType2 & arg2 ,
        const ArgType3 & arg3 ,
        const ArgType4 & arg4 ,
        const ArgType5 & arg5 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type
         data( rhs.shape() , arg0 , arg1 , arg2 , arg3 , arg4 , arg5 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 , class ArgType2 ,
            class ArgType3 , class ArgType4 , class ArgType5 ,
            class ArgType6 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement> & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 ,
        const ArgType2 & arg2 ,
        const ArgType3 & arg3 ,
        const ArgType4 & arg4 ,
        const ArgType5 & arg5 ,
        const ArgType6 & arg6 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type
         data( rhs.shape() , arg0 , arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  template< class rhsData ,
            class rhsLayout ,
            class rhsMemory ,
            class rhsManagement ,
            class ArgType0 , class ArgType1 , class ArgType2 ,
            class ArgType3 , class ArgType4 , class ArgType5 ,
            class ArgType6 , class ArgType7 >
  View( const View< rhsData , rhsLayout , rhsMemory , rhsManagement> & rhs ,
        const ArgType0 & arg0 ,
        const ArgType1 & arg1 ,
        const ArgType2 & arg2 ,
        const ArgType3 & arg3 ,
        const ArgType4 & arg4 ,
        const ArgType5 & arg5 ,
        const ArgType6 & arg6 ,
        const ArgType7 & arg7 )
    {
      typedef View< rhsData , rhsLayout , rhsMemory , rhsManagement > rhs_view ;
      typedef typename rhs_view::scalar_type       rhs_scalar_type ;
      typedef typename rhs_view::shape_type        rhs_shape_type ;
      typedef typename rhs_view::memory_space      rhs_memory_space ;
      typedef typename rhs_view::memory_management rhs_memory_management ;

      typedef typename
        Impl::ViewAssignable< scalar_type ,
                              memory_space ,
                              memory_management ,
                              rhs_scalar_type ,
                              rhs_memory_space ,
                              rhs_memory_management >::type  ok_assign ;

      // SubShape<*,*> only exists for valid subshapes:

      typedef typename
        Impl::SubShape< shape_type , rhs_shape_type >::type
          subshape_type ;

      const subshape_type
         data( rhs.shape(), arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7 );

      internal_private_assign( data.shape , rhs.m_ptr_on_device + data.offset );
    }

  /*------------------------------------------------------------------*/
  /*------------------------------------------------------------------*/
  /* Creation with allocation of memory on the device */

  View( const std::string & label , const shape_type shape )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;
    
    internal_private_create( label , shape );
  }

  explicit View( const std::string & label )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>() );
  }

  View( const std::string & label ,
        const size_t n0 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1,n2) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1,n2,n3) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1,n2,n3,n4) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 ,
        const size_t n5 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1,n2,n3,n4,n5) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 ,
        const size_t n5 ,
        const size_t n6 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1,n2,n3,n4,n5,n6) );
  }

  View( const std::string & label ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 ,
        const size_t n5 ,
        const size_t n6 ,
        const size_t n7 )
  {
    typedef typename
      Impl::ViewCreatable< scalar_type , memory_management >::type ok_create ;

    internal_private_create( label , shape_type::template create<memory_space>(n0,n1,n2,n3,n4,n5,n6,n7) );
  }

  /*------------------------------------------------------------------*/
  /** \brief  For unit testing shape mapping. */
  explicit View( const shape_type & shape )
    { internal_private_assign( shape , 0 ); }

  /*------------------------------------------------------------------*/
  /* Create an unmannaged view */

  View( scalar_type * data_ptr , const shape_type shape )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape , data_ptr );
  }

  explicit View( scalar_type * data_ptr )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>() , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1,n2) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1,n2,n3) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1,n2,n3,n4) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 ,
        const size_t n5 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1,n2,n3,n4,n5) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 ,
        const size_t n5 ,
        const size_t n6 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1,n2,n3,n4,n5,n6) , data_ptr );
  }

  View( scalar_type * data_ptr ,
        const size_t n0 ,
        const size_t n1 ,
        const size_t n2 ,
        const size_t n3 ,
        const size_t n4 ,
        const size_t n5 ,
        const size_t n6 ,
        const size_t n7 )
  {
    typedef typename
     Impl::StaticAssertSame< MemoryUnmanaged , memory_management >::type
       ok_create ;

    internal_private_assign( shape_type::template create<memory_space>(n0,n1,n2,n3,n4,n5,n6,n7) , data_ptr );
  }
};

//----------------------------------------------------------------------------

template< class DataType ,
          class LayoutType ,
          class DeviceType ,
          class ManagedType >
typename View< DataType , LayoutType , DeviceType , ManagedType >::HostMirror
create_mirror( const View<DataType,LayoutType,DeviceType,ManagedType > & );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Impl {
template< class DstScalar , class DstMemory , class DstManagement ,
          class SrcScalar , class SrcMemory , class SrcManagement >
struct ViewAssignable {};

/* Fully compatible view: */
template< class Scalar , class Memory , class Management >
struct ViewAssignable< Scalar , Memory , Management ,
                       Scalar , Memory , Management >
{ typedef true_type type ; };

/* Set const from non-const: */
template< class Scalar , class Memory , class Management >
struct ViewAssignable< const Scalar , Memory , Management ,
                             Scalar , Memory , Management >
{ typedef true_type type ; };

/* Set unmanaged from managed: */
template< class Scalar , class Memory >
struct ViewAssignable< Scalar , Memory , MemoryUnmanaged ,
                       Scalar , Memory , MemoryManaged >
{ typedef true_type type ; };

/* Set const unmanaged from managed: */
template< class Scalar , class Memory >
struct ViewAssignable< const Scalar , Memory , MemoryUnmanaged ,
                             Scalar , Memory , MemoryManaged >
{ typedef true_type type ; };


template< class Scalar , class Management >
struct ViewCreatable {};

template< class Scalar >
struct ViewCreatable< const Scalar , MemoryManaged > {};

/* Only non-const managed memory view may be created */
template< class Scalar >
struct ViewCreatable< Scalar , MemoryManaged >
{ typedef true_type type ; };

}
} // namespace KokkosArray

#include <impl/KokkosArray_View_factory.hpp>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif

