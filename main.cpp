#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <utility>
#include <vector>

class TerminalMode {
public:
    TerminalMode() {
        if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &original_) == 0) {
            termios raw = original_;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 1;
            raw.c_cc[VTIME] = 0;
            active_ = tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0;
        }
    }

    ~TerminalMode() {
        if (active_) {
            tcsetattr(STDIN_FILENO, TCSANOW, &original_);
        }
        std::cout << "\033[0m\033[?25h\n";
    }

    bool active() const { return active_; }

private:
    termios original_{};
    bool active_ = false;
};

enum class Color { Blue, Red, Purple, Rainbow };

void setColor(Color color, int row, int column) {
    if (color == Color::Rainbow) {
        const std::array<const char*, 6> colors{
            "\033[38;5;196m", "\033[38;5;202m", "\033[38;5;226m",
            "\033[38;5;46m", "\033[38;5;39m", "\033[38;5;201m"
        };
        std::cout << colors[static_cast<std::size_t>((row + column) % colors.size())];
        return;
    }

    switch (color) {
        case Color::Blue: std::cout << "\033[1;34m"; break;
        case Color::Red: std::cout << "\033[1;31m"; break;
        case Color::Purple: std::cout << "\033[1;35m"; break;
        case Color::Rainbow: break;
    }
}

const char* colorName(Color color) {
    switch (color) {
        case Color::Blue: return "blue";
        case Color::Red: return "red";
        case Color::Purple: return "purple";
        case Color::Rainbow: return "rainbow";
    }
    return "blue";
}

void draw(const std::vector<std::vector<bool>>& blocks,
          const std::vector<std::pair<int, int>>& enemies,
          int x, int y, Color color, const std::string& message) {
    const int boardHeight = static_cast<int>(blocks.size());
    const int boardWidth = static_cast<int>(blocks[0].size());
    const std::array<std::string, 7> letter{
        "BBBBBB ", "BB   BB", "BB   BB", "BBBBBB ",
        "BB   BB", "BB   BB", "BBBBBB "
    };
    int blockCount = 0;
    for (const auto& row : blocks) {
        for (bool occupied : row) {
            blockCount += occupied ? 1 : 0;
        }
    }

    std::cout << "\033[2J\033[H\033[?25l";
    for (int row = 0; row < static_cast<int>(letter.size()); ++row) {
        for (int column = 0; column < static_cast<int>(letter[row].size()); ++column) {
            if (letter[row][column] != ' ') {
                setColor(color, row, column);
            }
            std::cout << letter[row][column] << "\033[0m";
        }
        std::cout << '\n';
    }
    std::cout << "\n BLOCK WORLD | @ cursor | blocks: " << blockCount
              << " | enemies: " << enemies.size()
              << " | B color: " << colorName(color) << "\n"
              << '+' << std::string(static_cast<std::size_t>(boardWidth), '-') << "+\n";
    for (int row = 0; row < boardHeight; ++row) {
        std::cout << '|';
        for (int column = 0; column < boardWidth; ++column) {
            if (column == x && row == y) {
                std::cout << "\033[1;33m@";
            } else if (std::find(enemies.begin(), enemies.end(), std::pair<int, int>{column, row}) != enemies.end()) {
                std::cout << "\033[1;31mE";
            } else if (blocks[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)]) {
                setColor(color, row, column);
                std::cout << '#';
            } else {
                std::cout << "\033[90m.";
            }
            std::cout << "\033[0m";
        }
        std::cout << "|\n";
    }
    std::cout << '+' << std::string(static_cast<std::size_t>(boardWidth), '-') << "+\n"
              << "WASD move | C place | V break | F attack | E spawn enemy | Q quit\n"
              << "1 blue | 2 red | 3 purple | 4 rainbow | " << message << '\n'
              << std::flush;
}

int main() {
    TerminalMode terminal;
    if (!terminal.active()) {
        std::cerr << "Run this in a terminal that supports keyboard input.\n";
        return 1;
    }

    winsize size{};
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
    const int terminalWidth = size.ws_col > 0 ? size.ws_col : 80;
    const int terminalHeight = size.ws_row > 0 ? size.ws_row : 24;
    const int boardWidth = std::max(1, terminalWidth - 2);
    const int boardHeight = std::max(1, terminalHeight - 13);
    std::vector<std::vector<bool>> blocks(
        static_cast<std::size_t>(boardHeight),
        std::vector<bool>(static_cast<std::size_t>(boardWidth), false));
    int x = boardWidth / 2;
    int y = boardHeight / 2;
    Color color = Color::Blue;
    std::vector<std::pair<int, int>> enemies;
    std::mt19937 random(std::random_device{}());
    std::string message = "Ready";
    draw(blocks, enemies, x, y, color, message);

    char key = 0;
    while (read(STDIN_FILENO, &key, 1) == 1) {
        switch (key) {
            case 'w': case 'W': y = std::max(0, y - 1); break;
            case 'a': case 'A': x = std::max(0, x - 1); break;
            case 's': case 'S': y = std::min(boardHeight - 1, y + 1); break;
            case 'd': case 'D': x = std::min(boardWidth - 1, x + 1); break;
            case 'c': case 'C': {
                const bool enemyHere = std::find(enemies.begin(), enemies.end(), std::pair<int, int>{x, y}) != enemies.end();
                if (!enemyHere) {
                    blocks[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = true;
                    message = "Block placed";
                } else {
                    message = "Cannot place on an enemy";
                }
                break;
            }
            case 'v': case 'V':
                blocks[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = false;
                message = "Block broken";
                break;
            case 'e': case 'E': {
                std::vector<std::pair<int, int>> openCells;
                for (int row = 0; row < boardHeight; ++row) {
                    for (int column = 0; column < boardWidth; ++column) {
                        const auto cell = std::pair<int, int>{column, row};
                        if ((column != x || row != y) &&
                            !blocks[static_cast<std::size_t>(row)][static_cast<std::size_t>(column)] &&
                            std::find(enemies.begin(), enemies.end(), cell) == enemies.end()) {
                            openCells.push_back(cell);
                        }
                    }
                }
                if (openCells.empty()) {
                    message = "No open space to spawn an enemy";
                } else {
                    std::uniform_int_distribution<std::size_t> choose(0, openCells.size() - 1);
                    enemies.push_back(openCells[choose(random)]);
                    message = "Enemy spawned";
                }
                break;
            }
            case 'f': case 'F': {
                const auto target = std::find_if(enemies.begin(), enemies.end(), [x, y](const auto& enemy) {
                    return std::abs(enemy.first - x) + std::abs(enemy.second - y) <= 1;
                });
                if (target == enemies.end()) {
                    message = "No enemy close enough to attack";
                } else {
                    enemies.erase(target);
                    message = "Enemy defeated";
                }
                break;
            }
            case '1': color = Color::Blue; break;
            case '2': color = Color::Red; break;
            case '3': color = Color::Purple; break;
            case '4': color = Color::Rainbow; break;
            case 'q': case 'Q': return 0;
            default: break;
        }
        draw(blocks, enemies, x, y, color, message);
    }
    return 0;
}