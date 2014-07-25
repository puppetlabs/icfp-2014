#pragma once

#include <tuple>
#include <vector>

namespace LambdaWorld {
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
  * 2: Pill (`.`)
  * 3: Power pill (`o`)
  * 4: Fruit location (`%`)
  * 5: Lambda-Man starting position (`\`)
  * 6: Ghost starting position (`=`)
 */

enum GridCell {
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

enum GhostVit {
    STANDARD = 0,
    FRIGHT = 1,
    INVISIBLE = 2
};

// World map = a list of lists (row-major) representing the grid
/*
Note that the map does reflect the current status of all the pills and
power pills. The map does not however reflect the current location of
Lambda-Man or the ghosts, nor the presence of fruit. These items are
represented separately from the map.
 */
using WorldMap = std::vector<std::vector<GridCell>>;

// Location = (x, y) pair
using Location = std::pair<unsigned int, unsigned int>;
// 
/*
Lambda-Man status = vitality, current location, current direction, remaining lives, current score

Lambda-Man's vitality is a number which is a countdown to the expiry of
the active power pill, if any. It is 0 when no power pill is active.
  * 0: standard mode;
  * n > 0: power pill mode: the number of game ticks remaining while the
           power pill will will be active
 */
using LambdaManStat = std::tuple<unsigned int, Location, Direction, unsigned int, size_t>;

/*
The status of all the ghosts is a list with the status for each ghost.
The list is in the order of the ghost number, so each ghost always appears
in the same location in the list.

The status for each ghost is a 3-tuple consisting of
  1. the ghost's vitality
  2. the ghost's current location, as an (x,y) pair
  3. the ghost's current direction
 */
using GhostStat = std::tuple<GhostVit, Location, Direction>;

/*
The status of the fruit is a number which is a countdown to the expiry of
the current fruit, if any.
  * 0: no fruit present;
  * n > 0: fruit present: the number of game ticks remaining while the
           fruit will will be present.
 */
using WorldState = std::tuple<WorldMap, LambdaManStat, std::vector<GhostStat>, unsigned int>;

void runWorld(std::string world_map, std::string lambda_script, std::vector<std::string> ghost_scripts);
}

