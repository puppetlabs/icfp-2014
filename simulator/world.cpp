/*
 * Implement the lambda-man world mechanics: http://icfpcontest.org/specification.html#the-lambda-man-game-rules
 */
#include <boost/heap/fibonacci_heap.hpp>
#include "world.hpp"
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/regex.hpp>

using namespace std;
using namespace boost;
using namespace boost::algorithm;
using namespace LambdaWorld;

static const int XMOVE[4] = {0, 1, 0, -1};
static const int YMOVE[4] = {-1, 0, 1, 0};

static string printWorld(WorldState &world)
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
    WorldMap &wm = get<0>(world);
    vector<string> split_strings;
    trim(world_map);
    split_regex(split_strings, world_map, boost::regex("[\n\r]"));
    transform(split_strings.begin(), split_strings.end(), back_inserter(wm),
        [&](string s) {
            vector<GridCell> line;
            transform(s.begin(), s.end(), back_inserter(line),
                [&](char c) {
                    return gridCellLookup[c];
                });
            return line;
        });

    assert(print_world(world) == world_map);

    // Initialize lambda-man.
    Location lambdaManLoc;
    for (size_t i = 0; i < wm.size(); ++i) {
        auto &row = wm[i];
        for (size_t j = 0; j < row.size(); ++j) {
            if (row[j] == LAMBDAMAN) {
                lambdaManLoc = make_pair(j, i);
                break;
            }
        }
    }
    get<1>(world) = make_tuple(0, lambdaManLoc, UP, 3, 0);

    // Initialize no fruit present.
    get<3>(world) = 0;

    return world;
}

static bool isLegalMove(Location &loc, Direction &dir, WorldMap &wm)
{
    // Change in x and y based on direction.
    auto newx = loc.first + XMOVE[(size_t)dir];
    auto newy = loc.second + YMOVE[(size_t)dir];

    return !(newy >= wm.size() || newx >= wm[0].size() || wm[newy][newx] == WALL);
}

// Input: a world map, Lambda-Man AI script, N Ghost AI scripts

void LambdaWorld::runWorld(string world_map, string lambda_script, vector<string> ghost_scripts)
{
    WorldState world = process(world_map);

    size_t mapWidth = get<0>(world)[0].size();
    size_t mapHeight = get<0>(world).size();
    size_t EndOfLives = 127 * mapWidth * mapHeight * 16;

    // Initialize Lambda-Man and ghost AI processors.

    for (size_t tick = 0; tick < EndOfLives; ++tick) {
        WorldMap &wm = get<0>(world);
        // Run Lambda-Man and ghost processors.
        //  Pass in current WorldState and AI state, receive move (a Direction) and new AI state
    
        // Evaluate all Lambda-Man and ghost moves for the current tick.
        LambdaManStat &lambdaMan = get<1>(world);
        Location &lambdaManLoc = get<1>(lambdaMan);
        Direction lambdaManDir = LEFT; // TODO: Query AI
        if (isLegalMove(lambdaManLoc, lambdaManDir, wm)) {
            // Move Lambda-Man.
            lambdaManLoc.first += XMOVE[(size_t)lambdaManDir];
            lambdaManLoc.second += YMOVE[(size_t)lambdaManDir];
            cout << "Lambda-Man's location (" << lambdaManLoc.first << ", " << lambdaManLoc.second << ")" << endl;
        }

        // Actions (fright mode deactivating, fruit appearing/disappearing)
        auto &lambdaManVitality = get<0>(lambdaMan);
        if (lambdaManVitality > 0) {
            --lambdaManVitality;
        }
        auto &fruitLife = get<3>(world);
        if (fruitLife > 0) {
            --fruitLife;
        }
        // TODO: Ghosts

        // Check if Lambda-Man is occupying the same square as pills, power pills, or fruit
        auto &score = get<4>(lambdaMan);
        GridCell &lambdaMansCell = wm[lambdaManLoc.second][lambdaManLoc.first];
        switch (lambdaMansCell) {
            //  If pill, pill eaten and removed from game
            case PILL:
                lambdaMansCell = EMPTY;
                //score += 
                break;
            //  If power pill, power pill eaten and removed from game, fright mode activated
            case POWER_PILL:
                lambdaMansCell = EMPTY;
                //score +=
                break;
            //  If fruit, fruit eaten and removed from game
            case FRUIT:
                lambdaMansCell = EMPTY;
                //score +=
                break;
            // Else do nothing
            default:
                break;
        }

        // TODO
        // If ghost and Lambda-man occupy square
        //  If fright_mode, Lambda-Man eats ghost; move ghost
        //  Else, Lambda-Man loses life; move Lambda-Man
        //--get<4>(get<1>(world));

        // If all ordinary pills eaten, Lambda-Man wins, game over
        if (none_of(wm.begin(), wm.end(),
                [](vector<GridCell> row) {
                    return none_of(row.begin(), row.end(),
                        [](GridCell cell) {
                            return cell == PILL;
                        }
                    );
                })) {
            cout << "Lambda-Man Won" << endl;
            break;
        }

        // If Lambda-Man lives is 0, Lambda-Man loses, game over
        if (get<3>(get<1>(world)) == 0) {
            cout << "Lambda-Man Lost" << endl;
            break;
        }

        // Increment ticks in for loop
    }

    cout << "Game Over" << endl;
}

