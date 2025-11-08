# Reinforcement Learning Flappy Bird - Complete Technical Guide

## Table of Contents
1. [Introduction](#introduction)
2. [Basic Concepts](#basic-concepts)
3. [How the AI Works](#how-the-ai-works)
4. [Implementation Details](#implementation-details)
5. [Training Process](#training-process)
6. [Improvement Strategies](#improvement-strategies)
7. [Advanced Techniques](#advanced-techniques)
8. [Troubleshooting](#troubleshooting)

## Introduction

This project implements a Q-Learning agent that learns to play Flappy Bird through reinforcement learning. The AI starts with no knowledge of the game and learns optimal strategies through trial and error, eventually achieving expert-level gameplay.

## Basic Concepts

### What is Reinforcement Learning?

Reinforcement Learning (RL) is a type of machine learning where an agent learns by interacting with an environment. Key components:

- **Agent**: The AI player (the bird)
- **Environment**: The game world (pipes, gravity, boundaries)
- **State**: Current situation (bird position, velocity, pipe location)
- **Action**: What the agent can do (flap or don't flap)
- **Reward**: Feedback for actions (positive for survival, negative for collision)
- **Policy**: The strategy for choosing actions

### Q-Learning Algorithm

Q-Learning is a model-free RL algorithm that learns a Q-function: Q(state, action) → expected future reward.

**The Q-Learning update formula:**
```
Q(s,a) = (1-α) * Q(s,a) + α * [r + γ * max(Q(s',a'))]
```
Where:
- α (alpha) = learning rate (0.1 in our code)
- γ (gamma) = discount factor (0.95 in our code)
- r = immediate reward
- s' = next state
- a' = next action

## How the AI Works

### 1. State Representation (testTrain.cpp:288-331)

The AI perceives the game through 5 normalized features:

```cpp
state[0] = bird.y / 600.0f;           // Bird's vertical position (0-1)
state[1] = bird.velocity / 600.0f;    // Bird's velocity (-1 to 1)
state[2] = (pipe.x - bird.x) / 400.0f;// Distance to next pipe (0-1)
state[3] = pipe.gapY / 600.0f;        // Gap center position (0-1)
state[4] = (pipe.gapY - bird.y) / 300.0f; // Vertical distance to gap (-1 to 1)
```

### 2. State Discretization (testTrain.cpp:95-105)

Continuous states are converted to discrete bins for Q-table storage:

```cpp
int birdY = max(0, min(11, (int)(state[0] * 12)));     // 12 bins
int velocity = max(0, min(7, (int)((state[1] + 1.0f) * 4))); // 8 bins
int pipeX = max(0, min(9, (int)(state[2] * 10)));      // 10 bins
int vertDist = max(0, min(7, (int)((state[4] + 1.0f) * 4))); // 8 bins
```

Total possible states: 12 × 8 × 10 × 8 = 7,680

### 3. Action Selection (testTrain.cpp:113-140)

Uses ε-greedy strategy:
- With probability ε: Choose random action (exploration)
- With probability 1-ε: Choose best known action (exploitation)

```cpp
if (uniformDist(rng) < epsilon) {
    return rng() % 2;  // Random: 0 (don't flap) or 1 (flap)
}
// Otherwise, choose action with highest Q-value
```

### 4. Reward System (testTrain.cpp:333-423)

The reward structure shapes learning behavior:

```cpp
// Base survival reward
reward = 1.0f per frame

// Passing a pipe successfully
reward = 1000.0f

// Height bonus (encourages staying centered)
heightBonus = 1.0f - abs(bird.y - 300.0f) / 300.0f
reward += heightBonus * 0.1f

// Death penalty
reward = -1000.0f (collision or boundary hit)
```

### 5. Q-Table Update (testTrain.cpp:142-170)

After each action, the Q-table is updated:

```cpp
// Calculate target value
target = reward + gamma * max(Q(nextState, all_actions))

// Update Q-value
Q(state, action) = (1 - learningRate) * Q(state, action) + learningRate * target

// Decay exploration rate
epsilon *= epsilonDecay
```

## Implementation Details

### Game Components

#### Bird Class (testTrain.cpp:15-39)
- Position (x, y)
- Velocity affected by gravity (800 px/s²)
- Jump strength (-400 px/s)
- Size (20x20 pixels)

#### Pipe Class (testTrain.cpp:41-79)
- Random gap position (180-420 pixels)
- Fixed gap height (180 pixels)
- Movement speed (200 px/s)
- Collision detection rectangles

#### QAgent Class (testTrain.cpp:81-214)
Key parameters:
- **epsilon**: 1.0 → 0.05 (exploration rate)
- **epsilonDecay**: 0.9997 (gradual reduction)
- **learningRate**: 0.1 (Q-value update speed)
- **gamma**: 0.95 (future reward importance)

### Training Process

#### Episode Structure (testTrain.cpp:441-599)
1. Reset game environment
2. Get initial state
3. Loop until game over:
   - Select action using ε-greedy
   - Execute action
   - Observe reward and new state
   - Update Q-table
   - Move to new state
4. Record episode statistics
5. Save model periodically

#### Convergence Criteria (testTrain.cpp:536-541)
Training stops early if:
- 100+ consecutive successful episodes
- Best score > 30
- Episode count > 1000

## Improvement Strategies

### 1. State Space Optimization

**Current discretization:**
```cpp
// Reduce bins for faster convergence
int birdY = max(0, min(7, (int)(state[0] * 8)));  // 8 bins instead of 12
```

**Add relative velocity to pipe:**
```cpp
state[5] = clamp((bird.velocity - pipe.speed) / 400.0f, -1.0f, 1.0f);
```

### 2. Reward Shaping Improvements

**Progressive difficulty rewards:**
```cpp
float difficultyMultiplier = 1.0f + (episode / 1000.0f);
reward *= difficultyMultiplier;
```

**Smooth approach rewards:**
```cpp
if (approaching_gap_center) {
    reward += 5.0f;
}
```

**Penalty for extreme positions:**
```cpp
if (bird.y < 100 || bird.y > 500) {
    reward -= 2.0f;  // Discourage flying too high/low
}
```

### 3. Learning Rate Scheduling

**Adaptive learning rate:**
```cpp
learningRate = max(0.01f, 0.1f * pow(0.99f, episode));
```

**Experience-based adjustment:**
```cpp
if (qTable.size() > 5000) {
    learningRate *= 0.95f;
}
```

### 4. Exploration Strategies

**Boltzmann exploration (temperature-based):**
```cpp
float temperature = max(0.1f, 1.0f * exp(-episode / 500.0f));
float prob_flap = exp(Q[state][1] / temperature) / 
                  (exp(Q[state][0] / temperature) + exp(Q[state][1] / temperature));
```

**Optimistic initialization:**
```cpp
qTable[stateKey] = {10.0f, 10.0f};  // Start optimistic instead of {0, 0}
```

### 5. Memory and Experience Replay

**Experience buffer implementation:**
```cpp
struct Experience {
    vector<float> state;
    int action;
    float reward;
    vector<float> nextState;
    bool terminal;
};

vector<Experience> replayBuffer;
// Sample and learn from past experiences
```

## Advanced Techniques

### 1. Double Q-Learning
Reduces overestimation bias:
```cpp
// Maintain two Q-tables
unordered_map<string, vector<float>> qTable1, qTable2;

// Randomly update one
if (rand() % 2 == 0) {
    target = reward + gamma * qTable2[nextState][argmax(qTable1[nextState])];
    qTable1[state][action] = (1-α) * qTable1[state][action] + α * target;
}
```

### 2. Eligibility Traces (SARSA-λ)
Speeds up learning by updating multiple states:
```cpp
unordered_map<string, float> eligibilityTraces;
float lambda = 0.9f;

// Update traces
for (auto& trace : eligibilityTraces) {
    trace.second *= gamma * lambda;
}
eligibilityTraces[currentState] = 1.0f;

// Update all states in trace
for (auto& trace : eligibilityTraces) {
    qTable[trace.first][action] += alpha * delta * trace.second;
}
```

### 3. Neural Network Approximation (DQN)
For continuous state space:
```cpp
class NeuralNetwork {
    // Input: 5 state features
    // Hidden: 128 neurons
    // Output: 2 Q-values (don't flap, flap)
};
```

### 4. Prioritized Experience Replay
Learn from important experiences more often:
```cpp
priority[i] = abs(TD_error) + epsilon;
sampling_probability = priority[i] / sum(priorities);
```

### 5. Curriculum Learning
Gradually increase difficulty:
```cpp
// Start with wider gaps
float gapHeight = 220 - (episode / 100) * 4;  // 220 → 180

// Increase pipe speed
float pipeSpeed = 150 + (episode / 200) * 10;  // 150 → 200
```

## Performance Metrics

### Key Indicators
1. **Average Score**: Rolling average over last 500-1000 episodes
2. **Survival Time**: Frames survived before collision
3. **Success Rate**: Percentage of episodes with score > 0
4. **Q-Table Size**: Number of unique states encountered
5. **Convergence Speed**: Episodes to reach stable performance

### Expected Performance Milestones
- Episodes 0-100: Random behavior, rare successes
- Episodes 100-500: Learning basic flapping
- Episodes 500-1500: Consistent pipe passing
- Episodes 1500-3000: Refinement and optimization
- Episodes 3000+: Expert-level play (avg score > 15)

## Troubleshooting

### Common Issues and Solutions

#### 1. Slow Learning
**Problem**: Agent takes too many episodes to learn
**Solutions**:
- Increase learning rate (try 0.15-0.2)
- Reduce state space bins
- Increase initial epsilon decay
- Add reward shaping

#### 2. Unstable Performance
**Problem**: Score fluctuates wildly
**Solutions**:
- Decrease learning rate
- Increase gamma (0.97-0.99)
- Use experience replay
- Implement Double Q-Learning

#### 3. Getting Stuck
**Problem**: Agent always chooses same action
**Solutions**:
- Ensure epsilon doesn't decay too fast
- Check reward balance
- Add exploration bonus
- Reset epsilon periodically

#### 4. Memory Issues
**Problem**: Q-table grows too large
**Solutions**:
- Coarser state discretization
- Implement state pruning
- Use function approximation
- Limit maximum states

## Building and Running

### Compilation
```bash
# Training
g++ -o train testTrain.cpp -lSDL3 -std=c++17 -O3

# Playing with trained model
g++ -o play play.cpp -lSDL3 -std=c++17 -O3
```

### Training
```bash
# Train for 5000 episodes
./train 5000

# Output: trained_model.dat
```

### Testing
```bash
# Run trained agent
./play
```

## Model File Format

The `trained_model.dat` file contains:
```
[epsilon_value]
[q_table_size]
[state_key] [q_value_no_flap] [q_value_flap]
...
```

Example:
```
0.05
3456
5_3_7_4 -12.5 156.8
5_3_7_5 -8.2 203.4
...
```

## Future Improvements

1. **Deep Q-Network (DQN)**: Replace Q-table with neural network
2. **Policy Gradient Methods**: Direct policy optimization
3. **A3C Algorithm**: Asynchronous advantage actor-critic
4. **Transfer Learning**: Pre-train on similar games
5. **Hyperparameter Optimization**: Automatic tuning using grid/random search
6. **Visualization Tools**: Real-time Q-value heatmaps
7. **Multi-agent Training**: Competitive/cooperative learning
8. **Continuous Action Space**: Variable jump strength

## Conclusion

This implementation demonstrates fundamental reinforcement learning concepts through a practical game. The Q-Learning agent successfully learns to play Flappy Bird from scratch, achieving superhuman performance through experience and optimization. The modular design allows for easy experimentation with different algorithms and improvements.