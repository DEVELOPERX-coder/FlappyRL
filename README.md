# FlappyRL - Flappy Bird with Reinforcement Learning, Genetic Algorithm

A project that trains a "flappy bird" agent to play the game using a Genetic Algorithm and a simple Neural Network, built with C++ and SDL3.

### 🧑‍💻 General Information

- **Maintainer:** [Sachin Rayal / [LinkedIn](https://www.linkedin.com/in/sachin-rayal/) / [GitHub](https://github.com/DEVELOPERX-coder/FlappyRL)]
- **Date of Development:** November 2025
- **Keywords:** Flappy Bird, Genetic Algorithm, Neural Network, Neuroevolution, C++, SDL3, Game AI

### 🛡️ Badges

---

## I. Project Overview and Motivation

### The Problem

Flappy Bird is a game that is simple to understand but deceptively difficult to play. For an AI, the challenge is to navigate a continuous stream of obstacles based on limited sensory input. How can an agent learn to master this task without being explicitly programmed with the "correct" solution?

### The Solution

This project implements a **Genetic Algorithm (GA)** to train a population of Flappy Bird agents. Each agent (or "bird") is controlled by its own individual **Neural Network (NN)**. The game runs, and birds that survive longer and pass more pipes are deemed "more fit."

### The Contribution

This repository provides a complete, self-contained C++/SDL3 application that visually demonstrates the process of neuroevolution (evolving neural networks using genetic algorithms). By running the application, you can watch in real-time as the population of birds evolves from random, chaotic flapping (in Generation 1) to intelligent, obstacle-avoiding behavior in later generations.

---

## II. Table of Contents

- [I. Project Overview and Motivation]
- [II. Table of Contents]
- [III. Methodological Framework: Explaining the "How"]
  - [Architecture Overview]
  - [Algorithmic Deep Dive]
- [IV. Project Usage & "Real-Life" Application]
- [V. Getting Started: Installation and Setup]
- [VI. Project Roadmap, Limitations, and Future Work]
- [VII. Guidelines for Collaboration and Contribution]
- [VIII. Project Licensing]

---

## III. Methodological Framework: Explaining the "How"

### Architecture Overview

The project is broken down into several key classes that manage the game, the agents, and the environment:

- **`Game` Class:** The core of the application. It initializes the SDL3 window and renderer, manages the main game loop (`run`), and handles physics updates, rendering, and pipe spawning. It owns the `Population` object and the `std::vector<Pipe>` of obstacles.
- **`Population` Class:** Manages the entire population of `Bird` agents. It is initialized with a `populationSize` (15) and `mutationRate` (0.05). Its most important job is to call `evolveNewGeneration` when all birds are dead.
- **`Bird` Class:** This is the AI agent. It contains its own physics (coordinates, velocity, gravity) and a complete neural network. The NN is defined by its weights and biases (`weights_hh`, `weights_ho`, `bias_h`, `bias_o`). It also includes key methods like `feedForward`, `mutate`, and `flap`.
- **`Pipe` Class:** A simple class for the obstacles. It manages its own position, gap, and movement speed (including the vertical oscillation).
- **`Ray` Struct / `generate_rays()` Function:** The `generate_rays` function acts as the "eyes" for the bird. It casts `RAYS_NUMBER` (10) rays from the bird in a 180° arc. It detects the distance to the first object it hits (a pipe or the edge of the screen).

### Algorithmic Deep Dive

The learning process is a classic example of neuroevolution.

1.  **Input (Sensors):** For each bird, the `generate_rays` function casts **10 rays**. The distance calculated for each ray is used as the input. This gives the bird a 10-dimensional "view" of its immediate surroundings.

2.  **Neural Network (The "Brain"):** Each bird has a neural network with a hardcoded architecture, defined in `Population::Population`:

    - **10 Input Nodes:** One for each of the 10 raycast distances.
    - **Two 11-Node Hidden Layers:** `{11, 11}`. These layers process the input.
    - **1 Output Node:** This final node's value is passed through a `sigmoid` function, resulting in a value between 0 and 1.

3.  **Action (Decision Making):** In the `Game::run` loop, if the bird's `feedForward` output is **greater than 0.5**, the `bird.flap()` method is called. This single output node effectively decides "to flap" or "not to flap" at every frame.

4.  **Genetic Algorithm (Evolution):**

    - **Fitness Function:** When a bird is `gameOver`, its "fitness" is calculated. The formula is `fitness = (score * 10) + survivalFrames`. This strongly rewards passing pipes but also gives credit for simply staying alive.
    - **Selection (Elitism):** When all birds in the population are dead, `Population::evolveNewGeneration` is called. This function finds the **top 3 birds** (elites) from the previous generation based on their fitness.
    - **Reproduction & Mutation:** The new generation is created from these elites.
      - The top 3 elites are copied directly into the new generation.
      - The rest of the population is created by making mutated **clones** of these top 3 elites.
      - The `Bird::mutate` function applies the `mutationRate` (5%) chance to every single weight and bias in the bird's neural network, adding a small random value to it. This introduces new "ideas" into the population.

This cycle repeats, and with each generation, the population becomes better adapted to the environment.

---

## IV. Project Usage & "Real-Life" Application

### Quick Start

After compiling (see section V), simply run the executable:

```bash
# On Windows
./game.exe

# On Linux/macOS
./game
```

### What You Will See

- An SDL window will open, and the simulation will begin immediately.
- You will see **15 birds** (the `populationSize`) playing the game at the same time.
- **White Lines:** These are the rendered `Rays` for the birds, showing what they "see."
- **Different Colored Birds:** The birds are rendered in three different colors (`Bird_1.png`, `Bird_2.png`, `Bird_3.png`) to visually identify which elite "family" they belong to.
- **Console Output:** The console will log `Evolving Population : GENERATION : X` each time a new generation is created.

Watch as the first few generations die off almost immediately. After 10-20 generations, you will start to see birds that have "learned" to navigate the first few pipes.

### Controls

- **`Q` Key:** Quit the application.
- **Window Close Button:** Quit the application.

---

## V. Getting Started: Installation and Setup

### 1\. Prerequisites

Before you can compile, you must have the following dependencies installed on your system:

- **A C++ Compiler:** A modern C++ compiler that supports C++17 (e.g., `g++` from MinGW-w64 on Windows, or `g++` on Linux).
- **SDL3 Library:** The core SDL3 development library.
- **SDL3_image Library:** The SDL_image extension library, which is required for loading PNG textures.

You can download the development libraries from the [SDL website](https://www.google.com/search?q=https.github.com/libsdl-org/SDL/releases) (for SDL3) and [here](https://github.com/libsdl-org/SDL_image/releases) (for SDL3_image).

### 2\. Project Structure

This project expects a specific file structure to find the assets:

- `main.cpp`
- `Game/`
  - `Game.h`
  - `Game.cpp`
- `Resources/Image/`
  - `Icon.png`
  - `Background.png`
  - `Top_Pipe.png`
  - `Bottom_Pipe.png`
  - `Top_Wall.png`
  - `Bottom_Wall.png`
  - `Bird_1.png`
  - `Bird_2.png`
  - `Bird_3.png`
- `include/` (Place your SDL3 and SDL3_image headers here)
- `lib/` (Place your SDL3 and SDL3_image `.lib` or `.a` files here)

### 3\. Compilation

Open a terminal in the project's root directory and run the following command. This command is provided in the original `README.md` and assumes your headers are in `include/` and your libraries are in `lib/`.

```bash
g++ main.cpp Game/Game.cpp -Iinclude -Llib -lSDL3 -lSDL3_image -o game.exe
```

- `-Iinclude`: Tells the compiler to look for headers (`.h` files) in the `include` folder.
- `-Llib`: Tells the linker to look for libraries (`.lib` or `.a` files) in the `lib` folder.
- `-lSDL3 -lSDL3_image`: Links the necessary SDL libraries.
- `-o game.exe`: Specifies the output executable name (`game.exe` on Windows, or just `game` on Linux/macOS).

---

## VI. Project Roadmap, Limitations, and Future Work

### Scope and Current Limitations

- **No Crossover:** The current evolution strategy is Elitism + Mutation. It does **not** use two-parent crossover (where two "parent" birds combine their NN weights to create a "child").
- **Simple Inputs:** The NN only "sees" the distance to obstacles. It has no knowledge of its own `yCordinate`, its `velocity`, or the `yCordinate` of the next pipe's gap. Adding these could improve performance.
- **Hardcoded Architecture:** The NN architecture (`10, {11, 11}, 1`), population size (15), and mutation rate (0.05) are all hardcoded.
- **No GUI/Interaction:** The simulation is not interactive. Parameters cannot be changed at runtime.
- **No Saving:** The weights of a "champion" bird cannot be saved to a file or loaded later.

### Future Work (Roadmap)

- **Implement Crossover:** Introduce a two-parent crossover strategy to combine the weights of two high-fitness parents.
- **Add More NN Inputs:** Experiment with adding `bird.getYCordinate()`, `bird.velocity`, and `nearest->getYcordinate()` to the input vector to give the bird more self-awareness.
- **Configurable Parameters:** Refactor the code to allow NN architecture and GA parameters to be changed easily (e.g., from a config file or UI).
- **Save/Load Model:** Implement functions to serialize the weights of the best bird to a JSON or binary file, and a function to load them.
- **Add UI Elements:** Use a library like ImGui to render an overlay showing the current generation, mutation rate, and graphs of fitness over time.

---

## VII. Guidelines for Collaboration and Contribution

Contributions are welcome\! This project is a great sandbox for experimenting with neuroevolution.

If you have an idea for an improvement, feel free to fork the repository, make your changes, and submit a pull request. For larger changes, please open an issue first to discuss the proposed feature.

(For a more formal project, you would link to a `CONTRIBUTING.md` and `CODE_OF_CONDUCT.md` file here.)

---

## VIII. Project Licensing

This project is licensed under the **MIT License**.

This is a permissive license that lets people do almost anything they want with your code, including making and distributing closed-source versions.

See the `LICENSE.md` file in this repository for the full text.

to compile : g++ QTableGame.cpp -I./SDL3/include -L./SDL3/lib -lSDL3

g++ main.cpp Game/Game.cpp -Iinclude -Llib -lSDL3 -lSDL3_image -o game.exe
