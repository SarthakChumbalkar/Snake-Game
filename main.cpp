#include <windows.h>
#include <windowsx.h>
#include <conio.h>
#include <iostream>
#include <string>
#include <vector>    // CO5: STL vector
#include <algorithm> // CO5: sort the leaderboard
#include <deque>     // CO5: STL deque
#include <fstream>   // CO4: File handling
#include <stdexcept> // CO4: Exception handling
#include <sstream>   // CO5: Generic text conversion helper
#include <cstdlib>
#include <ctime>
#include <utility>

using namespace std;

// CO5: Template for converting different value types to a wide string.
template <typename T>
wstring ToWideString(const T &value)
{
    wostringstream stream;
    stream << value;
    return stream.str();
}

// Names are entered as basic keyboard characters, then converted for Win32 drawing.
wstring ToWideString(const string &value)
{
    return wstring(value.begin(), value.end());
}

// Simple UI colors.
const COLORREF C_BACKGROUND = RGB(17, 24, 39);
const COLORREF C_PANEL = RGB(27, 37, 54);
const COLORREF C_PANEL_LIGHT = RGB(37, 50, 70);
const COLORREF C_CELL = RGB(23, 34, 49);
const COLORREF C_GREEN = RGB(64, 220, 130);
const COLORREF C_GREEN_DARK = RGB(35, 160, 91);
const COLORREF C_TEXT = RGB(239, 245, 250);
const COLORREF C_MUTED = RGB(158, 174, 191);
const COLORREF C_YELLOW = RGB(255, 200, 87);
const COLORREF C_RED = RGB(255, 105, 105);

// CO1: Abstract base class demonstrates abstraction.
class Game
{
public:
    // CO3: Virtual functions support runtime polymorphism.
    virtual void Draw(HDC hdc) = 0;
    virtual void Update() = 0;
    virtual void OnKeyDown(WPARAM key) = 0;
    virtual void OnChar(WPARAM ch) = 0;
    virtual void OnMouseClick(int x, int y) = 0;
    virtual ~Game() {}
};

// CO2: Inheritance - SnakeGame inherits the Game interface.
class SnakeGame : public Game
{
private:
    static const int COLS = 24;
    static const int ROWS = 17;
    static const int CELL = 22;
    static const int BOARD_X = 28;
    static const int BOARD_Y = 118;
    static const int BOARD_W = COLS * CELL;
    static const int BOARD_H = ROWS * CELL;

    enum class Screen
    {
        Menu,
        NameEntry,
        Playing,
        Paused,
        GameOver
    };
    enum class Direction
    {
        Up,
        Down,
        Left,
        Right
    };

    struct LeaderboardEntry
    {
        string name;
        int score;
    };

    // CO1: Encapsulation - implementation details are private.
    deque<pair<int, int>> snake;
    vector<string> board;                 // CO5: STL vector stores the rendered board.
    vector<LeaderboardEntry> leaderboard; // CO5: STL leaderboard.
    pair<int, int> food;
    Direction direction;
    Screen screen;
    int score;
    int highScore;
    string playerName;
    bool resultRecorded;
    bool fileError;

    // Reusable GDI helper: draw text with a chosen size/color.
    void text(HDC hdc, const wstring &value, int x, int y,
              int width, int height, int size, COLORREF color,
              bool bold = false, UINT format = DT_LEFT | DT_VCENTER | DT_SINGLELINE)
    {
        HFONT font = CreateFontW(
            -size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(hdc, font);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, color);
        RECT r{x, y, x + width, y + height};
        DrawTextW(hdc, value.c_str(), -1, &r, format);
        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    void fill(HDC hdc, int x, int y, int width, int height, COLORREF color)
    {
        RECT r{x, y, x + width, y + height};
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(hdc, &r, brush);
        DeleteObject(brush);
    }

    void roundedPanel(HDC hdc, int x, int y, int width, int height,
                      COLORREF color, COLORREF border = C_PANEL)
    {
        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen = CreatePen(PS_SOLID, 1, border);
        HGDIOBJ oldBrush = SelectObject(hdc, brush);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        RoundRect(hdc, x, y, x + width, y + height, 12, 12);
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
    }

    void button(HDC hdc, int x, int y, int width, int height,
                const wstring &label)
    {
        roundedPanel(hdc, x, y, width, height, C_GREEN, C_GREEN);
        text(hdc, label, x, y, width, height, 16, C_BACKGROUND, true,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // CO4: Read the saved high score from a file.
    void loadHighScore()
    {
        ifstream file("snake_highscore.txt");
        if (!file.is_open())
        {
            // No file yet is normal on the first launch.
            highScore = 0;
            return;
        }

        if (!(file >> highScore) || highScore < 0)
        {
            throw runtime_error("The high-score file is invalid.");
        }
    }

    // CO4: Save the high score; failures throw an exception.
    void saveHighScore()
    {
        ofstream file("snake_highscore.txt");
        if (!file.is_open())
        {
            throw runtime_error("Could not open the high-score file.");
        }
        file << highScore;
        file.close();
        if (file.fail())
        {
            throw runtime_error("Could not write the high-score file.");
        }
    }

    // CO4: Load the saved top-three player names and scores.
    // The file is local to this PC; a true worldwide board needs an online server.
    void loadLeaderboard()
    {
        leaderboard.clear();
        ifstream file("snake_leaderboard.txt");
        if (!file.is_open())
            return; // Normal on the first launch.

        string line;
        while (getline(file, line))
        {
            const size_t separator = line.find('|');
            if (separator == string::npos)
                continue;

            int savedScore = 0;
            try
            {
                savedScore = stoi(line.substr(0, separator));
            }
            catch (const exception &)
            {
                continue; // Ignore a malformed row.
            }

            const string savedName = line.substr(separator + 1);
            if (savedName.empty() || savedScore < 0)
                continue;
            leaderboard.push_back({savedName, savedScore});
        }

        sort(leaderboard.begin(), leaderboard.end(),
             [](const LeaderboardEntry &a, const LeaderboardEntry &b)
             {
                 return a.score > b.score;
             });
        if (leaderboard.size() > 3)
            leaderboard.resize(3);

        if (!leaderboard.empty() && leaderboard.front().score > highScore)
        {
            highScore = leaderboard.front().score;
        }
    }

    // CO4: Persist the top-three leaderboard to a text file.
    void saveLeaderboard()
    {
        ofstream file("snake_leaderboard.txt");
        if (!file.is_open())
        {
            throw runtime_error("Could not open the leaderboard file.");
        }

        for (const auto &entry : leaderboard)
        {
            file << entry.score << '|' << entry.name << '\n';
        }
        file.close();
        if (file.fail())
        {
            throw runtime_error("Could not write the leaderboard file.");
        }
    }

    // Add a finished run once, sort scores, and keep only the top three.
    void recordResult()
    {
        if (resultRecorded)
            return;
        resultRecorded = true;

        if (playerName.empty())
            playerName = "Player";
        leaderboard.push_back({playerName, score});
        sort(leaderboard.begin(), leaderboard.end(),
             [](const LeaderboardEntry &a, const LeaderboardEntry &b)
             {
                 return a.score > b.score;
             });
        if (leaderboard.size() > 3)
            leaderboard.resize(3);

        if (!leaderboard.empty() && leaderboard.front().score > highScore)
        {
            highScore = leaderboard.front().score;
        }

        try
        {
            saveLeaderboard();
            saveHighScore();
            fileError = false;
        }
        catch (const exception &)
        {
            fileError = true;
        }
    }

    void endGame()
    {
        screen = Screen::GameOver;
        recordResult();
    }

    void createFood()
    {
        if (snake.size() >= static_cast<size_t>(COLS * ROWS))
        {
            endGame();
            return;
        }

        bool occupied;
        do
        {
            occupied = false;
            food = {rand() % COLS, rand() % ROWS};
            for (const auto &part : snake)
            {
                if (part == food)
                {
                    occupied = true;
                    break;
                }
            }
        } while (occupied);
    }

    // UI: Ask for a player name before every new/replayed game.
    void beginNameEntry()
    {
        playerName.clear();
        screen = Screen::NameEntry;
    }

    void startGame()
    {
        // CO1: Initialize a fresh game session.
        score = 0;
        direction = Direction::Right;
        screen = Screen::Playing;
        fileError = false;
        resultRecorded = false;
        snake.clear();

        const int startX = COLS / 2;
        const int startY = ROWS / 2;
        for (int i = 0; i < 5; ++i)
        {
            snake.push_back({startX - i, startY});
        }
        createFood();
    }

    bool isOpposite(Direction wanted) const
    {
        return (direction == Direction::Up && wanted == Direction::Down) ||
               (direction == Direction::Down && wanted == Direction::Up) ||
               (direction == Direction::Left && wanted == Direction::Right) ||
               (direction == Direction::Right && wanted == Direction::Left);
    }

    void setDirection(Direction wanted)
    {
        if (!isOpposite(wanted))
            direction = wanted;
    }

    void drawBoard(HDC hdc)
    {
        roundedPanel(hdc, BOARD_X - 6, BOARD_Y - 6,
                     BOARD_W + 12, BOARD_H + 12, C_PANEL, RGB(51, 69, 91));

        // CO5: A vector of strings represents each row of the board.
        board.assign(ROWS, string(COLS, '.'));
        for (const auto &part : snake)
        {
            board[part.second][part.first] = 'o';
        }
        if (!snake.empty())
        {
            board[snake.front().second][snake.front().first] = 'O';
        }
        if (screen != Screen::GameOver)
        {
            board[food.second][food.first] = '*';
        }

        HBRUSH emptyBrush = CreateSolidBrush(C_CELL);
        HBRUSH bodyBrush = CreateSolidBrush(C_GREEN_DARK);
        HBRUSH headBrush = CreateSolidBrush(C_GREEN);
        HBRUSH foodBrush = CreateSolidBrush(C_YELLOW);

        for (int row = 0; row < ROWS; ++row)
        {
            for (int col = 0; col < COLS; ++col)
            {
                RECT cellRect{
                    BOARD_X + col * CELL + 2,
                    BOARD_Y + row * CELL + 2,
                    BOARD_X + (col + 1) * CELL - 2,
                    BOARD_Y + (row + 1) * CELL - 2};
                HBRUSH useBrush = emptyBrush;
                char cell = board[row][col];
                if (cell == 'o')
                    useBrush = bodyBrush;
                if (cell == 'O')
                    useBrush = headBrush;
                if (cell == '*')
                    useBrush = foodBrush;
                FillRect(hdc, &cellRect, useBrush);
            }
        }

        DeleteObject(emptyBrush);
        DeleteObject(bodyBrush);
        DeleteObject(headBrush);
        DeleteObject(foodBrush);
    }

    void drawSidebar(HDC hdc)
    {
        const int x = 590;
        roundedPanel(hdc, x, BOARD_Y - 6, 230, BOARD_H + 12, C_PANEL,
                     RGB(51, 69, 91));

        text(hdc, L"YOUR SCORE", x + 20, 133, 190, 20, 12, C_MUTED, true);
        text(hdc, ToWideString(score), x + 20, 152, 190, 38, 31, C_GREEN, true);
        text(hdc, L"PLAYER", x + 20, 191, 190, 17, 10, C_MUTED, true);
        text(hdc, playerName.empty() ? L"—" : ToWideString(playerName),
             x + 20, 207, 190, 22, 14, C_TEXT, true);
        fill(hdc, x + 20, 233, 190, 1, RGB(57, 73, 94));

        text(hdc, L"BEST SCORE", x + 20, 240, 190, 18, 11, C_MUTED, true);
        text(hdc, ToWideString(highScore), x + 20, 257, 190, 31, 24,
             C_YELLOW, true);

        // UI: Show the top three saved players in the sidebar.
        roundedPanel(hdc, x + 16, 300, 198, 112, C_PANEL_LIGHT, C_PANEL_LIGHT);
        text(hdc, L"TOP 3 PLAYERS", x + 27, 306, 174, 21, 13, C_TEXT, true);

        if (leaderboard.empty())
        {
            text(hdc, L"No scores yet - be first!", x + 27, 342, 174, 24,
                 11, C_MUTED, false);
        }
        else
        {
            for (size_t i = 0; i < 3; ++i)
            {
                const int rowY = 333 + static_cast<int>(i) * 23;
                if (i < leaderboard.size())
                {
                    const wstring rank = ToWideString(static_cast<int>(i + 1)) + L". ";
                    wstring name = ToWideString(leaderboard[i].name);
                    if (name.size() > 11)
                        name = name.substr(0, 10) + L"…";
                    text(hdc, rank + name, x + 27, rowY, 130, 20, 11,
                         i == 0 ? C_YELLOW : C_TEXT, i == 0);
                    text(hdc, ToWideString(leaderboard[i].score), x + 157, rowY,
                         43, 20, 11, C_GREEN, true,
                         DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
                }
            }
        }

        roundedPanel(hdc, x + 16, 420, 198, 73, C_PANEL_LIGHT, C_PANEL_LIGHT);
        text(hdc, L"CONTROLS", x + 27, 425, 174, 18, 12, C_TEXT, true);
        text(hdc, L"W A S D  /  Arrow keys: Move", x + 27, 444, 174, 18,
             10, C_MUTED);
        text(hdc, L"SPACE: Pause   R: New game", x + 27, 464, 174, 18,
             10, C_MUTED);

        if (fileError)
        {
            text(hdc, L"Could not save scores.", x + 12, 494, 206, 16, 10,
                 C_RED, false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    void drawOverlay(HDC hdc)
    {
        // A simple centered card sits over the play area.
        if (screen == Screen::NameEntry)
        {
            roundedPanel(hdc, 112, 204, 360, 194, RGB(31, 44, 62), RGB(69, 91, 115));
        }
        else
        {
            roundedPanel(hdc, 112, 222, 360, 150, RGB(31, 44, 62), RGB(69, 91, 115));
        }

        if (screen == Screen::Menu)
        {
            text(hdc, L"READY TO PLAY?", 130, 238, 324, 34, 25, C_TEXT, true,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            text(hdc, L"Eat the food. Avoid the walls.", 130, 270, 324, 22,
                 13, C_MUTED, false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            button(hdc, 202, 305, 180, 44, L"START GAME");
            text(hdc, L"or press ENTER", 130, 350, 324, 18, 11, C_MUTED,
                 false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        else if (screen == Screen::NameEntry)
        {
            text(hdc, L"WHAT'S YOUR NAME?", 130, 218, 324, 31, 22, C_TEXT, true,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            text(hdc, L"Your score can join the top-three list.", 130, 248, 324, 22,
                 12, C_MUTED, false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            roundedPanel(hdc, 152, 278, 280, 37, C_CELL, RGB(69, 91, 115));
            const wstring inputText = playerName.empty()
                                          ? L"Type your name here..."
                                          : ToWideString(playerName) + L"_";
            text(hdc, inputText, 164, 282, 256, 29, 15,
                 playerName.empty() ? C_MUTED : C_TEXT, false,
                 DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            button(hdc, 202, 324, 180, 38, L"LET'S PLAY");
            text(hdc, L"Type a name, then press ENTER", 130, 364, 324, 20,
                 11, C_MUTED, false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        else if (screen == Screen::Paused)
        {
            text(hdc, L"PAUSED", 130, 238, 324, 36, 27, C_TEXT, true,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            text(hdc, L"Take a break. Your snake is waiting.", 130, 271, 324, 20,
                 12, C_MUTED, false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            button(hdc, 202, 305, 180, 44, L"RESUME");
            text(hdc, L"press SPACE to continue", 130, 350, 324, 18, 11, C_MUTED,
                 false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        else if (screen == Screen::GameOver)
        {
            text(hdc, L"GAME OVER", 130, 234, 324, 37, 27, C_RED, true,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            text(hdc, L"Final score: " + ToWideString(score), 130, 270, 324, 22,
                 13, C_MUTED, false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            button(hdc, 202, 305, 180, 44, L"PLAY AGAIN");
            text(hdc, L"or press ENTER to retry", 130, 350, 324, 18, 11, C_MUTED,
                 false, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

public:
    // CO1: Constructor initializes the object and its data.
    SnakeGame() : direction(Direction::Right), screen(Screen::Menu),
                  score(0), highScore(0), resultRecorded(true), fileError(false)
    {
        srand(static_cast<unsigned int>(time(nullptr)));
        try
        {
            loadHighScore();
        }
        catch (const exception &)
        {
            highScore = 0;
            fileError = true;
        }

        try
        {
            loadLeaderboard();
        }
        catch (const exception &)
        {
            fileError = true;
        }

        // Show a short snake preview behind the menu.
        const int startX = COLS / 2;
        const int startY = ROWS / 2;
        for (int i = 0; i < 5; ++i)
        {
            snake.push_back({startX - i, startY});
        }
        createFood();
    }

    // CO3: Overrides the virtual function in Game.
    void Draw(HDC hdc) override
    {
        // The window uses a fixed 850 x 570 client area. The HDC passed
        // here may be a memory DC used for double-buffered painting.
        RECT client{0, 0, 850, 570};
        HBRUSH background = CreateSolidBrush(C_BACKGROUND);
        FillRect(hdc, &client, background);
        DeleteObject(background);

        text(hdc, L"SNAKE", 28, 20, 220, 48, 34, C_GREEN, true);
        text(hdc, L"CLASSIC ARCADE", 31, 66, 250, 22, 12, C_MUTED, true);
        text(hdc, L"A tiny game built with C++ OOP", 470, 39, 350, 24,
             12, C_MUTED, false, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        drawBoard(hdc);
        drawSidebar(hdc);

        if (screen != Screen::Playing)
        {
            drawOverlay(hdc);
        }

        text(hdc, L"C++  |  OOP  |  STL", 28, 525, 220, 20, 10, C_MUTED);
        text(hdc, L"Collect the yellow food to gain 10 points", 330, 525,
             490, 20, 11, C_MUTED, false, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }

    // Game logic: only update while the game is being played.
    void Update() override
    {
        if (screen != Screen::Playing)
            return;

        pair<int, int> head = snake.front();
        switch (direction)
        {
        case Direction::Up:
            --head.second;
            break;
        case Direction::Down:
            ++head.second;
            break;
        case Direction::Left:
            --head.first;
            break;
        case Direction::Right:
            ++head.first;
            break;
        }

        // Wall collision.
        if (head.first < 0 || head.first >= COLS ||
            head.second < 0 || head.second >= ROWS)
        {
            endGame();
            return;
        }

        const bool eating = (head == food);

        // Self-collision. The tail may move away this turn unless eating.
        for (size_t i = 0; i < snake.size(); ++i)
        {
            if (snake[i] == head)
            {
                const bool isTail = (i == snake.size() - 1);
                if (!isTail || eating)
                {
                    endGame();
                    return;
                }
            }
        }

        snake.push_front(head);
        if (eating)
        {
            score += 10;
            if (score > highScore)
            {
                highScore = score;
                try
                {
                    saveHighScore();
                    fileError = false;
                }
                catch (const exception &)
                {
                    fileError = true;
                }
            }
            createFood();
            if (screen == Screen::GameOver)
                recordResult();
        }
        else
        {
            snake.pop_back();
        }
    }

    void OnKeyDown(WPARAM key) override
    {
        if (key == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return;
        }

        if (screen == Screen::NameEntry)
        {
            if (key == VK_BACK && !playerName.empty())
            {
                playerName.pop_back();
            }
            else if (key == VK_RETURN)
            {
                // Trim spaces so a blank name cannot be submitted.
                while (!playerName.empty() && playerName.back() == ' ')
                {
                    playerName.pop_back();
                }
                while (!playerName.empty() && playerName.front() == ' ')
                {
                    playerName.erase(playerName.begin());
                }
                if (!playerName.empty())
                    startGame();
            }
            return;
        }

        if (screen == Screen::Menu || screen == Screen::GameOver)
        {
            if (key == VK_RETURN || key == 'R')
                beginNameEntry();
            return;
        }

        if (key == 'R')
        {
            beginNameEntry();
            return;
        }

        if (screen == Screen::Paused)
        {
            if (key == VK_SPACE || key == VK_RETURN)
                screen = Screen::Playing;
            return;
        }

        if (key == VK_SPACE)
        {
            screen = Screen::Paused;
        }
        else if (key == VK_UP || key == 'W')
            setDirection(Direction::Up);
        else if (key == VK_DOWN || key == 'S')
            setDirection(Direction::Down);
        else if (key == VK_LEFT || key == 'A')
            setDirection(Direction::Left);
        else if (key == VK_RIGHT || key == 'D')
            setDirection(Direction::Right);
    }

    // UI: Capture typed characters on the name-entry screen.
    void OnChar(WPARAM ch) override
    {
        if (screen != Screen::NameEntry)
            return;
        if (ch >= 32 && ch <= 126 && ch != '|' && playerName.size() < 16)
        {
            playerName.push_back(static_cast<char>(ch));
        }
    }

    void OnMouseClick(int x, int y) override
    {
        if (screen == Screen::NameEntry)
        {
            const bool nameButton = (x >= 202 && x <= 382 && y >= 324 && y <= 362);
            if (nameButton)
            {
                while (!playerName.empty() && playerName.back() == ' ')
                {
                    playerName.pop_back();
                }
                while (!playerName.empty() && playerName.front() == ' ')
                {
                    playerName.erase(playerName.begin());
                }
                if (!playerName.empty())
                    startGame();
            }
            return;
        }

        // Start / resume / play-again button.
        const bool inButton = (x >= 202 && x <= 382 && y >= 305 && y <= 349);
        if (!inButton)
            return;

        if (screen == Screen::Menu || screen == Screen::GameOver)
        {
            beginNameEntry();
        }
        else if (screen == Screen::Paused)
        {
            screen = Screen::Playing;
        }
    }
};

// CO3: Calls are dispatched through a base-class pointer.
Game *g_game = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        SetTimer(hwnd, 1, 115, nullptr);
        return 0;

    case WM_TIMER:
        if (g_game)
        {
            g_game->Update();
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_KEYDOWN:
        if (g_game)
        {
            g_game->OnKeyDown(wParam);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_CHAR:
        if (g_game)
        {
            g_game->OnChar(wParam);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_LBUTTONDOWN:
        if (g_game)
        {
            g_game->OnMouseClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_ERASEBKGND:
        return 1; // Reduce flicker; the paint routine fills the background.

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT r;
        GetClientRect(hwnd, &r);
        const int width = r.right - r.left;
        const int height = r.bottom - r.top;
        HDC memoryDC = CreateCompatibleDC(hdc);
        HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
        HGDIOBJ oldBitmap = SelectObject(memoryDC, bitmap);

        if (g_game)
            g_game->Draw(memoryDC);
        BitBlt(hdc, 0, 0, width, height, memoryDC, 0, 0, SRCCOPY);

        SelectObject(memoryDC, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(memoryDC);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    SnakeGame snake;
    g_game = &snake;

    const wchar_t CLASS_NAME[] = L"SimpleSnakeGameWindow";

    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(nullptr, L"Could not register the game window.",
                    L"Snake Game", MB_ICONERROR | MB_OK);
        return 1;
    }

    // Fixed-size window keeps the simple layout aligned.
    RECT windowRect{0, 0, 850, 570};
    AdjustWindowRect(&windowRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Snake Game - C++ OOP",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr, nullptr, instance, nullptr);

    if (!hwnd)
    {
        MessageBoxW(nullptr, L"Could not create the game window.",
                    L"Snake Game", MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(hwnd, showCommand);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}
