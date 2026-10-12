#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "string.h"
#include "io.h"

// Snake game constants
#define GAME_WIDTH    44
#define GAME_HEIGHT   20
#define GAME_OFFSET_X 18   // Center on 80-col screen
#define GAME_OFFSET_Y 3    // Leave room for top score header
#define MAX_SNAKE_LENGTH 300

// Directions
#define DIR_UP    0
#define DIR_DOWN  1
#define DIR_LEFT  2
#define DIR_RIGHT 3

// Cell types
#define CELL_EMPTY      0
#define CELL_SNAKE      1
#define CELL_FOOD_NORM  2
#define CELL_FOOD_GOLD  3
#define CELL_WALL       4

static int snake_x[MAX_SNAKE_LENGTH];
static int snake_y[MAX_SNAKE_LENGTH];
static int snake_length;
static int snake_dir;

static int food_x, food_y;
static int food_type;     // CELL_FOOD_NORM or CELL_FOOD_GOLD
static int food_timer;    // Countdown for gold food

static int score;
static int level;
static int session_high_score = 0;
static int game_over;
static int is_paused;

static uint8_t game_grid[GAME_HEIGHT][GAME_WIDTH];
static uint32_t rng_state;

// Audio helpers for PC speaker
static void speaker_play(uint32_t freq) {
    if (freq == 0) return;
    uint32_t div = 1193180 / freq;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div & 0xFF));
    outb(0x42, (uint8_t)((div >> 8) & 0xFF));
    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) outb(0x61, tmp | 3);
}

static void speaker_stop(void) {
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

static void sound_eat_norm(void) {
    speaker_play(523); // C5
    timer_wait(2);
    speaker_stop();
}

static void sound_eat_gold(void) {
    speaker_play(659); // E5
    timer_wait(2);
    speaker_play(784); // G5
    timer_wait(3);
    speaker_stop();
}

static void sound_game_over(void) {
    speaker_play(400); timer_wait(4);
    speaker_play(300); timer_wait(4);
    speaker_play(200); timer_wait(6);
    speaker_stop();
}

// Pseudo-random generator
static uint32_t rand_next(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

static void place_food(void) {
    int attempts = 0;
    do {
        food_x = (rand_next() % (GAME_WIDTH - 2)) + 1;
        food_y = (rand_next() % (GAME_HEIGHT - 2)) + 1;
        attempts++;
    } while (game_grid[food_y][food_x] != CELL_EMPTY && attempts < 500);

    // 25% chance for Gold food
    if ((rand_next() % 4) == 0) {
        food_type = CELL_FOOD_GOLD;
        food_timer = 50; // stays for 50 ticks
    } else {
        food_type = CELL_FOOD_NORM;
        food_timer = 0;
    }

    game_grid[food_y][food_x] = food_type;
}

static void draw_cell(int gx, int gy) {
    int sx = GAME_OFFSET_X + gx;
    int sy = GAME_OFFSET_Y + gy;
    
    uint16_t* vga = (uint16_t*)0xB8000;
    uint16_t entry;

    switch (game_grid[gy][gx]) {
        case CELL_WALL:
            entry = (uint16_t)'#' | ((uint16_t)(VGA_COLOR_LIGHT_CYAN | (VGA_COLOR_BLACK << 4)) << 8);
            break;
        case CELL_SNAKE:
            if (gx == snake_x[0] && gy == snake_y[0]) {
                // Directional head
                char head_char = '@';
                if (snake_dir == DIR_UP)    head_char = '^';
                if (snake_dir == DIR_DOWN)  head_char = 'v';
                if (snake_dir == DIR_LEFT)  head_char = '<';
                if (snake_dir == DIR_RIGHT) head_char = '>';
                entry = (uint16_t)head_char | ((uint16_t)(VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4)) << 8);
            } else {
                entry = (uint16_t)'O' | ((uint16_t)(VGA_COLOR_GREEN | (VGA_COLOR_BLACK << 4)) << 8);
            }
            break;
        case CELL_FOOD_NORM:
            entry = (uint16_t)'*' | ((uint16_t)(VGA_COLOR_LIGHT_RED | (VGA_COLOR_BLACK << 4)) << 8);
            break;
        case CELL_FOOD_GOLD:
            entry = (uint16_t)'$' | ((uint16_t)(VGA_COLOR_LIGHT_BROWN | (VGA_COLOR_BLACK << 4)) << 8);
            break;
        default: // CELL_EMPTY
            entry = (uint16_t)' ' | ((uint16_t)(VGA_COLOR_BLACK | (VGA_COLOR_BLACK << 4)) << 8);
            break;
    }

    vga[sy * 80 + sx] = entry;
}

static void print_string_at(int x, int y, const char *str, uint8_t color) {
    uint16_t* vga = (uint16_t*)0xB8000;
    for (int i = 0; str[i]; i++) {
        vga[y * 80 + x + i] = (uint16_t)str[i] | ((uint16_t)color << 8);
    }
}

static void print_num_at(int x, int y, int val, uint8_t color) {
    char buf[16];
    if (val == 0) {
        print_string_at(x, y, "0", color);
        return;
    }
    int idx = 0, temp = val;
    char temp_buf[16];
    int t_idx = 0;
    while (temp > 0) {
        temp_buf[t_idx++] = '0' + (temp % 10);
        temp /= 10;
    }
    while (t_idx > 0) {
        buf[idx++] = temp_buf[--t_idx];
    }
    buf[idx] = '\0';
    print_string_at(x, y, buf, color);
}

static void draw_header(void) {
    uint8_t title_color = VGA_COLOR_LIGHT_CYAN | (VGA_COLOR_BLACK << 4);
    uint8_t score_color = VGA_COLOR_LIGHT_GREEN | (VGA_COLOR_BLACK << 4);
    uint8_t high_color  = VGA_COLOR_LIGHT_BROWN | (VGA_COLOR_BLACK << 4);
    uint8_t hint_color  = VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4);

    print_string_at(GAME_OFFSET_X, 0, "=== UzOS SUPER SNAKE ===", title_color);
    
    print_string_at(GAME_OFFSET_X, 1, "Score: ", hint_color);
    print_num_at(GAME_OFFSET_X + 7, 1, score, score_color);

    print_string_at(GAME_OFFSET_X + 16, 1, "High: ", hint_color);
    print_num_at(GAME_OFFSET_X + 22, 1, session_high_score, high_color);

    print_string_at(GAME_OFFSET_X + 32, 1, "Lvl: ", hint_color);
    print_num_at(GAME_OFFSET_X + 37, 1, level, score_color);

    print_string_at(GAME_OFFSET_X, GAME_OFFSET_Y + GAME_HEIGHT, 
                    " [Arrows/WASD] Move  [P] Pause  [R] Restart  [Q] Quit", hint_color);
}

static void draw_dialog_box(const char *line1, const char *line2, uint8_t color) {
    int box_width = 36;
    int box_height = 5;
    int start_x = GAME_OFFSET_X + (GAME_WIDTH - box_width) / 2;
    int start_y = GAME_OFFSET_Y + (GAME_HEIGHT - box_height) / 2;

    uint16_t* vga = (uint16_t*)0xB8000;
    uint8_t bg_color = VGA_COLOR_BLUE | (VGA_COLOR_BLACK << 4);

    // Fill background
    for (int y = 0; y < box_height; y++) {
        for (int x = 0; x < box_width; x++) {
            vga[(start_y + y) * 80 + start_x + x] = (uint16_t)' ' | ((uint16_t)bg_color << 8);
        }
    }

    // Print text centered
    int len1 = strlen(line1);
    int len2 = strlen(line2);
    print_string_at(start_x + (box_width - len1) / 2, start_y + 1, line1, color);
    print_string_at(start_x + (box_width - len2) / 2, start_y + 3, line2, VGA_COLOR_WHITE | (VGA_COLOR_BLACK << 4));
}

static void init_game(void) {
    extern volatile uint32_t timer_ticks;
    rng_state = timer_ticks ^ 0xDEADBEEF;

    memset(game_grid, CELL_EMPTY, sizeof(game_grid));

    // Walls
    for (int x = 0; x < GAME_WIDTH; x++) {
        game_grid[0][x] = CELL_WALL;
        game_grid[GAME_HEIGHT - 1][x] = CELL_WALL;
    }
    for (int y = 0; y < GAME_HEIGHT; y++) {
        game_grid[y][0] = CELL_WALL;
        game_grid[y][GAME_WIDTH - 1] = CELL_WALL;
    }

    snake_length = 4;
    snake_dir = DIR_RIGHT;
    int start_x = GAME_WIDTH / 2;
    int start_y = GAME_HEIGHT / 2;

    for (int i = 0; i < snake_length; i++) {
        snake_x[i] = start_x - i;
        snake_y[i] = start_y;
        game_grid[start_y][start_x - i] = CELL_SNAKE;
    }

    score = 0;
    level = 1;
    game_over = 0;
    is_paused = 0;

    place_food();

    // Clear VGA screen
    uint16_t* vga = (uint16_t*)0xB8000;
    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = (uint16_t)' ' | ((uint16_t)(VGA_COLOR_BLACK | (VGA_COLOR_BLACK << 4)) << 8);
    }

    // Draw game field
    for (int y = 0; y < GAME_HEIGHT; y++) {
        for (int x = 0; x < GAME_WIDTH; x++) {
            draw_cell(x, y);
        }
    }
    draw_header();
}

static void game_step(void) {
    if (is_paused || game_over) return;

    // Handle gold food timer
    if (food_type == CELL_FOOD_GOLD && food_timer > 0) {
        food_timer--;
        if (food_timer == 0) {
            // Gold food expired — replace with normal
            game_grid[food_y][food_x] = CELL_EMPTY;
            draw_cell(food_x, food_y);
            place_food();
            draw_cell(food_x, food_y);
        }
    }

    // Next head position
    int new_x = snake_x[0];
    int new_y = snake_y[0];

    switch (snake_dir) {
        case DIR_UP:    new_y--; break;
        case DIR_DOWN:  new_y++; break;
        case DIR_LEFT:  new_x--; break;
        case DIR_RIGHT: new_x++; break;
    }

    // Collision check
    if (game_grid[new_y][new_x] == CELL_WALL || game_grid[new_y][new_x] == CELL_SNAKE) {
        game_over = 1;
        if (score > session_high_score) {
            session_high_score = score;
        }
        sound_game_over();
        draw_dialog_box("=== GAME OVER! ===", "[R] Restart   [Q] Quit", VGA_COLOR_LIGHT_RED | (VGA_COLOR_BLACK << 4));
        return;
    }

    int target_cell = game_grid[new_y][new_x];
    int ate_norm = (target_cell == CELL_FOOD_NORM);
    int ate_gold = (target_cell == CELL_FOOD_GOLD);

    if (!ate_norm && !ate_gold) {
        // Erase tail
        int tail_idx = snake_length - 1;
        game_grid[snake_y[tail_idx]][snake_x[tail_idx]] = CELL_EMPTY;
        draw_cell(snake_x[tail_idx], snake_y[tail_idx]);

        for (int i = snake_length - 1; i > 0; i--) {
            snake_x[i] = snake_x[i - 1];
            snake_y[i] = snake_y[i - 1];
        }
    } else {
        // Grow snake
        if (snake_length < MAX_SNAKE_LENGTH) {
            for (int i = snake_length; i > 0; i--) {
                snake_x[i] = snake_x[i - 1];
                snake_y[i] = snake_y[i - 1];
            }
            snake_length++;
        }

        if (ate_norm) {
            score += 10;
            sound_eat_norm();
        } else if (ate_gold) {
            score += 30;
            sound_eat_gold();
        }

        if (score > session_high_score) {
            session_high_score = score;
        }

        level = 1 + (score / 50);
        place_food();
        draw_cell(food_x, food_y);
    }

    // Move head
    snake_x[0] = new_x;
    snake_y[0] = new_y;
    game_grid[new_y][new_x] = CELL_SNAKE;

    draw_cell(snake_x[0], snake_y[0]);
    if (snake_length > 1) {
        draw_cell(snake_x[1], snake_y[1]);
    }

    draw_header();
}

void cmd_snake(const char *args) {
    (void)args;

    init_game();

    extern volatile uint32_t timer_ticks;
    uint32_t last_tick = timer_ticks;

    while (1) {
        // Compute current speed based on level (starts at relaxed 14 ticks = 140ms, speeds up per level)
        int speed_val = 14 - (level - 1);
        if (speed_val < 4) speed_val = 4;
        uint32_t speed = (uint32_t)speed_val;

        // Process keyboard input
        char c = keyboard_pop_key();
        if (c) {
            if ((unsigned char)c == KEY_UP || c == 'w' || c == 'W') {
                if (snake_dir != DIR_DOWN && !is_paused) snake_dir = DIR_UP;
            } else if ((unsigned char)c == KEY_DOWN || c == 's' || c == 'S') {
                if (snake_dir != DIR_UP && !is_paused) snake_dir = DIR_DOWN;
            } else if ((unsigned char)c == KEY_LEFT || c == 'a' || c == 'A') {
                if (snake_dir != DIR_RIGHT && !is_paused) snake_dir = DIR_LEFT;
            } else if ((unsigned char)c == KEY_RIGHT || c == 'd' || c == 'D') {
                if (snake_dir != DIR_LEFT && !is_paused) snake_dir = DIR_RIGHT;
            } else if (c == 'p' || c == 'P' || c == ' ') {
                if (!game_over) {
                    is_paused = !is_paused;
                    if (is_paused) {
                        draw_dialog_box("=== GAME PAUSED ===", "[Space/P] Resume", VGA_COLOR_LIGHT_BROWN | (VGA_COLOR_BLACK << 4));
                    } else {
                        // Redraw game field over dialog box
                        for (int y = 0; y < GAME_HEIGHT; y++) {
                            for (int x = 0; x < GAME_WIDTH; x++) {
                                draw_cell(x, y);
                            }
                        }
                    }
                }
            } else if (c == 'r' || c == 'R') {
                init_game();
            } else if (c == 'q' || c == 'Q') {
                vga_init();
                vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_print_str("Thanks for playing Snake! Final score: ");
                vga_print_int(score);
                vga_print_str(" | Best Score: ");
                vga_print_int(session_high_score);
                vga_print_str("\n");
                return;
            }
        }

        // Game tick update
        if (timer_ticks - last_tick >= speed) {
            last_tick = timer_ticks;
            if (!game_over && !is_paused) {
                game_step();
            }
        }

        asm volatile("hlt");
    }
}
