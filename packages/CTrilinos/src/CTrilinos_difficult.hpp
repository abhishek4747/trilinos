
/*! @HEADER */
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
Questions? Contact M. Nicole Lemaster (mnlemas@sandia.gov)

************************************************************************
*/
/*! @HEADER */


/*! @file CTrilinos_difficult.hpp
 * @brief Difficult enum conversions for CTrilinos. */


#ifndef CTRILINOS_DIFFICULT_HPP
#define CTRILINOS_DIFFICULT_HPP


#include "CTrilinos_config.h"


#include <string>

#include "CTrilinos_enums.h"
#include "CTrilinos_exceptions.hpp"
#ifdef HAVE_CTRILINOS_IFPACK
#include "Ifpack.h"
#endif /* HAVE_CTRILINOS_IFPACK */


namespace CTrilinos {


#ifdef HAVE_CTRILINOS_IFPACK
Ifpack::EPrecType convert_to_difficult_enum( CT_EPrecType_E_t en );
#endif /* HAVE_CTRILINOS_IFPACK */

#ifdef HAVE_CTRILINOS_IFPACK
CT_EPrecType_E_t convert_from_difficult_enum( Ifpack::EPrecType en );
#endif /* HAVE_CTRILINOS_IFPACK */


} // namespace CTrilinos


#endif

