#ifndef NEURON_ROUTER_H
#define NEURON_ROUTER_H

#include <deque>
#include <functional>

#include "sparse_net_global.h"
#include "gen/sparse_net.pb.h"
#include "services/synapse_iterator.h"

namespace sparse_net_library {

using std::unique_ptr;
using std::vector;
using std::deque;
using std::atomic;

/**
 * @brief      This class describes a neuron router which iterates through the given @SparseNet,
               collecting a subset of Neurons from the thread, all of whom are able to be solved without
               waiting for any other Neurons. The subset is being collected based on the input relations
               between the Neurons. The Neurons at the beginning of the net only take in input data,
               so they already have their inputs ready. Any other Neurons build upon that, with each Iteration
               some additional @Neuron nodes are collected into a subset. That subset is later to be used by
               the @Solution_builder to compile @Partial_solutions.
               If a Neuron is solvable, its state is being set to "reserved", and collected into the subset.
               After an iteration the state update from the subset needs to be handled by whoever has access to
               the Neuron indexes inside.
 */
class Neuron_router{
public:
  Neuron_router(const SparseNet& sparse_net);
  Neuron_router(const Neuron_router& other);

  uint32 operator[](int index){
    return get_neuron_index_from_subset(index);
  }

  /**
   * @brief      Runs the given function with every index of the input of the Neuron
   *
   * @param[in]  neuron_index  The neuron index
   * @param[in]  to_run        THe function to run
   */
  void run_for_neuron_inputs(uint32 neuron_index, std::function< void(int) > to_run){
    if(net.neuron_array_size() > static_cast<int>(neuron_index)){
      Synapse_iterator iter(net.neuron_array(neuron_index).input_indices());
      iter.iterate(to_run);
    }else throw "Neuron index out of bounds!";
  }

  /**
   * @brief      Collects some Neurons into a solvable subset of the net
   *
   * @param[in]  arg_max_solve_threads     The argument maximum solve threads
   * @param[in]  arg_device_max_megabytes  The argument device maximum megabytes
   * @param[in]  strict                    In strict mode, reserved Neurons do not count as finished
   */
  void collect_subset(uint8 arg_max_solve_threads, sdouble32 arg_device_max_megabytes, bool strict = true);

  uint32 get_neuron_index_from_subset(uint32 subset_index){
    if((!collection_running)&&(0 < net_subset.size())){
      return net_subset[subset_index];
    }else throw "Invalid usage or Index out of bounds!";
  }

  bool get_first_neuron_index_from_subset(uint32& put_it_here){
    if((!collection_running)&&(0 < net_subset.size())){
      put_it_here = net_subset.front();
      return true;
    }else return false;
  }

  bool confirm_first_subset_element_processed(uint32 neuron_index){
    if((!collection_running)&&(0 < net_subset.size())&&(neuron_index == net_subset.front())){
      (neuron_states[neuron_index])->store(neuron_state_processed_value(neuron_index));
      net_subset.pop_front();
      return true;
    }else return false;
  }
  
  bool confirm_first_subset_element_ommitted(uint32 neuron_index){
    bool ret = false;
    if((0 < net_subset.size())&&(neuron_index == net_subset.front())){
      (neuron_states[neuron_index])->store(0);
      net_subset.pop_front();
      ret = true;
    }
    return ret;
  }

  uint32 get_subset_size(){
    return net_subset.size();
  }

  void reset_remaining_subset(void){
    for(uint32 neuron_index : net_subset){
      confirm_first_subset_element_ommitted(neuron_index);
    }
  }

  /**
   * @brief      Gives back Iteration state
   *
   * @return     true if the crrent iteration of the net is finished and resulted with a subset of it
   */
  bool finished() const{
    return (
      (static_cast<int>(output_layer_iterator) == (net.neuron_array_size()-1))
      &&(is_neuron_processed(output_layer_iterator))
    );
  }
  bool is_neuron_in_progress(uint32 neuron_index) const{
    return (neuron_number_of_inputs[neuron_index] > *neuron_states[neuron_index]);
  }
  bool is_neuron_reserved(uint32 neuron_index) const{
    return (neuron_state_reserved_value(neuron_index) == *neuron_states[neuron_index]);
  }
  bool is_neuron_solvable(uint32 neuron_index) const{
    return (neuron_number_of_inputs[neuron_index] == *neuron_states[neuron_index]);
  }
  bool is_neuron_processed(uint32 neuron_index) const{
    return (neuron_state_processed_value(neuron_index) == *neuron_states[neuron_index]);
  }
private:

  /**
   * @brief      Called form inside @collect_subset; A thread to handle @collect_subset
   *
   * @param[in]  arg_max_solve_threads     The argument maximum solve threads
   * @param[in]  arg_device_max_megabytes  The argument device maximum megabytes
   * @param[in]  thread_index              The thread index
   */
  void collect_subset_thread(uint8 arg_max_solve_threads, sdouble32 arg_device_max_megabytes, uint8 thread_index, bool strict);

  /**
   * @brief      Called form inside @collect_subset_thread; Checking the current Neuron and its input states
   *             updates its state accordingly
   *
   * @param      net            The Sparse Net to be used
   * @param      visiting       A Vector containing the currently visiting Neuron along with the path leading to it
   * @return     The next neuron to move the iteration to
   */
  uint32 get_next_neuron(vector<uint32>& visiting, bool strict);

  /**
   * @brief      Called form inside @collect_subset_thread; Adds a neuron into subset and updates relevant build states
   *
   * @param      net            The Sparse Net to be used
   * @param      visiting       A Vector containing the currently visiting Neuron along with the path leading to it
   * @param      visiting_next  The Next Neuron Candidate, which might be the same as the latest visit ( that means no candidates found to move to)
   */
  void add_neuron_into_subset(uint32 neuron_index);

  /**
   * @brief      Decides the next Neuron to iterate to and increases the output layer iterator if needed
   *
   * @param      visiting       The visiting
   * @param      visiting_next  The visiting next
   */
  void step(vector<uint32>& visiting, uint32 visiting_next);

  const SparseNet& net;

  bool collection_running = false;

  /**
   * Number of already processed output layer Neurons
   */
  atomic<uint32> output_layer_iterator;

  /**
   * For each @Neuron in @SparseNet stores the processed state. Values:
   *  - Number of processed children ( storing raw children number without synapse information )
   *  - Number of processed children + 1 in case the Neuron is reserved
   *  - Number of processed children + 2 in case the Neuron is processed
   */
  vector<unique_ptr<atomic<uint32>>> neuron_states;

  /**
   * Number of inputs a Neuron has, based on the input index synapse sizes
   */
  vector<uint32> neuron_number_of_inputs;

  /**
   * A subset of the net representing independent solutions
   */
  std::mutex net_subset_mutex;
  std::atomic<sdouble32> net_subset_size_bytes; /* The size of the currently partial solution to be built in bytes */
  deque<uint32> net_subset_index;
  deque<uint32> net_subset;

  /**
   * The number of times the algorithm ran to look for Neuron candidates, it is used to decide relevance to the currently finished subset.
   */
  uint16 iteration = 1; /* Has to start with 1, otherwise values mix with neuron processed value */

  /**
   * @brief       functions to help build partial solutions
   *
   * @param[in]  neuron_index  The neuron index inside @neuron_number_of_inputs and @neuron_states
   *
   * @return     Information depending on the function
   */
   uint32 neuron_state_reserved_value(uint32 neuron_index) const{
    return neuron_number_of_inputs[neuron_index] + 1u;
  }
   uint32 neuron_state_processed_value(uint32 neuron_index) const{
    return neuron_number_of_inputs[neuron_index] + 2u;
  }
   sint32 neuron_state_iteration_value(uint32 neuron_index) const{
    return (*neuron_states[neuron_index] - neuron_state_processed_value(neuron_index));
  }
   uint32 neuron_iteration_relevance(uint32 neuron_index) const{
    return static_cast<uint32>(std::max( 0, neuron_state_iteration_value(neuron_index) ));
  }
   sint32 neuron_state_next_iteration_value(uint32 neuron_index, uint16 iteration) const{
    return (neuron_state_processed_value(neuron_index) + iteration + 1u);
  }
   bool is_neuron_subset_candidate(uint32 neuron_index, uint16 iteration) const{
    return(
      (neuron_iteration_relevance(neuron_index) <= iteration)
      &&(!is_neuron_processed(neuron_index))
      &&(!is_neuron_reserved(neuron_index))
    );
  }
};

} /* namespace sparse_net_library */
#endif /* NEURON_ROUTER_H */
