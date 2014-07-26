#include "aiproc.hpp"

#include <sstream>
#include <limits>

static const auto THOUSAND = 1000;
static const auto MILLION = THOUSAND*THOUSAND;
static const auto MAX_CONS_CELLS = 10*MILLION;

static const auto PROGRAM_TIME = 3072 * THOUSAND;

namespace insn {
  using namespace aiproc;
  Instruction ldc(int32_t val) {
    return [val](State* state) {
      state->data_stack.push_back(val);
    };
  }

  Instruction ld(int32_t ctx, int32_t id) {
    return [ctx, id](State* state) {
      auto context = ctx;
      auto env = state->environment;
      while(context--) {
	env = env->parent;
	if(!env)
	  throw std::runtime_error("Tried to ld from a non-existent scope!");
      }
      if(id >= env->values.size())
	throw std::runtime_error("Tried to ld from a non-existent value!");
      state->data_stack.push_back(env->values[id]);
    };
  }

  Instruction ldf(counter addr) {
    return [addr](State* state) {
      Closure c;
      c.environ = state->environment;
      c.address = addr;
      state->data_stack.push_back(c);
    };
  }

  Instruction cons() {
    return [](State* state) {
      auto val1 = state->data_stack.back();
      state->data_stack.pop_back();
      auto val0 = state->data_stack.back();
      state->data_stack.pop_back();

      state->data_stack.push_back(Pair::create(state, val0, val1));
    };
  }

  Instruction rtn() {
    return [](State* state) {
      state->program = boost::get<counter>(state->control_stack.back());
      state->control_stack.pop_back();
      state->environment = boost::get<Environment::ptr>(state->control_stack.back());
      state->control_stack.pop_back();
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
    if(opcode == "ld")
      return ld(arg0, arg1);
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

  Pair::ptr State::run(Closure start, std::vector<Value> args) {
    program = start.address;

    // We need to track the number of instructions so we can fail
    // the execution if it takes too long.
    // The rule is "1 minute if it's main, else 1 second), and main
    // is defined to be at address 0. So if we're calling address 0
    // we use 60 seconds, else we just use 1.
    auto max_insns = program ? PROGRAM_TIME : 60 * PROGRAM_TIME;
    decltype(max_insns) executed_insns = 0;

    control_stack.push_back(Environment::ptr());
    control_stack.push_back(std::numeric_limits<counter>::max());
    environment = Environment::create(args, start.environ);

    for(;;) {
      auto pc = program;
      code[pc](this);
      if(pc == program)
	program++;

      // If control is empty, we've returned from our entry point.
      // It's time to stop execution.
      if(control_stack.empty())
	break;

      // We're on a clock.
      if(++executed_insns == max_insns)
	throw std::runtime_error("Program took too long to execute!");
    };

    // Return value is whatever is on top of the stack.
    auto result = boost::get<Pair::ptr>(data_stack.back());
    data_stack.pop_back();
    return result;
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
