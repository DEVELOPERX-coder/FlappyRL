#ifndef Game_H
#define GAME_H

#include <SDL3/SDL.h>
#include <vector>
#include <cstdlib> // srand
#include <cmath>
#include <algorithm>

class Bird
{
public:
    float xCordinate;
    float yCordinate;

    int size;

    float velocity;
    float gravity;
    float jumpStrength;

    int score;
    int fitness;
    bool gameOver;

    int i_nodes;
    int h_nodes;
    int o_nodes;

    std::vector<std::vector<float>> weights_ih;
    std::vector<std::vector<float>> weights_ho;
    std::vector<float> bias_h;
    std::vector<float> bias_o;

    Bird(int inputNodes, int hiddenNodes, int outputNodes);
    void flap();
    void update(float deltaTime);
    void reset();
    SDL_FRect getRect();

    std::vector<float> feedForward(const std::vector<float> &inputs);
    void mutate(float mutationRate);

    float sigmoid(float x);
    float randomFloat();
    float randomChance();
};

class Pipe
{
public:
    float xCordinate;
    float width;
    float gapHeight;
    float YCodinateGap;
    float speed;
    bool scored;

    Pipe(float starting_xCordinate, float groundHeight, float windowHeight);
    void update(float deltaTime);
    SDL_FRect getTopRect();
    SDL_FRect getBottomRect(float windoeHeight);
    bool isOffScreen();
    bool hasPassedBird(float bird_xCodinate);
};

struct GameState
{
    float bird_yCodinate;
    float gap_yCordinate;
    float horizontalDistToPipe;

    int score;
    bool gameOver;
};

class Population
{
public:
    std::vector<Bird> population;
    int generationNumber;
    float mutationRate;
    int populationSize;

    Population(int size, float mRate);
    std::vector<Bird> &getPopulation();
    void evolveNewGeneration();
};

class Game
{
public:
    SDL_Window *window;
    SDL_Renderer *renderer;

    int windowWidth;
    int windowHeight;

    float groundHeight;

    const int populationSize;
    const float mutationRate;

    Population population;
    std::vector<Pipe> pipes;

    float pipeSpawnTimer;
    float pipeSpawnInterval;

    int survivalFrames;

    Game();
    void resetGame();
    void run();

    void renderBackground();
    void renderPipes();
    void renderGround();
    void renderBirds();

    ~Game();
};

#endif