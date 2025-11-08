#include "Game.h"

Bird::Bird(int inputNodes, int hiddenNodes, int outputNodes) : xCordinate(100), yCordinate(300), size(20), velocity(0), gravity(800), jumpStrength(-400), score(0), fitness(0), gameOver(false), i_nodes(inputNodes), h_nodes(hiddenNodes), o_nodes(outputNodes)
{
    weights_ih.resize(h_nodes, std::vector<float>(i_nodes));
    for (int i = 0; i < h_nodes; ++i)
    {
        for (int j = 0; j < i_nodes; ++j)
        {
            weights_ih[i][j] = randomFloat();
        }
    }

    weights_ho.resize(o_nodes, std::vector<float>(h_nodes));
    for (int i = 0; i < o_nodes; ++i)
    {
        for (int j = 0; j < h_nodes; ++j)
        {
            weights_ho[i][j] = randomFloat();
        }
    }

    bias_h.resize(h_nodes);
    for (int i = 0; i < h_nodes; ++i)
    {
        bias_h[i] = randomFloat();
    }

    bias_o.resize(o_nodes);
    for (int i = 0; i < o_nodes; ++i)
    {
        bias_o[i] = randomFloat();
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
    std::vector<float> hidden_ouputs(h_nodes);
    for (int i = 0; i < h_nodes; ++i)
    {
        float weightedSum = 0.0f;
        for (int j = 0; j < i_nodes; ++j)
        {
            weightedSum += weights_ih[i][j] * inputs[j];
        }
        weightedSum += bias_h[i];
        hidden_ouputs[i] = sigmoid(weightedSum);
    }

    std::vector<float> final_outputs(o_nodes);
    for (int i = 0; i < o_nodes; ++i)
    {
        float weightedSum = 0.0f;
        for (int j = 0; j < h_nodes; ++j)
        {
            weightedSum += weights_ho[i][j] * hidden_ouputs[j];
        }
        weightedSum += bias_o[i];
        final_outputs[i] = sigmoid(weightedSum);
    }

    return final_outputs;
}

void Bird::mutate(float mutationRate)
{
    for (int i = 0; i < h_nodes; ++i)
    {
        for (int j = 0; j < i_nodes; ++j)
        {
            if (randomChance() < mutationRate)
            {
                weights_ih[i][j] += randomFloat();
            }
        }
    }

    for (int i = 0; i < o_nodes; ++i)
    {
        for (int j = 0; j < h_nodes; ++j)
        {
            if (randomChance() < mutationRate)
            {
                weights_ho[i][j] += randomFloat();
            }
        }
    }

    for (int i = 0; i < h_nodes; ++i)
    {
        if (randomChance() < mutationRate)
        {
            bias_h[i] += randomFloat();
        }
    }

    for (int i = 0; i < o_nodes; ++i)
    {
        if (randomChance() < mutationRate)
        {
            bias_o[i] += randomFloat();
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

Pipe::Pipe(float starting_Xcodinate, float groundHeight, float windowHeight) : xCordinate(starting_Xcodinate), width(60), gapHeight(180), speed(200), scored(false)
{
    YCodinateGap = 20 + gapHeight / 2 + (rand() % (int)(windowHeight - groundHeight - 20 - gapHeight));
}

void Pipe::update(float deltaTime)
{
    xCordinate -= speed * deltaTime;
}

SDL_FRect Pipe::getTopRect()
{
    return {xCordinate, 0, width, YCodinateGap - gapHeight / 2};
}

SDL_FRect Pipe::getBottomRect(float windowHeight)
{
    return {xCordinate, YCodinateGap + gapHeight / 2, width, windowHeight - (YCodinateGap + gapHeight / 2)};
}

bool Pipe::isOffScreen()
{
    return xCordinate + width < 0;
}

bool Pipe::hasPassedBird(float bird_xCordinate)
{
    return !scored && xCordinate + width < bird_xCordinate;
}

Population::Population(int size, float mRate) : generationNumber(1), mutationRate(mRate), populationSize(size)
{
    for (int i = 0; i < populationSize; ++i)
    {
        population.push_back(Bird(3, 4, 1));
    }
}

std::vector<Bird> &Population::getPopulation()
{
    return population;
}

void Population::evolveNewGeneration()
{
    int elitism = 0;
    for (int i = 0; i < populationSize; ++i)
    {
        if (population[i].fitness > population[elitism].fitness)
        {
            elitism = i;
        }
    }

    std::vector<Bird> newGeneration;
    newGeneration.reserve(populationSize);

    Bird elite = population[elitism];
    elite.reset();

    newGeneration.push_back(elite);

    for (int i = 1; i < populationSize; ++i)
    {
        Bird child = elite;
        child.mutate(mutationRate);
        newGeneration.push_back(child);
    }

    population = newGeneration;
    ++generationNumber;
}

Game::Game() : windowHeight(600), windowWidth(800), groundHeight(80), survivalFrames(0), pipeSpawnTimer(0), pipeSpawnInterval(2.8f), populationSize(10), mutationRate(0.05f), population(populationSize, mutationRate), pipes({Pipe(800, groundHeight, windowHeight)})
{
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
    pipes.push_back(Pipe(800, groundHeight, windowHeight));
    pipeSpawnTimer = 0;
    survivalFrames = 0;
}

void Game::run()
{
    bool running = true;

    Uint64 ticksPerSec = SDL_GetPerformanceFrequency();
    Uint64 lastTime = SDL_GetTicks();
    float deltaTime = 0.0f;

    SDL_Event event;

    while (running)
    {
        try
        {
            Uint64 currentTime = SDL_GetTicks();
            // deltaTime = (float)(currentTime - lastTime) * 1000 / (float)ticksPerSec;
            deltaTime = (float)(currentTime - lastTime) / 1000.0f;
            lastTime = currentTime;

            ++survivalFrames;

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
            if (pipeSpawnTimer >= pipeSpawnInterval)
            {
                pipes.push_back(Pipe(800, groundHeight, windowHeight));
                pipeSpawnTimer = 0;
            }

            Pipe *nearest = nullptr;

            for (auto &pipe : pipes)
            {
                pipe.update(deltaTime);

                if (nearest == nullptr && pipe.xCordinate + pipe.width > population.population[0].xCordinate)
                {
                    nearest = &pipe;
                }
            }

            bool foundAliveBird = false;

            for (int i = 0; i < populationSize; ++i)
            {
                Bird &bird = population.population[i];

                if (bird.gameOver)
                    continue;

                if (bird.feedForward({bird.yCordinate, nearest->YCodinateGap, nearest->xCordinate - bird.xCordinate})[0] > 0.5f)
                {
                    bird.flap();
                }

                if (bird.yCordinate > windowHeight - 80 || bird.yCordinate < 20)
                {
                    bird.gameOver = true;
                }

                bird.update(deltaTime);

                SDL_FRect birdRect = bird.getRect();
                SDL_FRect topPipe = nearest->getTopRect();
                SDL_FRect bottomPipe = nearest->getBottomRect(windowHeight);

                if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
                {
                    bird.gameOver = true;
                }

                if (nearest->hasPassedBird(bird.xCordinate))
                {
                    ++bird.score;
                    nearest->scored = true;
                }

                bird.fitness = bird.score * 10 + survivalFrames;

                if (bird.gameOver == false)
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
                SDL_Log("Evolving Population : GENERATION : %i", population.generationNumber);
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

    for (int y = 0; y < windowHeight - groundHeight; ++y)
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
        SDL_FRect bottomPipe = pipe.getBottomRect(windowHeight);

        SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255);
        SDL_RenderFillRect(renderer, &topPipe);
        SDL_RenderFillRect(renderer, &bottomPipe);
    }
}

void Game::renderBirds()
{
    for (int i = 0; i < populationSize; ++i)
    {
        Bird &bird = population.population[i];
        if (bird.gameOver)
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
    SDL_Color durt = {234, 208, 168};

    for (int y = windowHeight - groundHeight; y < windowHeight - groundHeight + 20; ++y)
    {
        SDL_SetRenderDrawColor(renderer, ground.r, ground.g, ground.b, ground.a);
        SDL_RenderLine(renderer, 0, y, windowWidth, y);
    }
    for (int y = windowHeight - groundHeight + 20; y < windowHeight; ++y)
    {
        SDL_SetRenderDrawColor(renderer, durt.r, durt.g, durt.b, durt.a);
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