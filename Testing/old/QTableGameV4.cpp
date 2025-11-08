#include <SDL3/SDL.h> // To use SDL
#include <vector>
#include <iostream>  // To use cout , cin
#include <fstream>   // To use ofstream, ifstream
#include <windows.h> // To use SetConsoleOutputCP
#include <chrono>    // To use clock
#include <algorithm> // To use remove_if
#include <cmath>     // To use sin

using namespace std;

struct Bird
{
    float x = 100;
    float y = 300;

    float velocity = 0;
    float gravity = 800;
    float jumpStrength = -400;

    int size = 20;

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
    float gapHeight = 180;
    float Ygap;
    float speed = 200;
    bool scored = false;

    Pipe(float startX) : x(startX)
    {
        Ygap = 180 + (rand() % 240);
    }

    void update(float deltaTime)
    {
        x -= speed * deltaTime;
    }

    SDL_FRect getTopRect()
    {
        return {x, 0, width, (Ygap - gapHeight / 2)};
    }

    SDL_FRect getBottomRect()
    {
        return {x, (Ygap + gapHeight / 2), width, (600 - (Ygap + gapHeight / 2))};
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

class QTable
{
private:
    vector<vector<vector<bool>>> decisionTable;

public:
    QTable() : decisionTable(800, vector<vector<bool>>(600, vector<bool>(800, 0))) {}

    void saveModel(const string &filename)
    {
        ofstream file(filename);

        for (int yB = 0; yB < 800; ++yB)
        {
            for (int x = 0; x < 600; ++x)
            {
                for (int yG = 0; yG < 800; ++yG)
                {
                    file << decisionTable[yB][x][yG];
                }
            }
        }

        file.close();
    }

    bool loadModel(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
            return false;

        bool decision;

        for (int yB = 0; yB < 800; ++yB)
        {
            for (int x = 0; x < 600; ++x)
            {
                for (int yG = 0; yG < 800; ++yG)
                {
                    if (file >> decision)
                    {
                        decisionTable[yB][x][yG] = decision;
                    }
                }
            }
        }

        file.close();
        return true;
    }

    bool selectDecision(const vector<int> &state)
    {
        return decisionTable[state[0]][state[1]][state[2]];
    }

    void updateDecision(const vector<int> &state)
    {
        decisionTable[state[0]][state[1]][state[2]] = ~decisionTable[state[0]][state[1]][state[2]];
    }
};

void trainQTable(int episodes)
{
    string QTableName = "FlappyBirdQTable.dat";
    QTable decisionTable;
    decisionTable.loadModel(QTableName);

    int bestScore = 0;
    int bestSurvival = 0;
    int totalFrames = 0;

    auto startTime = chrono::high_resolution_clock::now();

    cout << "🚀 Starting Flappy Bird QTable Training..." << endl;
    cout << "📊 Episodes: " << episodes << endl;
    cout << "══════════════════════════════════════════════════════════════════════════" << endl;

    SDL_Window *window;
    SDL_Renderer *renderer;

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        cout << "SDL Error: " << SDL_GetError() << endl;
        return;
    }

    window = SDL_CreateWindow("Flappy Bird Game", 800, 600, 0);
    if (!window)
    {
        cout << "Window Error: " << SDL_GetError() << endl;
        SDL_Quit();
        return;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        cout << "Renderer Error: " << SDL_GetError() << endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    for (int episode = 0; episode < episodes; ++episode)
    {
        cout << "📈 Ep: " << episode + 1 << endl;

        Bird bird;
        vector<Pipe> pipes = {Pipe(500)};

        int score = 0;
        bool gameOver = false;
        bool gameStarted = true;
        float pipeSpawnTimer = 0;
        float pipeSpawnInterval = 2.8f;
        int frameCount = 0;
        int survivalFrames = 0;

        int running = 1;

        vector<int> state = {(int)bird.y, (int)(pipes[0].x - bird.x), (int)(pipes[0].Ygap)};
        int steps = 0;
        Uint64 lastTime = SDL_GetTicks();

        while (running && steps < 20000)
        {
            try
            {
                Uint64 currentTime = SDL_GetTicks();
                float deltaTime = (currentTime - lastTime) / 1000.0f;

                if (deltaTime > 0.1f)
                    deltaTime = 0.1f;

                lastTime = currentTime;

                bool action = decisionTable.selectDecision(state);
                if (action)
                    bird.flap();
                bird.update(deltaTime);

                if (bird.y > 580 || bird.y < 20)
                {
                    gameOver = true;
                    running = 0;
                    decisionTable.updateDecision(state);
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

                    SDL_FRect birdRect = bird.getRect();
                    SDL_FRect topPipe = pipe.getTopRect();
                    SDL_FRect bottomPipe = pipe.getBottomRect();

                    if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
                    {
                        gameOver = true;
                        running = 0;
                        decisionTable.updateDecision(state);
                    }

                    if (pipe.hasPassedBird(bird.x))
                    {
                        ++score;
                        pipe.scored = true;
                    }
                }

                pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                                      { return pipe.isOffScreen(); }),
                            pipes.end());

                state[0] = bird.y;
                Pipe *nextPipe = nullptr;
                for (auto &pipe : pipes)
                {
                    if (pipe.x + pipe.width > bird.x)
                    {
                        nextPipe = &pipe;
                        state[1] = pipe.x - bird.x;
                        state[2] = pipe.Ygap;
                        break;
                    }
                }
                ++steps;

                SDL_SetRenderDrawColor(renderer, 32, 32, 70, 255);
                SDL_RenderClear(renderer);

                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_FRect birdBody = bird.getRect();
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
            catch (...)
            {
                cout << "Error in game loop..." << endl;
                SDL_Delay(100);
            }
        }

        totalFrames += frameCount;

        if (score > bestScore)
            bestScore = score;
        if (survivalFrames > bestSurvival)
            bestSurvival = survivalFrames;

        auto currentTime = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::seconds>(currentTime - startTime);

        cout << " | Best Score: " << bestScore << endl;
        cout << " | Time: " << duration.count() << "s" << endl;
    }

    decisionTable.saveModel(QTableName);

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::minutes>(endTime - startTime);

    cout << "══════════════════════════════════════════════════════════════════════════" << endl;
    cout << "✅ TRAINING COMPLETED!" << endl;
    cout << "🏆 Best Score: " << bestScore << endl;
    cout << "🥇 Best Survival: " << bestSurvival << " frames" << endl;
    cout << "🎮 Total Frames: " << totalFrames << endl;
    cout << "⏱️  Training Time: " << totalTime.count() << " minutes" << endl;
    cout << "💾 Model saved: " << QTableName << endl;

    if (renderer)
        SDL_DestroyRenderer(renderer);
    if (window)
        SDL_DestroyWindow(window);
    SDL_Quit();
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    cout << "🎮 ═══════════════════════════════════════════════════════════════════════════" << endl;
    cout << "🐦 FLAPPY BIRD AI - Training Program" << endl;
    cout << "🧠 Q-Learning with State Representation & Reward Shaping" << endl;
    cout << "   ═══════════════════════════════════════════════════════════════════════════" << endl;

    cout << "Run's are total training Game" << endl;
    cout << "Enter Number of Runs : " << endl;

    string runs;
    cin >> runs;

    int episodes = stoi(runs);
    if (episodes <= 0)
    {
        cout << "Error: Number of episodes must be positive" << endl;
        return 1;
    }
    if (episodes > 100000)
    {
        cout << "Error: Number of episodes must be < 1e5" << endl;
        return 1;
    }

    cout << "🎯 Starting training with " << episodes << " episodes..." << endl;
    trainQTable(episodes);

    cout << "\n🙏 Training completed!" << endl;
    return 0;
}