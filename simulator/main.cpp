/*
 * Runner for lambda-man world from assembly files for Ghost and Lambda Man programs.
 */

#include <iostream>
#include "world.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    // Read files from command-line.
    cout << "Loading files:";
    for(int i = 1; i < argc; ++i) {
        cout << " " << argv[i];
    }
    cout << endl;

    // Instantiate and run world.
    cout << "Instantiating world..." << endl;
    LambdaWorld world;
    world.run();
    return 0;
}

