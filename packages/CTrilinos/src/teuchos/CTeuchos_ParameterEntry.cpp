
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


#include "CTrilinos_config.h"

#include "CTrilinos_enums.h"
#include "CTeuchos_ParameterEntry.h"
#include "CTeuchos_ParameterEntry_Cpp.hpp"
#include "Teuchos_ParameterEntry.hpp"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_utils.hpp"
#include "CTrilinos_utils_templ.hpp"
#include "CTrilinos_Table.hpp"
#include "CTeuchos_any_Cpp.hpp"
#include "CTeuchos_ParameterList_Cpp.hpp"


namespace {


using Teuchos::RCP;
using CTrilinos::Table;


/* table to hold objects of type Teuchos::ParameterEntry */
Table<Teuchos::ParameterEntry>& tableOfParameterEntrys()
{
    static Table<Teuchos::ParameterEntry> loc_tableOfParameterEntrys(CT_Teuchos_ParameterEntry_ID);
    return loc_tableOfParameterEntrys;
}


} // namespace


//
// Definitions from CTeuchos_ParameterEntry.h
//


extern "C" {


CT_Teuchos_ParameterEntry_ID_t Teuchos_ParameterEntry_Create (  )
{
    return CTeuchos::storeNewParameterEntry(new Teuchos::ParameterEntry());
}

CT_Teuchos_ParameterEntry_ID_t Teuchos_ParameterEntry_Duplicate ( 
  CT_Teuchos_ParameterEntry_ID_t sourceID )
{
    const Teuchos::RCP<const Teuchos::ParameterEntry> source = 
        CTeuchos::getConstParameterEntry(sourceID);
    return CTeuchos::storeNewParameterEntry(new Teuchos::ParameterEntry(
        *source));
}

void Teuchos_ParameterEntry_Destroy ( 
  CT_Teuchos_ParameterEntry_ID_t * selfID )
{
    CTeuchos::removeParameterEntry(selfID);
}

void Teuchos_ParameterEntry_setAnyValue ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, 
  CT_Teuchos_any_ID_t valueID, boolean isDefault )
{
    const Teuchos::RCP<const Teuchos::any> value = CTeuchos::getConstany(
        valueID);
    CTeuchos::getParameterEntry(selfID)->setAnyValue(*value, ((isDefault) != 
        FALSE ? true : false));
}

void Teuchos_ParameterEntry_setDocString ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, const char docString[] )
{
    CTeuchos::getParameterEntry(selfID)->setDocString(std::string(docString));
}

CT_Teuchos_ParameterList_ID_t Teuchos_ParameterEntry_setList ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, boolean isDefault, 
  const char docString[] )
{
    return CTeuchos::storeParameterList(&( CTeuchos::getParameterEntry(
        selfID)->setList(((isDefault) != FALSE ? true : false), std::string(
        docString)) ));
}

double Teuchos_ParameterEntry_getValue_double ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, double * ptr )
{
    return CTeuchos::getConstParameterEntry(selfID)->getValue<double>(ptr);
}

int Teuchos_ParameterEntry_getValue_int ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, int * ptr )
{
    return CTeuchos::getConstParameterEntry(selfID)->getValue<int>(ptr);
}

CT_Teuchos_any_ID_t Teuchos_ParameterEntry_getAny ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, boolean activeQry )
{
    return CTeuchos::storeany(&( CTeuchos::getParameterEntry(selfID)->getAny(
        ((activeQry) != FALSE ? true : false)) ));
}

CT_Teuchos_any_ID_t Teuchos_ParameterEntry_getAny_const ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, boolean activeQry )
{
    return CTeuchos::storeConstany(&( CTeuchos::getConstParameterEntry(
        selfID)->getAny(((activeQry) != FALSE ? true : false)) ));
}

boolean Teuchos_ParameterEntry_isUsed ( 
  CT_Teuchos_ParameterEntry_ID_t selfID )
{
    return ((CTeuchos::getConstParameterEntry(
        selfID)->isUsed()) ? TRUE : FALSE);
}

boolean Teuchos_ParameterEntry_isList ( 
  CT_Teuchos_ParameterEntry_ID_t selfID )
{
    return ((CTeuchos::getConstParameterEntry(
        selfID)->isList()) ? TRUE : FALSE);
}

boolean Teuchos_ParameterEntry_isType_double ( 
  CT_Teuchos_ParameterEntry_ID_t selfID )
{
    return ((CTeuchos::getConstParameterEntry(
        selfID)->isType<double>()) ? TRUE : FALSE);
}

boolean Teuchos_ParameterEntry_isType_int ( 
  CT_Teuchos_ParameterEntry_ID_t selfID )
{
    return ((CTeuchos::getConstParameterEntry(
        selfID)->isType<int>()) ? TRUE : FALSE);
}

boolean Teuchos_ParameterEntry_isDefault ( 
  CT_Teuchos_ParameterEntry_ID_t selfID )
{
    return ((CTeuchos::getConstParameterEntry(
        selfID)->isDefault()) ? TRUE : FALSE);
}

const char * Teuchos_ParameterEntry_docString ( 
  CT_Teuchos_ParameterEntry_ID_t selfID )
{
    return CTeuchos::getConstParameterEntry(selfID)->docString().c_str();
}

void Teuchos_ParameterEntry_Assign ( 
  CT_Teuchos_ParameterEntry_ID_t selfID, 
  CT_Teuchos_ParameterEntry_ID_t sourceID )
{
    Teuchos::ParameterEntry& self = *( CTeuchos::getParameterEntry(selfID) );

    const Teuchos::RCP<const Teuchos::ParameterEntry> source = 
        CTeuchos::getConstParameterEntry(sourceID);
    self = *source;
}


} // extern "C"


//
// Definitions from CTeuchos_ParameterEntry_Cpp.hpp
//


/* get Teuchos::ParameterEntry from non-const table using CT_Teuchos_ParameterEntry_ID */
const Teuchos::RCP<Teuchos::ParameterEntry>
CTeuchos::getParameterEntry( CT_Teuchos_ParameterEntry_ID_t id )
{
    return tableOfParameterEntrys().get<Teuchos::ParameterEntry>(
        CTrilinos::abstractType<CT_Teuchos_ParameterEntry_ID_t>(id));
}

/* get Teuchos::ParameterEntry from non-const table using CTrilinos_Universal_ID_t */
const Teuchos::RCP<Teuchos::ParameterEntry>
CTeuchos::getParameterEntry( CTrilinos_Universal_ID_t id )
{
    return tableOfParameterEntrys().get<Teuchos::ParameterEntry>(id);
}

/* get const Teuchos::ParameterEntry from either the const or non-const table
 * using CT_Teuchos_ParameterEntry_ID */
const Teuchos::RCP<const Teuchos::ParameterEntry>
CTeuchos::getConstParameterEntry( CT_Teuchos_ParameterEntry_ID_t id )
{
    return tableOfParameterEntrys().getConst<Teuchos::ParameterEntry>(
        CTrilinos::abstractType<CT_Teuchos_ParameterEntry_ID_t>(id));
}

/* get const Teuchos::ParameterEntry from either the const or non-const table
 * using CTrilinos_Universal_ID_t */
const Teuchos::RCP<const Teuchos::ParameterEntry>
CTeuchos::getConstParameterEntry( CTrilinos_Universal_ID_t id )
{
    return tableOfParameterEntrys().getConst<Teuchos::ParameterEntry>(id);
}

/* store Teuchos::ParameterEntry (owned) in non-const table */
CT_Teuchos_ParameterEntry_ID_t
CTeuchos::storeNewParameterEntry( Teuchos::ParameterEntry *pobj )
{
    return CTrilinos::concreteType<CT_Teuchos_ParameterEntry_ID_t>(
        tableOfParameterEntrys().store<Teuchos::ParameterEntry>(pobj, true));
}

/* store Teuchos::ParameterEntry in non-const table */
CT_Teuchos_ParameterEntry_ID_t
CTeuchos::storeParameterEntry( Teuchos::ParameterEntry *pobj )
{
    return CTrilinos::concreteType<CT_Teuchos_ParameterEntry_ID_t>(
        tableOfParameterEntrys().store<Teuchos::ParameterEntry>(pobj, false));
}

/* store const Teuchos::ParameterEntry in const table */
CT_Teuchos_ParameterEntry_ID_t
CTeuchos::storeConstParameterEntry( const Teuchos::ParameterEntry *pobj )
{
    return CTrilinos::concreteType<CT_Teuchos_ParameterEntry_ID_t>(
        tableOfParameterEntrys().store<Teuchos::ParameterEntry>(pobj, false));
}

/* remove Teuchos::ParameterEntry from table using CT_Teuchos_ParameterEntry_ID */
void
CTeuchos::removeParameterEntry( CT_Teuchos_ParameterEntry_ID_t *id )
{
    CTrilinos_Universal_ID_t aid = 
        CTrilinos::abstractType<CT_Teuchos_ParameterEntry_ID_t>(*id);
    tableOfParameterEntrys().remove(&aid);
    *id = CTrilinos::concreteType<CT_Teuchos_ParameterEntry_ID_t>(aid);
}

/* remove Teuchos::ParameterEntry from table using CTrilinos_Universal_ID_t */
void
CTeuchos::removeParameterEntry( CTrilinos_Universal_ID_t *aid )
{
    tableOfParameterEntrys().remove(aid);
}

/* purge Teuchos::ParameterEntry table */
void
CTeuchos::purgeParameterEntry(  )
{
    tableOfParameterEntrys().purge();
}



