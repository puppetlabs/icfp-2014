/*
 * Implement the lambda-man world mechanics: http://icfpcontest.org/specification.html#the-lambda-man-game-rules
 */
#include "world.hpp"
#include <iostream>

using namespace std;
using namespace boost;

void LambdaWorld::run()
{
    size_t tick = 0, score = 0;
    unsigned int lives = 3;
    bool fright_mode = false;

    // std::pair<AI_state, std::function<std::pair LAMBDAMAN(world_state, undocumented)
    //


    while (true)
    {
        // Run Lambda-Man and ghost processors.
    
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
        ++tick;
    }
}

