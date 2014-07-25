#pragma once

#include <vector>
#include <functional>
#include <boost/variant.hpp>

namespace aiproc {
  using boost::variant;
  using boost::recursive_wrapper;

  struct Pair;
  struct Instruction;
  struct State;


  using counter = std::vector<Instruction>::size_type;

  using Value = boost::variant<
    int32_t,
    counter,
    boost::recursive_wrapper<Pair>>;

  struct Instruction {
    Value arg0;
    Value arg1;

    std::function<void(State*)> apply;
  };
  
  struct Pair {
    using ptr = std::shared_ptr<Pair>;
    Value car;
    Value cdr;

    std::shared_ptr<Pair> create(Value car, Value cdr);
    
  };

  struct Environment {
    using ptr = std::shared_ptr<Environment>;
    std::vector<Value> values;
    ptr parent;
  };

  struct State {
    std::vector<Value> stack;
    std::vector<counter> control;
    std::vector<Instruction> code;
    std::vector<Environment::ptr> environment;

    counter program;

    Value run(std::vector<Value> values, counter start);
  };
}
