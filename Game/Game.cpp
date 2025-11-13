#include "Game.h"

// ----- Rays Class Decleration Start -----

#define RAYS_NUMBER 10
void generate_rays(Bird &bird, Ray *rays, Pipe *nearest)
{

    for (int i = 0; i < RAYS_NUMBER; ++i)
    {
        double angle = -M_PI / 2 + ((double)i / (RAYS_NUMBER - 1)) * M_PI;

        Ray ray = {bird.getXCordinate(), bird.getYCordinate(), angle, 0, 0};

        int end_of_screen = 0;
        int object_hit = 0;

        double step = 1;
        double x_end = ray.startX;
        double y_end = ray.startY;

        while (!end_of_screen && !object_hit)
        {
            x_end += step * cos(ray.m);
            y_end += step * sin(ray.m);

            if (x_end <= 0 || x_end >= 800)
                end_of_screen = 1;
            if (y_end <= 20 || y_end >= 600 - 20)
                end_of_screen = 1;

            if (x_end >= nearest->getXCordinate() && x_end <= nearest->getXCordinate() + nearest->getWidth())
            {
                if (y_end <= nearest->getYcordinate() - nearest->getGapHeight() / 2 || y_end >= nearest->getYcordinate() + nearest->getGapHeight() / 2)
                {
                    object_hit = 1;
                }
            }
        }

        ray.endX = x_end;
        ray.endY = y_end;

        rays[i] = ray;
    }
}

// ----- Rays Class Decleration End -----

// ----- Bird Class Decleration Start -----

Bird::Bird(int inputNodes, std::vector<int> hiddenNodes, int outputNodes) : xCordinate(100), yCordinate(300), size(20), velocity(0), gravity(800), jumpStrength(-400), score(0), fitness(0), gameOver(false), i_nodes(inputNodes), h_nodes(hiddenNodes), o_nodes(outputNodes)
{
    weights_hh.resize(h_nodes.size());
    weights_hh[0].resize(h_nodes[0], std::vector<float>(i_nodes));
    for (int layer = 1; layer < h_nodes.size(); ++layer)
    {
        weights_hh[layer].resize(h_nodes[layer], std::vector<float>(h_nodes[layer - 1]));
    }

    for (int node = 0, layer = 0; node < h_nodes[0]; ++node)
    {
        for (int inp = 0; inp < i_nodes; ++inp)
        {
            weights_hh[layer][node][inp] = randomFloat();
        }
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
            weightedSum += weights_hh[0][node][prevNode] * inputs[prevNode];
        }
        weightedSum += bias_h[0][node];
        hidden_output[node] = sigmoid(weightedSum);
    }
    for (int layer = 1; layer < h_nodes.size(); ++layer)
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
    for (int node = 0, layer = 0; node < h_nodes[layer]; ++node)
    {
        for (int prevNode = 0; prevNode < i_nodes; ++prevNode)
        {
            if (randomChance() < mutationRate)
            {
                weights_hh[layer][node][prevNode] += randomFloat();
            }
        }
    }

    for (int layer = 1; layer < h_nodes.size(); ++layer)
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

    yCordinateGap = roofHeight + 10 + (gapHeight / 2) + (rand() % (int)(windowHeight - groundHeight - roofHeight - gapHeight - 20));
}

void Pipe::update(float deltaTime)
{
    xCordinate -= Xspeed * deltaTime;

    if (goingDown)
        yCordinateGap += Yspeed * deltaTime;
    else
        yCordinateGap -= Yspeed * deltaTime;

    if (yCordinateGap + gapHeight / 2 + 10 >= windowHeight - groundHeight)
    {
        goingDown = !goingDown;
    }
    if (yCordinateGap - gapHeight / 2 - 10 <= roofHeight)
    {
        goingDown = !goingDown;
    }
}

SDL_FRect Pipe::getTopRect()
{
    return {xCordinate, roofHeight, width, yCordinateGap - gapHeight / 2 - roofHeight};
}

SDL_FRect Pipe::getBottomRect()
{
    return {xCordinate, yCordinateGap + gapHeight / 2, width, windowHeight - (yCordinateGap + gapHeight / 2) - groundHeight};
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

float Pipe::getGapHeight()
{
    return gapHeight;
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
        population.push_back(Bird(10, {11, 11}, 1));
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
    ++start;

    for (; start < populationSize / 3; ++start)
    {
        Bird child = elite1;
        child.mutate(mutationRate);
        newGeneration.push_back(child);
    }

    Bird elite2 = population[topElitism[1]];
    elite2.reset();
    newGeneration.push_back(elite2);
    ++start;

    for (; start < (populationSize / 3) * 2; ++start)
    {
        Bird child = elite2;
        child.mutate(mutationRate);
        newGeneration.push_back(child);
    }

    Bird elite3 = population[topElitism[2]];
    elite3.reset();
    newGeneration.push_back(elite3);
    ++start;

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
    srand(static_cast<unsigned int>(time(NULL)));

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

    SDL_Surface *icon = IMG_Load("./Resources/Image/Icon.png");
    if (!icon)
    {
        SDL_Log("IMG_Load Error: %s", SDL_GetError());
    }
    else
    {
        SDL_SetWindowIcon(window, icon);
        SDL_DestroySurface(icon);
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
    survivalFrames = 0;
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

            std::vector<std::vector<double>> rayCollection;

            for (int i = 0; i < populationSize; ++i)
            {
                Bird &bird = population.getPopulation()[i];

                if (bird.getGameOver())
                    continue;

                Ray ray[RAYS_NUMBER];
                generate_rays(bird, ray, nearest);

                std::vector<float> input(RAYS_NUMBER);
                for (int r = 0; r < RAYS_NUMBER; ++r)
                {
                    rayCollection.push_back({ray[r].startX, ray[r].startY, ray[r].endX, ray[r].endY});
                    input[r] = sqrt(pow(ray[r].endX - ray[r].startX, 2) + pow(ray[r].endY - ray[r].startY, 2));
                }

                if (bird.feedForward(input)[0] > 0.5f)
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
            renderRays(rayCollection);
            renderPipes();
            renderRoof();
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
    SDL_Texture *back = IMG_LoadTexture(renderer, "./Resources/Image/Background.png");
    SDL_FRect background;
    background.w = windowWidth;
    background.h = windowHeight;
    background.x = 0;
    background.y = 0;

    SDL_RenderTexture(renderer, back, NULL, &background);
    SDL_DestroyTexture(back);
}

void Game::renderPipes()
{
    SDL_Texture *pipeT = IMG_LoadTexture(renderer, "./Resources/Image/Top_Pipe.png");
    SDL_Texture *pipeB = IMG_LoadTexture(renderer, "./Resources/Image/Bottom_Pipe.png");

    for (auto &pipe : pipes)
    {
        SDL_FRect topPipe = pipe.getTopRect();
        SDL_FRect bottomPipe = pipe.getBottomRect();

        SDL_RenderTexture(renderer, pipeT, NULL, &topPipe);
        SDL_RenderTexture(renderer, pipeB, NULL, &bottomPipe);
    }

    SDL_DestroyTexture(pipeT);
    SDL_DestroyTexture(pipeB);
}

void Game::renderRays(std::vector<std::vector<double>> rayCollection)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < rayCollection.size(); ++i)
    {
        SDL_FPoint start = {static_cast<float>(rayCollection[i][0]), static_cast<float>(rayCollection[i][1])};
        SDL_FPoint end = {static_cast<float>(rayCollection[i][2]), static_cast<float>(rayCollection[i][3])};

        SDL_RenderLine(renderer, start.x, start.y, end.x, end.y);
    }
}

void Game::renderBirds()
{
    int i = 0;
    SDL_Texture *bird1 = IMG_LoadTexture(renderer, "./Resources/Image/Bird_1.png");
    for (; i < populationSize / 3; ++i)
    {
        Bird &bird = population.getPopulation()[i];
        if (bird.getGameOver())
            continue;

        SDL_FRect birdBody = bird.getRect();
        SDL_RenderTexture(renderer, bird1, NULL, &birdBody);
    }
    SDL_DestroyTexture(bird1);

    SDL_Texture *bird2 = IMG_LoadTexture(renderer, "./Resources/Image/Bird_2.png");
    for (; i < populationSize * 2 / 3; ++i)
    {
        Bird &bird = population.getPopulation()[i];
        if (bird.getGameOver())
            continue;

        SDL_FRect birdBody = bird.getRect();
        SDL_RenderTexture(renderer, bird2, NULL, &birdBody);
    }
    SDL_DestroyTexture(bird2);

    SDL_Texture *bird3 = IMG_LoadTexture(renderer, "./Resources/Image/Bird_3.png");
    for (; i < populationSize; ++i)
    {
        Bird &bird = population.getPopulation()[i];
        if (bird.getGameOver())
            continue;

        SDL_FRect birdBody = bird.getRect();
        SDL_RenderTexture(renderer, bird3, NULL, &birdBody);
    }
    SDL_DestroyTexture(bird3);
}

void Game::renderRoof()
{
    SDL_Texture *roof = IMG_LoadTexture(renderer, "./Resources/Image/Top_Wall.png");
    SDL_FRect roofTop;
    roofTop.w = windowWidth;
    roofTop.h = roofHeight;
    roofTop.x = 0;
    roofTop.y = 0;

    SDL_RenderTexture(renderer, roof, NULL, &roofTop);
    SDL_DestroyTexture(roof);
}

void Game::renderGround()
{
    SDL_Texture *ground = IMG_LoadTexture(renderer, "./Resources/Image/Bottom_Wall.png");
    SDL_FRect groundBottom;
    groundBottom.w = windowWidth;
    groundBottom.h = groundHeight;
    groundBottom.x = 0;
    groundBottom.y = windowHeight - groundHeight;

    SDL_RenderTexture(renderer, ground, NULL, &groundBottom);
    SDL_DestroyTexture(ground);
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