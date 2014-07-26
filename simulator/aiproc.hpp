#pragma once

#include <vector>
#include <functional>
#include <boost/variant.hpp>

namespace aiproc {
  using boost::variant;
  using boost::recursive_wrapper;

  struct Pair;
  struct Environment;
  struct State;
  struct Closure;

  using Instruction = std::function<void(State*)>;
  using counter = std::vector<Instruction>::size_type;

  using Value = boost::variant<
    int32_t,
    boost::recursive_wrapper<Closure>,
    std::shared_ptr<Pair>,
    std::shared_ptr<Environment>,
    Instruction,
    counter >;

  struct Pair {
    using ptr = std::shared_ptr<Pair>;
    Value car;
    Value cdr;

    static ptr create(State* state, Value car, Value cdr);

    ~Pair();

  private:
    Pair(State* state, Value car, Value cdr);

    State* state;
  };

  struct Environment {
    using ptr = std::shared_ptr<Environment>;
    std::vector<Value> values;
    ptr parent;

    static ptr create(std::vector<Value> values, ptr parent);
    
  protected:
    Environment(std::vector<Value> values, ptr parent);
  };

  struct Closure {
    Environment::ptr environ;
    counter address;
  };

  struct State {
    // Stacks and registers, as defined in the spec
    std::vector<Instruction> code;
    std::vector<Value> data_stack;
    std::vector<Value> control_stack;
    counter program;
    Environment::ptr environment;

    // The number of cons cells in this machine.
    // Only 10 million are allowed.
    size_t cell_count=0;    

    Pair::ptr run(Closure start, std::vector<Value> args);
  };

  State compile_program(std::string);
}
