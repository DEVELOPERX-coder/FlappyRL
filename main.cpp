#include <iostream>
#include <windows.h>

#include "Game/Game.h"

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    printf("Training Started!...\n");

    Game game;
    game.run();

    printf("Training Stopped!...\n");
    return 0;
}