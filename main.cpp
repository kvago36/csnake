#include <iostream>
#include <vector>
#include <optional>
#include <random>
#include <ranges>
#include <cassert>

#include <SDL3/SDL.h>

const int MATRIX_SIZE = 27;
const float SIZE = 30.0;

enum class Direction { UP, DOWN, LEFT, RIGHT };

enum class Tie { Food, Particle };

struct Point {
    float x;
    float y;

    Point() : x(0.0), y(0.0) {
    }

    Point(float x, float y) : x(x), y(y) {
    }

    void draw(SDL_Renderer *renderer) {
        SDL_FRect square = {x * SIZE, y * SIZE, SIZE, SIZE};
        SDL_RenderFillRect(renderer, &square);
    }
};

class Board {
    std::optional<Tie> board[MATRIX_SIZE][MATRIX_SIZE];

public:
    Board() {
        for (int i = 0; i < MATRIX_SIZE; i++) {
            for (int j = 0; j < MATRIX_SIZE; j++) {
                if (i == 3 && j == 3) {
                    board[i][j] = Tie::Food;
                } else if ((i == 12 || i == 13 || i == 14) && j == 12) {
                    board[i][j] = Tie::Particle;
                } else {
                    board[i][j] = std::nullopt;
                }
            }
        }
    };

    std::optional<Point> generate_food() {
        std::vector<Point> empty;

        for (int i = 0; i < MATRIX_SIZE; i++) {
            for (int j = 0; j < MATRIX_SIZE; j++) {
                if (board[i][j] == std::nullopt) {
                    empty.push_back(Point(i, j)); // Умный указатель
                }
            }
        }

        if (empty.empty()) {
            return std::nullopt;
        }

        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_int_distribution<> distr(0, empty.size() - 1);

        int randomIndex = distr(gen);

        int i = static_cast<int>(empty[randomIndex].x);
        int j = static_cast<int>(empty[randomIndex].y);

        board[i][j] = Tie::Food;

        return empty[randomIndex];
    }

    std::optional<Tie> getValue(int row, int col) const {
        return board[row][col];
    }

    void setValue(int row, int col, std::optional<Tie> tie) {
        board[row][col] = tie;
    }
};

class Game {
    Board board;
    Point food;
    std::vector<Point> snake;
    Direction direction;

public:
    bool is_finished;
    bool is_paused;

    Game() : board(Board()), food(Point(3.0, 3.0)),
             snake{Point(12.0, 12.0), Point(13.0, 12.0), Point(14.0, 12.0)}, direction(Direction::LEFT), is_finished(false), is_paused(false) {
    };

    void change_direction(Direction pressed) {
        switch (direction) {
            case Direction::UP:
            case Direction::DOWN:
                if (pressed == Direction::LEFT || pressed == Direction::RIGHT) {
                    direction = pressed;
                }
                break;
            case Direction::RIGHT:
            case Direction::LEFT:
                if (pressed == Direction::UP || pressed == Direction::DOWN) {
                    direction = pressed;
                }
                break;
        }
    }

    void draw(SDL_Renderer *renderer) {
        food.draw(renderer);

        for (auto it = snake.begin(); it != snake.end(); ++it) {
            it->draw(renderer);
        }
    }

    void move_snake() {
        assert(!snake.empty());

        Point old_head = snake.front();
        Point tail = snake.back();
        Point new_head;

        switch (direction) {
            case Direction::UP:
                new_head.x = old_head.x;

                if (old_head.y == 0) {
                    new_head.y = MATRIX_SIZE - 1;
                } else {
                    new_head.y = old_head.y - 1;
                }
                break;
            case Direction::DOWN:
                new_head.x = old_head.x;

                if (old_head.y == MATRIX_SIZE - 1) {
                    new_head.y = 0;
                } else {
                    new_head.y = old_head.y + 1;
                }
                break;
            case Direction::RIGHT:
                new_head.y = old_head.y;

                if (old_head.x == MATRIX_SIZE - 1) {
                    new_head.x = 0;
                } else {
                    new_head.x = old_head.x + 1;
                }
                break;
            case Direction::LEFT:
                new_head.y = old_head.y;

                if (old_head.x == 0) {
                    new_head.x = MATRIX_SIZE - 1;
                } else {
                    new_head.x = old_head.x - 1;
                }
                break;
        }

        std::optional<Tie> tie = board.getValue(new_head.x, new_head.y);

        if (tie == Tie::Food) {
            auto new_food = board.generate_food();

            if (new_food == std::nullopt) {
                is_finished = true;
            } else {
                food = new_food.value();
            }
        }

        if (tie == Tie::Particle) {
            is_finished = true;
            return;
        }

        // if snake eat food no need to remove last piece
        if (tie != Tie::Food) {
            snake.pop_back();
        }

        snake.insert(snake.begin(), new_head);

        board.setValue(tail.x, tail.y, std::nullopt);
        board.setValue(new_head.x, new_head.y, Tie::Particle);
    }
};


int main() {
    std::cout << "Game started!" << std::endl;

    Game game;

    // Инициализация SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Ошибка инициализации SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Создание окна
    SDL_Window *window = SDL_CreateWindow("Snake Game", 27 * SIZE, 27 * SIZE, SDL_WINDOW_OPENGL);
    if (!window) {
        std::cerr << "Ошибка создания окна: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Создание рендерера
    SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);

    if (!renderer) {
        std::cerr << "Ошибка создания рендерера: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    const int FPS = 4; // Желаемая частота кадров
    const int FRAME_DELAY = 1000 / FPS; // Время на 1 кадр (мс)

    // Основной цикл
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        Uint64 startTick = SDL_GetTicks(); // Засекаем начало кадра

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT || game.is_finished) {
                std::cout << "Game finished!" << std::endl;

                quit = true;
            }

            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.scancode) {
                    case SDL_SCANCODE_UP:
                    case SDL_SCANCODE_W:
                        game.change_direction(Direction::UP);
                        break;
                    case SDL_SCANCODE_LEFT:
                    case SDL_SCANCODE_A:
                        game.change_direction(Direction::LEFT);

                        break;
                    case SDL_SCANCODE_DOWN:
                    case SDL_SCANCODE_S:
                        game.change_direction(Direction::DOWN);

                        break;
                    case SDL_SCANCODE_RIGHT:
                    case SDL_SCANCODE_D:
                        game.change_direction(Direction::RIGHT);
                        break;
                    case SDL_SCANCODE_P:
                    case SDL_SCANCODE_SPACE:
                        game.is_paused = !game.is_paused;
                    default:
                        break;
                }
            }
        }

        // Очистка экрана
        SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

        if (!game.is_paused) {
            game.move_snake();
        }

        game.draw(renderer);

        // Отображение изменений
        SDL_RenderPresent(renderer);

        Uint64 frameTime = SDL_GetTicks() - startTick; // Время, потраченное на кадр

        if (frameTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frameTime); // Ждем оставшееся время
        }
    }

    // Очистка ресурсов
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
