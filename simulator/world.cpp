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
using namespace aiproc;

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

static unsigned int scoreFruit(WorldState &world)
{
    vector<unsigned int> fruitPoints = {0, 100, 300, 500, 500, 700, 700, 1000, 1000, 2000, 2000, 3000, 3000, 5000};
    WorldMap &wm = get<WSMAP>(world);
    unsigned int level = ((wm.size() * wm[0].size()) + 99) / 100;
    if (level >= fruitPoints.size()) {
        return 5000;
    } else {
        return fruitPoints[level];
    }
}

static unsigned int scoreGhost(unsigned int eaten)
{
    switch (eaten) {
        case 0:
            return 200;
        case 1:
            return 400;
        case 2:
            return 800;
        default:
            return 1600;
    }
}

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

static WorldState process(string world_map, const string &lambda_script, const vector<string> &ghost_scripts)
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

    assert(printWorld(world) == world_map);

    // Initialize lambda-man and ghosts.
    Location lambdaManLoc;
    vector<GhostStat> &ghosts = get<WSGHOSTS>(world);
    for (size_t i = 0; i < wm.size(); ++i) {
        auto &row = wm[i];
        for (size_t j = 0; j < row.size(); ++j) {
            if (row[j] == LAMBDAMAN) {
                lambdaManLoc = make_pair(j, i);
            } else if (row[j] == GHOST) {
                ghosts.push_back(make_tuple(STANDARD, make_pair(j, i), DOWN, 0, make_pair(j, i)));
            }
        }
    }
    get<WSLAMBDA>(world) = make_tuple(0, lambdaManLoc, DOWN, 3, 0, LM_MOVE, aiproc::compile_program(lambda_script), Value(), Closure(), 0, lambdaManLoc);

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
    WorldState world = process(world_map, lambda_script, ghost_scripts);

    // setup the main entry point.
    Closure main;
    std::vector<Value> main_args;
    main_args.push_back(0); // world_state
    main_args.push_back(0); // UNKNOWN
    main.address = 0;

    // Run the main program to get the initial AI state and
    // our tick function
    auto result = get<LMPROC>(get<WSLAMBDA>(world)).run(main, main_args);
    get<LMSTATE>(get<WSLAMBDA>(world)) = result->car;
    get<LMFUNC>(get<WSLAMBDA>(world)) = boost::get<Closure>(result->cdr);

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
            // All pills eaten, double the score
            get<LMSCORE>(get<WSLAMBDA>(world)) *= 2;
            break;
        }

        // If Lambda-Man lives is 0, Lambda-Man loses, game over
        if (get<LMLIVES>(get<WSLAMBDA>(world)) == 0) {
            cout << "Lambda-Man Lost" << endl;
            break;
        }
    }

    cout << "Game Over" << endl;
    cout << "Score = " << get<LMSCORE>(get<WSLAMBDA>(world)) << endl;
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

            vector<Value> tick_args;
            tick_args.push_back(get<LMSTATE>(lambdaMan));
            tick_args.push_back(0); // world state
            auto result = get<LMPROC>(lambdaMan).run(get<LMFUNC>(lambdaMan), tick_args);
            get<LMFUNC>(lambdaMan).environ->values.clear(); // clear function args after call.
            get<LMSTATE>(lambdaMan) = result->car;
            Direction lambdaManDir = static_cast<Direction>(boost::get<int32_t>(result->cdr));

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
        // TODO: Determine ghost moves.

        // Actions (fright mode deactivating, fruit appearing/disappearing)
        auto &lambdaManVitality = get<LMVIT>(lambdaMan);
        if (lambdaManVitality > 0) {
            --lambdaManVitality;
        }
        if (lambdaManVitality == 0) {
            for (auto &g : get<WSGHOSTS>(world)) {
                get<GSVIT>(g) = STANDARD;
            }
        }

        // Enable fruit at correct UTC time
        auto &fruitLife = get<WSFRUIT>(world);
        auto &utc = get<WSUTC>(world);
        if (fruitLife > 0) {
            --fruitLife;
        } else if (utc >= FRUIT1_APPEAR && utc <= FRUIT1_EXPIRE) {
            fruitLife = FRUIT1_EXPIRE - utc;
        } else if (utc >= FRUIT2_APPEAR && utc <= FRUIT2_EXPIRE) {
            fruitLife = FRUIT2_EXPIRE - utc;
        }


        // Check if Lambda-Man is occupying the same square as pills, power pills, or fruit
        // Will only do anything if Lambda-Man moved this tick.
        auto &score = get<LMSCORE>(lambdaMan);
        GridCell &lambdaMansCell = wm[lambdaManLoc.second][lambdaManLoc.first];
        switch (lambdaMansCell) {
            case PILL:
                //  If pill, pill eaten and removed from game
                lambdaMansCell = EMPTY;
                score += 10;
                lmStep = LM_EATING;
                break;
            case POWER_PILL:
                //  If power pill, power pill eaten and removed from game, fright mode activated
                lambdaMansCell = EMPTY;
                score += 50;
                lambdaManVitality += FRIGHT_DURATION;
                lmStep = LM_EATING;
                get<LMEATEN>(lambdaMan) = 0;

                // Set all ghosts to FRIGHT-mode.
                for (auto &g : get<WSGHOSTS>(world)) {
                    get<GSVIT>(g) = FRIGHT;
                }
                break;
            case FRUIT:
                //  If fruit is active, fruit eaten and removed from game
                if (fruitLife > 0) {
                    score += scoreFruit(world);
                    lmStep = LM_EATING;
                }
                fruitLife = 0;
                break;
                // Else do nothing
            default:
                break;
        }

        // If ghost and Lambda-man occupy square
        //  If fright_mode, Lambda-Man eats ghost; move ghost
        //  Else, Lambda-Man loses life; move Lambda-Man
        for (auto &g : get<WSGHOSTS>(world)) {
            Location &loc = get<GSLOC>(g);
            if (loc == lambdaManLoc && get<GSVIT>(g) != INVISIBLE) {
                if (lambdaManVitality > 0) {
                    assert(get<GSVIT>(g) == FRIGHT);
                    score += scoreGhost(get<LMEATEN>(lambdaMan));
                    // Increment number eaten.
                    ++get<LMEATEN>(lambdaMan);
                    // TODO: Return ghost to its starting position.
                    get<GSVIT>(g) = INVISIBLE;
                    get<GSLOC>(g) = get<GSSTART>(g);
                } else {
                    --get<LMLIVES>(lambdaMan);
                    // Return all entities to starting positions.
                    get<LMLOC>(lambdaMan) = get<LMSTART>(lambdaMan);
                    for (auto &gh : get<WSGHOSTS>(world)) {
                        get<GSLOC>(gh) = get<GSSTART>(gh);
                    }
                }
                break;
            }
        }

        // Increment ticks in for loop
        --lmStep;
        ++get<WSUTC>(world);
        if (get<WSUTC>(world) >= get<WSEOL>(world)) {
            break;
        }
    }
    return;
}

