#include "aiproc.hpp"

#include <sstream>

static const auto THOUSAND = 1000;
static const auto MILLION = THOUSAND*THOUSAND;
static const auto MAX_CONS_CELLS = 10*MILLION;

static const auto PROGRAM_TIME = 3072 * THOUSAND;

namespace insn {
  using namespace aiproc;
  Instruction ldc(int32_t val) {
    return [val](State* state) {
      state->stack.push_back(val);
    };
  }

  Instruction ldf(counter addr) {
    return [addr](State* state) {
      auto cur_env = state->environment.back();
      Closure c;
      c.environ = Environment::create(std::vector<Value>(), cur_env);
      c.address = addr;
      state->stack.push_back(c);
    };
  }

  Instruction cons() {
    return [](State* state) {
      auto val1 = state->stack.back();
      state->stack.pop_back();
      auto val0 = state->stack.back();
      state->stack.pop_back();

      state->stack.push_back(Pair::create(state, val0, val1));
    };
  }

  Instruction rtn() {
    return [](State* state) {
      state->program = state->control.back();
      state->control.pop_back();
      state->environment.pop_back();
    };
  }

  Instruction parse(std::string line) {
    using namespace std;
    stringstream liness(line);
    vector<string> tokens;
    copy(istream_iterator<string>(liness),
	 istream_iterator<string>(),
	 back_inserter<vector<string>>(tokens));

    string opcode = tokens[0];
    int arg0, arg1;
    if(tokens.size() > 1)
      arg0 = atoi(tokens[1].c_str());
    if(tokens.size() > 2)
      arg1 = atoi(tokens[2].c_str());

    std::transform(opcode.begin(), opcode.end(), opcode.begin(), ::tolower);
    if(opcode == "ldc")
      return ldc(arg0);
    else if(opcode == "ldf")
      return ldf(arg0);
    else if(opcode == "cons")
      return cons();
    else if(opcode == "rtn")
      return rtn();
    else
      throw std::runtime_error("Unknown opcode: " + opcode);
  }
}

namespace aiproc {
  Pair::ptr Pair::create(State* state, Value car, Value cdr) {
    return ptr(new Pair(state, car, cdr));
  }

  Pair::Pair(State* state, Value car, Value cdr) : state(state), car(car), cdr(cdr) {
    state->cell_count++;
    if(state->cell_count > MAX_CONS_CELLS)
      throw std::runtime_error("WOAH. DUDE. Cool your jets.");
  }

  Pair::~Pair() {
    state->cell_count--;
  }

  Environment::ptr Environment::create(std::vector<Value> values, ptr parent) {
    return ptr(new Environment(values, parent));
  }

  Environment::Environment(std::vector<Value> values, ptr parent)
    : values(values), parent(parent) {}

  Pair::ptr State::run(Closure start) {
    program = start.address;

    // We need to track the number of instructions so we can fail
    // the execution if it takes too long.
    // The rule is "1 minute if it's main, else 1 second), and main
    // is defined to be at address 0. So if we're calling address 0
    // we use 60 seconds, else we just use 1.
    auto max_insns = program ? PROGRAM_TIME : 60 * PROGRAM_TIME;
    decltype(max_insns) executed_insns = 0;

    control.push_back(program);
    environment.push_back(start.environ);

    for(;;) {
      auto pc = program;
      code[pc](this);
      if(pc == program)
	program++;

      // If control is empty, we've returned from our entry point.
      // It's time to stop execution.
      if(control.empty())
	break;

      // We're on a clock.
      if(++executed_insns == max_insns)
	throw std::runtime_error("Program took too long to execute!");
    };

    // Return value is whatever is on top of the stack.
    return boost::get<Pair::ptr>(stack.back());
  };

  State compile_program(std::string prog) {
    State s;
    using namespace std;
    stringstream iss(prog);
    vector<string> lines;
    while(!iss.eof()) {
      string line;
      getline(iss, line);
      if(line.empty())
	continue;
      s.code.push_back(insn::parse(line));
    }

    return s;
  }
}
