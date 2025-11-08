#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <unordered_map>
#include <thread>

struct Bird
{
    float x = 100;
    float y = 300;
    float velocity = 0;
    float gravity = 800;       // Reduced for more control
    float jumpStrength = -400; // More reasonable jump
    int size = 20;             // Smaller bird for easier navigation

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
    float width = 60;      // Slightly narrower pipes
    float gapHeight = 180; // Larger gap for easier training
    float gapY = 300;
    float speed = 200; // Slightly slower for better learning
    bool scored = false;

    Pipe(float startX) : x(startX)
    {
        // More predictable gap positioning for initial training
        gapY = 180 + (rand() % 240); // Gap between 200-400
    }

    void update(float deltaTime)
    {
        x -= speed * deltaTime;
    }

    SDL_FRect getTopRect()
    {
        return {x, 0, width, (gapY - gapHeight / 2)};
    }

    SDL_FRect getBottomRect()
    {
        return {x, (gapY + gapHeight / 2), width, (600 - (gapY + gapHeight / 2))};
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

// Improved Q-Learning Agent with better state representation
class SuperiorQAgent
{
private:
    std::unordered_map<std::string, std::vector<float>> qTable;
    float epsilon;
    float epsilonDecay;
    float epsilonMin;
    float learningRate;
    float gamma;
    std::mt19937 rng;
    std::uniform_real_distribution<float> uniformDist;
    int lastAction = 0;

    // Simplified state discretization
    std::string getStateKey(const std::vector<float> &state)
    {
        // Simpler discretization for faster learning
        int birdY = std::max(0, std::min(11, (int)(state[0] * 12)));           // 12 bins for bird Y
        int velocity = std::max(0, std::min(7, (int)((state[1] + 1.0f) * 4))); // 8 bins for velocity
        int pipeX = std::max(0, std::min(9, (int)(state[2] * 10)));            // 10 bins for pipe distance
        int vertDist = std::max(0, std::min(7, (int)((state[4] + 1.0f) * 4))); // 8 bins for vertical distance to gap

        return std::to_string(birdY) + "_" + std::to_string(velocity) + "_" +
               std::to_string(pipeX) + "_" + std::to_string(vertDist);
    }

public:
    SuperiorQAgent() : epsilon(1.0f), epsilonDecay(0.9997f), epsilonMin(0.05f),
                       learningRate(0.1f), gamma(0.95f), rng(std::random_device{}()), uniformDist(0, 1)
    {
    }

    int selectAction(const std::vector<float> &state)
    {
        // Epsilon-greedy with proper exploration
        if (uniformDist(rng) < epsilon)
        {
            return rng() % 2;
        }

        std::string stateKey = getStateKey(state);
        if (qTable.find(stateKey) == qTable.end())
        {
            // Initialize Q-values to 0
            qTable[stateKey] = {0.0f, 0.0f};
            return rng() % 2; // Random action for new states
        }

        // Choose action with highest Q-value, with tie-breaking
        float q0 = qTable[stateKey][0]; // Don't flap
        float q1 = qTable[stateKey][1]; // Flap

        if (std::abs(q0 - q1) < 0.0001f)
        {
            // If Q-values are essentially equal, choose randomly
            return rng() % 2;
        }

        return (q0 > q1) ? 0 : 1;
    }

    void updateQ(const std::vector<float> &state, int action, float reward,
                 const std::vector<float> &nextState, bool terminal)
    {
        std::string stateKey = getStateKey(state);
        if (qTable.find(stateKey) == qTable.end())
        {
            qTable[stateKey] = {0.0f, 0.0f};
        }

        float target = reward;
        if (!terminal)
        {
            std::string nextStateKey = getStateKey(nextState);
            if (qTable.find(nextStateKey) == qTable.end())
            {
                qTable[nextStateKey] = {0.0f, 0.0f};
            }
            target += gamma * std::max(qTable[nextStateKey][0], qTable[nextStateKey][1]);
        }

        // Standard Q-learning update
        qTable[stateKey][action] = (1 - learningRate) * qTable[stateKey][action] + learningRate * target;

        // Decay epsilon
        if (epsilon > epsilonMin)
        {
            epsilon *= epsilonDecay;
        }
    }

    void saveModel(const std::string &filename)
    {
        std::ofstream file(filename);
        file << epsilon << std::endl;
        file << qTable.size() << std::endl;

        for (const auto &entry : qTable)
        {
            file << entry.first << " " << entry.second[0] << " " << entry.second[1] << std::endl;
        }
        file.close();
    }

    bool loadModel(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
            return false;

        file >> epsilon;
        size_t size;
        file >> size;

        qTable.clear();
        std::string stateKey;
        float q0, q1;

        for (size_t i = 0; i < size; i++)
        {
            if (file >> stateKey >> q0 >> q1)
            {
                qTable[stateKey] = {q0, q1};
            }
        }

        file.close();
        return true;
    }

    float getEpsilon() const { return epsilon; }
    void setEpsilon(float eps) { epsilon = eps; }
    size_t getQTableSize() const { return qTable.size(); }
};

// Enhanced Game Class with much better state representation
class SuperiorFlappyBirdAI
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    Bird bird;
    std::vector<Pipe> pipes;
    int score;
    bool gameOver;
    bool gameStarted;
    float pipeSpawnTimer;
    float pipeSpawnInterval;
    bool headless;
    int frameCount;
    int survivalFrames; // Track how long the bird survives

public:
    SuperiorFlappyBirdAI(bool headlessMode = false)
        : window(nullptr), renderer(nullptr), score(0), gameOver(false),
          gameStarted(true), pipeSpawnTimer(0), pipeSpawnInterval(2.8f),
          headless(headlessMode), frameCount(0), survivalFrames(0)
    {
        if (!headless)
        {
            init();
        }
        reset();
    }

    bool init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            std::cout << "SDL Error: " << SDL_GetError() << std::endl;
            return false;
        }

        window = SDL_CreateWindow("Superior Flappy Bird AI", 800, 600, 0);
        if (!window)
        {
            std::cout << "Window Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return false;
        }

        renderer = SDL_CreateRenderer(window, NULL);
        if (!renderer)
        {
            std::cout << "Renderer Error: " << SDL_GetError() << std::endl;
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
        gameStarted = true;
        pipeSpawnTimer = 0;
        frameCount = 0;
        survivalFrames = 0;

        // Start with first pipe at optimal distance
        pipes.push_back(Pipe(500));
    }

    std::vector<float> getState()
    {
        std::vector<float> state(5, 0.0f);

        // Normalized bird Y position (0-1)
        state[0] = std::clamp(bird.y / 600.0f, 0.0f, 1.0f);

        // Normalized bird velocity (-1 to 1)
        state[1] = std::clamp(bird.velocity / 600.0f, -1.0f, 1.0f);

        // Find the next pipe
        Pipe *nextPipe = nullptr;
        for (auto &pipe : pipes)
        {
            if (pipe.x + pipe.width > bird.x)
            {
                nextPipe = &pipe;
                break;
            }
        }

        if (nextPipe)
        {
            // Normalized horizontal distance to pipe (0-1)
            float distX = (nextPipe->x - bird.x) / 400.0f;
            state[2] = std::clamp(distX, 0.0f, 1.0f);

            // Normalized gap center position (0-1)
            state[3] = std::clamp(nextPipe->gapY / 600.0f, 0.0f, 1.0f);

            // Normalized vertical distance from bird to gap center (-1 to 1)
            float vertDist = (nextPipe->gapY - bird.y) / 300.0f;
            state[4] = std::clamp(vertDist, -1.0f, 1.0f);
        }
        else
        {
            // No pipe ahead
            state[2] = 1.0f;
            state[3] = 0.5f;
            state[4] = 0.0f;
        }

        return state;
    }

    float step(int action)
    {
        if (gameOver)
            return 0.0f;

        float deltaTime = 1.0f / 60.0f;
        float reward = 1.0f; // Constant reward for surviving each frame
        frameCount++;
        survivalFrames++;

        // Store previous state
        float prevY = bird.y;
        float prevVelocity = bird.velocity;

        // Take action
        if (action == 1)
        {
            bird.flap();
        }

        // Update bird
        bird.update(deltaTime);

        // Boundary collision check
        if (bird.y > 580 || bird.y < 20)
        {
            gameOver = true;
            return -1000.0f; // Large negative reward for dying
        }

        // Spawn pipes
        pipeSpawnTimer += deltaTime;
        if (pipeSpawnTimer >= pipeSpawnInterval)
        {
            pipes.push_back(Pipe(800));
            pipeSpawnTimer = 0;
        }

        // Find the next pipe for collision and scoring
        Pipe *nextPipe = nullptr;
        for (auto &pipe : pipes)
        {
            if (pipe.x + pipe.width > bird.x)
            {
                nextPipe = &pipe;
                break;
            }
        }

        // Update pipes and check collisions/scoring
        for (auto &pipe : pipes)
        {
            pipe.update(deltaTime);

            // Check collision with more precise detection
            SDL_FRect birdRect = bird.getRect();
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();

            if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) ||
                SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
            {
                gameOver = true;
                return -1000.0f; // Large negative reward for collision
            }

            // Check scoring with bonus for center passage
            if (pipe.hasPassedBird(bird.x))
            {
                score++;
                pipe.scored = true;

                // Big reward for successfully passing a pipe
                reward = 1000.0f;

                // std::cout << "🎯 Score: " << score << " (Center bonus: " << centerBonus << ")" << std::endl;
            }
        }

        // Small bonus for being at a good height (middle of screen)
        float heightBonus = 1.0f - std::abs(bird.y - 300.0f) / 300.0f;
        reward += heightBonus * 0.1f;

        // Remove off-screen pipes
        pipes.erase(std::remove_if(pipes.begin(), pipes.end(),
                                   [](Pipe &pipe)
                                   { return pipe.isOffScreen(); }),
                    pipes.end());

        return reward;
    }

    void render()
    {
        if (headless)
            return;

        // Enhanced rendering with better visual feedback

        // Gradient sky background
        for (int y = 0; y < 600; y++)
        {
            float t = (float)y / 600.0f;
            int r = (int)(135 + t * (100 - 135));
            int g = (int)(206 + t * (149 - 206));
            int b = (int)(235 + t * (237 - 235));
            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_FRect line = {0.0f, (float)y, 800.0f, 1.0f};
            SDL_RenderFillRect(renderer, &line);
        }

        // Moving clouds for better visual appeal
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 150);
        for (int i = 0; i < 6; i++)
        {
            float cloudX = 120 + i * 150 + (float)(frameCount % 1800) * 0.1f;
            if (cloudX > 900)
                cloudX -= 1050;
            float cloudY = 60 + i * 25 + sin((frameCount + i * 120) * 0.005f) * 12;

            for (int j = -18; j <= 18; j += 6)
            {
                for (int k = -9; k <= 9; k += 3)
                {
                    SDL_FRect cloud = {cloudX + j, cloudY + k, 22.0f, 16.0f};
                    SDL_RenderFillRect(renderer, &cloud);
                }
            }
        }

        // Enhanced pipes with better graphics
        for (auto &pipe : pipes)
        {
            // Pipe shadows for depth
            SDL_SetRenderDrawColor(renderer, 0, 80, 0, 200);
            SDL_FRect topShadow = pipe.getTopRect();
            SDL_FRect bottomShadow = pipe.getBottomRect();
            topShadow.x += 4;
            bottomShadow.x += 4;
            SDL_RenderFillRect(renderer, &topShadow);
            SDL_RenderFillRect(renderer, &bottomShadow);

            // Main pipes with improved color
            SDL_SetRenderDrawColor(renderer, 46, 160, 67, 255);
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();
            SDL_RenderFillRect(renderer, &topPipe);
            SDL_RenderFillRect(renderer, &bottomPipe);

            // Pipe highlights for 3D effect
            SDL_SetRenderDrawColor(renderer, 72, 201, 94, 255);
            SDL_FRect topHighlight = {topPipe.x + 4, topPipe.y, 10.0f, topPipe.h};
            SDL_FRect bottomHighlight = {bottomPipe.x + 4, bottomPipe.y, 10.0f, bottomPipe.h};
            SDL_RenderFillRect(renderer, &topHighlight);
            SDL_RenderFillRect(renderer, &bottomHighlight);

            // Pipe caps with enhanced look
            SDL_SetRenderDrawColor(renderer, 27, 94, 32, 255);
            SDL_FRect topCap = {topPipe.x - 8, topPipe.y + topPipe.h - 30, topPipe.w + 16, 30.0f};
            SDL_FRect bottomCap = {bottomPipe.x - 8, bottomPipe.y, bottomPipe.w + 16, 30.0f};
            SDL_RenderFillRect(renderer, &topCap);
            SDL_RenderFillRect(renderer, &bottomCap);

            // Gap center indicator for training visualization
            SDL_SetRenderDrawColor(renderer, 255, 255, 100, 80);
            SDL_FRect gapCenter = {pipe.x, pipe.gapY - 5, pipe.width, 10.0f};
            SDL_RenderFillRect(renderer, &gapCenter);
        }

        // Enhanced bird with better animation
        SDL_FRect birdRect = bird.getRect();

        // Bird shadow with motion blur effect
        SDL_SetRenderDrawColor(renderer, 200, 200, 0, 120);
        for (int i = 1; i <= 3; i++)
        {
            SDL_FRect shadowRect = {birdRect.x + i, birdRect.y + i, birdRect.w, birdRect.h};
            SDL_RenderFillRect(renderer, &shadowRect);
        }

        // Main bird body with dynamic coloring based on velocity
        int redComponent = std::clamp(255 - (int)(std::abs(bird.velocity) * 0.3f), 180, 255);
        SDL_SetRenderDrawColor(renderer, 255, redComponent, 0, 255);
        SDL_RenderFillRect(renderer, &birdRect);

        // Bird highlights with animation
        SDL_SetRenderDrawColor(renderer, 255, 255, 150, 255);
        float highlightOffset = sin(frameCount * 0.15f) * 2;
        SDL_FRect highlight = {birdRect.x + 4 + highlightOffset, birdRect.y + 4, birdRect.w - 8, birdRect.h - 12};
        SDL_RenderFillRect(renderer, &highlight);

        // Animated eye with direction
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        float eyeX = birdRect.x + birdRect.w - 8;
        if (bird.velocity < -100)
            eyeX -= 2; // Look up when rising fast
        else if (bird.velocity > 100)
            eyeX += 1; // Look forward when falling
        SDL_FRect eye = {eyeX, birdRect.y + 6, 4.0f, 4.0f};
        SDL_RenderFillRect(renderer, &eye);

        // Dynamic beak
        SDL_SetRenderDrawColor(renderer, 255, 165, 0, 255);
        float beakLength = 8.0f + (bird.velocity > 0 ? 2.0f : 0.0f);
        SDL_FRect beak = {birdRect.x + birdRect.w - 2, birdRect.y + 8, beakLength, 4.0f};
        SDL_RenderFillRect(renderer, &beak);

        // Professional UI with more information
        drawEnhancedUI();

        SDL_RenderPresent(renderer);
    }

    void drawEnhancedUI()
    {
        // Score panel with better design
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_FRect scoreBG = {20.0f, 20.0f, 300.0f, 70.0f};
        SDL_RenderFillRect(renderer, &scoreBG);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &scoreBG);

        // Score visualization with better scaling
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        for (int i = 0; i < std::min(score, 28); i++)
        {
            SDL_FRect scoreBlock = {(float)(30 + i * 9), 35.0f, 7.0f, 10.0f};
            SDL_RenderFillRect(renderer, &scoreBlock);
        }

        // Tens and hundreds
        if (score >= 10)
        {
            SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
            for (int i = 0; i < std::min(score / 10, 28); i++)
            {
                SDL_FRect tenBlock = {(float)(30 + i * 9), 50.0f, 7.0f, 12.0f};
                SDL_RenderFillRect(renderer, &tenBlock);
            }
        }

        if (score >= 100)
        {
            SDL_SetRenderDrawColor(renderer, 255, 69, 0, 255);
            for (int i = 0; i < std::min(score / 100, 28); i++)
            {
                SDL_FRect hundredBlock = {(float)(30 + i * 9), 67.0f, 7.0f, 15.0f};
                SDL_RenderFillRect(renderer, &hundredBlock);
            }
        }

        // Performance indicators
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 220);
        SDL_FRect perfBG = {500.0f, 20.0f, 280.0f, 70.0f};
        SDL_RenderFillRect(renderer, &perfBG);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &perfBG);

        // Velocity indicator with better visualization
        int velBars = std::clamp((int)(std::abs(bird.velocity) / 50), 0, 12);
        SDL_SetRenderDrawColor(renderer, bird.velocity > 0 ? 255 : 100, bird.velocity < 0 ? 255 : 100, 0, 255);
        for (int i = 0; i < velBars; i++)
        {
            SDL_FRect velBar = {(float)(510 + i * 12), 35.0f, 10.0f, 15.0f};
            SDL_RenderFillRect(renderer, &velBar);
        }

        // Survival time indicator
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 255);
        int survivalBars = std::min((survivalFrames / 100) % 20, 20);
        for (int i = 0; i < survivalBars; i++)
        {
            SDL_FRect survivalBar = {(float)(510 + i * 12), 60.0f, 10.0f, 8.0f};
            SDL_RenderFillRect(renderer, &survivalBar);
        }

        // Frame counter
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        int frameBars = (frameCount / 50) % 15;
        for (int i = 0; i < frameBars; i++)
        {
            SDL_FRect frameBar = {(float)(650 + i * 8), 25.0f, 6.0f, 6.0f};
            SDL_RenderFillRect(renderer, &frameBar);
        }
    }

    bool isGameOver() const { return gameOver; }
    int getScore() const { return score; }
    int getFrameCount() const { return frameCount; }
    int getSurvivalFrames() const { return survivalFrames; }

    ~SuperiorFlappyBirdAI()
    {
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        if (!headless)
            SDL_Quit();
    }
};

void trainSuperiorAgent(int episodes)
{
    SuperiorQAgent agent;
    SuperiorFlappyBirdAI game(true);

    std::vector<int> scores;
    std::vector<int> survivalTimes;
    int bestScore = 0;
    int bestSurvival = 0;
    int totalFrames = 0;
    int consecutiveSuccesses = 0;
    float bestAverage = 0;
    int recentBestScore = 0;

    auto startTime = std::chrono::high_resolution_clock::now();

    std::cout << "🚀 Starting SUPERIOR Flappy Bird AI Training..." << std::endl;
    std::cout << "📊 Episodes: " << episodes << std::endl;
    std::cout << "🧠 Enhanced Q-Learning with Superior State Representation" << std::endl;
    std::cout << "🎯 Target: Achieve consistent high scores (20+)" << std::endl;
    std::cout << "══════════════════════════════════════════════════════════════════════════" << std::endl;

    for (int episode = 0; episode < episodes; episode++)
    {
        game.reset();
        std::vector<float> state = game.getState();
        int steps = 0;

        while (!game.isGameOver() && steps < 20000) // Increased max steps
        {
            int action = agent.selectAction(state);
            float reward = game.step(action);
            std::vector<float> nextState = game.getState();

            agent.updateQ(state, action, reward, nextState, game.isGameOver());
            state = nextState;
            steps++;
        }

        int episodeScore = game.getScore();
        int survival = game.getSurvivalFrames();
        scores.push_back(episodeScore);
        survivalTimes.push_back(survival);
        totalFrames += game.getFrameCount();

        // Track consecutive successes (score > 0)
        if (episodeScore > 0)
        {
            consecutiveSuccesses++;
            if (episodeScore > recentBestScore)
            {
                recentBestScore = episodeScore;
            }
        }
        else
        {
            consecutiveSuccesses = 0;
        }

        // Update best scores
        if (episodeScore > bestScore)
        {
            bestScore = episodeScore;
            std::cout << "🏆 NEW BEST SCORE: " << bestScore << " (Episode " << episode + 1
                      << ", Survival: " << survival << " frames)" << std::endl;
        }

        if (survival > bestSurvival)
        {
            bestSurvival = survival;
        }

        // Progress reporting and model saving
        if (episode % 50 == 0 || episodeScore > 0 || episode == episodes - 1)
        {
            float recentAvg = 0;
            float recentSurvivalAvg = 0;
            int count = std::min(500, (int)scores.size());

            for (int i = scores.size() - count; i < scores.size(); i++)
            {
                recentAvg += scores[i];
                recentSurvivalAvg += survivalTimes[i];
            }
            recentAvg /= count;
            recentSurvivalAvg /= count;

            if (recentAvg > bestAverage)
            {
                bestAverage = recentAvg;
            }

            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);

            std::cout << "📈 Ep: " << std::setw(6) << episode + 1
                      << " | Score: " << std::setw(3) << episodeScore
                      << " | Best: " << std::setw(3) << bestScore
                      << " | Avg: " << std::setw(6) << std::fixed << std::setprecision(2) << recentAvg
                      << " | Surv: " << std::setw(5) << survival
                      << " | ε: " << std::setw(6) << std::setprecision(4) << agent.getEpsilon()
                      << " | States: " << std::setw(7) << agent.getQTableSize()
                      << " | Time: " << std::setw(4) << duration.count() << "s" << std::endl;
        }

        // Early convergence check with stricter criteria
        if (episode > 1000 && consecutiveSuccesses >= 100 && bestScore > 30)
        {
            std::cout << "🎉 EXCELLENT CONVERGENCE! Agent achieving consistent high scores!" << std::endl;
            break;
        }

        // Progressive difficulty adjustment
        if (episode > 0 && episode % 2000 == 0 && bestScore > 10)
        {
            std::cout << "📈 Progressive training milestone reached. Continuing..." << std::endl;
        }
    }

    agent.saveModel("trained_model.dat");

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::minutes>(endTime - startTime);

    // Calculate final statistics
    float finalAvg = 0;
    float finalSurvivalAvg = 0;
    int successfulEpisodes = 0;

    int recentCount = std::min(1000, (int)scores.size());
    for (int i = scores.size() - recentCount; i < scores.size(); i++)
    {
        finalAvg += scores[i];
        finalSurvivalAvg += survivalTimes[i];
        if (scores[i] > 0)
            successfulEpisodes++;
    }
    finalAvg /= recentCount;
    finalSurvivalAvg /= recentCount;

    std::cout << "══════════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "✅ SUPERIOR TRAINING COMPLETED!" << std::endl;
    std::cout << "🏆 Best Score: " << bestScore << std::endl;
    std::cout << "🥇 Best Survival: " << bestSurvival << " frames" << std::endl;
    std::cout << "📊 Final Average (1000 eps): " << std::fixed << std::setprecision(2) << finalAvg << std::endl;
    std::cout << "⏱️  Average Survival: " << std::fixed << std::setprecision(0) << finalSurvivalAvg << " frames" << std::endl;
    std::cout << "✨ Success Rate: " << std::fixed << std::setprecision(1) << (100.0f * successfulEpisodes / recentCount) << "%" << std::endl;
    std::cout << "🎮 Total Frames: " << totalFrames << std::endl;
    std::cout << "🧠 Q-Table Size: " << agent.getQTableSize() << " learned states" << std::endl;
    std::cout << "⏱️  Training Time: " << totalTime.count() << " minutes" << std::endl;
    std::cout << "💾 Model saved: trained_model.dat" << std::endl;

    if (finalAvg > 15.0f)
    {
        std::cout << "🌟 EXCEPTIONAL PERFORMANCE! AI achieved expert-level gameplay!" << std::endl;
    }
    else if (finalAvg > 8.0f)
    {
        std::cout << "👏 EXCELLENT PERFORMANCE! AI learned advanced strategies!" << std::endl;
    }
    else if (finalAvg > 3.0f)
    {
        std::cout << "👍 GOOD PERFORMANCE! AI shows solid learning progress!" << std::endl;
    }
    else
    {
        std::cout << "📚 AI is learning. Consider longer training for better results." << std::endl;
    }
}

int main(int argc, char *argv[])
{
    srand(time(nullptr));

    std::cout << "🎮 ═══════════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "🐦 SUPERIOR FLAPPY BIRD AI - Training Program" << std::endl;
    std::cout << "🧠 Enhanced Q-Learning with Superior State Representation & Reward Shaping" << std::endl;
    std::cout << "   ═══════════════════════════════════════════════════════════════════════════" << std::endl;

    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <number_of_episodes>" << std::endl;
        std::cout << "Example: " << argv[0] << " 5000" << std::endl;
        return 1;
    }

    int episodes = std::atoi(argv[1]);
    if (episodes <= 0)
    {
        std::cout << "Error: Number of episodes must be positive" << std::endl;
        return 1;
    }

    std::cout << "🎯 Starting training with " << episodes << " episodes..." << std::endl;
    trainSuperiorAgent(episodes);

    std::cout << "\n🙏 Training completed!" << std::endl;
    return 0;
}