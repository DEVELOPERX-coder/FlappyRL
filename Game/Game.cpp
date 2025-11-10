#include "Game.h"

// ----- Bird Class Decleration Start -----

Bird::Bird(int inputNodes, std::vector<int> hiddenNodes, int outputNodes) : xCordinate(100), yCordinate(300), size(20), velocity(0), gravity(800), jumpStrength(-400), score(0), fitness(0), gameOver(false), i_nodes(inputNodes), h_nodes(hiddenNodes), o_nodes(outputNodes)
{
    weights_ih.resize(h_nodes[0], std::vector<float>(i_nodes));
    for (int i = 0; i < h_nodes[0]; ++i)
    {
        for (int j = 0; j < i_nodes; ++j)
        {
            weights_ih[i][j] = randomFloat();
        }
    }

    weights_hh.resize(h_nodes.size());
    for (int layer = 1; layer < h_nodes.size(); ++layer)
    {
        weights_hh[layer].resize(h_nodes[layer], std::vector<float>(h_nodes[layer - 1]));
    }
    for (int layer = 1; layer < h_nodes.size(); ++layer)
    {
        for (int node = 0; node < h_nodes[layer]; ++node)
        {
            for (int weight = 0; weight < h_nodes[layer - 1]; ++weight)
            {
                weights_hh[layer][node][weight] = randomFloat();
            }
        }
    }

    weights_ho.resize(o_nodes, std::vector<float>(h_nodes[h_nodes.size() - 1]));
    for (int i = 0; i < o_nodes; ++i)
    {
        for (int j = 0; j < h_nodes[h_nodes.size() - 1]; ++j)
        {
            weights_ho[i][j] = randomFloat();
        }
    }

    bias_h.resize(h_nodes.size());
    for (int layer = 0; layer < h_nodes.size(); ++layer)
    {
        bias_h[layer].resize(h_nodes[layer]);
    }
    for (int layer = 0; layer < h_nodes.size(); ++layer)
    {
        for (int node = 0; node < h_nodes[layer]; ++node)
        {
            bias_h[layer][node] = randomFloat();
        }
    }

    bias_o.resize(o_nodes);
    for (int node = 0; node < o_nodes; ++node)
    {
        bias_o[node] = randomFloat();
    }
}

void Bird::flap()
{
    velocity = jumpStrength;
}

void Bird::update(float deltaTime)
{
    velocity += gravity * deltaTime;
    yCordinate += velocity * deltaTime;
}

void Bird::reset()
{
    xCordinate = 100;
    yCordinate = 300;
    velocity = 0;
    score = 0;
    fitness = 0;
    gameOver = false;
}

SDL_FRect Bird::getRect()
{
    return {xCordinate - (size >> 1), yCordinate - (size >> 1), (float)size, (float)size};
}

std::vector<float> Bird::feedForward(const std::vector<float> &inputs)
{
    std::vector<float> hidden_output(h_nodes[0]);
    for (int node = 0; node < h_nodes[0]; ++node)
    {
        float weightedSum = 0.0f;
        for (int prevNode = 0; prevNode < i_nodes; ++prevNode)
        {
            weightedSum += weights_ih[prevNode][node] * inputs[prevNode];
        }
    }

    for (int layer = 1; layer < (int)h_nodes.size(); ++layer)
    {
        std::vector<float> hidden_output_layer(h_nodes[layer]);
        for (int node = 0; node < h_nodes[layer]; ++node)
        {
            float weightedSum = 0.0f;
            for (int prevNode = 0; prevNode < h_nodes[layer - 1]; ++prevNode)
            {
                weightedSum += weights_hh[layer][node][prevNode] * hidden_output[prevNode];
            }
            weightedSum += bias_h[layer][node];
            hidden_output_layer[node] = sigmoid(weightedSum);
        }
        hidden_output = hidden_output_layer;
    }

    std::vector<float> final_outputs(o_nodes);
    for (int node = 0; node < o_nodes; ++node)
    {
        float weightedSum = 0.0f;
        for (int prevNode = 0; prevNode < h_nodes[(int)h_nodes.size() - 1]; ++prevNode)
        {
            weightedSum += weights_ho[node][prevNode] * hidden_output[prevNode];
        }
        weightedSum += bias_o[node];
        final_outputs[node] = sigmoid(weightedSum);
    }

    return final_outputs;
}

void Bird::mutate(float mutationRate)
{
    for (int node = 0; node < h_nodes[0]; ++node)
    {
        for (int prevNode = 0; prevNode < i_nodes; ++prevNode)
        {
            if (randomChance() < mutationRate)
            {
                weights_ih[node][prevNode] += randomFloat();
            }
        }
    }

    for (int layer = 1; layer < (int)h_nodes.size(); ++layer)
    {
        for (int node = 0; node < h_nodes[layer]; ++node)
        {
            for (int prevNode = 0; prevNode < h_nodes[layer - 1]; ++prevNode)
            {
                if (randomChance() < mutationRate)
                {
                    weights_hh[layer][node][prevNode] += randomFloat();
                }
            }
        }
    }

    for (int node = 0; node < o_nodes; ++node)
    {
        for (int prevNode = 0; prevNode < h_nodes[(int)h_nodes.size() - 1]; ++prevNode)
        {
            if (randomChance() < mutationRate)
            {
                weights_ho[node][prevNode] += randomFloat();
            }
        }
    }

    for (int layer = 0; layer < (int)h_nodes.size(); ++layer)
    {
        for (int node = 0; node < h_nodes[layer]; ++node)
        {
            if (randomChance() < mutationRate)
            {
                bias_h[layer][node] += randomFloat();
            }
        }
    }

    for (int node = 0; node < o_nodes; ++node)
    {
        if (randomChance() < mutationRate)
        {
            bias_o[node] += randomFloat();
        }
    }
}

float Bird::sigmoid(float x)
{
    return 1.0f / (1.0f + std::exp(-x));
}

float Bird::randomFloat()
{
    return (float)rand() / RAND_MAX * 2.0f - 1.0f;
}

float Bird::randomChance()
{
    return (float)rand() / RAND_MAX;
}

int Bird::getFitness()
{
    return fitness;
}

bool Bird::getGameOver()
{
    return gameOver;
}

void Bird::setGameOver(bool condition)
{
    gameOver = condition;
}

float Bird::getYCordinate()
{
    return yCordinate;
}

float Bird::getXCordinate()
{
    return xCordinate;
}

void Bird::incrementScore(int inc)
{
    score += inc;
}

void Bird::setFitness(int value)
{
    fitness = value;
}

int Bird::getScore()
{
    return score;
}

// ----- Bird Class Decleration End -----

// ----- Pipe Class Decleration Start -----

Pipe::Pipe(float startingXCordinate, float groundHeight, float roofHeight, float windowHeight)
{
    width = 60;
    gapHeight = 180;
    Xspeed = 200;
    Yspeed = 50;
    xCordinate = startingXCordinate;
    this->groundHeight = groundHeight;
    this->roofHeight = roofHeight;
    this->windowHeight = windowHeight;

    goingDown = true;

    yCordinateGap = roofHeight + (gapHeight / 2) + (rand() % (int)(windowHeight - groundHeight - roofHeight - gapHeight));
}

void Pipe::update(float deltaTime)
{
    xCordinate -= Xspeed * deltaTime;
    if (goingDown)
        yCordinateGap -= Yspeed * deltaTime;
    else
        yCordinateGap += Yspeed * deltaTime;

    if (yCordinateGap + gapHeight / 2 >= groundHeight)
    {
        goingDown != goingDown;
        yCordinateGap = groundHeight - gapHeight / 2;
    }
    if (yCordinateGap - gapHeight / 2 <= roofHeight)
    {
        goingDown != goingDown;
        yCordinateGap = roofHeight + gapHeight / 2;
    }
}

SDL_FRect Pipe::getTopRect()
{
    return {xCordinate, 0, width, yCordinateGap - gapHeight / 2};
}

SDL_FRect Pipe::getBottomRect()
{
    return {xCordinate, yCordinateGap + gapHeight / 2, width, windowHeight - (yCordinateGap + gapHeight / 2) - roofHeight - groundHeight};
}

bool Pipe::isOffScreen()
{
    return xCordinate + width < 0;
}

bool Pipe::hasPassedBird(float birdXCordinate)
{
    return xCordinate + width < birdXCordinate;
}

float Pipe::getXCordinate()
{
    return xCordinate;
}

float Pipe::getYcordinate()
{
    return yCordinateGap;
}

float Pipe::getWidth()
{
    return width;
}

// ----- Pipe Class Decleration End -----

// ----- Population Class Decleration Start -----

Population::Population(int size, float mRate)
{
    generationNumber = 1;
    mutationRate = mRate;
    populationSize = size;

    for (int i = 0; i < populationSize; ++i)
    {
        // population.push_back(Bird(10, {12, 12}, 1));
        population.push_back(Bird(3, {4}, 1));
    }
}

void Population::evolveNewGeneration()
{
    int elitism = 0;
    std::vector<int> topElitism(3, 0);
    for (int i = 0; i < populationSize; ++i)
    {
        if (population[i].getFitness() > population[topElitism[0]].getFitness())
        {
            topElitism[2] = topElitism[1];
            topElitism[1] = topElitism[0];
            topElitism[0] = i;
        }
        else if (population[i].getFitness() > population[topElitism[1]].getFitness())
        {
            topElitism[2] = topElitism[1];
            topElitism[1] = i;
        }
        else if (population[i].getFitness() > population[topElitism[2]].getFitness())
        {
            topElitism[2] = i;
        }
    }

    std::vector<Bird> newGeneration;
    newGeneration.reserve(populationSize);

    int start = 0;

    Bird elite1 = population[topElitism[0]];
    elite1.reset();

    newGeneration.push_back(elite1);
    for (; start < populationSize / 3; ++start)
    {
        Bird child = elite1;
        child.mutate(mutationRate);
        newGeneration.push_back(child);
    }

    Bird elite2 = population[topElitism[1]];
    elite2.reset();

    newGeneration.push_back(elite2);
    for (; start < populationSize / 3 * 2; ++start)
    {
        Bird child = elite2;
        child.mutate(mutationRate);
        newGeneration.push_back(child);
    }

    Bird elite3 = population[topElitism[2]];
    elite3.reset();

    newGeneration.push_back(elite3);
    for (; start < populationSize; ++start)
    {
        Bird child = elite3;
        child.mutate(mutationRate);
        newGeneration.push_back(child);
    }

    population = newGeneration;
    ++generationNumber;
}

std::vector<Bird> &Population::getPopulation()
{
    return population;
}

int Population::getGenerationNumber()
{
    return generationNumber;
}

// ----- Population Class Decleration End -----

// ----- Game Class Decleration Start -----

Game::Game() : populationSize(15), mutationRate(0.05f), population(populationSize, mutationRate)
{
    survivalFrames = 0;
    windowHeight = 600;
    windowWidth = 800;
    groundHeight = 5;
    roofHeight = 5;
    pipeSpawnTimer = 0;
    PipeSpawnInterval = 2.8f;

    pipes.push_back({Pipe(800, groundHeight, roofHeight, windowHeight)});

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Unable to Initialized SDL: %s", SDL_GetError());
        return;
    }
    window = SDL_CreateWindow("Flappy Bird GA", windowWidth, windowHeight, 0);
    if (!window)
    {
        SDL_Log("Unable to Initialized Window: %s", SDL_GetError());
        SDL_Quit();
        return;
    }
    renderer = SDL_CreateRenderer(window, NULL);
    if (!window)
    {
        SDL_Log("Unable to Initialized Renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
}

void Game::resetGame()
{
    pipes.clear();
    pipes.push_back({Pipe(800, groundHeight, roofHeight, windowHeight)});
    pipeSpawnTimer = 0;
}

void Game::run()
{
    bool running = true;

    Uint64 lastTickCheck = SDL_GetTicks();
    float deltaTime = 0.0f;

    SDL_Event event;

    while (running)
    {
        try
        {
            Uint64 currentTickCheck = SDL_GetTicks();
            deltaTime = (float)(currentTickCheck - lastTickCheck) / 1000.0f;
            lastTickCheck = currentTickCheck;

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_EVENT_QUIT)
                {
                    running = 0;
                }
                if (event.type == SDL_EVENT_KEY_DOWN)
                {
                    SDL_Keycode keyPressed = event.key.key;
                    if (keyPressed == SDLK_Q)
                    {
                        running = 0;
                    }
                }
            }

            pipeSpawnTimer += deltaTime;
            if (pipeSpawnTimer > PipeSpawnInterval)
            {
                pipes.push_back({Pipe(800, groundHeight, roofHeight, windowHeight)});
                pipeSpawnTimer = 0;
            }

            Pipe *nearest = nullptr;

            for (auto &pipe : pipes)
            {
                pipe.update(deltaTime);

                if (nearest == nullptr && pipe.getXCordinate() + pipe.getWidth() > population.getPopulation()[0].getXCordinate())
                {
                    nearest = &pipe;
                }
            }

            bool foundAliveBird = false;

            for (int i = 0; i < populationSize; ++i)
            {
                Bird &bird = population.getPopulation()[i];

                if (bird.getGameOver())
                    continue;

                if (bird.feedForward({bird.getYCordinate(), nearest->getYcordinate(), nearest->getXCordinate() - bird.getXCordinate()})[0] > 0.5f)
                {
                    bird.flap();
                }

                if (bird.getYCordinate() > windowHeight - groundHeight || bird.getYCordinate() < roofHeight)
                {
                    bird.setGameOver(true);
                }

                bird.update(deltaTime);

                SDL_FRect birdRect = bird.getRect();
                SDL_FRect topPipe = nearest->getTopRect();
                SDL_FRect bottomPipe = nearest->getBottomRect();

                if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
                {
                    bird.setGameOver(true);
                }

                if (nearest->hasPassedBird(bird.getXCordinate()))
                {
                    bird.incrementScore(1);
                }

                bird.setFitness(bird.getScore() * 10 + survivalFrames);

                if (bird.getGameOver() == false)
                    foundAliveBird = true;
            }

            pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                                  { return pipe.isOffScreen(); }),
                        pipes.end());

            // Rendering Part Start
            renderBackground();
            renderPipes();
            renderGround();
            renderBirds();
            SDL_RenderPresent(renderer);
            // Rendering Part End

            if (!foundAliveBird)
            {
                resetGame();
                population.evolveNewGeneration();
                SDL_Log("Evolving Population : GENERATION : %i", population.getGenerationNumber());
            }
        }
        catch (...)
        {
            SDL_Log("Error in game loop... : %s", SDL_GetError());
            SDL_Delay(100);
        }

        SDL_Delay(17); // ~60FPS
    }
}

void Game::renderBackground()
{
    SDL_Color color1 = {30, 15, 117, 255};   // #1e0f75
    SDL_Color color2 = {55, 133, 216, 255};  // #3785d8
    SDL_Color color3 = {173, 198, 229, 255}; // #adc6e5

    for (int y = roofHeight; y < windowHeight - groundHeight; ++y)
    {
        float t;
        SDL_Color start, end;

        if (y < (windowHeight >> 1))
        {
            t = (float)y / (windowHeight >> 1);
            start = color1;
            end = color2;
        }
        else
        {
            t = (float)(y - (windowHeight >> 1)) / (windowHeight >> 1);
            start = color2;
            end = color3;
        }

        Uint8 r = (1 - t) * start.r + t * end.r;
        Uint8 g = (1 - t) * start.g + t * end.g;
        Uint8 b = (1 - t) * start.b + t * end.b;

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderLine(renderer, 0, y, windowWidth, y);
    }
}

void Game::renderPipes()
{
    for (auto &pipe : pipes)
    {
        SDL_FRect topPipe = pipe.getTopRect();
        SDL_FRect bottomPipe = pipe.getBottomRect();

        SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255);
        SDL_RenderFillRect(renderer, &topPipe);
        SDL_RenderFillRect(renderer, &bottomPipe);
    }
}

void Game::renderBirds()
{
    for (int i = 0; i < populationSize; ++i)
    {
        Bird &bird = population.getPopulation()[i];
        if (bird.getGameOver())
            continue;

        // Generate a unique color based on index
        Uint8 r = (Uint8)((i * 40) % 256);
        Uint8 g = (Uint8)((i * 85) % 256);
        Uint8 b = (Uint8)((i * 130) % 256);

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_FRect birdBody = bird.getRect();
        SDL_RenderFillRect(renderer, &birdBody);
    }
}

void Game::renderGround()
{
    SDL_Color ground = {34, 139, 34, 255};

    for (int y = windowHeight - groundHeight; y < windowHeight; ++y)
    {
        SDL_SetRenderDrawColor(renderer, ground.r, ground.g, ground.b, ground.a);
        SDL_RenderLine(renderer, 0, y, windowWidth, y);
    }
}

Game::~Game()
{
    if (window)
    {
        SDL_DestroyWindow(window);
    }
    if (renderer)
    {
        SDL_DestroyRenderer(renderer);
    }
    SDL_Quit();
    SDL_Log("SDL shut down successfully");
}

// ----- Game Class Decleration End -----