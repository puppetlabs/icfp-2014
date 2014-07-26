#include "aiproc.hpp"

using namespace aiproc;

#include <iostream>
#include <fstream>

using namespace std;

int main() {
  std::ifstream stream("../../ai.gcc");
  std::string prog((std::istreambuf_iterator<char>(stream)),
		   std::istreambuf_iterator<char>());
  auto processor = aiproc::compile_program(prog);

  // setup the main entry point.
  Closure main;
  std::vector<Value> main_args;
  main_args.push_back(0); // world_state
  main_args.push_back(0); // UNKNOWN
  main.address = 0;

  // Run the main program to get the initial AI state and
  // our tick function
  auto result = processor.run(main, main_args);
  auto ai_state = result->car;
  auto tick = boost::get<Closure>(result->cdr);
  // MAINLOOP. WOOHOOOOOOOOOO
  for(;;) {
    std::vector<Value> tick_args;
    tick_args.push_back(ai_state);
    tick_args.push_back(0); // world state
    result = processor.run(tick, tick_args);
    tick.environ->values.clear(); // clear function args after call.
    ai_state = result->car;
    auto next_move = result->cdr;

    // UPDATE WORLD (or just show off our expected result)
    cout << boost::get<int32_t>(next_move) << endl;
  }

  return 0;
}
