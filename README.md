# TOWER-OF-HANOI
A game to solve a tower of hanoi
# Towers of Hanoi — Complete Project Documentation

## Table of Contents
1. [What is the Towers of Hanoi?](#1-what-is-the-towers-of-hanoi)
2. [Project Overview](#2-project-overview)
3. [Project Files](#3-project-files)
4. [How the Terminal Version Works](#4-how-the-terminal-version-works)
5. [How the GTK3 Graphical Version Works](#5-how-the-gtk3-graphical-version-works)
6. [Game Controls and Interface](#6-game-controls-and-interface)
7. [How to Install and Compile](#7-how-to-install-and-compile)
8. [How to Run](#8-how-to-run)
9. [The Algorithm Explained](#9-the-algorithm-explained)
10. [Mathematical Formula](#10-mathematical-formula)

---

## 1. What is the Towers of Hanoi?

The Towers of Hanoi is a classic mathematical puzzle invented by the French mathematician Édouard Lucas in 1883.

**The rules are simple:**
- There are **3 pegs** (also called rods or towers)
- There are **N disks** of different sizes stacked on the first peg, largest at the bottom and smallest at the top
- The goal is to move **all disks** from the starting peg to the destination peg
- You can only move **one disk at a time**
- You can **never place a larger disk on top of a smaller disk**

**Example with 3 disks:**
```
Initial state:          Final state:
Peg 1   Peg 2  Peg 3   Peg 1  Peg 2  Peg 3
  |       |      |        |      |      |
 [1]      |      |        |      |     [1]
[===]     |      |        |      |    [===]
[=====]   |      |        |      |   [=====]
```

---

## 2. Project Overview

This project contains **two versions** of the Towers of Hanoi solver written in C:

| Version | File | Description |
|---------|------|-------------|
| Terminal | `hanoi-2.c` | Text-based console solver with ASCII diagram |
| Graphical | `hanoi_gtk-1.c` | Full GTK3 GUI game with coloured disks |

Both versions solve the puzzle automatically using the **recursive algorithm** and show you every step of the solution.

---

## 3. Project Files

```
tower/
├── hanoi-2.c         — Terminal version source code
├── hanoi_gtk-1.c     — GTK3 graphical version source code
├── Makefile          — Build system (compile both versions)
└── README.md         — This documentation file
```

---

## 4. How the Terminal Version Works

### What it does
When you run the terminal version, it asks you three questions:
1. How many disks do you want to move? (1 to 10)
2. Which peg do the disks start on? (1, 2 or 3)
3. Which peg do you want to move them to? (1, 2 or 3)

Then it solves the puzzle and prints **every single move** with an ASCII drawing of the pegs after each move.

### Example output (3 disks, Peg 1 to Peg 3)
```
====================================
      TOWERS OF HANOI SOLVER
====================================
Enter number of disks (1-10)   : 3
Enter starting peg    (1/2/3)  : 1
Enter destination peg (1/2/3)  : 3

Moving 3 disk(s) from peg 1 to peg 3 (using peg 2 as temp):
Total moves expected: 7  (= 2^3 - 1)
------------------------------------

  Initial state:

    Peg 1         Peg 2         Peg 3
    |             |             |
   [1]            |             |
  [===]           |             |
 [=====]          |             |
  =========   =========   =========

  Step  1 / 7  |  Move disk [size 1] :  Peg 1  -->  Peg 3
  Step  2 / 7  |  Move disk [size 2] :  Peg 1  -->  Peg 2
  ...
====================================
  DONE! All 3 disk(s) moved in 7 step(s).
====================================
```

### Key functions in hanoi-2.c

| Function | What it does |
|----------|-------------|
| `main()` | Asks the user for input, sets up the puzzle, starts the solver |
| `init_pegs()` | Places all disks on the starting peg before the game begins |
| `hanoi()` | The recursive function that solves the puzzle step by step |
| `move_disk()` | Physically moves one disk from one peg to another in the program's memory |
| `print_state()` | Draws the ASCII picture of all three pegs after each move |

### How the ASCII drawing works
The `print_state()` function draws each peg row by row from top to bottom. For each row it checks how many disks are on each peg and draws the correct disk width using `=` characters. Larger disks use more `=` characters and smaller disks use fewer.

---

## 5. How the GTK3 Graphical Version Works

### What it does
The GTK3 version opens a **real game window** with:
- A dark-themed graphical canvas showing the three pegs
- Coloured disks drawn with gradient effects
- Buttons to step through the solution manually or watch it automatically
- A settings bar to choose the number of disks, start peg, and destination peg

### How the window is built
The program uses the **GTK3 library** to create the window and **Cairo** (a drawing library included with GTK3) to draw the pegs and disks graphically.

When the program starts it:
1. Creates the main window
2. Adds a drawing canvas at the top
3. Adds an info bar showing the current step
4. Adds a settings row (disk count, from peg, to peg, speed)
5. Adds a button row at the bottom
6. Calls `reset_game()` to set up the initial puzzle state
7. Opens the window and starts the GTK event loop

### How the drawing works
Every time something changes (a disk moves, a button is clicked), the program calls `gtk_widget_queue_draw()` which tells GTK to redraw the canvas. The `on_draw()` function then uses Cairo to paint:

1. The dark blue gradient background
2. The title text "TOWERS OF HANOI"
3. The three golden peg poles and their base platforms
4. Each disk as a coloured rounded rectangle with a gradient fill and a number label
5. A green "🎉 SOLVED!" banner when all disks have been moved

### Disk colours
Each disk size has its own colour so you can easily track it:

| Disk Size | Colour |
|-----------|--------|
| 1 (smallest) | Red |
| 2 | Orange |
| 3 | Yellow |
| 4 | Green |
| 5 | Sky Blue |
| 6 | Indigo |
| 7 | Violet |
| 8 (largest) | Pink |

### How moves are pre-calculated
Before any move is shown on screen, the program calls `build_move_list()` which runs the full recursive algorithm and stores **every move** in a list called `move_list[]`. This means:
- Clicking **Next** simply reads the next move from the list and applies it
- Clicking **Prev** reads the same move and reverses it (undo)
- **Auto Play** uses a timer to automatically call Next every few milliseconds

### Key functions in hanoi_gtk-1.c

| Function | What it does |
|----------|-------------|
| `main()` | Creates the window, all widgets, and starts GTK |
| `reset_game()` | Resets all pegs and rebuilds the move list from scratch |
| `build_move_list()` | Recursively calculates all moves and stores them |
| `apply_move()` | Executes one move forward (used by Next and Auto Play) |
| `undo_move()` | Reverses one move (used by Prev button) |
| `on_draw()` | Paints the entire canvas using Cairo every time it refreshes |
| `draw_disk()` | Draws one single disk with gradient colour and size label |
| `on_next()` | Called when the Next button is clicked |
| `on_prev()` | Called when the Prev button is clicked |
| `on_auto()` | Starts or pauses the automatic step-by-step playback |
| `on_reset()` | Called when the Reset button is clicked |
| `on_speed_changed()` | Adjusts the auto-play speed when the slider moves |
| `update_ui()` | Updates the step counter label and button sensitivity |
| `apply_css()` | Applies the dark purple theme to the whole window |

---

## 6. Game Controls and Interface

### Info bar (below the canvas)
Shows the current step number and describes the last move made. For example:
```
Step 3 / 7    Moved disk [size 1]  :  Peg 3  ➜  Peg 2
```

### Settings row

| Control | What it does |
|---------|-------------|
| **Disks** (number spinner) | Choose how many disks to use (1 to 8) |
| **From** (dropdown) | Choose which peg the disks start on |
| **To** (dropdown) | Choose which peg the disks should end up on |
| **Speed** (slider) | Control how fast Auto Play moves (slow left, fast right) |

### Button row

| Button | What it does |
|--------|-------------|
| **↺ Reset** | Restarts the puzzle from the beginning with the current settings |
| **◀ Prev** | Goes back one step (undoes the last move) |
| **Next ▶** | Executes the next move in the solution |
| **▶ Auto Play** | Automatically plays through all moves one by one |
| **⏸ Pause** | Pauses Auto Play (same button changes label when playing) |
| **ℹ About** | Shows information about the program |

### Peg label colours
- **Green label** = the starting peg (where disks begin)
- **Blue label** = the destination peg (where disks need to go)
- **Grey label** = the temporary peg (used as intermediate storage)

---

## 7. How to Install and Compile

### On Windows with MSYS2 (recommended)

**Step 1 — Open MSYS2 MinGW64** (the blue terminal in your Start menu)

**Step 2 — Install required packages (first time only):**
```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gtk3
pacman -S mingw-w64-x86_64-toolchain
pacman -S make
```
Press **Y** then Enter when asked.

**Step 3 — Navigate to your project folder:**
```bash
cd /c/Users/YourName/Desktop/tower
```

**Step 4 — Compile:**
```bash
make gtk
```
Or compile directly without the Makefile:
```bash
gcc $(pkg-config --cflags gtk+-3.0) -o hanoi_gtk.exe hanoi_gtk-1.c $(pkg-config --libs gtk+-3.0) -lm -Wall
```

### On Linux (Ubuntu / Debian)

**Install GTK3 (first time only):**
```bash
sudo apt install libgtk-3-dev gcc make
```

**Compile:**
```bash
make gtk
```

### Makefile commands

| Command | What it does |
|---------|-------------|
| `make` | Compiles both versions |
| `make gtk` | Compiles only the graphical version |
| `make terminal` | Compiles only the terminal version |
| `make run` | Compiles and immediately runs the GTK game |
| `make run-term` | Compiles and immediately runs the terminal version |
| `make clean` | Deletes the compiled `.exe` files |

---

## 8. How to Run

### Graphical version (GTK3)
```bash
./hanoi_gtk.exe        # Windows
./hanoi_gtk            # Linux
```
A game window will open immediately.

### Terminal version
```bash
./hanoi.exe            # Windows
./hanoi                # Linux
```
The program will ask you to type the number of disks and peg numbers.

---

## 9. The Algorithm Explained

Both versions use the same **recursive algorithm**. Here is how it works in plain language:

To move **N disks** from peg A to peg C (using peg B as helper):

1. Move the top **N-1 disks** from peg A to peg B (using peg C as helper)
2. Move the **largest disk** (disk N) from peg A to peg C
3. Move the **N-1 disks** from peg B to peg C (using peg A as helper)

The trick is that step 1 and step 3 are the **same problem** but smaller. The function keeps calling itself with one fewer disk until it reaches 1 disk, which is simply moved directly.

**In code (simplified):**
```c
void hanoi(int n, int from, int to, int temp) {
    if (n == 1) {
        move disk from --> to    // base case: just move it
        return;
    }
    hanoi(n-1, from, temp, to); // Step 1: move n-1 disks out of the way
    move disk from --> to;       // Step 2: move the biggest disk
    hanoi(n-1, temp, to, from); // Step 3: move n-1 disks to destination
}
```

---

## 10. Mathematical Formula

The minimum number of moves needed to solve the puzzle with N disks is always:

```
Moves = 2^N - 1
```

| Disks | Moves needed |
|-------|-------------|
| 1 | 1 |
| 2 | 3 |
| 3 | 7 |
| 4 | 15 |
| 5 | 31 |
| 6 | 63 |
| 7 | 127 |
| 8 | 255 |

This is the **optimal** solution — it is mathematically impossible to solve the puzzle in fewer moves. The recursive algorithm always produces this exact number of moves.

---

*Project written in C using GTK3 and Cairo for the graphical interface.*

