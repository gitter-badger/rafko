#include "test/catch.hpp"

#include "models/gen/sparse_net.pb.h"
#include "services/sparse_net_builder.h"

#include "models/gen/solution.pb.h"
#include "services/solution_builder.h"

#include "services/neuron_router.h"

namespace sparse_net_library_test {

using std::unique_ptr;
using std::make_unique;
using std::deque;
using std::vector;

using sparse_net_library::uint16;
using sparse_net_library::uint32;
using sparse_net_library::SparseNetBuilder;
using sparse_net_library::SparseNet;
using sparse_net_library::Neuron_router;

/*###############################################################################################
 * Testing if the iteration is correctly processing the Sparse net
 * - Building a Fully COnnected Multilayered Net
 * - Each iteration has to add the corresponding layer
 *    Because of the structure of a fully connected Net, one iteration would involve one layer exactly
 * */
TEST_CASE( "Testing Neural Network Iteration Routing", "[neuron_iteration][small]" ){
  /* Build a net */
  vector<uint32> layer_structure = {2,3,3,5};
  unique_ptr<SparseNetBuilder> net_builder = make_unique<SparseNetBuilder>();
  net_builder->input_size(5).output_neuron_number(5).expectedInputRange(5.0);
  SparseNet* net(net_builder->denseLayers(layer_structure));
  net_builder.reset();

  /* Testing the collected subset in each iteration in the net */
  uint16 iteration = 1; /* Has to start with 1, otherwise values mix with neuron processed value */
  Neuron_router net_iterator = Neuron_router(*net);

  uint32 layer_start = 0;
  uint32 tmp_index;
  bool last_run = false;
  CHECK( false == net_iterator.finished() );
  while(!net_iterator.finished()){ /* Until the whole output layer is processed */
    net_iterator.collect_subset(iteration,1,500.0);
    /* For a fully connected Dense Layer, each iteration subset should be the actual layer */
    vector<uint32> subset;
    while(net_iterator.get_first_neuron_index_from_subset(tmp_index)){
      subset.push_back(tmp_index);
      net_iterator.confirm_first_subset_element_processed(tmp_index);
    }
    REQUIRE(( 
      (iteration <= layer_structure.size()) /* Has to finish sooner, than there are layers */
      ||((0 == subset.size())&&(!last_run)) /* With the exception of the last iteration */
    )); /* ..where only the output_layer_iterator  is updated to the end */
    /*!Note: Iteration starts from 1! so equality is needed here */
    if(0 < subset.size()){
      for(uint32 i = 0; i < layer_structure[iteration-1]; ++i){ /* Find all indexes inside the layer in the current subset */
        CHECK( std::find(subset.begin(), subset.end(), layer_start + i) != subset.end() );
      }
    }else{
      last_run = true; 
    }
    layer_start += layer_structure[iteration-1];

    subset.clear();

     ++iteration;
  }

}

} /* namespace sparse_net_library_test */
