# Snake Game — C++ OOP Project

A simple desktop Snake game built in **C++** using the **Windows API (Win32/GDI)**. It features a graphical interface, player-name entry, score tracking, and a leaderboard that saves the top three scores locally on the computer.

## Features

- Graphical Windows game window with a start screen and game board
- Enter a player name before each game
- Move the snake and collect food to earn points
- Collision detection for walls and the snake's own body
- Pause, resume, restart, and exit controls
- Current score and best score display
- Local top-three leaderboard showing player names and scores
- Saves high scores and leaderboard data to text files
- Demonstrates all five course outcomes for an Object-Oriented Programming (OOP) course

## Screenshot

Add a screenshot of the game here after running it, for example:

```markdown
![Snake Game screenshot](screenshots/snake-game.png)
```

## Requirements

- Windows 10 or Windows 11
- A C++ compiler that supports the Windows API, such as MinGW-w64/GCC
- C++11 or newer

> This version uses `windows.h`, Win32/GDI, and the Windows message loop. It is intended for Windows and will not compile unchanged on Linux or macOS.

## Build and Run

1. Download or clone this repository.
2. Open PowerShell or Command Prompt in the project folder.
3. Compile the source file:

```powershell
g++ snake_gui_leaderboard.cpp -std=c++11 -o SnakeGame.exe -mwindows -lgdi32 -luser32
```

4. Run the game:

```powershell
.\SnakeGame.exe
```

Make sure `snake_gui_leaderboard.cpp` is in the current folder before compiling.

## Controls

| Key | Action |
|---|---|
| `W` / `↑` | Move up |
| `A` / `←` | Move left |
| `S` / `↓` | Move down |
| `D` / `→` | Move right |
| `Space` | Pause / resume |
| `R` | Start a new game or enter a new player name |
| `Enter` | Confirm a player name, start, or retry where shown |
| `Esc` | Exit the game |

You can also use the on-screen buttons with the mouse.

## Scoring and Leaderboard

- Collecting food adds **10 points**.
- The game ends if the snake hits a wall or itself.
- The player is asked for a name before starting a new game.
- After a run ends, the game updates and saves the top three scores.

The game creates these files in its **current working directory**:

- `snake_highscore.txt` — best score
- `snake_leaderboard.txt` — top-three player names and scores

These files are local to that computer. The leaderboard is **not a worldwide or online leaderboard**; sharing scores across different computers would require an online service or database.

## Course Outcomes Demonstrated

| Course Outcome | Concepts in the project |
|---|---|
| **CO1: Object-oriented programming** | Classes, objects, encapsulation, constructors, and abstraction through an abstract base class |
| **CO2: Inheritance** | `SnakeGame` inherits from the `Game` base class |
| **CO3: Polymorphism** | Virtual functions are overridden in `SnakeGame` and called through a `Game*` base-class pointer |
| **CO4: Exception and file handling** | `ifstream`, `ofstream`, exception handling with `try` / `catch`, and persistent score files |
| **CO5: Templates and STL** | A function template, `vector`, `deque`, algorithms such as `sort`, and `pair` |

## Project Structure

```text
Snake-Game/
├── snake_gui_leaderboard.cpp   # Complete game source code
└── README.md                   # Project documentation
```

The two score files are generated when the game runs, so they do not need to be included in the repository.

## Future Improvements

- Add sound effects and background music
- Add difficulty levels and adjustable speed
- Add an online leaderboard shared between computers
- Add more visual themes

## License

This project is intended for learning and academic demonstration. Add a license file if you plan to distribute it publicly or specify reuse permissions.
