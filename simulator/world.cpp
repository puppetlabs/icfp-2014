/*
 * Implement the lambda-man world mechanics: http://icfpcontest.org/specification.html#the-lambda-man-game-rules
 */
#include "world.hpp"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;

static string print_world(WorldState &world)
{
    string grid;
    vector<char> gridCellPrinter = {'#', ' ', '.', 'o', '%', '\\', '='};
    for (auto &row : get<0>(world)) {
        for (auto &cell : row) {
            grid += gridCellPrinter[(size_t)cell];
        }
        grid += '\n';
    }
    trim(grid);
    return grid;
}

static WorldState process(string world_map)
{
    WorldState world;
    map<char, GridCell> gridCellLookup = {{'#', WALL}, {' ', EMPTY}, {'.', PILL}, {'o', POWER_PILL}, {'%', FRUIT}, {'\\', LAMBDAMAN}, {'=', GHOST}};

    cout << "Processing map " << endl << world_map << endl;

    // Split map by lines, then into individual characters and transform them to enums.
    vector<string> wm;
    trim(world_map);
    split_regex(wm, world_map, boost::regex("[\n\r]"));
    transform(wm.begin(), wm.end(), back_inserter(get<0>(world)),
        [&](string s) {
            vector<GridCell> line;
            transform(s.begin(), s.end(), back_inserter(line),
                [&](char c) {
                    return gridCellLookup[c];
                });
            return line;
        });

    assert(print_world(world) == world_map);

    return world;
}

// Input: a world map, Lambda-Man AI script, N Ghost AI scripts

void LambdaWorld::run(string world_map, string lambda_script, vector<string> ghost_scripts)
{
    WorldState world = process(world_map);

    size_t mapWidth = get<0>(world)[0].size();
    size_t mapHeight = get<0>(world).size();
    size_t score = 0, EndOfLives = 127 * mapWidth * mapHeight * 16;
    unsigned int lives = 3;
    bool fright_mode = false;

    // Initialize Lambda-Man and ghost AI processors.

    for (size_t tick = 0; tick < EndOfLives; ++tick) {
        // Run Lambda-Man and ghost processors.
        //  Pass in current WorldState and AI state, receive move (a Direction) and new AI state
    
        // Evaluate all Lambda-Man and ghost moves for the current tick.

        // Actions (fright mode deactivating, fruit appearing/disappearing)

        // Check if Lambda-Man is occupying the same square as pills, power pills, or fruit
        //  If pill, pill eaten and removed from game
        //  If power pill, power pill eaten and removed from game, fright mode activated
        //  If fruit, fruit eaten and removed from game

        // If ghost and Lambda-man occupy square
        //  If fright_mode, Lambda-Man eats ghost; move ghost
        //  Else, Lambda-Man loses life; move Lambda-Man
        --lives;

        // If all ordinary pills eaten, Lambda-Man wins, game over

        // If Lambda-Man lives is 0, Lambda-Man loses, game over
        if (lives == 0) {
            cout << "Game Over" << endl;
            break;
        }

        // Increment ticks
    }
}

