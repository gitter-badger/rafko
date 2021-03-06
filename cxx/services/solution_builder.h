#ifndef SOLUTION_BUILDER_H
#define SOLUTION_BUILDER_H

#include "sparse_net_global.h"

#include <memory>
#include <deque>
#include <vector>
#include <atomic>

#include "gen/sparse_net.pb.h"
#include "gen/solution.pb.h"
#include "models/service_context.h"
#include "services/neuron_router.h"
#include "services/synapse_iterator.h"

namespace sparse_net_library {

using std::vector;

/**
 * @brief      Front-end to create a @Soltuion to solve a @SparseNet. @max_solve_threads
 *             determines the maximum number of threads to be used inside the @build function.
 *             A @Solution_chain is built up by a 2D array of @Partial_solutions. The first row
 *             is independent because they are mostly processing only inputs to the Neural network
 *             and Neurons driectly dependent on them.
 *             Any further @Partial_solution messages depend on the @Partial_solution in thep previous row.
 *             In case there is only one used device for the net, which has insufficient internal
 *             memory for a big @Partial_solution, it can be divided into multiple smaller ones,
 *             which are executed sequentially.
 *             The separation of the net into decoupled independent partial solutions enable
 *             distributed computing based on micro-services, as the elements inside @Decoupled_solutions
 *             can be solved in an independent manner. Dependencies inside the Neural network are represented
 *             in the order of the elements in a @solution_chain.
 */
class Solution_builder{
public:
  /**
   * @brief      Set the number of threads to be used while generating the solution to the given @SparseNet
   *
   * @param[in]  number  The number of threads to be used
   *
   * @return     Builder reference for chaining
   */
  Solution_builder& max_solve_threads(uint8 number){
    arg_max_solve_threads = number;
    is_max_solve_threads_set = true;
    return *this;
  }

  Solution_builder& device_max_megabytes(sdouble32 megabytes){
    arg_device_max_megabytes = megabytes;
    return *this;
  }

  /**
   * @brief      Set the used arena pointer
   *
   * @param      arena  The arena
   *
   * @return     Builder reference for chaining
   */
  Solution_builder& arena_ptr(google::protobuf::Arena* arena){
    arg_arena_ptr = arena;
    return *this;
  }

  /**
   * @brief      Sets parameters providable from a service context
   *
   * @param[in]  context  The context
   *
   * @return     Builder reference for chaining
   */
  Solution_builder& service_context(Service_context context){
    return max_solve_threads(context.get_max_solve_threads())
    .device_max_megabytes(context.get_device_max_megabytes())
    .arena_ptr(context.get_arena_ptr());
  }

  /**
   * @brief      Build the Solution to be solved by @Solution_solver
   *
   * @param[in]  net   The net
   *
   * @return     Builder reference for chaining
   */
  Solution* build(const SparseNet& net);

private:
  /**
   * Helper variables to see if different required arguments are set inside the builder
   */
  bool is_max_solve_threads_set = false;
  google::protobuf::Arena* arg_arena_ptr = nullptr;
  uint8 arg_max_solve_threads = 1;
  sdouble32 arg_device_max_megabytes = 2.0 /* GB */ * 1024.0/* MB */;
};

} /* namespace sparse_net_library */

#endif /* SOLUTION_BUILDER_H */
