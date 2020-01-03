#ifndef SparseNetBUILDER_H
#define SparseNetBUILDER_H

#include <vector>
#include <memory>

#include "gen/sparse_net.pb.h"
#include "models/transfer_function_info.h"
#include "models/weight_initializer.h"
#include "sparse_net_global.h"

namespace sparse_net_library {

using std::shared_ptr;

/**
 * @brief SparseNetBuilder: Builder class to compile Sparse Neural Networks
 * There are Two ways to use this class. One is to add the required building blocks of a Network 
 * manually. The Other is to use one of the higher level construction functions like @SparseNetBuilder::denseLayers.
 * Some parameters needed to be added unconditionally, which is checked by @SparseNetBuilder::io_pre_requisites_set.
 * Ownership of the returned built SparseNet depends on the arena argument value:
 * - In case the arena is a nullptr, the ownership is of the caller of the function
 * - In case the arena is valid, it has the ownership
 */
class SparseNetBuilder
{  
public:
  
  /**
   * @brief      SparseNetBuilder::input_size: sets the number of expected inputs for the SparseNet object to be built
   *
   * @param[in]  size  The size
   *
   * @return     builder reference for chaining
   */
  SparseNetBuilder& input_size(uint32 size);

  /**
   * @brief      sets the number of expected outputs for the SparseNet object to be built
   *
   * @param[in]  size  The size
   *
   * @return     builder reference for chaining
   */
  SparseNetBuilder& output_neuron_number(uint32 size);

  /**
   * @brief      Sets the expected range of inputs to the net
   *
   * @param[in]  range  The range
   *
   * @return   
   */
  SparseNetBuilder& expectedInputRange(sdouble32 range);

  /**
   * @brief      Sets the Weight initializer to a manual one, overwriting the default weight
   *             intialization assigned for any builder interface, except @SparseNetBuilder::build. 
   *
   * @param[in]  initializer  The initializer
   *
   * @return     Builder reference for chaining
   */
  SparseNetBuilder& weight_initializer(shared_ptr<Weight_initializer> initializer);

  /**
   * @brief      set the given neuron_array and transfer its ownership to the builder
   *
   * @param[in]  arr   The array of neurons to be transferred
   *
   * @return     Builder reference for chaining
   */
  SparseNetBuilder& neuron_array(vector<Neuron> arr);

  /**
   * @brief      set the given weight table and transfer ownership to the builder
   *
   * @param[in]  table  The table to be transferred
   *
   * @return     reference for chaining
   */
  SparseNetBuilder& weight_table(vector<sdouble32> table);

  /**
   * @brief    Sets the Google protobuffer arena reference in the builder, to 
   *           make allocation more effective, assign built net ownership automatically.
   *           It's an optional parameter
   *
   * @param    arena  The pointer to a protobuf arena
   *
   * @return   builder reference for chaining
   */
  SparseNetBuilder& arena_ptr(google::protobuf::Arena* arena);

  /**
   * @brief      Sets an optional argument which restricts transfer functions by layer ( usable with @denseLayers )
   *
   * @param[in]  allowed_transfer_functions_by_layer  The allowed transfer functions by layer
   *
   * @return     { description_of_the_return_value }
   */
  SparseNetBuilder& allowed_transfer_functions_by_layer(vector<vector<transfer_functions> > filter);

  /**
   * @brief      creates a Fully connected feedforward neural network based on the IO arguments and
   *             and function arguments
   *
   * @param[in]  layerSizes         how many layers will there be in the result
   *                    and how big are those layers going to be
   * @param[in]  allowed_transfer_functions_by_layer  The allowed transfer functions per layer
   *
   * @return   the built neural network
   */
  SparseNet* denseLayers(vector<uint32> layer_sizes, vector<vector<transfer_functions>> allowed_transfer_functions_by_layer);

  /**
   * @brief      Same as above, but without any Transfer function restrictions
   *
   * @param[in]  layer_sizes  The layer sizes
   *
   * @return     the built neural network
   */
  SparseNet* denseLayers(vector<uint32> layer_sizes);

  /**
   * @brief    creates a Neural network from the given Arguments. Requires the following
   *       components to be set:  
   *       - The Neuron Array contains in an array the initialized Neuron 
   *       - The Weight Table containing the weights used by the Neural network
   *       Building Networks like this is very dangerous, as the integrity of the components are not 
   *       checked, therefore the user of this interface should be responsible for the behavior of the 
   *       resulted Neural Network.
   *
   * @return   the built neural network
   */
  SparseNet* build();

private:
  /**
   * Helper variables to see if different required arguments are set inside the builder
   */
  bool is_input_size_set = false;
  bool is_output_neuron_number_set = false;
  bool is_expected_input_range_set = false;
  bool is_weight_table_set = false;
  bool is_weight_initializer_set = false;
  bool is_neuron_array_set = false;
  bool is_allowed_transfer_functions_by_layer_set = false;

  /**
   * The absolute value of the amplitude of one average input datapoint. It supports weight initialization.
   */
  sdouble32 arg_expected_input_range = Transfer_function_info::get_average_output_range(TRANSFER_FUNCTION_IDENTITY);

  /**
   * The array containing the neurons while SparseNetBuilder::build is used
   */
  vector<Neuron> arg_neuron_array;

  /**
   * The array containing the used weights in the network while SparseNetBuilder::build is used
   */
  vector<sdouble32> arg_weight_table;

  /**
   * Weight Initializer argument, which guides the initial net Weights
   */
  shared_ptr<Weight_initializer> arg_weight_initer;

  /**
   * Number of inputs the net-to-be-built shall accept
   */
  uint32 arg_input_size = 0;

  /**
   * Number of Neurons the net-to-be-built shall have as output
   */
  uint32 arg_output_neuron_number = 0;

  vector<vector<transfer_functions> > arg_allowed_transfer_functions_by_layer;


  /**
   * Points to the Arena which may be optionally given to the builder for 
   * a more effective net allocation.
   */
  google::protobuf::Arena* arg_arena = nullptr;

  /**
   * @brief SparseNetBuilder::set_neuron_array: moves the neuron_array argument into the SparseNet
   * @param arr: the neuron array to be added to the @SparseNet object net
   * @param net: the new owner of the neuron_array
   */
  void set_neuron_array(SparseNet* net);

  /**
   * @brief SparseNetBuilder::set_weight_table: moves the weightTable argument into the SparseNet
   * @param table: the array of floating point numbers to be added to the @SparseNet object net
   * @param net: the new owner of the weightTable
   */
  void set_weight_table(SparseNet* net);

};

} /* namespace sparse_net_library */
#endif // SparseNetBUILDER_H