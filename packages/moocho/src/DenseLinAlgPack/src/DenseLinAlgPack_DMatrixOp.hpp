// //////////////////////////////////////////////////////////////////////////////////
// DMatrixOp.hpp
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.
//
// Basic DMatrix / DMatrixSlice operation functions.
//
// Changes: 6/9/98:
//		*	I simplified the set of functions to only include direct analogs to
//			BLAS routines.  Addition operations are derived from these and some
//			of the simplifications are given in LinAlgPackOp.h.
//		*	Testing for aliasing was removed since it provides overhead
//			and is only a problem if the lhs argment apears in the rhs.  Also it
//			will simpify maintance.
//		*	Changed the naming convension for matrices so that so that they all
//			(rectangular, triangular and symmetric) all use just "M".
//		*	Triangular and symmetric matrices are now aggregated into simple
//			classes to simplify the interface and usage.
//		*	6/12/98: the assignment functions were moved into a seperate file
//			to remove circular dependencies that existed.

#ifndef GEN_MATRIX_OP_H
#define GEN_MATRIX_OP_H

#include "DenseLinAlgPackTypes.hpp"
#include "DenseLinAlgPackAssertOp.hpp"
#include "DMatrixClass.hpp"
#include "DMatrixAsTriSym.hpp"
#include "DVectorOp.hpp"

/* * @name {\bf Basic DMatrix Operation Functions (Level 2,3 BLAS)}.
  *
  * These funtions perform that basic linear algebra operations involving; vectors,
  * rectangular matrices, triangular matrices, and symmetric matrices.  The types
  * for the matrices passed to these functions are DMatrix and DMatrixSlice.
  * These rectangular matrices can be treated conseptually as square triangular (DMatrixSliceTri)
  * and symmetric (DMatrixSliceSym) matrices.  The functions are ment to provide the basic computations
  * for wrapper classes for triangular (unit diagonal, uppper and lower etc.)
  * and symetric (upper or lower triangular storage) which will provide better type
  * safty than is achieved by using these function directly.
  *
  * The implementations of these functions takes care of the following details:
  *
  * <ul>
  *	<li> Resizing DMatrix LHS on assignment
  *	<li> Checking preconditions (sizes of arguments) if \Ref{LINALGPACK_CHECK_RHS_SIZES} is defined
  * </ul>
  *
  * The preconditions for all of the functions are that the sizes of the rhs arguments
  * and lhs arguments agree.  If they do not and \Ref{LINALGPACK_CHECK_RHS_SIZES}
  * is defined, then those functions will throw a #std::length_error# exception.
  *
  * Naming convenstion for algebric functions.
  *
  * Algebraic funcitons are named according to their types and the operations on those
  * types.  For example, condider the functions:
  *
  *		#V_VpV(...)#  =>   #DVectorSlice = DVectorSlice + DVectorSlice#	\\
  *		#Vp_StV(...)#  =>   #DVectorSlice += Scalar * DVectorSlice#
  *
  * Reading the first function identifier from left to right, the first letter 'V' stands for the
  * type of the lhs argument.  The underscore character is ment to stand for the equal sign '='.
  * The last part of the identifer name, 'VpV' stands for the binary plus operation "V + V".
  * In the second function the characters 'p_' stands for the '+=' operation.
  *
  * The character identifiers for the types of the operands are (all uppercase):
  *
  * \begin{center}
  * \begin{tabular}{ll}
  *		types										&	abreviation	\\
  *	\hline
  *		scalar (value_type)							&		S		\\
  *		1-D vectors (DVector (lhs), DVectorSlice)		&		V		\\
  *		2-D matrices (DMatrix (lhs)
  *			, DMatrixSlice, DMatrixSliceTri, DMatrixSliceTriEle
  *			, DMatrixSliceSym)								&		M		\\
  * \end{tabular}
  * \end{center}
  *
  * The identifers for the arithmetic operations are (all lowercase):
  *
  * \begin{center}
  * \begin{tabular}{ll}
  *		Arithmetic operation						&	abreviation	\\
  *	\hline
  *		+ (binary plus)								&		p		\\
  *		- (binary minus and unary negation)			&		m		\\
  *		* (binary multiplation, times)				&		t		\\
  * \end{tabular}
  * \end{center}
  *
  * The types and order of the arguments to these algebraic functions is also
  * associated with the names of the identifer of the function.  The lhs
  * argument(s) always appears first as in the identifer name.  The the
  * rhs argment(s) appear as they appear in the identifer name.  Each type
  * operand as one or more arguments associated with it.  For example, a 'V'
  * for DVectorSlice in a rhs expression only has the type argument 'const DVectorSlice&'.
  * A matrix, whether it is rectangular (DMatrixSlice), triangular (DMatrixSliceTri, DMatrixSliceTriEle)
  * or symmetric (DMatrixSliceSym).
  *
  * The identifer names and the corresponding type arguments are:
  *
  * \begin{center}
  * \begin{tabular}{ll}
  *		{\bf Abreviation}				&	{\bf Arguments}															\\
  *	\hline
  *		S								&	#value_type#													\\
  *		V (DVector, lhs only)			&	#DVector&#														\\
  *		V (DVectorSlice, lhs)			&	#DVectorSlice&#													\\
  *		V (DVectorSlice, rhs)			&	#const DVectorSlice&#											\\
  *		M (DMatrix, lhs only)			&	#DMatrix&#													\\
  *		M (DMatrixSlice, lhs)			&	#DMatrixSlice&#												\\
  *		M (DMatrixSlice, rhs)			&	#const DMatrixSlice&, BLAS_Cpp::Transp#						\\
  *		M (Element-wise operation
  *			Triangular DMatrixSlice
  *			, lhs)						&	#DMatrixSliceTriEle&#													\\
  *		M (Element-wise operation
  *			Triangular DMatrixSlice
  *			, rhs)						&	#const DMatrixSliceTriEle&, BLAS_Cpp::Transp#							\\
  *		M (Structure dependent operation
  *			Triangular DMatrixSlice
  *			, rhs only)					&	#const DMatrixSliceTri&, BLAS_Cpp::Transp#								\\
  *		M (Symmetric DMatrixSlice
  *			, lhs)						&	#DMatrixSliceTri&#														\\
  *		M (Symmetric DMatrixSlice
  *			, rhs)						&	#cosnt DMatrixSliceTri&, BLAS_Cpp::Transp#								\\
  * \end{tabular}
  * \end{center}
  *
  * Using the table above you can deduce the argments by looking at the function identifer name.
  * For example, the function #V_MtV# using a triangular matrix and a DVector as the lhs the argument
  * type list is:
  * #(DVector&, const DMatrixSliceTri&, BLAS_Cpp::Transp, BLAS_Cpp::Diag, const DVectorSlice&)#.
  *
  * The only variation on this rule is with operations such as: \\
  * vs_lhs = alpha * op(gms_rhs1) * vs_rhs2 + beta * vs_lhs.\\
  * For this type of operation the vs_lhs argument is not included twise as the function would
  * be prototyped as:
  *
  * #void Vp_StMtV(DVectorSlice& vs_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
  *			, BLAS_Cpp::Transp trans_rhs1, const DVectorSlice& vs_rhs2, value_type beta);#
  *
  * These operations are designed to work with the LinAlgPackOp template functions.
  * These template functions provide default implementations for variations on
  * these operations.  The BLAS operations for triangular matrix solves do not fit in with
  * this system of template functions.
  *
  * In general it is not allowed that the lhs argument appear in the rhs expression.
  * The only exception to this are the Level 2 and 3 BLAS operations that involve
  * triangular matrices.  Here it is allowed because this is the way the BLAS routines
  * are defined.
  */
// @{

namespace DenseLinAlgPack {

// /////////////////////////////////////////////////////////////////////////////////////////
/* * @name {\bf Element-wise Algebraic Operations}.
  *
  * These functions that implement element-wise operations for rectangular
  * and triangular regions of matrices.  The rhs operands can be transposed
  * (op(rhs1) = rhs) or non-transposed (op(rhs1) = trans(rhs1)).
  *
  * Functions for triangular matrices allow mixes of upper and lower
  * triangular regions.  Therefore, they provide the machinary for
  * element-wise operations for triangular and symmetric matrices.
  */
// @{

/// gms_lhs *= alpha (BLAS xSCAL)
void Mt_S(DMatrixSlice* gms_lhs, value_type alpha);

/// gms_lhs = diag(vs_rhs) * op(gms_rhs) [Row or column scaling]
void M_diagVtM( DMatrixSlice* gms_lhs, const DVectorSlice& vs_rhs
                , const DMatrixSlice& gms_rhs, BLAS_Cpp::Transp trans_rhs );

/// tri_lhs *= alpha (BLAS xSCAL)
void Mt_S(DMatrixSliceTriEle* tri_lhs, value_type alpha);

/// tri_lhs += alpha * tri_rhs (BLAS xAXPY)
void Mp_StM(DMatrixSliceTriEle* tri_lhs, value_type alpha, const DMatrixSliceTriEle& tri_rhs);

/* * @name LinAlgOpPack compatable (compile-time polymorphism).
  */
// @{

/// gms_lhs += alpha * op(gms_rhs) (BLAS xAXPY)
void Mp_StM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSlice& gms_rhs
	, BLAS_Cpp::Transp trans_rhs);

/// gms_lhs += alpha * op(sym_rhs) (BLAS xAXPY)
void Mp_StM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSliceSym& sym_rhs
	, BLAS_Cpp::Transp trans_rhs);

/// gms_lhs += alpha * op(tri_rhs) (BLAS xAXPY)
void Mp_StM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs
	, BLAS_Cpp::Transp trans_rhs);

// @}

// inline
// /// tri_lhs += alpha * tri_rhs (needed for LinAlgOpPack)
// void Mp_StM(DMatrixSliceTriEle* tri_lhs, value_type alpha, const DMatrixSliceTriEle& tri_rhs
//	, BLAS_Cpp::Transp)
//{
//	Mp_StM(tri_lhs, alpha, tri_rhs);
//}

//		end Element-wise Algebraic Operations
// @}

// /////////////////////////////////////////////////////////////////////////////////////
/* * @name {\bf Level-2 BLAS (vector-matrtix) Liner Algebra Operations}.
  *
  * These functions are setup as nearly direct calls to the level-2 BLAS.
  */

// @{

/// vs_lhs = alpha * op(gms_rhs1) * vs_rhs2 + beta * vs_lhs (BLAS xGEMV)
void Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DVectorSlice& vs_rhs2, value_type beta = 1.0);

///
/* * vs_lhs = alpha * op(sym_rhs1) * vs_rhs2 + beta * vs_lhs. (BLAS xSYMV).
  *
  * The transpose argument #trans_rhs1# is ignored and is only included so that
  * it is compatable with the LinAlgPackOp template functions.
  */
void Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha, const DMatrixSliceSym& sym_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DVectorSlice& vs_rhs2, value_type beta = 1.0);

///
/* * v_lhs = op(tri_rhs1) * vs_rhs2 (BLAS xTRMV)
  *
  * Here vs_rhs2 and v_lhs can be the same vector.
  *
  * Here vs_rhs2 is assigned to v_lhs so if v_lhs is the same as vs_rhs2
  * no unnecessary copy will be performed before the BLAS function trmv(...)
  * is called.
  */
void V_MtV(DVector* v_lhs, const DMatrixSliceTri& tri_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const DVectorSlice& vs_rhs2);

///
/* * vs_lhs = op(tri_rhs1) * vs_rhs2 (BLAS xTRMV)
  *
  * Same as previous accept for a DVectorSlice as the lhs.
  */
void V_MtV(DVectorSlice* vs_lhs, const DMatrixSliceTri& tri_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const DVectorSlice& vs_rhs2);

///
/* * vs_lhs = alpha * op(tri_rhs1) * vs_rhs2 + beta * vs_lhs.
  *
  * This function is needed for use with the LinAlgPackOp template functions.
  * Here vs_lhs and vs_rhs2 may be the same vector because of the way that
  * the template functions work.
  *
  * This function calls #V_MtV(tmp, tri_rhs1, trans_rhs1, vs_rhs2);#
  * where #tmp# is a temporary vector to hold the result of the operation.
  *
  */
void Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DVectorSlice& vs_rhs2, value_type beta = 1.0);

///
/* * v_lhs = inv(op(tri_rhs1)) * vs_rhs2 (BLAS xTRSV)
  *
  * Here vs_rhs2 is assigned to v_lhs so if v_lhs is the same as vs_rhs2
  * no unnecessary copy will be performed before the BLAS function trsv(...)
  * is called.
  *
  * There are no LinAlgPackOp template functions compatable with this operation.
  */
void V_InvMtV(DVector* v_lhs, const DMatrixSliceTri& tri_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const DVectorSlice& vs_rhs2);

///
/* * vs_lhs = inv(op(tri_rhs1)) * vs_rhs2 (BLAS xTRSV)
  *
  * Same as above except for DVectorSlice as lhs.
  */
void V_InvMtV(DVectorSlice* vs_lhs, const DMatrixSliceTri& tri_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const DVectorSlice& vs_rhs2);

///
/* * gms_lhs = alpha * vs_rhs1 * vs_rhs2' + gms_lhs (BLAS xGER).
  *
  * This results in a direct call the the BLAS function ger(...).
  * Since this function is performing a special linear algebra operation (a rank-1 update)
  * it does not use the specal naming sceme as the rest of the more typical operations.
  * The arguments are ordered similarly to the BLAS specification.
  *
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void ger(value_type alpha, const DVectorSlice& vs_rhs1, const DVectorSlice& vs_rhs2
		 , DMatrixSlice* gms_lhs);

///
/* * sym_lhs = alpha * vs_rhs * vs_rhs' + sym_lhs (BLAS xSYR).
  *
  * This results in a direct call the the BLAS function syr(...).
  * Since this function is performing a special linear algebra operation (a rank-1 update)
  * it does not use the specal naming sceme as the rest of the more typical operations.
  * The arguments are ordered similarly to the BLAS specification.
  *
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void syr(value_type alpha, const DVectorSlice& vs_rhs, DMatrixSliceSym* sym_lhs);

///
/* * sym_lhs = alpha * vs_rhs1 * vs_rhs2' + alpha * vs_rhs2 * vs_rhs1' + sym_lhs (BLAS xSYR2).
  * 
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void syr2(value_type alpha, const DVectorSlice& vs_rhs1, const DVectorSlice& vs_rhs2
	, DMatrixSliceSym* sym_lhs);

//		end Level-2 BLAS (vector-matrtix) Liner Algebra Operations
// @}

// //////////////////////////////////////////////////////////////////////////////////////////
/* * @name {\bf Level-3 BLAS (matrix-matrix) Linear Algebra Operations}.
  *
  */

// @{
//		begin Level-3 BLAS (matrix-matrix) Linear Algebra Operations

// ////////////////////////////
/* * @name Rectangular Matrices
  */
// @{

///
/* * gms_lhs = alpha * op(gms_rhs1) * op(gms_rhs2) + beta * gms_lhs (BLAS xGEMV).
  *
  * This function results in a nearly direct call the the BLAS gemv(...) function.
  * No temporaries need to be created.
  */
void Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2, value_type beta = 1.0);

//	end Rectangular Matrices
// @}

// ////////////////////////////
/* * @name Symmetric Matrices
  */
// @{

///
/* * gms_lhs = alpha * op(sym_rhs1) * op(gms_rhs2) + beta * gms_lhs (left) (BLAS xSYMM).
  *
  * The straight BLAS call would be:
  *
  * gms_lhs = alpha * sym_rhs1 * gms_rhs2 + beta * gms_lhs
  *
  * but for compatability with the LinAlgPackOp template functions this form is
  * used instead.  The first transpose argument #trans_rhs1# is ignorned.
  * If #trans_rhs2 == BLAS_Cpp::trans# then a temporary copy of #gms_rhs2# must
  * be made to directly call the BLAS function symm(...).
  *
  */
void Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSliceSym& sym_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2, value_type beta = 1.0);

///
/* * gms_lhs = alpha * op(gms_rhs1) * op(sym_rhs2) + beta * gms_lhs (right) (BLAS xSYMM).
  *
  * This function is similar to the previous one accept the symmeric matrix now appears
  * to the right.  Again #trans_rhs2# is ignored and a tempory matrix will be created
  * if #trans_rhs1 == BLAS_Cpp::trans#.
  */
void Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSliceSym& sym_rhs2
	, BLAS_Cpp::Transp trans_rhs2, value_type beta = 1.0);

///
/* * sym_lhs = alpha * op(gms_rhs) * op(gms_rhs')  + beta * sym_lhs (BLAS xSYRK).
  *
  * This results in a direct call the the BLAS function syrk(...).
  * Since this function is performing a special linear algebra operation (a rank-k update)
  * it does not use the specal naming sceme as the rest of the more typical operations.
  * The arguments are ordered similarly to the BLAS specification.
  *
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void syrk(BLAS_Cpp::Transp trans, value_type alpha, const DMatrixSlice& gms_rhs
	, value_type beta, DMatrixSliceSym* sym_lhs);

///
/* * sym_lhs = alpha * op(gms_rhs1) * op(gms_rhs2') + alpha * op(gms_rhs2) * op(gms_rhs1')
  *		+ beta * sym_lhs (BLAS xSYR2K).
  *
  * Like syrk(...) this is a specialized linear algebra operation and does not follow the
  * standard naming sceme.
  *
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void syr2k(BLAS_Cpp::Transp trans,value_type alpha, const DMatrixSlice& gms_rhs1
	, const DMatrixSlice& gms_rhs2, value_type beta, DMatrixSliceSym* sym_lhs);

//	end Symmetric Matrices
// @}

// ////////////////////////////
/* * @name Triangular Matrices
  */
// @{

///
/* * gm_lhs = alpha * op(tri_rhs1) * op(gms_rhs2) (left) (BLAS xTRMM).
  *
  * Here op(gms_rhs2) and gms_lhs can be the same matrix .
  *
  * For the BLAS operation trmm(...) to be called #assign(gm_lhs,gms_rhs2,trans_rhs2)#
  * is called first.
  */
void M_StMtM(DMatrix* gm_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gms_lhs = alpha * op(tri_rhs1) * op(gms_rhs2) (left) (BLAS xTRMM).
  *
  * Same as above accept for DMatrixSlice as the lhs.
  */
void M_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gm_lhs = alpha * op(gms_rhs1) * op(tri_rhs2) (right) (BLAS xTRMM).
  *
  * Here op(gms_rhs1) and gms_lhs can be the same matrix .
  *
  * For the BLAS operation trmm(...) to be called #assign(gm_lhs,gms_rhs1,trans_rhs1)#
  * is called first.  This form is used so that it conforms to
  * the LinAlgPackOp template functions.
  */
void M_StMtM(DMatrix* gm_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSliceTri& tri_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gms_lhs = alpha * op(gms_rhs1) * op(tri_rhs2) (right) (BLAS xTRMM).
  *
  * Same as above accept for DMatrixSlice as the lhs.
  */
void M_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSliceTri& tri_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gms_lhs = alpha * op(tri_rhs1) * op(gms_rhs2) + beta * gms_lhs (left).
  *
  * This form is included to conform with the LinAlgPackOp template functions.
  * For this to work, a temporary (#tmp#) is created to hold the result of the
  * matrix-matrix product.
  *
  * It calls #M_StMtM(&tmp,alpha,tri_rhs1,trans_rhs1,gms_rhs2,trans_rhs2);#
  */
void Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2, value_type beta = 1.0);

///
/* * gms_lhs = alpha * op(gms_rhs1) * op(tri_rhs2) + beta * gms_lhs (right).
  *
  * This form is included to conform with the LinAlgPackOp template functions.
  * For this to work, a temporary (#tmp#) is created to hold the result of the
  * matrix-matrix product.
  *
  * It calls #M_StMtM(&tmp,alpha,gms_rhs1,trans_rhs1,tri_rhs2,trans_rhs2);#
  */
void Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSliceTri& tri_rhs2
	, BLAS_Cpp::Transp trans_rhs2, value_type beta = 1.0);

///
/* * gm_lhs = alpha * inv(op(tri_rhs1)) * op(gms_rhs2) (left) (BLAS xTRSM).
  *
  * For the BLAS trsm(...) function to be called #assign(gm_lhs,gms_rhs2,trans_rhs2)#
  * is called first.
  *
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void M_StInvMtM(DMatrix* gm_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gms_lhs = alpha * inv(op(tri_rhs1)) * op(gms_rhs2) (left) (BLAS xTRSM).
  *
  * Same as above accept for DMatrixSlice as the lhs.
  */
void M_StInvMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSliceTri& tri_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gm_lhs = alpha * op(gms_rhs1) * inv(op(tri_rhs2)) (right) (BLAS xTRSM).
  *
  * For the BLAS trsm(...) function to be called #assign(gm_lhs,gms_rhs1,trans_rhs1)#
  * is called first.
  *
  * There is no analog to this operation in the LinAlgPackOp template functions.
  */
void M_StMtInvM(DMatrix* gm_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSliceTri& tri_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

///
/* * gms_lhs = alpha * op(gms_rhs1) * inv(op(tri_rhs2)) (right) (BLAS xTRSM).
  *
  * Same as above accept for DMatrixSlice as the lhs.
  */
void M_StMtInvM(DMatrixSlice* gm_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const DMatrixSliceTri& tri_rhs2
	, BLAS_Cpp::Transp trans_rhs2);

//	end Triangular Matrices
// @}

//		end Level-3 BLAS (matrix-matrix) Linear Algebra Operations
// @}

} // end namespace DenseLinAlgPack

// //////////////////////////////////////////////////////////////////////////////////////
// Inline function definitions

//		 end Basic DMatrix Operation Functions
// @}

#endif	// GEN_MATRIX_OP_H
