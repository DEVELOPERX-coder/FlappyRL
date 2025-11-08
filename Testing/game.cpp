#include <SDL3/SDL.h>
#include <iostream>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <cmath>
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

    float w1 = 0;
    float w2 = 0;
    float w3 = 0;
    float b = 0;

    Bird(float Nw1, float Nw2, float Nw3, float Nb)
    {
        w1 = Nw1;
        w2 = Nw2;
        w3 = Nw3;
        b = Nb;
    }

    void update(float deltaTime)
    {
        velocity += gravity * deltaTime;
        y += velocity * deltaTime;
    }

    void flap()
    {
        velocity = jumpStrength;
    }

    SDL_FRect getRect()
    {
        return {(x - size / 2), (y - size / 2), (float)size, (float)size};
    }
};

struct Pipe
{
    float x;
    float width = 60;

    float gapSize = 100;
    float gapHeight = 200;

    float speed = 200;
    bool scored = false;

    Pipe(float startX) : x(startX)
    {
        gapHeight = 200 + (rand() % 300);
    }

    void update(float deltaTime)
    {
        x -= speed * deltaTime;
    }

    SDL_FRect getTopRect()
    {
        return {x, 0, width, (gapHeight - gapSize / 2)};
    }

    SDL_FRect getBottomRect()
    {
        return {x, (gapHeight + gapSize / 2), width, (600 - (gapHeight + gapSize / 2))};
    }

    bool isOffScreen()
    {
        return x + width < 0;
    }

    bool hasPassedBird(float birdX)
    {
        return !scored && x + width < birdX;
    }
};

class Game
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;

    int windowWidth = 800;
    int windowHeight = 600;

    int frameCount;
    float pipeSpawnTimer;
    float pipeSpawnInterval = 2.8f;
    int generation = 0;

    vector<Pipe> pipes;
    vector<Bird> birds;

    float w1 = 0.8;
    float w2 = -1;
    float w3 = 0;
    float b = 0;

    float n = 0.01;

    bool InitializeSDL()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            cout << "SDL Initialization Error: " << SDL_GetError() << endl;
            return false;
        }

        window = SDL_CreateWindow("Flappy Bird", windowWidth, windowHeight, 0);
        if (!window)
        {
            cout << "Window Creation Error: " << SDL_GetError() << endl;
            SDL_Quit();
            return false;
        }

        renderer = SDL_CreateRenderer(window, NULL);
        if (!renderer)
        {
            cout << "Renderer Creation Error: " << SDL_GetError() << endl;
            SDL_Quit();
            return false;
        }

        return true;
    }

    void reset()
    {
        w1 = 0.75;
        w2 = -0.8;
        w3 = 0;
        b = 0;
        birds.clear();

        birds.push_back(Bird(w1, w2, w3, b));

        // // Create variations for w1
        // for (float i = -1; i <= 1; i += 0.1)
        // {
        //     Bird newBird(w1 + i, w2, w3, b);
        //     birds.push_back(newBird);
        // }

        // // Create variations for w2
        // for (float i = -1; i <= 1; i += 0.1)
        // {
        //     Bird newBird(w1, w2 + i, w3, b);
        //     birds.push_back(newBird);
        // }

        // // Create variations for w3
        // for (float i = -1; i <= 1; i += 0.1)
        // {
        //     Bird newBird(w1, w2, w3 + i, b);
        //     birds.push_back(newBird);
        // }

        // // Create variations for b
        // for (float i = -1; i <= 1; i += 0.1)
        // {
        //     Bird newBird(w1, w2, w3, b + i);
        //     birds.push_back(newBird);
        // }

        cout << "Generation " << generation << " started with " << birds.size() << " birds" << endl;

        pipes.clear();
        pipeSpawnTimer = 0;
        frameCount = 0;

        pipes.push_back(Pipe(800));
    }

    void handleEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
                exit(0);
            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                switch (event.key.key)
                {
                case SDLK_SPACE:
                    cout << "Pressed Space " << endl;
                    break;
                case SDLK_ESCAPE:
                    cout << "Closing The Game!" << endl;
                    exit(0);
                    break;
                }
            }
        }
    }

    void update(float deltaTime)
    {
        int maxScore = 25;
        int maxScoreIndex = rand() % birds.size();

        bool dead = true;

        for (int i = 0; i < birds.size(); ++i)
        {
            if (!birds[i].gameOver)
            {
                birds[i].update(deltaTime);
                birds[i].score++; // Increase score for survival each frame
                dead = false;

                // Neural network decision making
                if (!pipes.empty())
                {
                    float birdHeight = birds[i].y / 600;
                    float topPipeHeight = pipes[0].gapHeight / 600;
                    float bottomPipeHeight = (pipes[0].gapHeight + pipes[0].gapSize) / 600;

                    float y = (birdHeight * w1 + topPipeHeight * w2 + bottomPipeHeight * w3 + b) / 600;

                    // Simple perceptron: weighted sum + bias
                    float decision = 1 / (1 + exp(-y));

                    // If decision is positive, flap
                    if (decision > 0.5)
                    {
                        birds[i].flap();
                    }
                }

                if (birds[i].y > 580 || birds[i].y < 20)
                {
                    birds[i].gameOver = true;
                }

                if (!pipes.empty())
                {
                    SDL_FRect birdRect = birds[i].getRect();
                    SDL_FRect topPipe = pipes[0].getTopRect();
                    SDL_FRect bottomPipe = pipes[0].getBottomRect();

                    if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
                    {
                        birds[i].gameOver = true;
                    }

                    if (!pipes[0].scored && pipes[0].hasPassedBird(birds[i].x))
                    {
                        birds[i].score += 10;
                        pipes[0].scored = true;
                    }
                }
            }
            else
            {
                if (maxScore < birds[i].score)
                {
                    maxScore = birds[i].score;
                    maxScoreIndex = i;
                }
            }
        }

        if (dead)
        {
            generation++;
            cout << "Generation " << generation << " ended. Best bird score: " << maxScore << endl;
            cout << "Current weights - w1: " << w1 << ", w2: " << w2 << ", w3: " << w3 << ", b: " << b << endl;

            w1 += (birds[maxScoreIndex].w1 - w1) * n;
            w2 += (birds[maxScoreIndex].w2 - w2) * n;
            w3 += (birds[maxScoreIndex].w3 - w3) * n;
            b += (birds[maxScoreIndex].b - b) * n;

            cout << "New weights - w1: " << w1 << ", w2: " << w2 << ", w3: " << w3 << ", b: " << b << endl
                 << endl;

            cout << maxScoreIndex << endl;

            reset();
        }

        pipeSpawnTimer += deltaTime;
        if (pipeSpawnTimer >= pipeSpawnInterval)
        {
            pipes.push_back(Pipe(800));
            pipeSpawnTimer = 0;
        }

        for (auto &pipe : pipes)
        {
            pipe.update(deltaTime);
        }

        pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                              { return pipe.isOffScreen(); }),
                    pipes.end());
    }

    void render()
    {
        // Gradient Sky Background
        for (int y = 0; y < 600; ++y)
        {
            float t = (float)y / 600.0f;
            int r = (int)(135 + t * (100 - 135));
            int g = (int)(206 + t * (149 - 206));
            int b = (int)(235 + t * (237 - 235));

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_FRect line = {0.0f, (float)y, 800.0f, 1.0f};
            SDL_RenderFillRect(renderer, &line);
        }

        // pipes
        for (auto &pipe : pipes)
        {
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();

            // Main Pipes
            SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255);
            SDL_RenderFillRect(renderer, &topPipe);
            SDL_RenderFillRect(renderer, &bottomPipe);
        }

        // Enhanced bird with animation
        for (auto &bird : birds)
        {
            if (!bird.gameOver) // Only render alive birds
            {
                SDL_FRect birdRect = bird.getRect();

                // Bird shadow
                SDL_SetRenderDrawColor(renderer, 180, 180, 0, 80);
                SDL_FRect shadowRect = {birdRect.x + 3, birdRect.y + 3, birdRect.w, birdRect.h};
                SDL_RenderFillRect(renderer, &shadowRect);

                SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255); // Yellow for alive birds

                SDL_RenderFillRect(renderer, &birdRect);

                // Bird details
                SDL_SetRenderDrawColor(renderer, 255, 255, 150, 255);
                SDL_FRect highlight = {birdRect.x + 6, birdRect.y + 6, birdRect.w - 12, birdRect.h - 18};
                SDL_RenderFillRect(renderer, &highlight);
            }
        }

        SDL_RenderPresent(renderer);
    }

public:
    Game() : window(nullptr), renderer(nullptr), frameCount(0), pipeSpawnTimer(0)
    {
        InitializeSDL();
        reset();
    }

    void run()
    {
        if (!window || !renderer)
        {
            cout << "Failed to Initialize SDL properly. Exiting." << endl;
            return;
        }

        Uint64 lastTime = SDL_GetTicks();

        bool running = true;
        while (running)
        {
            try
            {
                Uint64 currentTime = SDL_GetTicks();
                float deltaTime = (currentTime - lastTime) / 1000.0f;

                if (deltaTime > 0.1f)
                    deltaTime = 0.1f;

                lastTime = currentTime;

                handleEvents();
                update(deltaTime);
                render();
                SDL_Delay(33.33); // ~30FPS = 1000ms / 33.33ms wait
            }
            catch (...)
            {
                cout << "Error in game loop, continuing..." << endl;
                SDL_Delay(100);
            }
        }
    }

    ~Game()
    {
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    Game game;
    game.run();

    return 0;
}