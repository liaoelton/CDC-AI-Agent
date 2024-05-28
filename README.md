# Chinese Dark Chess AI

This is the final project for the "NTU, Theory of Computer Games (Fall 2021)" course. This repository contains the implementation of an AI Agent for playing Chinese Dark Chess, leveraging a variety of AI strategies and optimizations. The AI includes algorithms like Nega Scout and features such as time control, transposition tables, dynamic search extensions, and more. It's designed to enhance gameplay, offering both competitive strategies and deep insights into the game's dynamics.

## Features

- **Nega Scout Algorithm:** Implemented using pseudocode from course slides, includes optimizations like star-series functions for flipping pieces and window testing for enhanced move decisions.

- **Time Control:** Utilizes an iterative deepening framework with a function to check for time limits, ensuring the AI performs optimally under time constraints.

- **Transposition Table:** Uses Zobrist’s hash for efficient move and state management, improving calculation speed and accuracy.

- **Dynamic Search Extension:** Identifies and extends search in potentially critical game situations, enhancing the AI’s ability to handle complex scenarios.

- **Iterative Deepening Aspiration Search (IDAS):** Employs a two-layer initial search to set boundaries for further deepening, optimizing performance across different game phases.

- **Evaluation Functions:** Incorporates multiple strategies for evaluating game states, including static piece scores and distance-based scoring.

- **Advanced Heuristics:** Implements refutation tables, killer heuristic, and history heuristic to improve move ordering and cut effectiveness.
