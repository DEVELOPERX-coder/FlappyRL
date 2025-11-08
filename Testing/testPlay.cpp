#include <SDL3/SDL.h>
#include <iostream>
#include <windows.h>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <fstream>

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
    unordered_map<string, vector<float>> qTable;

    string getStateKey(const vector<float> &state)
    {
        int posBirdY = state[0];
        int posGapY = state[1];

        return to_string(posBirdY) + "_" + to_string(posGapY);
    }

public:
    QAgent();

    int selectAction(const vector<float> &state)
    {
        string stateKey = getStateKey(state);
        if (qTable.find(stateKey) == qTable.end())
        {
            qTable[stateKey] = {1.0f, 0.0f};
            return 1;
        }

        float action = qTable[stateKey][0];
        float reward = qTable[stateKey][1];

        return action;
    }

    void updateQ(const vector<float> &state, int action, float reward, const vector<float> &nextState, bool terminal)
    {
        string stateKey = getStateKey(state);
        if (qTable.find(stateKey) == qTable.end())
        {
            qTable[stateKey] = {1.0f, 0.0f};
        }

        float target = reward;
        if (!terminal)
        {
            string nextStateKey = getStateKey(nextState);
            if (qTable.find(nextStateKey) == qTable.end())
            {
                qTable[nextStateKey] = {1.0f, 0.0f};
            }
        }
    }

    void saveModel(const string &filename)
    {
        ofstream file(filename);
        file << qTable.size() << endl;

        for (const auto &entry : qTable)
        {
            file << entry.first << " " << entry.second[0] << " " << entry.second[1] << endl;
        }

        file.close();
    }

    bool loadModel(const string &filename)
    {
        ifstream file(filename);
        if (!file.is_open())
            return false;
        size_t size;
        file >> size;

        qTable.clear();
        string stateKey;
        float action, reward;

        for (size_t i = 0; i < size; ++i)
        {
            if (file >> stateKey >> action >> reward)
            {
                qTable[stateKey] = {action, reward};
            }
        }

        file.close();
        return true;
    }

    size_t getQTableSize() const { return qTable.size(); }
};

class Game
{
private:
    SDL_Window *window;
    SDL_Renderer *renderer;
    Bird bird;
    vector<Pipe> pipes;
    int frameCount;
    float pipeSpawnTimer;
    float pipeSpawnInterval;
    int score;
    bool gameOver;

public:
    Game() : window(nullptr), renderer(nullptr), frameCount(0), pipeSpawnTimer(0), pipeSpawnInterval(2.8f), score(0), gameOver(false)
    {
        Initialize();
    }

    bool Initialize()
    {
        window = SDL_CreateWindow("Reinforced Flappy Bird", 800, 600, 0);
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
        frameCount = 0;

        pipes.push_back(Pipe(800));
    }

    vector<float> getState()
    {
        vector<float> state(2, 0.0f);
        state[0] = bird.y;
        state[1] = -1;

        if (pipes.size() > 0)
            state[1] = pipes[0].gapY / 2 + pipes[0].getTopRect().h;

        return state;
    }

    float step(int action)
    {
        if (gameOver)
            return 0.0f;

        float deltaTime = 1.0f / 33.33f;
        float reward = 1.0f;
        ++frameCount;

        if (action == 1)
            bird.flap();

        bird.update(deltaTime);
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
                case SDLK_SPACE:
                    bird.flap();
                    break;
                case SDLK_R:
                    cout << "Pressed R" << endl;
                    break;

                case SDLK_ESCAPE:
                    cout << "Closing the game!" << endl;
                    exit(0);
                    break;
                }
            }
        }
    }

    void update(float deltaTime)
    {
        if (gameOver)
        {
            reset();
        }

        ++frameCount;

        bird.update(deltaTime);

        if (bird.y > 580 || bird.y < 20)
        {
            gameOver = true;
            cout << "Game OVER! : Bird Struck By Height" << endl;
            cout << " SCORE IS : " << score << endl;
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

            if (SDL_HasRectIntersectionFloat(&birdRect, &topPipe) || SDL_HasRectIntersectionFloat(&birdRect, &bottomPipe))
            {
                gameOver = true;
                cout << "Bird Collided with Piller" << endl;
                cout << "SCORE : " << score << endl;
                return;
            }

            if (pipe.hasPassedBird(bird.x))
            {
                ++score;
                pipe.scored = true;
                // cout << "Score Increase : " << score << endl;
            }
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

        // Moving Clouds
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 120);
        for (int i = 0; i < 4; ++i)
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

            // Main Pipes
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
            SDL_FRect topCap = {topPipe.x - 4, topPipe.y + topPipe.h - 25, topPipe.w + 12, 25.0f};
            SDL_FRect bottomCap = {bottomPipe.x - 4, bottomPipe.y, bottomPipe.w + 12, 25.0f};
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

        drawUI();

        SDL_RenderPresent(renderer);
    }

    void drawUI()
    {
        // Score panel
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
        SDL_FRect scoreBG = {5.0f, 550.0f, 200.0f, 50.0f};
        SDL_RenderFillRect(renderer, &scoreBG);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderRect(renderer, &scoreBG);

        // Draw "SCORE:" label
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        // S
        SDL_FRect s1 = {10, 564, 8, 8};
        SDL_FRect s5 = {18, 572, 8, 8};
        SDL_FRect s2 = {10, 564, 16, 4};
        SDL_FRect s3 = {10, 571, 16, 4};
        SDL_FRect s4 = {10, 578, 16, 4};
        SDL_RenderFillRect(renderer, &s1);
        SDL_RenderFillRect(renderer, &s5);
        SDL_RenderFillRect(renderer, &s2);
        SDL_RenderFillRect(renderer, &s3);
        SDL_RenderFillRect(renderer, &s4);

        // C
        SDL_FRect c1 = {28, 564, 8, 16};
        SDL_FRect c2 = {28, 564, 16, 4};
        SDL_FRect c3 = {28, 578, 16, 4};
        SDL_RenderFillRect(renderer, &c1);
        SDL_RenderFillRect(renderer, &c2);
        SDL_RenderFillRect(renderer, &c3);

        // O
        SDL_FRect o1 = {46, 564, 8, 16};
        SDL_FRect o2 = {58, 564, 8, 16};
        SDL_FRect o3 = {46, 564, 20, 4};
        SDL_FRect o4 = {46, 578, 20, 4};
        SDL_RenderFillRect(renderer, &o1);
        SDL_RenderFillRect(renderer, &o2);
        SDL_RenderFillRect(renderer, &o3);
        SDL_RenderFillRect(renderer, &o4);

        // R
        SDL_FRect r1 = {68, 564, 6, 18};
        SDL_FRect r2 = {68, 564, 16, 4};
        SDL_FRect r3 = {68, 571, 16, 4};
        SDL_FRect r4 = {79, 564, 5, 7};
        SDL_FRect r5 = {79, 575, 5, 7};
        SDL_RenderFillRect(renderer, &r1);
        SDL_RenderFillRect(renderer, &r2);
        SDL_RenderFillRect(renderer, &r3);
        SDL_RenderFillRect(renderer, &r4);
        SDL_RenderFillRect(renderer, &r5);

        // E
        SDL_FRect e1 = {86, 564, 8, 16};
        SDL_FRect e2 = {86, 564, 16, 4};
        SDL_FRect e3 = {86, 571, 12, 4};
        SDL_FRect e4 = {86, 578, 16, 4};
        SDL_RenderFillRect(renderer, &e1);
        SDL_RenderFillRect(renderer, &e2);
        SDL_RenderFillRect(renderer, &e3);
        SDL_RenderFillRect(renderer, &e4);

        // Colon
        SDL_FRect colon1 = {106, 567, 3, 3};
        SDL_FRect colon2 = {106, 575, 3, 3};
        SDL_RenderFillRect(renderer, &colon1);
        SDL_RenderFillRect(renderer, &colon2);

        // Draw actual score number
        SDL_SetRenderDrawColor(renderer, 255, 215, 0, 255);
        drawNumber(score, 120, 563);
    }

    void drawNumber(int number, int x, int y)
    {
        string numStr = to_string(number);
        for (size_t i = 0; i < numStr.length(); ++i)
        {
            int digit = numStr[i] - '0';
            drawDigit(digit, x + i * 25, y);
        }
    }

    void drawDigit(int digit, int x, int y)
    {
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

    void run()
    {
        if (!window || !renderer)
        {
            cout << "Failed  to initialized SDL properly. Exiting." << endl;
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
                SDL_Delay(33.33); // 30 FPS
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