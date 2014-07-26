/*
 * Runner for lambda-man world from assembly files for Ghost and Lambda Man programs.
 */

#include <iostream>
#include "world.hpp"

using namespace std;
using namespace LambdaWorld;
static const char *world_map = "\
#######################\n\
#..........#..........#\n\
#.###.####.#.####.###.#\n\
#o###.####.#.####.###o#\n\
#.....................#\n\
#.###.#.#######.#.###.#\n\
#.....#....#....#.....#\n\
#####.#### # ####.#####\n\
#   #.#    =    #.#   #\n\
#####.# ### ### #.#####\n\
#    .  # === #  .    #\n\
#####.# ####### #.#####\n\
#   #.#    %    #.#   #\n\
#####.# ####### #.#####\n\
#..........#..........#\n\
#.###.####.#.####.###.#\n\
#o..#......\\......#..o#\n\
###.#.#.#######.#.#.###\n\
#.....#....#....#.....#\n\
#.########.#.########.#\n\
#.....................#\n\
#######################\n\
";

static const char *lambda_prog =
  "LDC 0\n"
  "LDF 4\n"
  "CONS\n"
  "RTN\n"
  "LDC 0\n"
  "LDC 1\n"
  "CONS\n"
  "RTN\n";

int main(int argc, char *argv[])
{
    // Read files from command-line. Game board, 
    cout << "Loading files:";
    for(int i = 1; i < argc; ++i) {
        cout << " " << argv[i];
    }
    cout << endl;

    // Instantiate and run world.
    cout << "Instantiating world..." << endl;
    runWorld(world_map, lambda_prog, {""});
    return 0;
}

