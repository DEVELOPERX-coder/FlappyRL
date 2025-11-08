#include <iostream>
#include <windows.h>

#include "Game.h"

using namespace std;

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    cout << "Flappy Bird AI - GA (Genetic Algorithm)" << endl;
    cout << "Starting Training with 3-4-1 NN" << endl;

    cout << "Press Q to exit" << endl;
    Game game;
    game.run();
    return 0;
}