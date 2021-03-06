#include "test/catch.hpp"

#include <vector>
#include <memory>

#include "test/test_mockups.h"

#include "gen/solution.pb.h"
#include "gen/sparse_net.pb.h"
#include "models/transfer_function.h"
#include "models/service_context.h"
#include "services/solution_solver.h"
#include "services/partial_solution_solver.h"
#include "services/synapse_iterator.h"
#include "services/sparse_net_builder.h"
#include "services/solution_builder.h"

namespace sparse_net_library_test{

using std::reference_wrapper;

using sparse_net_library::uint8;
using sparse_net_library::uint16;
using sparse_net_library::uint32;
using sparse_net_library::sdouble32;
using sparse_net_library::Partial_solution_solver;
using sparse_net_library::Partial_solution;
using sparse_net_library::Solution_solver;
using sparse_net_library::Solution;
using sparse_net_library::Synapse_iterator;
using sparse_net_library::Synapse_interval;
using sparse_net_library::Transfer_function;
using sparse_net_library::COST_FUNCTION_QUADRATIC;
using sparse_net_library::Service_context;

/*###############################################################################################
 * Testing if the solution solver produces a correct output, given a manually constructed
 * @Solution.
 * - 2 rows and two columns shall be constructed.
 * - @Partial_solution [0][0]: takes the whole of the input
 * - @Partial_solution [0][1]: takes half of the input
 * - @Partial_solution [1][0]: takes the whole of the previous row
 * - @Partial_solution [1][1]: takes half from each previous @Partial_solution
 */
void test_solution_solver_multithread(uint16 threads){

  using std::copy;

  /* Define the input, @Solution and partial solution table */
  Service_context context = Service_context().set_max_solve_threads(threads);
  Solution solution;
  solution.set_neuron_number(8);
  solution.set_output_neuron_number(4);
  solution.add_cols(2); /* Every row shall have 2 columns */
  solution.add_cols(2);
  *solution.add_partial_solutions() = Partial_solution();
  *solution.add_partial_solutions() = Partial_solution();
  *solution.add_partial_solutions() = Partial_solution();
  *solution.add_partial_solutions() = Partial_solution();

  vector<vector<reference_wrapper<Partial_solution>>> partial_solutions= {
    {*solution.mutable_partial_solutions(0),*solution.mutable_partial_solutions(1)},
    {*solution.mutable_partial_solutions(2),*solution.mutable_partial_solutions(3)}
  };

  vector<sdouble32> network_inputs = {5.1,10.3,3.2,9.4};
  Synapse_interval temp_synapse_interval;

  /* [0][0]: Whole of the input */
  manual_2_neuron_partial_solution(partial_solutions[0][0], network_inputs.size(),0);
  temp_synapse_interval.set_starts(Synapse_iterator::synapse_index_from_input_index(0));
  temp_synapse_interval.set_interval_size(network_inputs.size());
  *partial_solutions[0][0].get().add_input_data() = temp_synapse_interval;
  Partial_solution_solver partial_solution_solver_0_0 = Partial_solution_solver(partial_solutions[0][0]);

  /* [0][1]: Half of the input */
  manual_2_neuron_partial_solution(partial_solutions[0][1], network_inputs.size()/2,2);
  temp_synapse_interval.set_starts(Synapse_iterator::synapse_index_from_input_index(network_inputs.size()/2));
  temp_synapse_interval.set_interval_size(network_inputs.size()/2);
  *partial_solutions[0][1].get().add_input_data() = temp_synapse_interval;
  Partial_solution_solver partial_solution_solver_0_1 = Partial_solution_solver(partial_solutions[0][1]);

  /* [1][0]: Whole of the previous row's data --> neuron [0] to [3] */
  manual_2_neuron_partial_solution(partial_solutions[1][0],4,4);
  temp_synapse_interval.set_starts(0);
  temp_synapse_interval.set_interval_size(4);
  *partial_solutions[1][0].get().add_input_data() = temp_synapse_interval;
  Partial_solution_solver partial_solution_solver_1_0 = Partial_solution_solver(partial_solutions[1][0]);

  /* [1][1]: Half of the previous row's data ( in the middle) --> neuron [1] to [2] */
  manual_2_neuron_partial_solution(partial_solutions[1][1],2,6);
  temp_synapse_interval.set_starts(1);
  temp_synapse_interval.set_interval_size(2);
  *partial_solutions[1][1].get().add_input_data() = temp_synapse_interval;
  Partial_solution_solver partial_solution_solver_1_1 = Partial_solution_solver(partial_solutions[1][1]);

  vector<sdouble32> neuron_data = vector<sdouble32>(solution.neuron_number());
  vector<sdouble32> expected_neuron_data = vector<sdouble32>(solution.neuron_number());
  vector<sdouble32> collected_output(2);
  vector<sdouble32> network_output;

  /* Solve the compiled Solution */
  Solution_solver solution_solver(solution,context);
  srand (time(nullptr));
  for(uint8 variant_iterator = 0; variant_iterator < 100; variant_iterator++){
    if(0 < variant_iterator){ /* modify some weights and stuff */
      for(int i = 0; i < partial_solutions[0][0].get().weight_table_size(); ++i){
        partial_solutions[0][0].get().set_weight_table(i,static_cast<sdouble32>(rand()%11) / 10.0);
      } /* Modify weights */
      for(int i = 0; i < partial_solutions[0][1].get().weight_table_size(); ++i){
        partial_solutions[0][1].get().set_weight_table(i,static_cast<sdouble32>(rand()%11) / 10.0);
      } /* Modify weights */
      for(int i = 0; i < partial_solutions[1][0].get().weight_table_size(); ++i){
        partial_solutions[1][0].get().set_weight_table(i,static_cast<sdouble32>(rand()%11) / 10.0);
      } /* Modify weights */
      for(int i = 0; i < partial_solutions[1][1].get().weight_table_size(); ++i){
        partial_solutions[1][1].get().set_weight_table(i,static_cast<sdouble32>(rand()%11) / 10.0);
      } /* Modify weights */

      /* Modify Biases, memory filters and transfer functions */
      partial_solutions[0][0].get().set_weight_table(partial_solutions[0][0].get().bias_index(0),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[0][0].get().set_weight_table(partial_solutions[0][0].get().bias_index(1),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[0][0].get().set_weight_table(partial_solutions[0][0].get().memory_filter_index(0),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[0][0].get().set_weight_table(partial_solutions[0][0].get().memory_filter_index(1),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[0][0].get().set_neuron_transfer_functions(rand()%(partial_solutions[0][0].get().neuron_transfer_functions_size()),Transfer_function::next());

      partial_solutions[0][1].get().set_weight_table(partial_solutions[0][1].get().bias_index(0),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[0][1].get().set_weight_table(partial_solutions[0][1].get().bias_index(1),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[0][1].get().set_weight_table(partial_solutions[0][1].get().memory_filter_index(0),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[0][1].get().set_weight_table(partial_solutions[0][1].get().memory_filter_index(1),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[0][1].get().set_neuron_transfer_functions(rand()%(partial_solutions[0][1].get().neuron_transfer_functions_size()),Transfer_function::next());

      partial_solutions[1][0].get().set_weight_table(partial_solutions[1][0].get().bias_index(0),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[1][0].get().set_weight_table(partial_solutions[1][0].get().bias_index(1),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[1][0].get().set_weight_table(partial_solutions[1][0].get().memory_filter_index(0),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[1][0].get().set_weight_table(partial_solutions[1][0].get().memory_filter_index(1),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[1][0].get().set_neuron_transfer_functions(rand()%(partial_solutions[1][0].get().neuron_transfer_functions_size()),Transfer_function::next());

      partial_solutions[1][1].get().set_weight_table(partial_solutions[1][1].get().bias_index(0),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[1][1].get().set_weight_table(partial_solutions[1][1].get().bias_index(1),static_cast<sdouble32>(rand()%110) / 10.0);
      partial_solutions[1][1].get().set_weight_table(partial_solutions[1][1].get().memory_filter_index(0),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[1][1].get().set_weight_table(partial_solutions[1][1].get().memory_filter_index(1),static_cast<sdouble32>(rand()%11) / 10.0);
      partial_solutions[1][1].get().set_neuron_transfer_functions(rand()%(partial_solutions[1][1].get().neuron_transfer_functions_size()),Transfer_function::next());
    }
    /* Calculate the expected output */
    manual_2_neuron_result(
      network_inputs,expected_neuron_data,partial_solutions[0][0],0
    ); /* row 0, column 0 */
    manual_2_neuron_result(
      {network_inputs.begin()+2,network_inputs.end()},expected_neuron_data,partial_solutions[0][1],2
    ); /* row 0, column 1 */
    manual_2_neuron_result(
      {expected_neuron_data.begin(),expected_neuron_data.begin() + 4},expected_neuron_data,partial_solutions[1][0],4
    ); /* row 1, column 0 */
    manual_2_neuron_result(
      {expected_neuron_data.begin() + 1,expected_neuron_data.begin() + 3},expected_neuron_data,partial_solutions[1][1],6
    ); /* row 1, column 1 */

    /* Solve the net */
    /* row 0, column 0 */
    partial_solution_solver_0_0.collect_input_data(network_inputs,neuron_data);
    collected_output = partial_solution_solver_0_0.solve();
    REQUIRE( 2 == collected_output.size() );
    copy(collected_output.begin(),collected_output.end(),neuron_data.begin() + 0u);

    /* row 0, column 1 */
    partial_solution_solver_0_1.collect_input_data(network_inputs,neuron_data);
    collected_output = partial_solution_solver_0_1.solve();
    REQUIRE( 2 == collected_output.size() );
    copy(collected_output.begin(),collected_output.end(),neuron_data.begin() + 2u);

    /* row 1, column 0 */
    partial_solution_solver_1_0.collect_input_data(network_inputs,neuron_data);
    collected_output = partial_solution_solver_1_0.solve();
    REQUIRE( 2 == collected_output.size() );
    copy(collected_output.begin(),collected_output.end(),neuron_data.begin() + 4u);

    /* row 1, column 1 */
    partial_solution_solver_1_1.collect_input_data(network_inputs,neuron_data);
    collected_output = partial_solution_solver_1_1.solve();
    REQUIRE( 2 == collected_output.size() );
    copy(collected_output.begin(),collected_output.end(),neuron_data.begin() + 6u);

    network_output = solution_solver.solve(network_inputs);
    REQUIRE( network_output.size() == solution.output_neuron_number() );
    for(uint32 i = 0; i < network_output.size(); ++i){
      CHECK(
        Approx(neuron_data[solution.neuron_number() - solution.output_neuron_number() + i]).epsilon(0.00000000000001)
        == expected_neuron_data[solution.neuron_number() - solution.output_neuron_number() + i]
      );
      CHECK(
        Approx(network_output[i]).epsilon(0.00000000000001)
        == expected_neuron_data[solution.neuron_number() - solution.output_neuron_number() + i]
      );
    }
  }
}

TEST_CASE("Solution solver manual testing","[solve][small][manual-solve]"){
  test_solution_solver_multithread(1);
  test_solution_solver_multithread(2);
  test_solution_solver_multithread(10);
}

/*###############################################################################################
 * Testing if the solution solver produces a correct output, given a built @SparseNet
 */
void testing_solution_solver_manually(google::protobuf::Arena* arena){
  using std::unique_ptr;
  using std::make_unique;
  using sparse_net_library::Sparse_net_builder;
  using sparse_net_library::Solution_builder;
  using sparse_net_library::SparseNet;

  vector<uint32> net_structure = {2,4,3,10,20};
  vector<sdouble32> net_input = {10.0,20.0,30.0,40.0,50.0};

  /* Build the described net */
  unique_ptr<Sparse_net_builder> net_builder = make_unique<Sparse_net_builder>();
  net_builder->input_size(5).expected_input_range(5.0)
  .cost_function(COST_FUNCTION_QUADRATIC).arena_ptr(arena);
  SparseNet* net(net_builder->dense_layers(net_structure));
  net_builder.reset();

  /* Generate solution from Net */
  unique_ptr<Solution_builder> solution_builder = make_unique<Solution_builder>();
  Solution* solution = solution_builder->max_solve_threads(4).device_max_megabytes(2048).arena_ptr(arena).build(*net);
  Solution_solver solver(*solution);
  vector<sdouble32> result = solver.solve(net_input);
  vector<sdouble32> expected_neuron_data = vector<sdouble32>(net->neuron_array_size());
  manaual_fully_connected_network_result(net_input, expected_neuron_data, net_structure,*net);
  vector<sdouble32> expected_result = {expected_neuron_data.end() - net->output_neuron_number(),expected_neuron_data.end()};
  /* Verify if the calculated values match the expected ones */
  REQUIRE( net_structure.back() == result.size() );
  REQUIRE( expected_result.size() == result.size() );
  for(uint32 result_iterator = 0; result_iterator < expected_result.size(); ++result_iterator){
    CHECK( Approx(result[result_iterator]).epsilon(0.00000000000001) == expected_result[result_iterator]);
  }

  /* Re-veriy with guaranted multiple partial solutions */
  sdouble32 solution_size = solution->SpaceUsedLong() /* Bytes *// 1024.0 /* KB *// 1024.0 /* MB */;
  if(nullptr == arena) delete solution;
  solution = solution_builder->max_solve_threads(4).device_max_megabytes(solution_size/4.0).arena_ptr(arena).build(*net);
  Solution_solver solver2 = Solution_solver(*solution);
  result = solver2.solve(net_input);

  /* Verify once more if the calculated values match the expected ones */
  for(uint32 result_iterator = 0; result_iterator < expected_result.size(); ++result_iterator){
    CHECK( Approx(result[result_iterator]).epsilon(0.00000000000001) == expected_result[result_iterator]);
  }
}

TEST_CASE("Solution Solver test based on Fully Connected Dense Net", "[solve][build-solve]"){
  testing_solution_solver_manually(nullptr);
}

} /* namespace sparse_net_library_test */
