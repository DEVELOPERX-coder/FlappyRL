#ifndef Game_H
#define GAME_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <vector>
#include <cstdlib> // srand
#include <cmath>
#include <algorithm>
#include <ctime> // time

class Bird
{
private:
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
    std::vector<int> h_nodes;
    int o_nodes;

    std::vector<std::vector<std::vector<float>>> weights_hh;
    std::vector<std::vector<float>> weights_ho;
    std::vector<std::vector<float>> bias_h;
    std::vector<float> bias_o;

public:
    Bird(int inputNodes, std::vector<int> hiddenNodes, int outputNodes);

    void flap();
    void update(float deltaTime);
    void reset();

    SDL_FRect getRect();

    std::vector<float> feedForward(const std::vector<float> &inputs);
    void mutate(float mutationRate);

    float sigmoid(float x);
    float randomFloat();
    float randomChance();

    int getFitness();
    void setFitness(int value);
    bool getGameOver();
    void setGameOver(bool condition);
    float getYCordinate();
    float getXCordinate();
    void incrementScore(int inc);
    int getScore();
};

class Pipe
{
private:
    float xCordinate;
    float width;
    float gapHeight;
    float yCordinateGap;
    float Xspeed;
    float Yspeed;
    float windowHeight;
    float roofHeight;
    float groundHeight;

    bool goingDown;

public:
    Pipe(float startingXCordinate, float groundHeight, float roofHeight, float windowHeight);

    void update(float deltaTime);

    SDL_FRect getTopRect();
    SDL_FRect getBottomRect();

    bool isOffScreen();
    bool hasPassedBird(float birdXCordinate);

    float getXCordinate();
    float getYcordinate();
    float getWidth();
};

class Population
{
private:
    std::vector<Bird> population;
    int generationNumber;
    float mutationRate;
    int populationSize;

public:
    Population(int size, float mRate);
    void evolveNewGeneration();
    std::vector<Bird> &getPopulation();
    int getGenerationNumber();
};

class Game
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;

    int windowWidth;
    int windowHeight;

    float roofHeight;
    float groundHeight;

    int populationSize;
    float mutationRate;

    Population population;
    std::vector<Pipe> pipes;

    float pipeSpawnTimer;
    float PipeSpawnInterval;

    int survivalFrames;

public:
    Game();

    void resetGame();
    void run();

    void renderBackground();
    void renderScore();
    void renderPipes();
    void renderRoof();
    void renderGround();
    void renderBirds();

    ~Game();
};

#endif