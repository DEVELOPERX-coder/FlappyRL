#include <SDL3/SDL.h>
#include <iostream> // To use rand()
#include <vector>
#include <algorithm>

using namespace std;

class Bird
{
public:
    float xCord = 300;
    float yCord = 200;
    int score = 0;
    int birdSize = 50;
    float flapForce = -400.0f;
    float gravity = 800.0f;
    float velocity = 0.0f;

public:
    Bird() {}

    void flap()
    {
        velocity = flapForce;
    }

    void update(float deltaTime)
    {
        velocity += gravity * deltaTime;
        yCord += velocity * deltaTime;
    }

    SDL_FRect body()
    {
        SDL_FRect birdBody;
        birdBody.x = xCord - birdSize / 2;
        birdBody.y = yCord - birdSize / 2;
        birdBody.w = birdSize;
        birdBody.h = birdSize;
        return birdBody;
    }
};

class Pipe
{
public:
    float xCord;
    float width = 60;
    float gapSize = 180;
    float gapHeight = 300;
    float speed = 200;
    bool scored = false;

public:
    Pipe(float startX) : xCord(startX)
    {
        gapHeight = 180 + (rand() % 240);
    }

    void update(float deltaTime)
    {
        xCord -= speed * deltaTime;
    }

    SDL_FRect getTopRect()
    {
        SDL_FRect topPipe;
        topPipe.x = (int)(xCord) - (width / 2);
        topPipe.y = 0;
        topPipe.w = width;
        topPipe.h = (int)(gapHeight) - (gapSize / 2);
        return topPipe;
    }

    SDL_FRect getBottomRect()
    {
        SDL_FRect bottomPipe;
        bottomPipe.x = (int)(xCord) - (width / 2);
        bottomPipe.y = gapHeight + gapSize / 2;
        bottomPipe.w = width;
        bottomPipe.h = 600 - ((int)(gapHeight) + (gapSize / 2));
        return bottomPipe;
    }

    bool isOffScreen()
    {
        return xCord + width / 2 < 0;
    }

    bool hasPassedBird(float birdX)
    {
        return !scored && xCord + width < birdX;
    }
};

int main()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Unable to Initialize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Flappy Bird Game", 800, 600, 0);
    if (window == NULL)
    {
        SDL_Log("Unable to create Window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL)
    {
        SDL_Log("Unable to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Uint64 perSecTicks = SDL_GetPerformanceFrequency();
    Uint64 lastTick = SDL_GetPerformanceCounter();
    float deltaTime = 0.0f;

    int running = 1;
    SDL_Event event;

    const Uint8 *keyState = (const Uint8 *)SDL_GetKeyboardState(NULL);

    Bird bird;
    vector<Pipe> pipes;
    float pipeSpawnTimer = 0;
    float pipeSpawnInterval = 2.8f;

    while (running)
    {
        Uint64 currentTick = SDL_GetPerformanceCounter();
        deltaTime = (float)(currentTick - lastTick) / (float)perSecTicks;
        lastTick = currentTick;

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

        bird.update(deltaTime);

        if (bird.yCord > 580 || bird.yCord < 20)
        {
            cout << bird.yCord << endl;
            SDL_Log("Game Over");
            return 1;
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

            SDL_FRect birdRect = bird.body();
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();

            if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
            {
                cout << "Bird Collided with Piller" << endl;
                return 1;
            }

            if (pipe.hasPassedBird(bird.xCord))
            {
                ++bird.score;
                pipe.scored = true;
            }
        }

        pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                              { return pipe.isOffScreen(); }),
                    pipes.end());

        SDL_SetRenderDrawColor(renderer, 32, 32, 70, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_FRect birdBody = bird.body();
        SDL_RenderFillRect(renderer, &birdBody);

        for (auto &pipe : pipes)
        {
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();

            SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255);
            SDL_RenderFillRect(renderer, &topPipe);
            SDL_RenderFillRect(renderer, &bottomPipe);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    SDL_Log("SDL shut down successfully");
    return 0;
}