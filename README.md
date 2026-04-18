# 🎮 TETRIS – DDA & Bresenham Edition

A Computer Graphics Lab project implementing the classic **Tetris game** using **OpenGL (FreeGLUT) and C++**, with a focus on fundamental graphics algorithms such as **DDA Line Drawing** and **Bresenham Line Algorithm**.

---

## 📌 Overview

This project demonstrates how low-level graphics algorithms can be integrated into a real-time interactive application. The game renders a complete Tetris environment including grid, falling tetrominoes, scoring system, and UI panel.

Unlike typical implementations using game engines, this project directly applies **computer graphics algorithms** in rendering.

---

## ✨ Features

- 🎯 Fully playable Tetris game  
- 📐 **DDA Algorithm** for grid rendering  
- 📏 **Bresenham Algorithm** for block borders  
- 🎮 Real-time keyboard controls  
- 📊 Score, Level, Lines, Time tracking  
- 🔮 Next piece preview  
- 👻 Ghost piece (landing prediction)  
- ⏸ Pause and Restart functionality  
- ⚡ Dynamic speed increase with level  

---

## 🛠️ Technologies Used

- **C++**
- **OpenGL**
- **FreeGLUT / GLUT**
- **Standard Libraries** (`<cmath>`, `<ctime>`, `<vector>`)

---

## 🧠 Algorithms Used

### 🔹 DDA (Digital Differential Analyzer)
Used to draw grid lines by calculating intermediate points using floating-point increments.

### 🔹 Bresenham Line Algorithm
Used for drawing block borders and UI elements efficiently using integer arithmetic.

## ⚙️ How to Run

### 🖥️ Using Terminal (MinGW / g++)

#### 1. Install Dependencies
- Install **MinGW (g++)**
- Install **FreeGLUT**

#### 2. Compile

```bash
- g++ code.cpp -o code -lfreeglut -lglu32 -lopengl32
- ./code.exe
