#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <mpi.h>
#include <stk_util/util/ParameterList.hpp>
#include <stk_io/StkMeshIoBroker.hpp>
#include <fieldNameTestUtils.hpp>

namespace
{
  TEST(StkMeshIoBrokerHowTo, writeHeartbeatCSVChangePrecision)
  {

    const std::string file_name = "Heartbeat-CSV.txt";
    MPI_Comm communicator = MPI_COMM_WORLD;

    stk::util::ParameterList parameters;
    
    {
      // ========================================================================
      // INITIALIZATION...
      // Add some parameters to write and read...
      parameters.set_param("PI", -3.14159);  // Double 
      parameters.set_param("Answer", 42);   // Integer

      std::vector<double> my_vector;
      my_vector.push_back(2.78);
      my_vector.push_back(5.30);
      my_vector.push_back(6.21);
      parameters.set_param("some_doubles", my_vector);   // Vector of doubles...
    
      std::vector<int> ages;
      ages.push_back(55);
      ages.push_back(49);
      ages.push_back(21);
      ages.push_back(19);
    
      parameters.set_param("Ages", ages);   // Vector of integers...
    }

    {
      // ========================================================================
      // EXAMPLE USAGE...
      // Begin use of stk io heartbeat file...
      stk::io::StkMeshIoBroker stkIo(communicator);

      // Define the heartbeat output...
      Ioss::PropertyManager hb_props;
      hb_props.add(Ioss::Property("PRECISION", 10));
      size_t heartbeat_index = stkIo.add_heartbeat_output(file_name, stk::io::CSV, hb_props);

      stk::util::ParameterMapType::const_iterator i = parameters.begin();
      stk::util::ParameterMapType::const_iterator iend = parameters.end();
      for (; i != iend; ++i) {
	const std::string parameterName = (*i).first;
	stk::util::Parameter &parameter = parameters.get_param(parameterName);

	// Tell heartbeat database which global variables should be output at each step...
	stkIo.add_heartbeat_global(heartbeat_index, parameterName, parameter.value, parameter.type);
      }

      // Now output the global variables...
      int timestep_count = 1;
      double time = 0.0;
      for (int step=1; step <= timestep_count; step++) {
	stkIo.process_heartbeat_output(heartbeat_index, step, time);
      }
    }

    {
      // ========================================================================
      // VERIFICATION:
      // open the heartbeat file...
      std::ifstream heartbeat(file_name.c_str());
      std::string header_line;
      std::string data_line;

      std::string expected_header_line = "             Time,            Ages_1,            Ages_2,            Ages_3,            Ages_4,            Answer,                PI,    some_doubles_1,    some_doubles_2,    some_doubles_3";
      std::string expected_data_line = " 0.0000000000e+00,                55,                49,                21,                19,                42, -3.1415900000e+00,  2.7800000000e+00,  5.3000000000e+00,  6.2100000000e+00";

      EXPECT_TRUE(!std::getline(heartbeat, header_line).fail());
      EXPECT_STREQ(header_line.c_str(), expected_header_line.c_str());
      EXPECT_TRUE(!std::getline(heartbeat, data_line).fail());
      EXPECT_STREQ(data_line.c_str(), expected_data_line.c_str());
    }

    // ========================================================================
    // CLEANUP:
    unlink(file_name.c_str());
  }
}
