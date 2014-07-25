#include "aiproc.hpp"

using namespace aiproc;

#include <iostream>

using namespace std;

const char* prog = 
  "LDC 0\n"
  "LDF 4\n"
  "CONS\n"
  "RTN\n"
  "LDC 0\n"
  "LDC 1\n"
  "CONS\n"
  "RTN\n";

int main() {
  auto processor = aiproc::compile_program(prog);

  // setup the main entry point.
  Closure main;
  std::vector<Value> main_args;
  main_args.push_back(0); // world_state
  main_args.push_back(0); // UNKNOWN
  main.address = 0;
  main.environ = Environment::create(main_args, nullptr);

  // Run the main program to get the initial AI state and
  // our tick function
  auto result = processor.run(main);
  auto ai_state = result->car;
  auto tick = boost::get<Closure>(result->cdr);
  // MAINLOOP. WOOHOOOOOOOOOO
  for(;;) {
    tick.environ->values.push_back(ai_state);
    tick.environ->values.push_back(0); // world state
    result = processor.run(tick);
    tick.environ->values.clear(); // clear function args after call.
    ai_state = result->car;
    auto next_move = result->cdr;

    // UPDATE WORLD (or just show off our expected result)
    cout << boost::get<int32_t>(next_move) << endl;
  }

  return 0;
}
