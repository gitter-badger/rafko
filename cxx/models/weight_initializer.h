#ifndef weight_initializer_H
#define weight_initializer_H


#include "gen/sparse_net.pb.h"

#include "sparse_net_global.h"

namespace sparse_net_library {

  using std::min;
  using std::max;
  using std::numeric_limits;

class Weight_initializer
{
public:
  /**
   * @brief      Constructs the object.
   */
  Weight_initializer() noexcept{};

  /**
   * @brief      Calculate a weight which fits the Neuron the most based on the configuration parameters
   *             The basis of the number is the transfer function given in the function argument
   *
   * @param[in]  used_transfer_function  The used transfer function
   *
   * @return     The Calculated weight
   */
  virtual sdouble32 next_weight_for(transfer_functions used_transfer_function) const = 0;

  /**
   * @brief      Calculate a number which fits the Neuron the most based on the configuration parameters
   *
   * @return     The Calculated Memory ratio
   */
  virtual sdouble32 next_memory_filter() const = 0;

  /**
   * @brief      Calculate a bias which fits the Neuron the most based on the configuration parameters
   *
   * @return     The Calculated Bias value
   */
  virtual sdouble32 next_bias() const = 0;

  /**
   * @brief      Sets the functions expected parameters
   *
   * @param[in]  expected_input_number             The exponent input number
   * @param[in]  expected_input_maximum_value_     The exponent input maximum
   */
  void set(uint32 expected_input_number_, sdouble32 expected_input_maximum_value_){
    expected_input_number = max(1u,expected_input_number_);

    if( /* Primitive check if the given number causes overflow or not */
      (numeric_limits<sdouble32>::max() > (expected_input_number_ * abs(expected_input_maximum_value_)))
    ){
      expected_input_maximum_value = expected_input_maximum_value_;
    }else if(0.0 == expected_input_maximum_value_){
      expected_input_maximum_value = numeric_limits<sdouble32>::epsilon();
    }else{ /* Overflow! Use maximum value */
      expected_input_maximum_value = numeric_limits<sdouble32>::max() / expected_input_number_;
    }
  }

  /**
   * @brief      Calculate a weight which fits the Neuron the most based on the configuration parameters
   *             The basis of the number is the Transfer_function::TRANSFER_FUNCTION_IDENTITY
   *
   * @return     The Calculated Weight value
   */
  sdouble32 next_weight() const{
    return next_weight_for(TRANSFER_FUNCTION_IDENTITY);
  }
protected:

  /**
   * @brief      Limits the given weight into the limits used in the Neural Network
   *
   * @param[in]  weight  The weight
   *
   * @return     Limited value
   */
  sdouble32 limit_weight(sdouble32 weight) const{
    return min(1.0,max(-1.0,weight));
  }

  /**
   * Number of estimated @Neuron inputs expected
   */
  uint32 expected_input_number = 0;

  /**
   * Estimated Maximum value of one @Neuron input
   */
  sdouble32 expected_input_maximum_value = 0.0;
};

} /* namespace sparse_net_library */
#endif /* weight_initializer_H */
