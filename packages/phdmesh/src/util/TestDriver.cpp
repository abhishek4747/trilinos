/*------------------------------------------------------------------------*/
/*      phdMesh : Parallel Heterogneous Dynamic unstructured Mesh         */
/*                Copyright (2007) Sandia Corporation                     */
/*                                                                        */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*                                                                        */
/*  This library is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU Lesser General Public License as        */
/*  published by the Free Software Foundation; either version 2.1 of the  */
/*  License, or (at your option) any later version.                       */
/*                                                                        */
/*  This library is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     */
/*  Lesser General Public License for more details.                       */
/*                                                                        */
/*  You should have received a copy of the GNU Lesser General Public      */
/*  License along with this library; if not, write to the Free Software   */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   */
/*  USA                                                                   */
/*------------------------------------------------------------------------*/
/**
 * @author H. Carter Edwards
 */

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <util/TPI.h>
#include <util/Parallel.hpp>
#include <util/ParallelComm.hpp>
#include <util/ParallelReduce.hpp>
#include <util/ParallelInputStream.hpp>

#include <util/TestDriver.hpp>

namespace phdmesh {

int test_driver(
  ParallelMachine comm , std::istream & is , const TestDriverMap & dmap )
{
  const double time_init = phdmesh::wall_time();

  const unsigned p_rank = parallel_machine_rank( comm );

  TPI_ThreadPool pool = NULL ;

  int result = 0 ;

  try {
    while ( is.good() && ! is.eof() ) {
      if ( isspace( is.peek() ) ) {
        is.ignore();
      }
      else {

        char buffer[1024] ; buffer[0] = 0 ;

        is.getline( buffer , sizeof(buffer) );

        std::string str_line( buffer );

        std::istringstream is_line( str_line );

        if ( p_rank == 0 ) { std::cout << buffer << std::endl ; }

        if ( is_line.peek() != '#' ) {

          std::string is_key ; is_line >> is_key ;

          if ( is_key.empty() ) {
            ;
          }
          else if ( is_key == std::string("threadpool") ) {
            unsigned ntasks = 1 ;
            if ( is_line.good() ) { is_line >> ntasks ; }
            if ( pool != NULL ) { TPI_Finalize(); }
            TPI_Init( ntasks , & pool );
          }
          else {
            TestDriverMap::const_iterator iter = dmap.find( is_key );

            if ( iter != dmap.end() ) {
              const TestSubprogram ts = (*iter).second ;
              (*ts)( comm , pool , is_line );
            }
            else {
              if ( p_rank == 0 ) {
                std::cout << "Unknown test '" << is_key
                          << "', known tests = {" << std::endl ;
                for ( iter = dmap.begin() ; iter != dmap.end() ; ++iter ) {
                  std::cout << "  " << (*iter).first << std::endl ;
                }
                std::cout << "}" << std::endl ;
              }
              throw std::runtime_error( std::string("Given unknown test") );
            }
          }
        }
      }
    }
  }
  catch( const std::exception & x ) {
    std::cout << 'P' << p_rank << ": " << x.what() << std::endl ;
    result = -1 ;
  }

  TPI_Finalize();

  const double time_fin = phdmesh::wall_time();

  if ( p_rank == 0 )  {
    std::cout << "Total time = " << ( time_fin - time_init ) << " seconds"
              << std::endl ;
  }

  return result ;
}

}

