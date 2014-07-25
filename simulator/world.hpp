#pragma once

#include <boost/heap/fibonacci_heap.hpp>
#include <tuple>

/*
The state of the world is encoded as follows:

A 4-tuple consisting of

1. The map;
2. the status of Lambda-Man;
3. the status of all the ghosts;
4. the status of fruit at the fruit location.

The map is encoded as a list of lists (row-major) representing the 2-d
grid. An enumeration represents the contents of each grid cell:

  * 0: Wall (`#`)
  * 1: Empty (`<space>`)
  * 2: Pill 
  * 3: Power pill
  * 4: Fruit location
  * 5: Lambda-Man starting position
  * 6: Ghost starting position
 */

enum Grid {
    WALL = 0,
    EMPTY = 1,
    PILL = 2,
    POWER_PILL = 3,
    FRUIT = 4,
    LAMBDAMAN = 5,
    GHOST = 6
};

enum Direction {
    UP = 0,
    RIGHT = 1,
    DOWN = 2,
    LEFT = 3
};

using WorldMap = std::vector<std::vector<unsigned char>>;
using Location = std::pair<unsigned int, unsigned int>;
// Lambda-Man status = vitality, current location, current direction, remaining lives, current score
using LambdaManStat = std::tuple<unsigned int, Location, Direction, unsigned int, unsigned int>;
using WorldState = std::tuple<WorldMap, LambdaManStat, int, int>;

struct LambdaWorld {
    boost::heap::fibonacci_heap<int> moves;

    void run();
};

