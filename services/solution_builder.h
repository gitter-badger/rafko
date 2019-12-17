#ifndef SOLUTION_CHAIN_BUILDER_H

#include <vector>

#include "models/gen/sparse_net.pb.h"
#include "models/gen/solution.pb.h"

#include "sparse_net_global.h"

namespace sparse_net_library {

using std::vector;

/**
 * @brief      Front-end to create a @Soltuion to solve a @SparseNet. @used_devices_number
 *             determines how many @Solution_chain messages shall be generated by the 
 *             @build function inside the returned @Solution. A @Solution_chain is built up by 
 *             an array of @Partial_solution messages, which depend on one another. The first
 *             @Partial solution is independent of any other partial solutions because it is 
 *             processing only inputs to the Neural network. Any other @Partial_solution messages
 *             inside the same @Solution_chain depend on the one in the previous index. 
 *             In case there is only one used device for the net, which has insufficient internal
 *             usable memory, multiple independent partial solutions may be created by the 
 *             @Solution_builder, to separate the whole solution into smaller Partial_solution 
 *             messages which do not depend on one another, but the device is able to execute them
 *             sequentially.
 *             Setting @used_devices_number > 1 divides the independent @Partial_solutions into smaller
 *             independent ones, and places them in separate Solution_chains. Setting this number to a high
 *             value enables big networks(too big to be solved on a single device) to be run in a distributable
 *             fashion in paralell across multiple devices.
 */
class Solution_builder{
public:
  /**
   * @brief      Set the number of devices to be used while solving the given @SparseNet
   *
   * @param[in]  number  The number of devices to be used
   *
   * @return     Builder reference for chaining
   */
  Solution_builder& used_devices_number(uint32 number);

  /**
   * @brief      Set the size of the available internal memory for each used device in bytes
   *
   * @param[in]  device_index  which device 
   * @param[in]  bytesize      The size of the devices internal memory in Bytes
   *
   * @return     Builder reference for chaining
   */
  Solution_builder& device_available_byte_size(uint32 device_index, uint32 size);   

  /**
   * @brief      Build the Solution to be solved by @Solution_solver
   *
   * @param[in]  net   The net
   *
   * @return     Builder reference for chaining
   */
  Solution* build( const SparseNet* net );

private:
  /**
   * Helper variables to see if different required arguments are set inside the builder
   */
  bool is_used_devices_number_set = false; 
  vector<bool> is_device_available_byte_size_set;

  uint32 arg_used_devices_number = 0;
  vector<uint32> arg_device_available_byte_size;
};

} /* namespace sparse_net_library */

#endif /* SOLUTION_CHAIN_BUILDER_H */