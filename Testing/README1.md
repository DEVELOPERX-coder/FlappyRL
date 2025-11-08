# FlappyRL - Flappy Bird with Reinforcement Learning

🐦 A modern implementation of Flappy Bird featuring AI-powered gameplay using advanced Q-Learning algorithms.

## 🌟 Features

- **Classic Flappy Bird Gameplay**: Traditional mechanics with enhanced graphics
- **AI-Powered Bird**: Advanced Q-Learning agent that learns to play autonomously
- **Dual Mode**: Switch between manual control and AI mode during gameplay
- **Professional Graphics**: Enhanced visual effects with gradients, shadows, and animations
- **Real-time Performance**: Optimized 60 FPS gameplay with smooth animations
- **Training System**: Complete AI training pipeline with progress tracking
- **Multiple Models**: Save and load different AI models for various skill levels

## 🎮 Game Modes

### Interactive Play Mode (`flappy_play.exe`)

- **Manual Control**: Play the classic Flappy Bird yourself
- **AI Assistant**: Toggle AI mode to watch the trained agent play
- **Real-time Switching**: Press 'A' to switch between manual and AI control
- **Debug Information**: Press 'D' to view AI state information

### AI Training Mode (`flappy_ai.exe`)

- **Advanced Q-Learning**: Superior state representation with 5-dimensional state space
- **Automated Training**: Train agents with customizable episode counts
- **Performance Tracking**: Real-time statistics and progress monitoring
- **Model Management**: Automatic saving of best-performing models
- **Benchmark Testing**: Comprehensive performance evaluation

## 🚀 Quick Start

### Prerequisites

- Windows (with MSYS2/MinGW-w64 for compilation)
- SDL3 library (included in project)
- C++17 compatible compiler

### Building the Project

```bash
# Compile the interactive game
g++ -std=c++17 -I./SDL3/include -L./SDL3/lib flappy_play.cpp -lSDL3 -o flappy_play.exe

# Compile the AI training system
g++ -std=c++17 -I./SDL3/include -L./SDL3/lib flappy_ai.cpp -lSDL3 -o flappy_train.exe
```

### Running the Game

#### Interactive Play

```bash
# Play manually
./flappy_play.exe

# Play with pre-trained AI model
./flappy_play.exe model_file.dat
```

#### AI Training

```bash
# Quick training (default: 5000 episodes)
./flappy_train.exe

# Custom training
./flappy_train.exe train 10000

# Watch trained AI play
./flappy_train.exe play superior_best_model.dat

# Quick demo
./flappy_train.exe demo

# Intensive training (20000 episodes)
./flappy_train.exe intensive

# Performance benchmark
./flappy_train.exe benchmark
```

## 🎯 Controls

### Interactive Mode

- **SPACE**: Flap bird / Start game / Restart
- **A**: Toggle AI mode ON/OFF
- **R**: Reset/Restart game
- **D**: Toggle debug information display
- **ESC**: Quit game

### Training Mode

- **SPACE**: Skip current episode (during visualization)
- **ENTER**: Continue to next episode
- **ESC**: Quit training

## 🧠 AI System

### Advanced Q-Learning Algorithm

The AI uses a sophisticated Q-Learning implementation with:

- **5-Dimensional State Space**:

  1. Bird Y position (normalized 0-1)
  2. Bird velocity (normalized -1 to 1)
  3. Horizontal distance to next pipe (normalized 0-1)
  4. Gap center Y position (normalized 0-1)
  5. Vertical distance from bird to gap center (normalized -1 to 1)

- **Enhanced Reward System**:

  - Survival rewards for staying alive
  - Collision penalties with distance-based scaling
  - Score bonuses with center-passage rewards
  - Velocity control incentives
  - Position optimization rewards

- **Adaptive Learning**:
  - Epsilon-greedy exploration with decay
  - Dynamic learning rate adjustment
  - Progressive difficulty scaling
  - Multiple model saving strategies

### Model Files

The training system generates several model files:

- `superior_trained_model.dat`: Final trained model
- `superior_best_model.dat`: Highest scoring model
- `superior_average_model.dat`: Best average performance model
- `superior_survival_model.dat`: Longest survival model

## 📊 Performance Metrics

The AI system tracks comprehensive performance metrics:

- **Score Performance**: Average and peak scores
- **Survival Time**: Frame-based survival tracking
- **Success Rate**: Percentage of successful episodes
- **Learning Progress**: Q-table size and exploration rate
- **Training Efficiency**: Episodes per minute and convergence time

### Typical Performance Levels

- **Beginner**: 0-3 average score
- **Intermediate**: 3-8 average score
- **Advanced**: 8-15 average score
- **Expert**: 15-25 average score
- **Master**: 25+ average score

## 🛠️ Technical Details

### Architecture

- **Language**: C++17
- **Graphics**: SDL3 (cross-platform multimedia library)
- **AI Algorithm**: Enhanced Q-Learning with superior state representation
- **Rendering**: Hardware-accelerated 2D graphics with effects
- **Performance**: Optimized for 60 FPS with minimal memory usage

### System Requirements

- **OS**: Windows 10/11 (primary), Linux/macOS (with SDL3)
- **Memory**: 50MB RAM minimum
- **Graphics**: Any modern graphics card with 2D acceleration
- **Storage**: 10MB disk space
- **CPU**: Any modern processor (training benefits from faster CPUs)

## 🎨 Visual Features

### Enhanced Graphics

- **Gradient Sky**: Dynamic sky background with color transitions
- **Animated Clouds**: Moving cloud formations with parallax effects
- **3D Pipe Effects**: Shadows, highlights, and depth illusions
- **Animated Bird**: Dynamic bird with directional awareness and color coding
- **Professional UI**: Clean interface with real-time statistics
- **Particle Effects**: Motion blur and shadow effects

### Display Elements

- **Real-time Score**: Digital score display with proper number rendering
- **Performance Meters**: Velocity and survival indicators
- **Mode Indicators**: Visual feedback for AI/manual modes
- **Debug Information**: Comprehensive state visualization
- **Progress Tracking**: Training progress with visual feedback

## 🔧 Customization

### Training Parameters

You can modify training parameters in the code:

- Episode count: Adjust training duration
- Learning rate: Control learning speed
- Epsilon decay: Modify exploration strategy
- Reward structure: Customize reward functions
- State representation: Adjust state discretization

### Game Physics

Customize game difficulty by modifying:

- Bird gravity and jump strength
- Pipe speed and gap size
- Collision detection sensitivity
- Scoring thresholds

## 📈 Training Tips

### For Best Results

1. **Start with default settings** for initial training
2. **Use intensive mode** for maximum performance
3. **Test multiple models** to find the best performer
4. **Monitor training progress** through console output
5. **Save frequently** during long training sessions

### Troubleshooting

- **Low performance**: Increase training episodes
- **Poor convergence**: Adjust learning parameters
- **Crashes**: Check SDL3 installation and dependencies
- **Model loading errors**: Verify file paths and permissions

## 🤝 Contributing

This project welcomes contributions! Areas for improvement:

- Additional AI algorithms (Deep Q-Learning, Policy Gradients)
- Cross-platform compatibility enhancements
- Graphics and animation improvements
- Performance optimizations
- New game features and modes

## 📄 License

This project is available under the MIT License. See LICENSE file for details.

## 🙏 Acknowledgments

- **SDL Development Team**: For the excellent SDL3 multimedia library
- **Flappy Bird**: Original game concept by Dong Nguyen
- **Q-Learning Research**: Based on reinforcement learning principles
- **Community**: Thanks to all contributors and testers

---

**Created with ❤️ for learning and entertainment**

_Experience the perfect blend of classic gaming and modern AI technology!_
