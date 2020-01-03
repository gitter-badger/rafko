#ifndef sparse_net_global_H
#define sparse_net_global_H

namespace sparse_net_library {

/***
 * Future Features:
 *  - Add Output Normalizer to Neuron (?)
 *  - Store vanishing gradient / exploding gradient statistics based on indexes
 *  - Look out for https://stackoverflow.com/questions/26960906/protobuf-repeated-fields-deserialization
 *  - Implement uint64 usage to support giant nets
 * TODOs:
 *  - #1 Include License information for every File
 *  - #2 in SparseNetBuilder::neuron_array validate the whole of the array if possible
 *  - #3 Set Arena options for memory usage
 *  - #4 Remove actual_index from @Partial solution, substitue with @output_data synapse
 *  - #5 Implement weight sharing in @Partial_solution_builder
 *  - #6 
 *  - #7 use Exception Class with String formatter
 *  - #8 Add Partial Solution tests to more complex structures as well
 *  - #9 
 *  - #10 
 *  - #11 
 *  - #12 test transfer function info
 *  - #13 use the protobuffer version verifier
 *  - #14 make make paralell by -j
 *  - #15 
 */
/**
 * GLOBAL TYPES
 */
typedef unsigned long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef signed long sint64;
typedef signed int sint32;
typedef signed short sint16;
typedef signed char sint8;
typedef double sdouble32;
typedef uint16* p_uint16;
typedef sdouble32* p_sdouble32;

} /* namespace sparse_net_library */
#endif /* defined sparse_net_global_H */