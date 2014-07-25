#pragma once

#include <boost/variant.hpp>

namespace aiproc {
  using boost::variant;
  using boost::recursive_wrapper;

  struct Pair;
  struct State;
  struct Closure;

  using Instruction = std::function<void(State*)>;
  using counter = std::vector<Instruction>::size_type;

  using Value = boost::variant<
    int32_t,
    boost::recursive_wrapper<Closure>,
    std::shared_ptr<Pair> >;

  /*  struct Instruction {
    Value arg0;
    Value arg1;

    std::function<void(State*)> exec;
    }; */
  
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
    std::vector<Value> stack;
    std::vector<counter> control;
    std::vector<Instruction> code;
    std::vector<Environment::ptr> environment;
    size_t cell_count=0;
    counter program;

    Pair::ptr run(Closure start);
  };

  State compile_program(std::string);
}
