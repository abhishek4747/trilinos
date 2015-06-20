// Copyright (c) 2013, Sandia Corporation.
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of Sandia Corporation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#ifndef stk_mesh_FieldParallel_hpp
#define stk_mesh_FieldParallel_hpp

#include <stk_util/stk_config.h>
#include <stk_mesh/base/Types.hpp>      // for EntityProc
#include <stk_mesh/base/FieldTraits.hpp>  // for FieldTraits
#include <stk_mesh/base/FieldBase.hpp>  // for FieldBase
#include <stk_mesh/base/BulkData.hpp>
#include <stk_util/parallel/Parallel.hpp>  // for ParallelMachine
#include <stk_util/parallel/ParallelComm.hpp>  // for CommAll
#include <stk_util/environment/ReportHandler.hpp>  // for ThrowRequireMsg

#include <stddef.h>                     // for size_t
#include <vector>                       // for vector

namespace stk { namespace mesh { class Ghosting; } }

namespace stk {
namespace mesh {

/**
 * This file contains some helper functions that are part of the Field API.
 */

/** Send field-data from entities to their ghosts, for a specified 'ghosting'.
 * For entities that are ghosted, this function updates field-data from the
 * original entity to the ghosts.
 */
void communicate_field_data(
  const Ghosting                        & ghosts ,
  const std::vector< const FieldBase *> & fields );

void communicate_field_data(
  const BulkData                        & mesh ,
  const std::vector< const FieldBase *> & fields );

/** Copy data for the given fields, from owned entities to shared-but-not-owned entities.
 * I.e., shared-but-not-owned entities get an update of the field-data from the owned entity.
*/
inline
void copy_owned_to_shared( const BulkData& mesh,
                           const std::vector< const FieldBase *> & fields )
{
  communicate_field_data(*mesh.ghostings()[0], fields);
}

//----------------------------------------------------------------------

/** Sum/Max/Min (assemble) field-data for the specified fields on shared entities such that each shared entity
 * will have the same field values on each sharing proc.
 */
void parallel_sum(const BulkData& mesh, const std::vector<FieldBase*>& fields);
void parallel_max(const BulkData& mesh, const std::vector<FieldBase*>& fields);
void parallel_min(const BulkData& mesh, const std::vector<FieldBase*>& fields);


//
//  Generalized comm plans
//
//  This plan assumes the send and recv lists have identical sizes so no extra sizing communications are needed
//
template<typename T>
void parallel_data_exchange_sym_t(std::vector< std::vector<T> > &send_lists,
                                  std::vector< std::vector<T> > &recv_lists,
                                  MPI_Comm &mpi_communicator )
{
  //
  //  Determine the number of processors involved in this communication
  //
#if defined( STK_HAS_MPI)
  const int msg_tag = 10242;
  int num_procs = stk::parallel_machine_size(mpi_communicator);
  int class_size = sizeof(T);

  //
  //  Send the actual messages as raw byte streams.
  //
  std::vector<MPI_Request> recv_handles(num_procs);
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    recv_lists[iproc].resize(send_lists[iproc].size());
    if(recv_lists[iproc].size() > 0) {
      char* recv_buffer = (char*)&recv_lists[iproc][0];
      int recv_size = recv_lists[iproc].size()*class_size;
      MPI_Irecv(recv_buffer, recv_size, MPI_CHAR,
                iproc, msg_tag, mpi_communicator, &recv_handles[iproc]);
    }
  }
  MPI_Barrier(mpi_communicator);
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(send_lists[iproc].size() > 0) {
      char* send_buffer = (char*)&send_lists[iproc][0];
      int send_size = send_lists[iproc].size()*class_size;
      MPI_Send(send_buffer, send_size, MPI_CHAR,
               iproc, msg_tag, mpi_communicator);
    }
  }
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(recv_lists[iproc].size() > 0) {
      MPI_Status status;
      MPI_Wait( &recv_handles[iproc], &status );
    }
  }
#endif
}

//
//  This plan assumes the send and recv lists are matched, but that the actual ammount of data to send is unknown.
//  A processor knows which other processors it will be receiving data from, but does not know who much data.
//  Thus the comm plan is known from the inputs, but an additional message sizing call must be done.
//
template<typename T>
void parallel_data_exchange_sym_unknown_size_t(std::vector< std::vector<T> > &send_lists,
                                               std::vector< std::vector<T> > &recv_lists,
                                               MPI_Comm &mpi_communicator )
{
#if defined( STK_HAS_MPI)
  const int msg_tag = 10242;
  int num_procs = stk::parallel_machine_size(mpi_communicator);
  int class_size = sizeof(T);

  //
  //  Send the message sizes
  //
  std::vector<int> send_msg_sizes(num_procs);
  std::vector<int> recv_msg_sizes(num_procs);
  std::vector<MPI_Request> recv_handles(num_procs);

  for(int iproc = 0; iproc < num_procs; ++iproc) {
    send_msg_sizes[iproc] = send_lists[iproc].size();
  }    
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(recv_lists[iproc].size()>0) {
      MPI_Irecv(&recv_msg_sizes[iproc], 1, MPI_INT, iproc, msg_tag, mpi_communicator, &recv_handles[iproc]);
    }
  }
  MPI_Barrier(mpi_communicator);
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(send_lists[iproc].size()>0) {
      MPI_Send(&send_msg_sizes[iproc], 1, MPI_INT, iproc, msg_tag, mpi_communicator);
    }
  }
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(recv_lists[iproc].size() > 0) {
      MPI_Status status;
      MPI_Wait( &recv_handles[iproc], &status );
      recv_lists[iproc].resize(recv_msg_sizes[iproc]);
    }
  }
  //
  //  Send the actual messages as raw byte streams.
  //
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(recv_lists[iproc].size() > 0) {
      char* recv_buffer = (char*)&recv_lists[iproc][0];
      int recv_size = recv_lists[iproc].size()*class_size;
      MPI_Irecv(recv_buffer, recv_size, MPI_CHAR,
                iproc, msg_tag, mpi_communicator, &recv_handles[iproc]);
    }
  }
  MPI_Barrier(mpi_communicator);
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(send_lists[iproc].size() > 0) {
      char* send_buffer = (char*)&send_lists[iproc][0];
      int send_size = send_lists[iproc].size()*class_size;
      MPI_Send(send_buffer, send_size, MPI_CHAR,
               iproc, msg_tag, mpi_communicator);
    }
  }
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(recv_lists[iproc].size() > 0) {
      MPI_Status status;
      MPI_Wait( &recv_handles[iproc], &status );
    }
  }
#endif
}

std::vector<int> ComputeReceiveList(std::vector<int>& sendSizeArray, MPI_Comm &mpi_communicator);

//
//  Parallel_Data_Exchange: General object exchange template with unknown comm plan
//
template<typename T>
void parallel_data_exchange_t(std::vector< std::vector<T> > &send_lists,
                              std::vector< std::vector<T> > &recv_lists,
                              MPI_Comm &mpi_communicator ) {
  //
  //  Determine the number of processors involved in this communication
  //
  const int msg_tag = 10242;
  int num_procs;
  MPI_Comm_size(mpi_communicator, &num_procs);
  int my_proc;
  MPI_Comm_rank(mpi_communicator, &my_proc);
  ThrowRequire((unsigned int) num_procs == send_lists.size() && (unsigned int) num_procs == recv_lists.size());
  int class_size = sizeof(T);
  //
  //  Determine number of items each other processor will send to the current processor
  //
  std::vector<int> global_number_to_send(num_procs);
  for(int iproc=0; iproc<num_procs; ++iproc) {
    global_number_to_send[iproc] = send_lists[iproc].size();
  }
  std::vector<int> numToRecvFrom = ComputeReceiveList(global_number_to_send, mpi_communicator);
  //
  //  Send the actual messages as raw byte streams.
  //
  std::vector<MPI_Request> recv_handles(num_procs);
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    recv_lists[iproc].resize(numToRecvFrom[iproc]);
    if(recv_lists[iproc].size() > 0) {
      char* recv_buffer = (char*)&recv_lists[iproc][0];
      int recv_size = recv_lists[iproc].size()*class_size;
      MPI_Irecv(recv_buffer, recv_size, MPI_CHAR,
                iproc, msg_tag, mpi_communicator, &recv_handles[iproc]);
    }
  }
  MPI_Barrier(mpi_communicator);
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(send_lists[iproc].size() > 0) {
      char* send_buffer = (char*)&send_lists[iproc][0];
      int send_size = send_lists[iproc].size()*class_size;
      MPI_Send(send_buffer, send_size, MPI_CHAR,
               iproc, msg_tag, mpi_communicator);
    }
  }
  for(int iproc = 0; iproc < num_procs; ++iproc) {
    if(recv_lists[iproc].size() > 0) {
      MPI_Status status;
      MPI_Wait( &recv_handles[iproc], &status );
    }
  }
}

} // namespace mesh
} // namespace stk

#endif

