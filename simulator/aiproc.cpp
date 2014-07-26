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

  Instruction add() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 + val2;
    };
  }

  Instruction sub() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 - val2;
    };
  }

  Instruction mul() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 * val2;
    };
  }

  Instruction div() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 / val2;
    };
  }

  Instruction ceq() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 == val2 ? 1 : 0;
    };
  }

  Instruction cgt() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 > val2 ? 1 : 0;
    };
  }

  Instruction cgte() {
    return [](State* state) {
      auto val2 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      auto val1 = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.back() = val1 >= val2 ? 1 : 0;
    };
  }

  Instruction atom() {
    return [](State* state) {
      auto val = state->data_stack.back();
      if(val.type() == typeid(int32_t))
	state->data_stack.back() = 1;
      else
	state->data_stack.back() = 0;
    };
  }

  Instruction cons() {
    return [](State* state) {
      auto val1 = state->data_stack.back();
      state->data_stack.pop_back();
      auto val0 = state->data_stack.back();
      state->data_stack.back() = Pair::create(state, val0, val1);
    };
  }

  Instruction car() {
    return [](State* state) {
      auto val = boost::get<Pair::ptr>(state->data_stack.back());
      state->data_stack.back() = val->car;
    };
  }

  Instruction cdr() {
    return [](State* state) {
      auto val = boost::get<Pair::ptr>(state->data_stack.back());
      state->data_stack.back() = val->cdr;
    };
  }

  Instruction sel(counter t, counter f) {
    return [t, f](State* state) {
      auto cond = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      state->control_stack.push_back(state->program+1);
      state->program = cond ? t : f;
    };
  }

  Instruction join() {
    return [](State* state) {
      auto addr = boost::get<counter>(state->control_stack.back());
      state->control_stack.pop_back();
      state->program = addr;
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

  Instruction ap(int32_t arg_count) {
    return [arg_count](State* state) {
      auto fn = boost::get<Closure>(state->data_stack.back());
      state->data_stack.pop_back();
      std::vector<Value> args(arg_count);
      for(int32_t i = arg_count; i ; i--) {
	args[i-1] = state->data_stack.back();
	state->data_stack.pop_back();
      }
      auto env = Environment::create(args, fn.environ);
      state->control_stack.push_back(state->environment);
      state->control_stack.push_back(state->program+1);
      state->environment = env;
      state->program = fn.address;
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

  Instruction dum(int32_t size) {
    return [size](State* state) {
      auto env = Environment::create(std::vector<Value>(), state->environment);
      env->values.resize(size);
      state->environment = env;
    };
  }

  Instruction rap(int32_t arg_count) {
    return [arg_count](State* state) {
      auto fn = boost::get<Closure>(state->data_stack.back());
      state->data_stack.pop_back();
      if(fn.environ != state->environment)
	throw std::runtime_error("Frame Mismatch");
      if(arg_count != state->environment->values.size())
	throw std::runtime_error("Frame Mismatch");
      for(int32_t i = arg_count; i; i--) {
	state->environment->values[i-1] = state->data_stack.back();
	state->data_stack.pop_back();
      }
      state->control_stack.push_back(state->environment->parent);
      state->control_stack.push_back(state->program+1);
      state->program = fn.address;
    };
  }

  Instruction tsel(int32_t t, int32_t f) {
    return [t, f](State* state) {
      auto val = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      state->program = val ? t : f;
    };
  }

  Instruction tap(int32_t arg_count) {
    return [arg_count](State* state) {
      auto fn = boost::get<Closure>(state->data_stack.back());
      state->data_stack.pop_back();
      std::vector<Value> args(arg_count);
      for(int32_t i = arg_count; i; i--) {
	args[i-1] = state->data_stack.back();
	state->data_stack.pop_back();
      }
      auto env = state->environment;
      bool need_env = false;
      while(env) {
	if(env == fn.environ) {
	  need_env = true;
	  break;
	}
	env = env->parent;
      }
      if(need_env) {
	state->environment = Environment::create(args, fn.environ);
      } else {
	state->environment->values = args;
      }
    };
  }

  Instruction trap(int32_t arg_count) {
    return [arg_count](State* state) {
      auto fn = boost::get<Closure>(state->data_stack.back());
      state->data_stack.pop_back();
      if(fn.environ != state->environment)
	throw std::runtime_error("Frame Mismatch");
      if(arg_count != state->environment->values.size())
	throw std::runtime_error("Frame Mismatch");
      for(int32_t i = arg_count; i; i--) {
	state->environment->values[i-1] = state->data_stack.back();
	state->data_stack.pop_back();
      }
      state->program = fn.address;
    };
  }

  Instruction st(int32_t ctx, int32_t id) {
    return [ctx, id](State* state) {
      auto context = ctx;
      auto env = state->environment;
      while(context--) {
	env = env->parent;
	if(!env)
	  throw std::runtime_error("Tried to st to a non-existent scope!");
      }
      if(id >= env->values.size())
	throw std::runtime_error("Tried to st to a non-existent value!");
      env->values[id] = state->data_stack.back();
      state->data_stack.pop_back();
    };
  }

  Instruction debug() {
    return [](State* state) {
      auto val = boost::get<int32_t>(state->data_stack.back());
      state->data_stack.pop_back();
      std::cout << val << std::endl;
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


#define OPCODE0(op) if(opcode == #op) return op(); else
#define OPCODE1(op) if(opcode == #op) return op(arg0); else
#define OPCODE2(op) if(opcode == #op) return op(arg0, arg1); else

    OPCODE1(ldc)
    OPCODE2(ld)
    OPCODE0(add)
    OPCODE0(sub)
    OPCODE0(mul)
    OPCODE0(div)
    OPCODE0(ceq)
    OPCODE0(cgt)
    OPCODE0(cgte)
    OPCODE0(atom)
    OPCODE0(cons)
    OPCODE0(car)
    OPCODE0(cdr)
    OPCODE2(sel)
    OPCODE0(join)
    OPCODE1(ldf)
    OPCODE1(ap)
    OPCODE0(rtn)
    OPCODE1(dum)
    OPCODE1(rap)
    OPCODE2(tsel)
    OPCODE1(tap)
    OPCODE1(trap)
    OPCODE2(st)
    OPCODE0(debug)
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
