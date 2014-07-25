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
using namespace LambdaWorld;

static const int XMOVE[4] = {0, 1, 0, -1};
static const int YMOVE[4] = {-1, 0, 1, 0};
static const int FRIGHT_DURATION = 127*20;

static const int FRUIT1_APPEAR = 127*200;
static const int FRUIT1_EXPIRE = 127*280;
static const int FRUIT2_APPEAR = 127*400;
static const int FRUIT2_EXPIRE = 127*480;

static const int LM_MOVE = 127;
static const int LM_EATING = 137;
static const int GHOST0 = 130;
static const int GHOST0_FRIGHT = 195;
static const int GHOST1 = 132;
static const int GHOST1_FRIGHT = 198;
static const int GHOST2 = 134;
static const int GHOST2_FRIGHT = 201;
static const int GHOST3 = 136;
static const int GHOST3_FRIGHT = 204;

static string printWorld(WorldState &world)
{
    string grid;
    vector<char> gridCellPrinter = {'#', ' ', '.', 'o', '%', '\\', '='};
    for (auto &row : get<WSMAP>(world)) {
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
    WorldMap &wm = get<WSMAP>(world);
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
    get<WSLAMBDA>(world) = make_tuple(0, lambdaManLoc, UP, 3, 0, LM_MOVE);

    // Initialize no fruit present.
    get<WSFRUIT>(world) = 0;

    // Initialize time-out countdown.
    get<WSEOL>(world) = 127 * wm[0].size() * wm.size() * 16;
    get<WSUTC>(world) = 0;

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

    // Initialize Lambda-Man and ghost AI processors.
    while (get<WSUTC>(world) < get<WSEOL>(world)) {
        step(world);

        // Check ending conditions.
        WorldMap &wm = get<WSMAP>(world);
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
        if (get<LMLIVES>(get<WSLAMBDA>(world)) == 0) {
            cout << "Lambda-Man Lost" << endl;
            break;
        }
    }

    cout << "Game Over" << endl;
}

void LambdaWorld::step(WorldState &world)
{
    WorldMap &wm = get<WSMAP>(world);
    LambdaManStat &lambdaMan = get<WSLAMBDA>(world);

    for (bool stop = false; !stop; ) {

        // Run Lambda-Man and ghost processors.
        //  Pass in current WorldState and AI state, receive move (a Direction) and new AI state

        // Evaluate all Lambda-Man and ghost moves for the current tick.
        auto &lmStep = get<LMSTEP>(lambdaMan);
        Location &lambdaManLoc = get<LMLOC>(lambdaMan);
        if (lmStep == 0) {
            Direction lambdaManDir = LEFT; // TODO: Query AI
            if (isLegalMove(lambdaManLoc, lambdaManDir, wm)) {
                // Move Lambda-Man.
                lambdaManLoc.first += XMOVE[(size_t)lambdaManDir];
                lambdaManLoc.second += YMOVE[(size_t)lambdaManDir];
                cout << "Lambda-Man's location[" << get<WSUTC>(world) << "] (" << lambdaManLoc.first;
                cout << ", " << lambdaManLoc.second << ")" << endl;
            }
            lmStep = LM_MOVE;
            stop = true;
        }

        // Actions (fright mode deactivating, fruit appearing/disappearing)
        auto &lambdaManVitality = get<LMVIT>(lambdaMan);
        if (lambdaManVitality > 0) {
            --lambdaManVitality;
        }
        // TODO: Enable fruit at correct UTC time
        auto &fruitLife = get<WSFRUIT>(world);
        if (fruitLife > 0) {
            --fruitLife;
        }
        // TODO: Ghosts

        // Check if Lambda-Man is occupying the same square as pills, power pills, or fruit
        // Will only do anything if Lambda-Man moved this tick.
        auto &score = get<LMSCORE>(lambdaMan);
        GridCell &lambdaMansCell = wm[lambdaManLoc.second][lambdaManLoc.first];
        switch (lambdaMansCell) {
            case PILL:
                //  If pill, pill eaten and removed from game
                lambdaMansCell = EMPTY;
                //score += 
                lmStep = LM_EATING;
                break;
            case POWER_PILL:
                //  If power pill, power pill eaten and removed from game, fright mode activated
                lambdaMansCell = EMPTY;
                //score +=
                //lambdaManVitality +=
                lmStep = LM_EATING;
                break;
            case FRUIT:
                //  If fruit is active, fruit eaten and removed from game
                if (fruitLife > 0) {
                    //score +=
                    lmStep = LM_EATING;
                }
                fruitLife = 0;
                break;
                // Else do nothing
            default:
                break;
        }

        // TODO
        // If ghost and Lambda-man occupy square
        //  If fright_mode, Lambda-Man eats ghost; move ghost
        //  Else, Lambda-Man loses life; move Lambda-Man
        //--get<LMLIVES>(lambdaMan);

        // Increment ticks in for loop
        // TODO: Countdown
        --lmStep;
        ++get<WSUTC>(world);
        if (get<WSUTC>(world) >= get<WSEOL>(world)) {
            break;
        }
    }
    return;
}

