#include <SDL3/SDL.h>
#include <iostream>
#include <windows.h> // To set console Encoding to show emoji
#include <random>
#include <fstream>
#include <algorithm>

#include <map>
#include <vector>

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
    float gapY = 300;
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

class QAgent
{
private:
    map<string, vector<float>> qTable;
    float epsilon;
    mt19937 rng; // Random Generator
    uniform_real_distribution<float> uniformDist;

    string getStateKey(const vector<float> &state)
    {
        // Must match the discretization in train.cpp EXACTLY
        int birdY = max(0, min(11, (int)(state[0] * 12)));           // 12 bins for bird Y
        int velocity = max(0, min(7, (int)((state[1] + 1.0f) * 4))); // 8 bins for velocity
        int pipeX = max(0, min(9, (int)(state[2] * 10)));            // 10 bins for pipe distance
        int vertDist = max(0, min(7, (int)((state[4] + 1.0f) * 4))); // 8 bins for vertical distance

        return to_string(birdY) + "_" + to_string(velocity) + "_" + to_string(pipeX) + "_" + to_string(vertDist);
    }

public:
    QAgent() : epsilon(0.0f), rng(random_device{}()), uniformDist(0, 1) {}

    int selectAction(const vector<float> &state)
    {
        // During play, no exploration (epsilon = 0)
        if (uniformDist(rng) < epsilon)
            return rng() % 2;

        string stateKey = getStateKey(state);
        if (qTable.find(stateKey) == qTable.end())
        {
            // If state not in Q-table, default to not flapping
            return 0;
        }

        float q0 = qTable[stateKey][0]; // Don't flap
        float q1 = qTable[stateKey][1]; // Flap
        
        // Tie-breaking: if Q-values are very close, prefer not flapping
        if (abs(q0 - q1) < 0.0001f)
        {
            return 0; // Default to not flapping
        }
        
        return (q0 > q1) ? 0 : 1;
    }

    bool loadModel(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
            return false;

        file >> epsilon;
        size_t size;
        file >> size;

        qTable.clear();
        string stateKey;
        float q0, q1;

        for (size_t i = 0; i < size; ++i)
        {
            if (file >> stateKey >> q0 >> q1)
                qTable[stateKey] = {q0, q1};
        }

        file.close();
        return true;
    }

    void setEpsilon(float eps) { epsilon = eps; }
    size_t getQTableSize() const { return qTable.size(); }
};

class FlappyBirdGame
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    Bird bird;
    vector<Pipe> pipes;
    int score;
    bool gameOver;
    bool gameStarted;
    float pipeSpawnTimer;
    float pipeSpawnInterval;
    int frameCount;
    QAgent *agent;
    bool showDebugInfo;
    float gameOverTimer;
    float autoRestartDelay;
    int maxScore;
    int currentMaxScore;

public:
    FlappyBirdGame()
        : window(nullptr), renderer(nullptr), score(0), gameOver(false),
          gameStarted(false), pipeSpawnTimer(0), pipeSpawnInterval(2.8f),
          frameCount(0), agent(nullptr), showDebugInfo(false),
          gameOverTimer(0), autoRestartDelay(1.0f), maxScore(0), currentMaxScore(0)
    {
        init();
        reset();
    }

    bool init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            cout << "SDL Error: " << SDL_GetError() << endl;
            return false;
        }

        window = SDL_CreateWindow("Flappy Bird AI - Professional Edition", 800, 600, 0);
        if (!window)
        {
            cout << "Window Error: " << SDL_GetError() << endl;
            SDL_Quit();
            return false;
        }

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
        gameStarted = true; // Auto-start the game
        pipeSpawnTimer = 0;
        frameCount = 0;
        gameOverTimer = 0;

        pipes.push_back(Pipe(600));
    }

    vector<float> getState()
    {
        vector<float> state(5, 0.0f);

        // Must match training state representation EXACTLY
        state[0] = clamp(bird.y / 600.0f, 0.0f, 1.0f);
        state[1] = clamp(bird.velocity / 600.0f, -1.0f, 1.0f);

        // Find next pipe
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
            state[2] = clamp(distX, 0.0f, 1.0f);
            
            // Normalized gap center position (0-1)
            state[3] = clamp(nextPipe->gapY / 600.0f, 0.0f, 1.0f);
            
            // Normalized vertical distance from bird to gap center (-1 to 1)
            float vertDist = (nextPipe->gapY - bird.y) / 300.0f;
            state[4] = clamp(vertDist, -1.0f, 1.0f);
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

    void handleEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                exit(0);
            }

            if (event.type == SDL_EVENT_KEY_DOWN)
            {
                switch (event.key.key)
                {
                case SDLK_R:
                    reset();
                    break;

                case SDLK_D:
                    showDebugInfo = !showDebugInfo;
                    cout << "Debug Info: " << (showDebugInfo ? "ON" : "OFF") << endl;
                    break;

                case SDLK_ESCAPE:
                    exit(0);
                    break;
                }
            }
        }
    }

    void update(float deltaTime)
    {
        if (!gameStarted)
            return;

        // Handle game over state with auto-restart
        if (gameOver)
        {
            gameOverTimer += deltaTime;
            if (gameOverTimer >= autoRestartDelay)
            {
                reset();
            }
            return;
        }

        frameCount++;

        if (agent)
        {
            vector<float> state = getState();
            if (agent->selectAction(state) == 1)
            {
                bird.flap();
            }
        }

        bird.update(deltaTime);

        if (bird.y > 580 || bird.y < 20)
        {
            gameOver = true;
            gameOverTimer = 0;
            if (score > maxScore)
            {
                maxScore = score;
                cout << "🏆 NEW MAX SCORE: " << maxScore << "! Final Score: " << score << " (Frames: " << frameCount << ") - Restarting in " << autoRestartDelay << "s..." << endl;
            }
            else
            {
                cout << "Game Over! Final Score: " << score << " (Max: " << maxScore << ", Frames: " << frameCount << ") - Restarting in " << autoRestartDelay << "s..." << endl;
            }
            return;
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

            if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) ||
                SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
            {
                gameOver = true;
                gameOverTimer = 0;
                if (score > maxScore)
                {
                    maxScore = score;
                    cout << "🏆 NEW MAX SCORE: " << maxScore << "! Collision! Final Score: " << score << " (Frames: " << frameCount << ") - Restarting in " << autoRestartDelay << "s..." << endl;
                }
                else
                {
                    cout << "Collision! Final Score: " << score << " (Max: " << maxScore << ", Frames: " << frameCount << ") - Restarting in " << autoRestartDelay << "s..." << endl;
                }
                return;
            }

            if (pipe.hasPassedBird(bird.x))
            {
                score++;
                pipe.scored = true;
                cout << "Score: " << score << endl;
            }
        }

        pipes.erase(remove_if(pipes.begin(), pipes.end(),
                              [](Pipe &pipe)
                              { return pipe.isOffScreen(); }),
                    pipes.end());
    }

    void render()
    {
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

        // Moving clouds
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
        for (int i = 0; i < 4; i++)
        {
            float cloudX = 100 + i * 200 + (float)(frameCount % 1200) * 0.1f;
            if (cloudX > 900)
                cloudX -= 1100;
            float cloudY = 60 + i * 25 + sin((frameCount + i * 150) * 0.008f) * 8;

            for (int j = -12; j <= 12; j += 6)
            {
                for (int k = -6; k <= 6; k += 3)
                {
                    SDL_FRect cloud = {cloudX + j, cloudY + k, 18.0f, 12.0f};
                    SDL_RenderFillRect(renderer, &cloud);
                }
            }
        }

        // Enhanced pipes with 3D effect
        for (auto &pipe : pipes)
        {
            SDL_FRect topPipe = pipe.getTopRect();
            SDL_FRect bottomPipe = pipe.getBottomRect();

            // Pipe shadows
            SDL_SetRenderDrawColor(renderer, 0, 60, 0, 180);
            SDL_FRect topShadow = {topPipe.x + 4, topPipe.y, topPipe.w, topPipe.h};
            SDL_FRect bottomShadow = {bottomPipe.x + 4, bottomPipe.y, bottomPipe.w, bottomPipe.h};
            SDL_RenderFillRect(renderer, &topShadow);
            SDL_RenderFillRect(renderer, &bottomShadow);

            // Main pipes
            SDL_SetRenderDrawColor(renderer, 46, 139, 87, 255);
            SDL_RenderFillRect(renderer, &topPipe);
            SDL_RenderFillRect(renderer, &bottomPipe);

            // Pipe highlights
            SDL_SetRenderDrawColor(renderer, 60, 179, 113, 255);
            SDL_FRect topHighlight = {topPipe.x + 3, topPipe.y, 12.0f, topPipe.h};
            SDL_FRect bottomHighlight = {bottomPipe.x + 3, bottomPipe.y, 12.0f, bottomPipe.h};
            SDL_RenderFillRect(renderer, &topHighlight);
            SDL_RenderFillRect(renderer, &bottomHighlight);

            // Pipe caps with depth
            SDL_SetRenderDrawColor(renderer, 34, 100, 34, 255);
            SDL_FRect topCap = {topPipe.x - 6, topPipe.y + topPipe.h - 25, topPipe.w + 12, 25.0f};
            SDL_FRect bottomCap = {bottomPipe.x - 6, bottomPipe.y, bottomPipe.w + 12, 25.0f};
            SDL_RenderFillRect(renderer, &topCap);
            SDL_RenderFillRect(renderer, &bottomCap);
        }

        // Enhanced bird with animation
        SDL_FRect birdRect = bird.getRect();

        // Bird shadow
        SDL_SetRenderDrawColor(renderer, 180, 180, 0, 80);
        SDL_FRect shadowRect = {birdRect.x + 3, birdRect.y + 3, birdRect.w, birdRect.h};
        SDL_RenderFillRect(renderer, &shadowRect);

        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255); // Red for AI

        SDL_RenderFillRect(renderer, &birdRect);

        // Bird details
        SDL_SetRenderDrawColor(renderer, 255, 255, 150, 255);
        SDL_FRect highlight = {birdRect.x + 6, birdRect.y + 6, birdRect.w - 12, birdRect.h - 18};
        SDL_RenderFillRect(renderer, &highlight);

        // Animated eye
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        float eyeOffset = sin(frameCount * 0.1f) * 2;
        SDL_FRect eye = {birdRect.x + birdRect.w - 12 + eyeOffset, birdRect.y + 10, 8.0f, 8.0f};
        SDL_RenderFillRect(renderer, &eye);

        // Dynamic beak
        SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
        SDL_FRect beak = {birdRect.x + birdRect.w - 3, birdRect.y + 14, 12.0f, 6.0f};
        SDL_RenderFillRect(renderer, &beak);

        // Professional UI elements
        drawUI();

        // Remove start screen since we auto-start

        if (gameOver)
        {
            drawGameOverScreen();
        }

        if (showDebugInfo)
        {
            drawDebugInfo();
        }

        SDL_RenderPresent(renderer);
    }

    void drawUI()
    {
        // Score panel
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_FRect scoreBG = {30.0f, 30.0f, 200.0f, 50.0f};
        SDL_RenderFillRect(renderer, &scoreBG);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &scoreBG);

        // Draw "SCORE:" label
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // S
        SDL_FRect s1 = {40, 37, 8, 16};
        SDL_FRect s2 = {40, 37, 16, 4};
        SDL_FRect s3 = {40, 44, 16, 4};
        SDL_FRect s4 = {40, 51, 16, 4};
        SDL_RenderFillRect(renderer, &s1);
        SDL_RenderFillRect(renderer, &s2);
        SDL_RenderFillRect(renderer, &s3);
        SDL_RenderFillRect(renderer, &s4);

        // C
        SDL_FRect c1 = {62, 37, 8, 16};
        SDL_FRect c2 = {62, 37, 16, 4};
        SDL_FRect c3 = {62, 51, 16, 4};
        SDL_RenderFillRect(renderer, &c1);
        SDL_RenderFillRect(renderer, &c2);
        SDL_RenderFillRect(renderer, &c3);

        // O
        SDL_FRect o1 = {84, 37, 8, 16};
        SDL_FRect o2 = {96, 37, 8, 16};
        SDL_FRect o3 = {84, 37, 20, 4};
        SDL_FRect o4 = {84, 51, 20, 4};
        SDL_RenderFillRect(renderer, &o1);
        SDL_RenderFillRect(renderer, &o2);
        SDL_RenderFillRect(renderer, &o3);
        SDL_RenderFillRect(renderer, &o4);

        // R
        SDL_FRect r1 = {108, 37, 8, 16};
        SDL_FRect r2 = {108, 37, 16, 4};
        SDL_FRect r3 = {108, 44, 16, 4};
        SDL_FRect r4 = {116, 37, 8, 7};
        SDL_FRect r5 = {116, 48, 8, 7};
        SDL_RenderFillRect(renderer, &r1);
        SDL_RenderFillRect(renderer, &r2);
        SDL_RenderFillRect(renderer, &r3);
        SDL_RenderFillRect(renderer, &r4);
        SDL_RenderFillRect(renderer, &r5);

        // E
        SDL_FRect e1 = {130, 37, 8, 16};
        SDL_FRect e2 = {130, 37, 16, 4};
        SDL_FRect e3 = {130, 44, 12, 4};
        SDL_FRect e4 = {130, 51, 16, 4};
        SDL_RenderFillRect(renderer, &e1);
        SDL_RenderFillRect(renderer, &e2);
        SDL_RenderFillRect(renderer, &e3);
        SDL_RenderFillRect(renderer, &e4);

        // Colon
        SDL_FRect colon1 = {152, 40, 3, 3};
        SDL_FRect colon2 = {152, 48, 3, 3};
        SDL_RenderFillRect(renderer, &colon1);
        SDL_RenderFillRect(renderer, &colon2);

        // Draw actual score number
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        drawNumber(score, 165, 40);

        // Max score panel
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_FRect maxScoreBG = {250.0f, 30.0f, 150.0f, 50.0f};
        SDL_RenderFillRect(renderer, &maxScoreBG);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &maxScoreBG);

        // Draw "MAX:" label
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // M
        SDL_FRect m1 = {260, 37, 8, 16};
        SDL_FRect m2 = {276, 37, 8, 16};
        SDL_FRect m3 = {268, 37, 8, 8};
        SDL_FRect m4 = {260, 37, 24, 4};
        SDL_RenderFillRect(renderer, &m1);
        SDL_RenderFillRect(renderer, &m2);
        SDL_RenderFillRect(renderer, &m3);
        SDL_RenderFillRect(renderer, &m4);

        // A
        SDL_FRect a1 = {290, 44, 8, 9};
        SDL_FRect a2 = {306, 44, 8, 9};
        SDL_FRect a3 = {290, 37, 24, 4};
        SDL_FRect a4 = {290, 44, 24, 4};
        SDL_RenderFillRect(renderer, &a1);
        SDL_RenderFillRect(renderer, &a2);
        SDL_RenderFillRect(renderer, &a3);
        SDL_RenderFillRect(renderer, &a4);

        // X
        SDL_FRect x1 = {320, 44, 8, 9};
        SDL_FRect x2 = {336, 44, 8, 9};
        SDL_FRect x3 = {324, 37, 8, 7};
        SDL_FRect x4 = {332, 51, 8, 2};
        SDL_RenderFillRect(renderer, &x1);
        SDL_RenderFillRect(renderer, &x2);
        SDL_RenderFillRect(renderer, &x3);
        SDL_RenderFillRect(renderer, &x4);

        // Colon
        SDL_FRect colonMax1 = {350, 40, 3, 3};
        SDL_FRect colonMax2 = {350, 48, 3, 3};
        SDL_RenderFillRect(renderer, &colonMax1);
        SDL_RenderFillRect(renderer, &colonMax2);

        // Draw max score number
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
        drawNumber(maxScore, 360, 40);

        // Mode indicator
        SDL_SetRenderDrawColor(renderer, 255, 100, 100, 200);
        SDL_FRect modeIndicator = {720.0f, 30.0f, 50.0f, 25.0f};
        SDL_RenderFillRect(renderer, &modeIndicator);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &modeIndicator);

        // Performance meter
        SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200);
        SDL_FRect perfBG = {650.0f, 70.0f, 120.0f, 40.0f};
        SDL_RenderFillRect(renderer, &perfBG);

        // Velocity indicator
        int velBars = max(0, min(10, (int)(abs(bird.velocity) / 80)));
        for (int i = 0; i < velBars; i++)
        {
            SDL_SetRenderDrawColor(renderer, bird.velocity > 0 ? 255 : 100, bird.velocity < 0 ? 255 : 100, 0, 255);
            SDL_FRect velBar = {(float)(660 + i * 10), 80.0f, 8.0f, 20.0f};
            SDL_RenderFillRect(renderer, &velBar);
        }
    }

    void drawStartScreen()
    {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_FRect overlay = {0.0f, 0.0f, 800.0f, 600.0f};
        SDL_RenderFillRect(renderer, &overlay);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_FRect titleBG = {200.0f, 150.0f, 400.0f, 300.0f};
        SDL_RenderFillRect(renderer, &titleBG);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &titleBG);

        // Draw "FLAPPY BIRD" in stylized blocks
        SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255);
        // F
        SDL_FRect f1 = {220, 180, 15, 40};
        SDL_FRect f2 = {220, 180, 30, 8};
        SDL_FRect f3 = {220, 195, 25, 8};
        SDL_RenderFillRect(renderer, &f1);
        SDL_RenderFillRect(renderer, &f2);
        SDL_RenderFillRect(renderer, &f3);

        // L
        SDL_FRect l1 = {260, 180, 15, 40};
        SDL_FRect l2 = {260, 212, 30, 8};
        SDL_RenderFillRect(renderer, &l1);
        SDL_RenderFillRect(renderer, &l2);

        // A
        SDL_FRect a1 = {300, 188, 15, 32};
        SDL_FRect a2 = {315, 188, 15, 32};
        SDL_FRect a3 = {300, 180, 30, 8};
        SDL_FRect a4 = {300, 195, 30, 8};
        SDL_RenderFillRect(renderer, &a1);
        SDL_RenderFillRect(renderer, &a2);
        SDL_RenderFillRect(renderer, &a3);
        SDL_RenderFillRect(renderer, &a4);

        // P
        SDL_FRect p1 = {340, 180, 15, 40};
        SDL_FRect p2 = {340, 180, 25, 8};
        SDL_FRect p3 = {340, 195, 25, 8};
        SDL_FRect p4 = {355, 188, 10, 7};
        SDL_RenderFillRect(renderer, &p1);
        SDL_RenderFillRect(renderer, &p2);
        SDL_RenderFillRect(renderer, &p3);
        SDL_RenderFillRect(renderer, &p4);

        // P
        SDL_FRect p5 = {375, 180, 15, 40};
        SDL_FRect p6 = {375, 180, 25, 8};
        SDL_FRect p7 = {375, 195, 25, 8};
        SDL_FRect p8 = {390, 188, 10, 7};
        SDL_RenderFillRect(renderer, &p5);
        SDL_RenderFillRect(renderer, &p6);
        SDL_RenderFillRect(renderer, &p7);
        SDL_RenderFillRect(renderer, &p8);

        // Y
        SDL_FRect y1 = {410, 180, 15, 15};
        SDL_FRect y2 = {425, 180, 15, 15};
        SDL_FRect y3 = {417, 195, 8, 25};
        SDL_RenderFillRect(renderer, &y1);
        SDL_RenderFillRect(renderer, &y2);
        SDL_RenderFillRect(renderer, &y3);

        // BIRD
        SDL_SetRenderDrawColor(renderer, 255, 69, 0, 255);
        // B
        SDL_FRect b1 = {220, 240, 15, 40};
        SDL_FRect b2 = {220, 240, 25, 8};
        SDL_FRect b3 = {220, 255, 25, 8};
        SDL_FRect b4 = {220, 272, 25, 8};
        SDL_FRect b5 = {235, 248, 10, 7};
        SDL_FRect b6 = {235, 263, 10, 9};
        SDL_RenderFillRect(renderer, &b1);
        SDL_RenderFillRect(renderer, &b2);
        SDL_RenderFillRect(renderer, &b3);
        SDL_RenderFillRect(renderer, &b4);
        SDL_RenderFillRect(renderer, &b5);
        SDL_RenderFillRect(renderer, &b6);

        // I
        SDL_FRect i1 = {260, 240, 25, 8};
        SDL_FRect i2 = {260, 272, 25, 8};
        SDL_FRect i3 = {267, 240, 8, 40};
        SDL_RenderFillRect(renderer, &i1);
        SDL_RenderFillRect(renderer, &i2);
        SDL_RenderFillRect(renderer, &i3);

        // R
        SDL_FRect r1 = {300, 240, 15, 40};
        SDL_FRect r2 = {300, 240, 25, 8};
        SDL_FRect r3 = {300, 255, 25, 8};
        SDL_FRect r4 = {315, 248, 10, 7};
        SDL_FRect r5 = {315, 263, 15, 17};
        SDL_RenderFillRect(renderer, &r1);
        SDL_RenderFillRect(renderer, &r2);
        SDL_RenderFillRect(renderer, &r3);
        SDL_RenderFillRect(renderer, &r4);
        SDL_RenderFillRect(renderer, &r5);

        // D
        SDL_FRect d1 = {340, 240, 15, 40};
        SDL_FRect d2 = {340, 240, 20, 8};
        SDL_FRect d3 = {340, 272, 20, 8};
        SDL_FRect d4 = {355, 248, 8, 24};
        SDL_RenderFillRect(renderer, &d1);
        SDL_RenderFillRect(renderer, &d2);
        SDL_RenderFillRect(renderer, &d3);
        SDL_RenderFillRect(renderer, &d4);

        // Instructions
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        // "PRESS SPACE TO START"
        for (int i = 0; i < 18; i++)
        {
            SDL_FRect letter = {(float)(220 + i * 18), 320.0f, 15.0f, 12.0f};
            SDL_RenderFillRect(renderer, &letter);
        }

        // "A - Toggle AI Mode"
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        for (int i = 0; i < 16; i++)
        {
            SDL_FRect letter = {(float)(220 + i * 20), 350.0f, 17.0f, 10.0f};
            SDL_RenderFillRect(renderer, &letter);
        }

        // "R - Restart Game"
        for (int i = 0; i < 14; i++)
        {
            SDL_FRect letter = {(float)(220 + i * 22), 370.0f, 19.0f, 10.0f};
            SDL_RenderFillRect(renderer, &letter);
        }
    }

    void drawGameOverScreen()
    {
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 150);
        SDL_FRect overlay = {150.0f, 200.0f, 500.0f, 200.0f};
        SDL_RenderFillRect(renderer, &overlay);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &overlay);

        // Draw "GAME OVER" text
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // G
        SDL_FRect g1 = {200, 230, 15, 25};
        SDL_FRect g2 = {200, 230, 25, 8};
        SDL_FRect g3 = {200, 247, 25, 8};
        SDL_FRect g4 = {215, 238, 10, 17};
        SDL_FRect g5 = {208, 238, 17, 8};
        SDL_RenderFillRect(renderer, &g1);
        SDL_RenderFillRect(renderer, &g2);
        SDL_RenderFillRect(renderer, &g3);
        SDL_RenderFillRect(renderer, &g4);
        SDL_RenderFillRect(renderer, &g5);

        // A
        SDL_FRect a1 = {235, 238, 15, 17};
        SDL_FRect a2 = {250, 238, 15, 17};
        SDL_FRect a3 = {235, 230, 30, 8};
        SDL_FRect a4 = {235, 238, 30, 8};
        SDL_RenderFillRect(renderer, &a1);
        SDL_RenderFillRect(renderer, &a2);
        SDL_RenderFillRect(renderer, &a3);
        SDL_RenderFillRect(renderer, &a4);

        // M
        SDL_FRect m1 = {275, 230, 15, 25};
        SDL_FRect m2 = {305, 230, 15, 25};
        SDL_FRect m3 = {283, 230, 15, 15};
        SDL_FRect m4 = {275, 230, 45, 8};
        SDL_RenderFillRect(renderer, &m1);
        SDL_RenderFillRect(renderer, &m2);
        SDL_RenderFillRect(renderer, &m3);
        SDL_RenderFillRect(renderer, &m4);

        // E
        SDL_FRect e1 = {330, 230, 15, 25};
        SDL_FRect e2 = {330, 230, 25, 8};
        SDL_FRect e3 = {330, 238, 20, 8};
        SDL_FRect e4 = {330, 247, 25, 8};
        SDL_RenderFillRect(renderer, &e1);
        SDL_RenderFillRect(renderer, &e2);
        SDL_RenderFillRect(renderer, &e3);
        SDL_RenderFillRect(renderer, &e4);

        // Space

        // O
        SDL_FRect o1 = {375, 230, 15, 25};
        SDL_FRect o2 = {405, 230, 15, 25};
        SDL_FRect o3 = {375, 230, 45, 8};
        SDL_FRect o4 = {375, 247, 45, 8};
        SDL_RenderFillRect(renderer, &o1);
        SDL_RenderFillRect(renderer, &o2);
        SDL_RenderFillRect(renderer, &o3);
        SDL_RenderFillRect(renderer, &o4);

        // V
        SDL_FRect v1 = {430, 230, 15, 17};
        SDL_FRect v2 = {460, 230, 15, 17};
        SDL_FRect v3 = {437, 247, 15, 8};
        SDL_RenderFillRect(renderer, &v1);
        SDL_RenderFillRect(renderer, &v2);
        SDL_RenderFillRect(renderer, &v3);

        // E
        SDL_FRect e5 = {485, 230, 15, 25};
        SDL_FRect e6 = {485, 230, 25, 8};
        SDL_FRect e7 = {485, 238, 20, 8};
        SDL_FRect e8 = {485, 247, 25, 8};
        SDL_RenderFillRect(renderer, &e5);
        SDL_RenderFillRect(renderer, &e6);
        SDL_RenderFillRect(renderer, &e7);
        SDL_RenderFillRect(renderer, &e8);

        // R
        SDL_FRect r1 = {520, 230, 15, 25};
        SDL_FRect r2 = {520, 230, 25, 8};
        SDL_FRect r3 = {520, 238, 25, 8};
        SDL_FRect r4 = {535, 230, 10, 8};
        SDL_FRect r5 = {535, 246, 15, 9};
        SDL_RenderFillRect(renderer, &r1);
        SDL_RenderFillRect(renderer, &r2);
        SDL_RenderFillRect(renderer, &r3);
        SDL_RenderFillRect(renderer, &r4);
        SDL_RenderFillRect(renderer, &r5);

        // Display score as number
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);

        // Draw "SCORE:" text
        SDL_FRect s1 = {250, 280, 12, 20}; // S
        SDL_FRect s2 = {250, 280, 20, 6};
        SDL_FRect s3 = {250, 287, 20, 6};
        SDL_FRect s4 = {250, 294, 20, 6};
        SDL_RenderFillRect(renderer, &s1);
        SDL_RenderFillRect(renderer, &s2);
        SDL_RenderFillRect(renderer, &s3);
        SDL_RenderFillRect(renderer, &s4);

        // Draw actual score number
        drawNumber(score, 320, 280);

        // Auto-restart countdown
        int countdown = (int)(autoRestartDelay - gameOverTimer) + 1;
        if (countdown > 0)
        {
            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
            drawNumber(countdown, 390, 280);
        }

        // Instructions
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        // "AUTO RESTART IN:"
        for (int i = 0; i < 15; i++)
        {
            SDL_FRect letter = {(float)(200 + i * 20), 330.0f, 18.0f, 12.0f};
            SDL_RenderFillRect(renderer, &letter);
        }
    }

    void drawNumber(int number, int x, int y)
    {
        string numStr = to_string(number);
        for (size_t i = 0; i < numStr.length(); i++)
        {
            int digit = numStr[i] - '0';
            drawDigit(digit, x + i * 25, y);
        }
    }

    void drawDigit(int digit, int x, int y)
    {
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);

        switch (digit)
        {
        case 0:
        {
            SDL_FRect d0_1 = {(float)x, (float)y, 10, 20};
            SDL_FRect d0_2 = {(float)(x + 15), (float)y, 10, 20};
            SDL_FRect d0_3 = {(float)x, (float)y, 25, 6};
            SDL_FRect d0_4 = {(float)x, (float)(y + 14), 25, 6};
            SDL_RenderFillRect(renderer, &d0_1);
            SDL_RenderFillRect(renderer, &d0_2);
            SDL_RenderFillRect(renderer, &d0_3);
            SDL_RenderFillRect(renderer, &d0_4);
            break;
        }
        case 1:
        {
            SDL_FRect d1_1 = {(float)(x + 8), (float)y, 8, 20};
            SDL_RenderFillRect(renderer, &d1_1);
            break;
        }
        case 2:
        {
            SDL_FRect d2_1 = {(float)x, (float)y, 25, 6};
            SDL_FRect d2_2 = {(float)x, (float)(y + 7), 25, 6};
            SDL_FRect d2_3 = {(float)x, (float)(y + 14), 25, 6};
            SDL_FRect d2_4 = {(float)(x + 15), (float)y, 10, 13};
            SDL_FRect d2_5 = {(float)x, (float)(y + 7), 10, 13};
            SDL_RenderFillRect(renderer, &d2_1);
            SDL_RenderFillRect(renderer, &d2_2);
            SDL_RenderFillRect(renderer, &d2_3);
            SDL_RenderFillRect(renderer, &d2_4);
            SDL_RenderFillRect(renderer, &d2_5);
            break;
        }
        case 3:
        {
            SDL_FRect d3_1 = {(float)x, (float)y, 25, 6};
            SDL_FRect d3_2 = {(float)x, (float)(y + 7), 25, 6};
            SDL_FRect d3_3 = {(float)x, (float)(y + 14), 25, 6};
            SDL_FRect d3_4 = {(float)(x + 15), (float)y, 10, 20};
            SDL_RenderFillRect(renderer, &d3_1);
            SDL_RenderFillRect(renderer, &d3_2);
            SDL_RenderFillRect(renderer, &d3_3);
            SDL_RenderFillRect(renderer, &d3_4);
            break;
        }
        case 4:
        {
            SDL_FRect d4_1 = {(float)x, (float)y, 10, 13};
            SDL_FRect d4_2 = {(float)(x + 15), (float)y, 10, 20};
            SDL_FRect d4_3 = {(float)x, (float)(y + 7), 25, 6};
            SDL_RenderFillRect(renderer, &d4_1);
            SDL_RenderFillRect(renderer, &d4_2);
            SDL_RenderFillRect(renderer, &d4_3);
            break;
        }
        case 5:
        {
            SDL_FRect d5_1 = {(float)x, (float)y, 25, 6};
            SDL_FRect d5_2 = {(float)x, (float)(y + 7), 25, 6};
            SDL_FRect d5_3 = {(float)x, (float)(y + 14), 25, 6};
            SDL_FRect d5_4 = {(float)x, (float)y, 10, 13};
            SDL_FRect d5_5 = {(float)(x + 15), (float)(y + 7), 10, 13};
            SDL_RenderFillRect(renderer, &d5_1);
            SDL_RenderFillRect(renderer, &d5_2);
            SDL_RenderFillRect(renderer, &d5_3);
            SDL_RenderFillRect(renderer, &d5_4);
            SDL_RenderFillRect(renderer, &d5_5);
            break;
        }
        case 6:
        {
            SDL_FRect d6_1 = {(float)x, (float)y, 10, 20};
            SDL_FRect d6_2 = {(float)x, (float)y, 25, 6};
            SDL_FRect d6_3 = {(float)x, (float)(y + 7), 25, 6};
            SDL_FRect d6_4 = {(float)x, (float)(y + 14), 25, 6};
            SDL_FRect d6_5 = {(float)(x + 15), (float)(y + 7), 10, 13};
            SDL_RenderFillRect(renderer, &d6_1);
            SDL_RenderFillRect(renderer, &d6_2);
            SDL_RenderFillRect(renderer, &d6_3);
            SDL_RenderFillRect(renderer, &d6_4);
            SDL_RenderFillRect(renderer, &d6_5);
            break;
        }
        case 7:
        {
            SDL_FRect d7_1 = {(float)x, (float)y, 25, 6};
            SDL_FRect d7_2 = {(float)(x + 15), (float)y, 10, 20};
            SDL_RenderFillRect(renderer, &d7_1);
            SDL_RenderFillRect(renderer, &d7_2);
            break;
        }
        case 8:
        {
            SDL_FRect d8_1 = {(float)x, (float)y, 10, 20};
            SDL_FRect d8_2 = {(float)(x + 15), (float)y, 10, 20};
            SDL_FRect d8_3 = {(float)x, (float)y, 25, 6};
            SDL_FRect d8_4 = {(float)x, (float)(y + 7), 25, 6};
            SDL_FRect d8_5 = {(float)x, (float)(y + 14), 25, 6};
            SDL_RenderFillRect(renderer, &d8_1);
            SDL_RenderFillRect(renderer, &d8_2);
            SDL_RenderFillRect(renderer, &d8_3);
            SDL_RenderFillRect(renderer, &d8_4);
            SDL_RenderFillRect(renderer, &d8_5);
            break;
        }
        case 9:
        {
            SDL_FRect d9_1 = {(float)x, (float)y, 10, 13};
            SDL_FRect d9_2 = {(float)(x + 15), (float)y, 10, 20};
            SDL_FRect d9_3 = {(float)x, (float)y, 25, 6};
            SDL_FRect d9_4 = {(float)x, (float)(y + 7), 25, 6};
            SDL_FRect d9_5 = {(float)x, (float)(y + 14), 25, 6};
            SDL_RenderFillRect(renderer, &d9_1);
            SDL_RenderFillRect(renderer, &d9_2);
            SDL_RenderFillRect(renderer, &d9_3);
            SDL_RenderFillRect(renderer, &d9_4);
            SDL_RenderFillRect(renderer, &d9_5);
            break;
        }
        }
    }

    void drawDebugInfo()
    {
        vector<float> state = getState();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
        SDL_FRect debugBG = {10.0f, 120.0f, 200.0f, 150.0f};
        SDL_RenderFillRect(renderer, &debugBG);

        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderRect(renderer, &debugBG);

        // State visualization
        for (int i = 0; i < 5; i++)
        {
            float value = state[i];
            int bars = (int)((value + 1.0f) * 10);

            for (int j = 0; j < bars && j < 20; j++)
            {
                SDL_FRect bar = {(float)(20 + j * 8), (float)(140 + i * 25), 6.0f, 15.0f};
                SDL_RenderFillRect(renderer, &bar);
            }
        }
    }

    void setAgent(QAgent *aiAgent)
    {
        agent = aiAgent;
    }

    void run()
    {
        if (!window || !renderer)
        {
            cout << "Failed to initialize SDL properly. Exiting." << endl;
            return;
        }

        Uint64 lastTime = SDL_GetTicks();

        cout << "===============================================================================" << endl;
        cout << "FLAPPY BIRD AI - Professional Gaming Experience" << endl;
        cout << "Controls:" << endl;
        cout << "   R     - Reset game manually" << endl;
        cout << "   D     - Toggle debug info" << endl;
        cout << "   ESC   - Quit" << endl;
        cout << "AI will be " << (agent ? "ACTIVE" : "INACTIVE") << " when toggled" << endl;
        cout << "Game auto-restarts every " << autoRestartDelay << " seconds after game over" << endl;
        cout << "===============================================================================" << endl;

        bool running = true;
        while (running)
        {
            try
            {
                Uint64 currentTime = SDL_GetTicks();
                float deltaTime = (currentTime - lastTime) / 1000.0f;

                // Cap deltaTime to prevent huge jumps
                if (deltaTime > 0.1f)
                    deltaTime = 0.1f;

                lastTime = currentTime;

                handleEvents();
                update(deltaTime);
                render();

                SDL_Delay(16); // 60 FPS
            }
            catch (...)
            {
                cout << "Error in game loop, continuing..." << endl;
                SDL_Delay(100); // Brief pause before continuing
            }
        }
    }

    ~FlappyBirdGame()
    {
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(CP_UTF8);
    srand(time(nullptr));

    cout << "🎮 ═══════════════════════════════════════════════════════════════════════════" << endl;
    cout << "🐦 FLAPPY BIRD AI - Interactive Gameplay" << endl;
    cout << "🎯 Load a trained model to watch AI play!" << endl;
    cout << "   ═══════════════════════════════════════════════════════════════════════════" << endl;

    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " <model_file>" << endl;
        cout << "Example: " << argv[0] << " trained_model.dat" << endl;
        return 1;
    }

    QAgent agent;
    FlappyBirdGame game;
    string modelPath = argv[1];
    
    cout << "🤖 Loading AI model from: " << modelPath << endl;

    if (agent.loadModel(modelPath))
    {
        cout << "✅ Successfully loaded model with " << agent.getQTableSize() << " learned states!" << endl;
        agent.setEpsilon(0.0f); // No exploration during play
        game.setAgent(&agent);
        cout << "\n🚀 Starting game with auto-restart every 1 second..." << endl;
        cout << "Max score will be tracked across all games." << endl;
        game.run();
    }
    else
    {
        cout << "❌ Failed to load model: " << modelPath << endl;
        return 1;
    }

    return 0;
}