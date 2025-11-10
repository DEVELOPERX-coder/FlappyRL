# FlappyRL - Flappy Bird with Reinforcement Learning

## Approach 1 : Saving State : QTable

- 800 Values of Y(bird height) , 800 Values of Y(Gab Height) , 800 Values of X(Bird - Pipe Distance)
- Total ~200MB of data store in QTable if uses 11Bit for 3 variables and 1 bit for flap decision
- Total ~61MB if uses vector with only decision bit

- We will use vector method
- Y(bird Height) - X(Distance) - Y(gap Height) as arrangement

to compile : g++ QTableGame.cpp -I./SDL3/include -L./SDL3/lib -lSDL3

g++ main.cpp Game/Game.cpp -Iinclude -Llib -lSDL3 -lSDL3_image -o game.exe
