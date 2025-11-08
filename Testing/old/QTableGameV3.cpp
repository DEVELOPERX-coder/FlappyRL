#include <SDL3/SDL.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <windows.h>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <string> // Added for stoi
#include <array>  // A good choice for fixed-size Q-tables

using namespace std;

// --- State Discretization Constants ---
// We will "bin" our continuous state into a smaller, discrete table.
// This makes the state space much smaller and allows the AI to learn.

// Horizontal distance to pipe: e.g., map -100px to 500px into 30 bins
const int H_DIST_BINS = 30;
const float H_DIST_MIN = -100.0f;
const float H_DIST_MAX = 500.0f;
const float H_DIST_BIN_SIZE = (H_DIST_MAX - H_DIST_MIN) / H_DIST_BINS; // ~20.0f

// Vertical distance to gap center: e.g., map -300px to 300px into 30 bins
const int V_DIST_BINS = 30;
const float V_DIST_MIN = -300.0f;
const float V_DIST_MAX = 300.0f;
const float V_DIST_BIN_SIZE = (V_DIST_MAX - V_DIST_MIN) / V_DIST_BINS; // ~20.0f

// Bird velocity: e.g., map -400 (jump) to 400 (falling) into 10 bins
const int VEL_BINS = 10;
const float VEL_MIN = -400.0f;
const float VEL_MAX = 400.0f;
const float VEL_BIN_SIZE = (VEL_MAX - VEL_MIN) / VEL_BINS; // ~80.0f

// Helper function to clamp a value
// template <typename T>
// T clamp(T value, T minVal, T maxVal)
// {
//     return max(minVal, min(value, maxVal));
// }

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
        // Clamp velocity to reasonable bounds
        velocity = clamp(velocity, jumpStrength, 500.0f);
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
    float Ygap; // Center of the gap
    float speed = 200;
    bool scored = false;

    Pipe(float startX) : x(startX)
    {
        // Ygap is the center of the gap
        Ygap = 120 + (rand() % (600 - 240)); // Ensure gap is not too close to edges
    }

    void update(float deltaTime)
    {
        x -= speed * deltaTime;
    }

    SDL_FRect getTopRect()
    {
        float topPipeHeight = Ygap - (gapHeight / 2.0f);
        return {x, 0, width, topPipeHeight};
    }

    SDL_FRect getBottomRect()
    {
        float bottomPipeY = Ygap + (gapHeight / 2.0f);
        float bottomPipeHeight = 600.0f - bottomPipeY;
        return {x, bottomPipeY, width, bottomPipeHeight};
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

// Represents our new, smaller, discretized state
struct State
{
    int h_dist_bin;
    int v_dist_bin;
    int vel_bin;
};

class QTable
{
private:
    // Our decision table is now sized by our bin constants
    vector<vector<vector<bool>>> decisionTable;

public:
    QTable() : decisionTable(H_DIST_BINS, vector<vector<bool>>(V_DIST_BINS, vector<bool>(VEL_BINS, 0)))
    {
        // Initialize all decisions to 0 (don't flap)
    }

    void saveModel(const string &filename)
    {
        ofstream file(filename, ios::binary);
        if (!file.is_open())
        {
            cout << "Error: Could not open file to save model: " << filename << endl;
            return;
        }

        for (int h = 0; h < H_DIST_BINS; ++h)
        {
            for (int v = 0; v < V_DIST_BINS; ++v)
            {
                for (int vel = 0; vel < VEL_BINS; ++vel)
                {
                    // Write bool as a single char
                    char decision = decisionTable[h][v][vel] ? 1 : 0;
                    file.write(&decision, sizeof(decision));
                }
            }
        }
        file.close();
    }

    bool loadModel(const string &filename)
    {
        ifstream file(filename, ios::binary);
        if (!file.is_open())
        {
            cout << "No existing model found. Starting fresh." << endl;
            return false;
        }

        for (int h = 0; h < H_DIST_BINS; ++h)
        {
            for (int v = 0; v < V_DIST_BINS; ++v)
            {
                for (int vel = 0; vel < VEL_BINS; ++vel)
                {
                    char decision;
                    if (file.read(&decision, sizeof(decision)))
                    {
                        decisionTable[h][v][vel] = (decision == 1);
                    }
                    else
                    {
                        cout << "Error reading model file or file is corrupt. Starting fresh." << endl;
                        file.close();
                        // Reset table if file is corrupt
                        decisionTable = vector<vector<vector<bool>>>(H_DIST_BINS, vector<vector<bool>>(V_DIST_BINS, vector<bool>(VEL_BINS, 0)));
                        return false;
                    }
                }
            }
        }
        file.close();
        cout << "Successfully loaded model: " << filename << endl;
        return true;
    }

    bool selectDecision(const State &state)
    {
        // State is already binned, so it's a safe index
        return decisionTable[state.h_dist_bin][state.v_dist_bin][state.vel_bin];
    }

    void updateDecision(const State &state)
    {
        // Flip the decision for this state
        decisionTable[state.h_dist_bin][state.v_dist_bin][state.vel_bin] = !decisionTable[state.h_dist_bin][state.v_dist_bin][state.vel_bin];
    }
};

class FlappyBirdGame
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;

    Bird bird;
    vector<Pipe> pipes;

    int score = 0;
    bool gameOver;
    float pipeSpawnTimer;
    float pipeSpawnInterval;

public:
    FlappyBirdGame() : window(nullptr), renderer(nullptr), score(0), gameOver(false), pipeSpawnTimer(0), pipeSpawnInterval(2.0f)
    {
        init();
        reset();
    }

    bool init()
    {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) // SDL_Init returns 0 on success
        {
            cout << "SDL Error: " << SDL_GetError() << endl;
            return false;
        }

        window = SDL_CreateWindow("Flappy Bird AI", 800, 600, 0);
        if (!window)
        {
            cout << "Window Error: " << SDL_GetError() << endl;
            SDL_Quit();
            return false;
        }

        // renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED); // Use accelerated renderer
        renderer = SDL_CreateRenderer(window, NULL);
        if (!renderer)
        {
            cout << "Renderer Error: " << SDL_GetError() << endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return false;
        }

        return true;
    }

    void reset()
    {
        bird = Bird();
        pipes.clear();
        score = 0;
        gameOver = false;
        pipeSpawnTimer = 0;

        // Start with one pipe
        pipes.push_back(Pipe(500));
    }

    // This is the new, crucial function
    State getState()
    {
        Pipe *nextPipe = nullptr;
        float minPipeDist = 10000.0f;

        // Find the closest pipe that is in front of the bird
        for (auto &pipe : pipes)
        {
            float dist = pipe.x + pipe.width - bird.x; // Dist from bird to *back* of pipe
            if (dist > 0 && dist < minPipeDist)
            {
                minPipeDist = dist;
                nextPipe = &pipe;
            }
        }

        float h_dist, v_dist, vel;

        if (nextPipe == nullptr)
        {
            // No pipes on screen or all are behind.
            // Return a "default" state: pipe is far away, bird is level, 0 velocity.
            h_dist = H_DIST_MAX; // Far away
            v_dist = 0;          // Level
            vel = 0;
        }
        else
        {
            // Calculate relative features
            h_dist = nextPipe->x - bird.x;
            v_dist = bird.y - nextPipe->Ygap; // Positive = bird is below gap
            vel = bird.velocity;
        }

        // --- Now, discretize (bin) these features ---

        int h_bin = (int)((h_dist - H_DIST_MIN) / H_DIST_BIN_SIZE);
        int v_bin = (int)((v_dist - V_DIST_MIN) / V_DIST_BIN_SIZE);
        int vel_bin = (int)((vel - VEL_MIN) / VEL_BIN_SIZE);

        // Clamp bins to be within our table dimensions
        State state;
        state.h_dist_bin = clamp(h_bin, 0, H_DIST_BINS - 1);
        state.v_dist_bin = clamp(v_bin, 0, V_DIST_BINS - 1);
        state.vel_bin = clamp(vel_bin, 0, VEL_BINS - 1);

        return state;
    }

    // Returns true if game is still running, false if game is over
    bool step(bool action, float deltaTime) // Now takes deltaTime
    {
        if (gameOver)
            return false;

        if (action)
            bird.flap();

        bird.update(deltaTime);

        // Check for out of bounds (top/bottom)
        if (bird.y > 600 || bird.y < 0)
        {
            gameOver = true;
            return false;
        }

        // --- Pipe Logic ---
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

            // Check for collision
            if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
            {
                gameOver = true;
                return false; // Game over
            }

            // Check for scoring
            if (pipe.hasPassedBird(bird.x))
            {
                pipe.scored = true;
                ++score;
            }
        }

        // Remove pipes that are off-screen
        pipes.erase(remove_if(pipes.begin(), pipes.end(), [](Pipe &pipe)
                              { return pipe.isOffScreen(); }),
                    pipes.end());

        return true; // Game is still running
    }

    void render()
    {
        // Simple blue sky
        SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
        SDL_RenderClear(renderer);

        // Draw Pipes
        for (auto &pipe : pipes)
        {
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();

            // Main Pipes
            SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255); // Green
            SDL_RenderFillRect(renderer, &topPipe);
            SDL_RenderFillRect(renderer, &bottomPipe);
        }

        // Draw Bird
        SDL_FRect birdRect = bird.getRect();
        SDL_SetRenderDrawColor(renderer, 255, 220, 0, 255); // Yellow
        SDL_RenderFillRect(renderer, &birdRect);

        SDL_RenderPresent(renderer);
    }

    bool isGameOver() const { return gameOver; }
    int getScore() const { return score; }

    ~FlappyBirdGame()
    {
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        SDL_Quit(); // Quit SDL subsystem
    }
};

void trainQTable(int episodes)
{
    string QTableName = "FlappyBirdQTable.dat";
    QTable decisionTable;
    decisionTable.loadModel(QTableName);

    FlappyBirdGame game;

    int bestScore = 0;
    int totalSteps = 0;

    auto startTime = chrono::high_resolution_clock::now();

    cout << "🚀 Starting Flappy Bird AI Training..." << endl;
    cout << "📊 Episodes: " << episodes << endl;
    cout << "💾 State Table Size: " << H_DIST_BINS << "x" << V_DIST_BINS << "x" << VEL_BINS << " = " << (H_DIST_BINS * V_DIST_BINS * VEL_BINS) << " states" << endl;
    cout << "══════════════════════════════════════════════════════════════════════════" << endl;

    Uint64 lastTime = SDL_GetTicks();

    for (int episode = 0; episode < episodes; ++episode)
    {
        game.reset();
        State state = game.getState();
        int episodeSteps = 0;

        while (!game.isGameOver() && episodeSteps < 50000) // Max steps per episode
        {
            Uint64 currentTime = SDL_GetTicks();
            float deltaTime = (currentTime - lastTime) / 1000.0f;

            // Clamp deltaTime to prevent huge physics jumps if lagging
            if (deltaTime > 0.033f)
                deltaTime = 0.033f;

            lastTime = currentTime;

            // 1. Select action from Q-table
            bool action = decisionTable.selectDecision(state);

            // 2. Take action and get new state
            bool stillAlive = game.step(action, deltaTime);
            State nextState = game.getState();

            // 3. Update decision table
            // This is the simplest "learning": if you died, flip the decision
            // A true Q-learning algorithm would be more complex.
            if (!stillAlive)
            {
                decisionTable.updateDecision(state);
            }

            // 4. Move to next state
            state = nextState;

            ++episodeSteps;
            ++totalSteps;

            // --- MODIFICATION ---
            // Render every frame to watch the AI learn
            game.render();
            // Add a small delay so we can see it
            // Adjust the delay (e.g., 5, 10, or 1) to speed up/slow down
            SDL_Delay(5);
            // --- END MODIFICATION ---

            /* // Original code (renders only every 10 episodes for speed)
            // Only render every N episodes to speed up training
            if (episode % 10 == 0)
            {
                game.render();
                // Add a small delay so we can see it
                // SDL_Delay(5);
            }
            */
        }

        int episodeScore = game.getScore();
        if (episodeScore > bestScore)
            bestScore = episodeScore;

        // Log progress every 100 episodes
        if (episode % 100 == 0 || episode == episodes - 1)
        {
            auto currentTime = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::seconds>(currentTime - startTime);

            cout << "📈 Ep: " << (episode + 1)
                 << " | Score: " << episodeScore
                 << " | Best: " << bestScore
                 << " | Steps: " << episodeSteps
                 << " | Time: " << duration.count() << "s" << endl;
        }
    }

    decisionTable.saveModel(QTableName);

    auto endTime = chrono::high_resolution_clock::now();
    auto totalTime = chrono::duration_cast<chrono::minutes>(endTime - startTime);

    cout << "══════════════════════════════════════════════════════════════════════════" << endl;
    cout << "✅ TRAINING COMPLETED!" << endl;
    cout << "🏆 Best Score: " << bestScore << endl;
    cout << "🎮 Total Steps: " << totalSteps << endl;
    cout << "⏱️  Training Time: " << totalTime.count() << " minutes" << endl;
    cout << "💾 Model saved: " << QTableName << endl;
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    srand(time(0)); // Seed random number generator

    cout << "🎮 ═══════════════════════════════════════════════════════════════════════════" << endl;
    cout << "🐦 FLAPPY BIRD AI - Training Program" << endl;
    cout << "🧠 Decision Table (Binned State)" << endl;
    cout << "   ═══════════════════════════════════════════════════════════════════════════" << endl;

    cout << "Enter Number of Training Episodes (e.g., 10000): " << endl;

    string runs;
    cin >> runs;

    int episodes;
    try
    {
        episodes = stoi(runs);
    }
    catch (const std::exception &e)
    {
        cout << "Error: Invalid number." << endl;
        return 1;
    }

    if (episodes <= 0)
    {
        cout << "Error: Number of episodes must be positive." << endl;
        return 1;
    }
    if (episodes > 1000000)
    {
        cout << "Warning: Large number of episodes. This may take a long time." << endl;
    }

    cout << "🎯 Starting training with " << episodes << " episodes..." << endl;
    trainQTable(episodes);

    cout << "\n🙏 Training completed!" << endl;
    cout << "Press Enter to exit..." << endl;
    cin.ignore(); // Wait for newline
    cin.get();    // Wait for user to press Enter
    return 0;
}
