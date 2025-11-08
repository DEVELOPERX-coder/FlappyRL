#include <iostream>
#include <windows.h>
#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib> // For random numbers
#include <ctime>   // To seed the random number generator
#include <numeric> // For std::exp

using namespace std;

struct Bird
{
    float x = 100;
    float y = 300;

    int size = 20;

    float velocity = 0;
    float gravity = 800;
    float jumpStrength = -400;

    int score = 0;
    bool gameOver = false;

    Bird() {}

    void flap()
    {
        velocity = jumpStrength;
    }

    void update(float deltaTime)
    {
        velocity += gravity * deltaTime;
        y += velocity * deltaTime;
    }

    SDL_FRect body()
    {
        return {x - (size >> 1), y - (size >> 1), (float)size, (float)size};
    }
};

struct Pipe
{
    float x;
    float width = 60;
    float gapHeight = 180;
    float gapY;
    float speed = 200;
    bool scored = false;

    Pipe(float startX) : x(startX)
    {
        gapY = 180 + (rand() % 240);
    }

    void update(float deltaTime)
    {
        x -= speed * deltaTime;
    }

    SDL_FRect getTopRect()
    {
        return {x, 0, width, gapY - gapHeight / 2};
    }

    SDL_FRect getBottomRect(float windowHeight)
    {
        return {x, gapY + gapHeight / 2, width, windowHeight - (gapY + gapHeight / 2)};
    }

    bool isOffScreen()
    {
        return x + width < 0;
    }

    bool hasPassedBird(float birdx)
    {
        return !scored && x + width < birdx;
    }
};

struct GameState
{
    // The 3 inputs you specified
    float birdY;
    float pipeGapY;
    float horizontalDistToPipe;

    // Extra info for the learning algorithm
    int score;
    bool gameOver;
};

class NeuralNetwork
{
private:
    int i_nodes;
    int h_nodes;
    int o_nodes;

    // Weights
    std::vector<std::vector<float>> weights_ih; // Input -> Hidden
    std::vector<std::vector<float>> weights_ho; // Hidden -> Output

    // Biases
    std::vector<float> bias_h; // Hidden layer
    std::vector<float> bias_o; // Output layer

public:
    NeuralNetwork(int inputNodes, int hiddenNodes, int outputNodes)
    {
        i_nodes = inputNodes;
        h_nodes = hiddenNodes;
        o_nodes = outputNodes;

        // Seed the random number generator once
        static bool seeded = false;
        if (!seeded)
        {
            srand(static_cast<unsigned>(time(0)));
            seeded = true;
        }

        // Initialize weights_ih (Input -> Hidden)
        weights_ih.resize(h_nodes, std::vector<float>(i_nodes));
        for (int i = 0; i < h_nodes; ++i)
        {
            for (int j = 0; j < i_nodes; ++j)
            {
                weights_ih[i][j] = randomFloat();
            }
        }

        // Initialize weights_ho (Hidden -> Output)
        weights_ho.resize(o_nodes, std::vector<float>(h_nodes));
        for (int i = 0; i < o_nodes; ++i)
        {
            for (int j = 0; j < h_nodes; ++j)
            {
                weights_ho[i][j] = randomFloat();
            }
        }

        // Initialize bias_h (Hidden biases)
        bias_h.resize(h_nodes);
        for (int i = 0; i < h_nodes; ++i)
        {
            bias_h[i] = randomFloat();
        }

        // Initialize bias_o (Output biases)
        bias_o.resize(o_nodes);
        for (int i = 0; i < o_nodes; ++i)
        {
            bias_o[i] = randomFloat();
        }
    }

    std::vector<float> feedForward(const std::vector<float> &inputs)
    {
        // --- STAGE 1: Calculate Hidden Layer ---
        std::vector<float> hidden_outputs(h_nodes);
        for (int i = 0; i < h_nodes; ++i)
        {
            float weightedSum = 0.0f;
            for (int j = 0; j < i_nodes; ++j)
            {
                weightedSum += weights_ih[i][j] * inputs[j];
            }
            weightedSum += bias_h[i];
            hidden_outputs[i] = sigmoid(weightedSum);
        }

        // --- STAGE 2: Calculate Output Layer ---
        std::vector<float> final_outputs(o_nodes);
        for (int i = 0; i < o_nodes; ++i)
        {
            float weightedSum = 0.0f;
            for (int j = 0; j < h_nodes; ++j)
            {
                // Your correct line:
                weightedSum += weights_ho[i][j] * hidden_outputs[j];
            }
            weightedSum += bias_o[i];
            final_outputs[i] = sigmoid(weightedSum);
        }

        return final_outputs;
    }

    void mutate(float mutationRate)
    {
        // 1. Loop through Input -> Hidden weights
        for (int i = 0; i < h_nodes; ++i)
        {
            for (int j = 0; j < i_nodes; ++j)
            {
                // "Roll the dice"
                if (randomChance() < mutationRate)
                {
                    // "Nudge" the weight by a small random value
                    weights_ih[i][j] += randomFloat() * 0.1;
                }
            }
        }

        // 2. Loop through Hidden -> Output weights
        for (int i = 0; i < o_nodes; ++i)
        {
            for (int j = 0; j < h_nodes; ++j)
            {
                if (randomChance() < mutationRate)
                {
                    weights_ho[i][j] += randomFloat() * 0.1;
                }
            }
        }

        // 3. Loop through Hidden biases
        for (int i = 0; i < h_nodes; ++i)
        {
            if (randomChance() < mutationRate)
            {
                bias_h[i] += randomFloat() * 0.1;
            }
        }

        // 4. Loop through Output biases
        for (int i = 0; i < o_nodes; ++i)
        {
            if (randomChance() < mutationRate)
            {
                bias_o[i] += randomFloat() * 0.1;
            }
        }
    }

private:
    float sigmoid(float x)
    {
        return 1.0f / (1.0f + std::exp(-x));
    }

    float randomFloat()
    {
        return ((float)rand() / (RAND_MAX)) * 2.0f - 1.0f; // -1.0 to 1.0
    }

    // A new helper for a random number between 0.0 and 1.0
    float randomChance()
    {
        return (float)rand() / (RAND_MAX);
    }
};

struct BirdAgent
{
    NeuralNetwork brain;
    int fitness = 0;

    // Your logic goes right here:
    BirdAgent(int i_nodes, int h_nodes, int o_nodes) : brain(i_nodes, h_nodes, o_nodes)
    {
        // The body can be empty because the initializer list did all the work!
    }
};

class Population
{
private:
    std::vector<BirdAgent> population;
    int generationNumber = 1;
    float mutationRate;
    int populationSize;

public:
    Population(int size, float mRate)
    {
        populationSize = size;
        mutationRate = mRate;

        for (int i = 0; i < populationSize; ++i)
        {
            population.push_back(BirdAgent(3, 4, 1));
        }
    }

    // A function to get the whole population so we can run the game
    std::vector<BirdAgent> &getPopulation()
    {
        return population;
    }

    void evolveNewGeneration()
    {
        // 1. SELECTION (Find the best parent)
        std::sort(population.begin(), population.end(), [&](const BirdAgent &a, const BirdAgent &b)
                  {
                      return a.fitness > b.fitness; // Sorts best to worst
                  });

        std::cout << "Generation: " << generationNumber;
        std::cout << " | Best Fitness: " << population[0].fitness << std::endl;

        // 2. REPRODUCTION
        // Create a new generation, filling it with copies of the best bird
        std::vector<BirdAgent> newGeneration(populationSize, population[0]);

        // 3. MUTATION (with elitism)
        // We skip i = 0 to keep the best bird un-mutated
        for (int i = 1; i < populationSize; ++i)
        {
            newGeneration[i].brain.mutate(mutationRate);
        }

        // 4. REPLACE
        // Replace the old population with the new one
        population = newGeneration;
        generationNumber++;
    }
};

class Game
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;

    int windowWidth = 800;
    int windowHeight = 600;

    Bird bird;
    vector<Pipe> pipes = {Pipe(800)};

    float pipeSpawnTimer = 0;
    float pipeSpawnInterval = 2.8f;

public:
    Game()
    {
        Init();
    }

    void Init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            SDL_Log("Unable to Initialize SDL: %s", SDL_GetError());
            return;
        }
        window = SDL_CreateWindow("Flappy Bird Game", windowWidth, windowHeight, 0);
        if (!window)
        {
            SDL_Log("Unable to Initialize Window: %s", SDL_GetError());
            SDL_Quit();
            return;
        }
        renderer = SDL_CreateRenderer(window, NULL);
        if (!renderer)
        {
            SDL_Log("Unable to Initialize Renderer: %s", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            return;
        }
    }

    void resetGame()
    {
        bird = Bird();              // Re-create the bird
        pipes.clear();              // Clear all pipes
        pipes.push_back(Pipe(800)); // Add the first pipe
        pipeSpawnTimer = 0;
    }

    void run()
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
                deltaTime = (float)(currentTime - lastTime) / 1000.0f;
                lastTime = currentTime;

                while (SDL_PollEvent(&event))
                {
                    if (event.type == SDL_EVENT_QUIT)
                    {
                        running = 0;
                    }
                    if (event.type == SDL_EVENT_KEY_DOWN)
                    {
                        SDL_Keycode keyPressed = event.key.key;
                        if (keyPressed == SDLK_SPACE)
                        {
                            bird.flap();
                        }
                    }
                }

                if (bird.y > windowHeight - 80 || bird.y < 20)
                {
                    SDL_Log("Game Ended");
                    bird.gameOver = true;
                    running = 0;
                }

                bird.update(deltaTime);

                pipeSpawnTimer += deltaTime;
                if (pipeSpawnTimer >= pipeSpawnInterval)
                {
                    pipes.push_back(Pipe(800));
                    pipeSpawnTimer = 0;
                }

                for (auto &pipe : pipes)
                {
                    pipe.update(deltaTime);

                    SDL_FRect birdRect = bird.body();
                    SDL_FRect topPipe = pipe.getTopRect();
                    SDL_FRect bottomPipe = pipe.getBottomRect(windowHeight);

                    if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
                    {
                        bird.gameOver = true;
                        running = 0;
                    }

                    if (pipe.hasPassedBird(bird.x))
                    {
                        ++bird.score;
                        pipe.scored = true;
                    }
                }

                pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                                      { return pipe.isOffScreen(); }),
                            pipes.end());

                renderBackground();

                for (auto &pipe : pipes)
                {
                    SDL_FRect topPipe = pipe.getTopRect();
                    SDL_FRect bottomPipe = pipe.getBottomRect(windowHeight);

                    SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255);
                    SDL_RenderFillRect(renderer, &topPipe);
                    SDL_RenderFillRect(renderer, &bottomPipe);
                }

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_FRect birdBody = bird.body();
                SDL_RenderFillRect(renderer, &birdBody);

                SDL_RenderPresent(renderer);

                SDL_Delay(33.3333); // for 30fps
            }
            catch (...)
            {
                SDL_Log("Error in game loop...: %s", SDL_GetError());
                SDL_Delay(100);
            }
        }
    }

    void renderBackground()
    {
        SDL_Color color1 = {30, 15, 117, 255};   // #1e0f75
        SDL_Color color2 = {55, 133, 216, 255};  // #3785d8
        SDL_Color color3 = {173, 198, 229, 255}; // #adc6e5

        for (int y = 0; y < windowHeight - 80; ++y)
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

        SDL_Color ground = {34, 139, 34, 255};
        SDL_Color durt = {234, 208, 168};

        for (int y = windowHeight - 80; y < windowHeight - 60; ++y)
        {
            SDL_SetRenderDrawColor(renderer, ground.r, ground.g, ground.b, ground.a);
            SDL_RenderLine(renderer, 0, y, windowWidth, y);
        }
        for (int y = windowHeight - 60; y < windowHeight; ++y)
        {
            SDL_SetRenderDrawColor(renderer, durt.r, durt.g, durt.b, durt.a);
            SDL_RenderLine(renderer, 0, y, windowWidth, y);
        }
    }

    // This function will run one "frame" of the game
    GameState gameStep(bool shouldFlap)
    {
        // We'll use a fixed delta time for the AI
        // to make the simulation consistent
        float fixedDeltaTime = 1.0f / 60.0f; // 60 FPS

        if (shouldFlap)
        {
            bird.flap();
        }

        // --- All this logic is moved from your run() loop ---
        bird.update(fixedDeltaTime);

        bool isGameOver = (bird.y > windowHeight - 80 || bird.y < 20);

        pipeSpawnTimer += fixedDeltaTime;
        if (pipeSpawnTimer >= pipeSpawnInterval)
        {
            pipes.push_back(Pipe(800));
            pipeSpawnTimer = 0;
        }

        Pipe *nextPipe = nullptr;
        for (auto &pipe : pipes)
        {
            pipe.update(fixedDeltaTime);

            // Find the closest pipe that is in front of the bird
            if (pipe.x + pipe.width > bird.x)
            {
                nextPipe = &pipe;
                break; // Found the next pipe
            }

            SDL_FRect birdRect = bird.body();
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect(windowHeight);

            if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
            {
                isGameOver = true;
            }

            if (pipe.hasPassedBird(bird.x))
            {
                ++bird.score;
                pipe.scored = true;
            }
        }

        pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                              { return pipe.isOffScreen(); }),
                    pipes.end());
        // --- End of moved logic ---

        bird.gameOver = isGameOver;

        // --- Prepare the GameState to return ---
        GameState state;
        state.birdY = bird.y;
        state.score = bird.score;
        state.gameOver = bird.gameOver;

        if (nextPipe != nullptr)
        {
            state.pipeGapY = nextPipe->gapY;
            state.horizontalDistToPipe = nextPipe->x - bird.x;
        }
        else
        {
            // No pipes on screen, just return default values
            state.pipeGapY = windowHeight / 2.0f;
            state.horizontalDistToPipe = windowWidth; // Far away
        }

        return state;
    }

    ~Game()
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
        SDL_Log("SDL shut down successfully.");
    }
};

// int main()
// {
//     SetConsoleOutputCP(CP_UTF8);

//     Game game;
//     game.run();

//     return 0;
// }

int main()
{
    // (Include Game.h, Population.h, etc.)

    // --- 1. SETUP ---
    const int POPULATION_SIZE = 50;
    const float MUTATION_RATE = 0.01; // 1% mutation rate
    const int MAX_GENERATIONS = 100;

    // Create the game environment
    Game game;

    // Create the population of birds
    Population population(POPULATION_SIZE, MUTATION_RATE);

    // --- 2. MAIN TRAINING LOOP ---
    for (int gen = 0; gen < MAX_GENERATIONS; ++gen)
    {
        // Get the current list of birds
        std::vector<BirdAgent> &currentBirds = population.getPopulation();

        // --- 3. SIMULATION LOOP (for each bird) ---
        for (BirdAgent &bird : currentBirds)
        {
            game.resetGame();
            bird.fitness = 0;

            // Your idea: Get the very first state
            GameState currentState = game.gameStep(false);

            // This is the individual bird's game loop
            while (!currentState.gameOver)
            {
                // --- 4. THE "THINKING" PART ---

                // A. Prepare inputs for the neural network
                std::vector<float> inputs = {
                    currentState.birdY,
                    currentState.pipeGapY,
                    currentState.horizontalDistToPipe};

                // B. Ask the bird's brain for a decision
                // How do we call the brain's feedForward function?
                std::vector<float> outputs = bird.brain.feedForward(inputs);

                // C. Decide if the output means "flap"
                // (Let's say output > 0.5 means flap)
                bool shouldFlap = (outputs[0] > 0.5f);

                // D. Step the game forward with that decision
                //    and get the *new* state for the *next* loop
                currentState = game.gameStep(shouldFlap);

                // E. Update fitness
                bird.fitness = currentState.score;
            }
        }

        // --- 5. EVOLUTION ---
        // All birds have played. Time to evolve!
        population.evolveNewGeneration();
    }
}