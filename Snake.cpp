#include <iostream>

#include <random>
#include <functional>
#include <utility>
#include <algorithm>
#include <iterator>
#include <numeric>

#include <chrono>
#include <time.h>
#include <thread>

#include <deque>
#include <set>
#include <vector>

#include <Windows.h>

const int GRID_SIZE = 25;

enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

void cls()
{
    static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD topLeft = { 0, 0 };

    std::cout.flush();

    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        abort();
    }
    DWORD length = csbi.dwSize.X * csbi.dwSize.Y;

    DWORD written;

    FillConsoleOutputCharacter(hOut, TEXT(' '), length, topLeft, &written);

    FillConsoleOutputAttribute(hOut, csbi.wAttributes, length, topLeft, &written);

    SetConsoleCursorPosition(hOut, topLeft);
}

void showConsoleCursor(bool showFlag)
{
    static const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_CURSOR_INFO cursorInfo;

    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = showFlag;
    SetConsoleCursorInfo(out, &cursorInfo);
}

void setCursorPosition(int x, int y)
{
    static const HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    std::cout.flush();
    COORD coord = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hOut, coord);
}

void run() {
    // Hide console cursor to prevent random cursor flickering
    showConsoleCursor(false);

    // Pseudo number generator
    std::mt19937::result_type seed = time(0);
    auto generator = std::bind(
        std::uniform_int_distribution<int>(1, GRID_SIZE),
        std::mt19937(seed)
    );

    std::deque<std::pair<int, int>> snake{ std::make_pair(13, generator()) }; // row, column
    std::set<int> positions;

    std::pair<int, int> apple_position = std::make_pair(5, generator()); // row, column

    Direction snake_direction = Direction::UP;

    // Initialize display with whitespace
    std::vector<std::vector<char>> display(GRID_SIZE + 2, std::vector<char>(GRID_SIZE + 2, ' '));

    // Initialize all available apple positions vector
    std::vector<std::pair<int, int>> available_apple_positions;


    bool alive = true;

    // Game loop
    while (alive) {

        std::vector<std::vector<char>> next_display(GRID_SIZE + 2, std::vector<char>(GRID_SIZE + 2));

        // Render at 60 frames per second
        auto start_time = std::chrono::system_clock::now() + std::chrono::milliseconds(16);

        // Check for user input
        if (GetAsyncKeyState('W') & 0x8000 && snake_direction != Direction::DOWN) {
            snake_direction = Direction::UP;
        }
        else if (GetAsyncKeyState('S') & 0x8000 && snake_direction != Direction::UP) {
            snake_direction = Direction::DOWN;
        }
        else if (GetAsyncKeyState('A') & 0x8000 && snake_direction != Direction::RIGHT) {
            snake_direction = Direction::LEFT;
        }
        else if (GetAsyncKeyState('D') & 0x8000 && snake_direction != Direction::LEFT) {
            snake_direction = Direction::RIGHT;
        }

        // Handle snake placement
        int head_row = snake.front().first,
            head_column = snake.front().second;

        switch (snake_direction) {
            case Direction::UP:
                head_row -= 1;
                break;
            case Direction::DOWN:
                head_row += 1;
                break;
            case Direction::LEFT:
                head_column -= 1;
                break;
            case Direction::RIGHT:
                head_column += 1;
                break;
            default:
                break;
        }

        // Find new head position and determine if apple collision occured
        std::pair<int, int> new_position = std::make_pair(head_row, head_column);
        snake.push_front(new_position);

        if (new_position.first == apple_position.first
            && new_position.second == apple_position.second) {
            auto uid = std::uniform_int_distribution<int>(0, available_apple_positions.size() - 1);
            auto engine = std::mt19937(seed);

            apple_position = available_apple_positions[uid(engine)];
        }
        else {
            snake.pop_back();
        }

        // Clear available apple positions vector
        available_apple_positions.clear();

        // Determine all spaces occupied by a snake
        std::transform(
            snake.begin(),
            snake.end(),
            std::inserter(positions, positions.begin()),
            [](const std::pair<int, int>& snake_position) -> int {
                return snake_position.first * GRID_SIZE + snake_position.second;
            });

        // Draw board
        for (int i = 0; i < GRID_SIZE + 2; ++i) { // Row
            for (int j = 0; j < GRID_SIZE + 2; ++j) { // Column
                char tile = '.';

                if (i == 0
                    || i == GRID_SIZE + 1
                    || j == 0
                    || j == GRID_SIZE + 1) {
                    tile = 'x';
                }
                else if (i == apple_position.first
                    && j == apple_position.second) {
                    tile = 'o';
                }
                else if (positions.find(i * GRID_SIZE + j) != positions.end()) {
                    tile = 'S';
                }

                if (tile == '.') {
                    available_apple_positions.push_back(std::make_pair(i, j));
                }

                next_display[i][j] = tile;
                setCursorPosition(j, i);
                std::cout << display[i][j] << ' ';
            }
        }

        // Check for wall collision
        if (head_column == 0
            || head_column == GRID_SIZE + 1
            || head_row == 0
            || head_row == GRID_SIZE + 1) {
            alive = false;
        }

        // Check for snake body collision
        // Since the we utilize a set for numerical snake positions,
        // if the size of it is less than the deque, we know we collided
        if (positions.size() < snake.size()) {
            alive = false;
        }

        // Reset for next frame
        positions.clear();
        std::cout.flush();
        display = std::move(next_display);

        // Sleep
        std::this_thread::sleep_for(start_time - std::chrono::system_clock::now());
    }

    // Final buffer frame

    for (int i = 0; i < GRID_SIZE + 2; ++i) {
        for (int j = 0; j < GRID_SIZE + 2; ++j) {
            setCursorPosition(j, i);
            std::cout << display[i][j] << ' ';
        }
    }

    std::cout.flush();
}


int main()
{

    bool exit = false;

    while (!exit) {
        run();

        // Re-display cursor to indicate user input
        showConsoleCursor(true);

        std::cout << "\n\nGAME OVER\n\nPRESS ENTER TO PLAY AGAIN OR ESCAPE TO EXIT:" << std::endl;

        // Wait for ENTER key input
        while (true) {
            if (GetKeyState(VK_RETURN) & 0x8000) {
                cls();
                break;
            }
            else if (GetKeyState(VK_ESCAPE) & 0x8000) {
                exit = true;
                break;
            }
        }
    }
    return 0;
}
