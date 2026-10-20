/*
 * cmd_snake.c - `snake`: the classic game, in text mode.
 *
 * The board is a grid of cells (empty, wall, snake, food). The snake is
 * an array of positions, head first. Every game tick the head moves one
 * cell; the tail follows unless the snake just ate.
 *
 *   arrows / WASD  turn        P / Space  pause
 *   R              restart     Q          quit (the old screen comes back)
 *
 * The game borrows the whole screen (vga_fullscreen) and centres the
 * board, so it works in every display mode.
 *
 * Uses: vga_fullscreen / vga_put_at / vga_write_at (device_management
 *       vga), keyboard_pop_key, speaker_beep, timer_ticks.
 */

#include "shell_commands.h"
#include "vga.h"
#include "glyphs.h"
#include "keyboard.h"
#include "speaker.h"
#include "timer.h"
#include "string.h"

#define WIDTH       44              // board size, walls included
#define HEIGHT      20
#define TEXT_ROWS   4               // 2 status lines above, 1 help line below, 1 gap
#define MAX_LENGTH  ((WIDTH - 2) * (HEIGHT - 2))   // the whole inside of the board

enum cell { EMPTY, WALL, SNAKE, FOOD, GOLD };
enum dir  { UP, DOWN, LEFT_DIR, RIGHT_DIR };

static uint8_t board[HEIGHT][WIDTH];
static int body_x[MAX_LENGTH], body_y[MAX_LENGTH];  // [0] is the head
static int length;
static enum dir heading;            // where the player wants to go
static enum dir moved;              // where the snake last actually went
static int food_x, food_y, gold_ticks;
static int score, best, paused, over;
static uint32_t random_state;
static int left, top;               // where the board sits on the screen

/* ---------- Helpers ---------- */

static uint32_t random_next(void) {         // a simple linear congruential generator
    random_state = random_state * 1103515245 + 12345;
    return (random_state >> 16) & 0x7FFF;
}

static void text_at(int x, int y, const char *s, enum vga_color fg) {
    vga_write_at(x, y, s, fg, VGA_COLOR_BLACK);
}

static void number_at(int x, int y, int value, enum vga_color fg) {
    char digits[12];
    int n = 0;
    do { digits[n++] = '0' + value % 10; value /= 10; } while (value);
    while (n)
        vga_put_at(x++, y, digits[--n], fg, VGA_COLOR_BLACK);
    vga_put_at(x, y, ' ', fg, VGA_COLOR_BLACK);    // erase a longer old number
}

/* ---------- Drawing ---------- */

static void draw_cell(int x, int y) {
    char c = ' ';
    enum vga_color fg = VGA_COLOR_BLACK;

    switch (board[y][x]) {
    case WALL:  c = '\xB1'; fg = VGA_COLOR_CYAN;     break;   // ▒
    case FOOD:  c = GLYPH_STAR[0]; fg = VGA_COLOR_LIGHT_RED; break;
    case GOLD:  c = '$'; fg = VGA_COLOR_LIGHT_BROWN; break;
    case SNAKE:
        fg = VGA_COLOR_GREEN;
        c = '\xFE';                                  // ■
        if (x == body_x[0] && y == body_y[0]) {     // the head points where it goes
            static const char head[] = { '^', 'v', '<', '>' };
            c = head[moved];
            fg = VGA_COLOR_LIGHT_GREEN;
        }
        break;
    }
    vga_put_at(left + x, top + y, c, fg, VGA_COLOR_BLACK);
}

static void draw_board(void) {
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++)
            draw_cell(x, y);
}

static void draw_status(void) {
    text_at(left, top - 3, "=== UzOS SNAKE ===", VGA_COLOR_LIGHT_CYAN);
    text_at(left, top - 2, "Score:", VGA_COLOR_LIGHT_GREY);
    number_at(left + 7, top - 2, score, VGA_COLOR_LIGHT_GREEN);
    text_at(left + 16, top - 2, "Best:", VGA_COLOR_LIGHT_GREY);
    number_at(left + 22, top - 2, best, VGA_COLOR_LIGHT_BROWN);
    text_at(left - 4, top + HEIGHT, "[Arrows/WASD] move  [P] pause  [R] restart  [Q] quit",
            VGA_COLOR_LIGHT_GREY);
}

static void draw_message(const char *line1, const char *line2, enum vga_color fg) {
    int y = top + HEIGHT / 2 - 1;
    text_at(left + (WIDTH - strlen(line1)) / 2, y, line1, fg);
    text_at(left + (WIDTH - strlen(line2)) / 2, y + 2, line2, VGA_COLOR_WHITE);
}

/* ---------- Game rules ---------- */

// A random empty cell; if random tries keep hitting the snake (the board
// is nearly full), the first empty cell. Returns 0 if there is none.
static int find_free_cell(int *x, int *y) {
    for (int tries = 0; tries < 200; tries++) {
        *x = 1 + random_next() % (WIDTH - 2);
        *y = 1 + random_next() % (HEIGHT - 2);
        if (board[*y][*x] == EMPTY)
            return 1;
    }
    for (*y = 1; *y < HEIGHT - 1; (*y)++)
        for (*x = 1; *x < WIDTH - 1; (*x)++)
            if (board[*y][*x] == EMPTY)
                return 1;
    return 0;
}

static void place_food(void) {
    int x, y;
    if (!find_free_cell(&x, &y))
        return;                     // no free cell left: you have won

    food_x = x;
    food_y = y;
    int gold = random_next() % 4 == 0;              // 1 in 4 is worth more
    board[y][x] = gold ? GOLD : FOOD;
    gold_ticks = gold ? 50 : 0;                     // gold disappears after a while
    draw_cell(x, y);
}

static void new_game(void) {
    random_state = timer_ticks ^ 0xDEADBEEF;
    memset(board, EMPTY, sizeof(board));
    for (int x = 0; x < WIDTH; x++)
        board[0][x] = board[HEIGHT - 1][x] = WALL;
    for (int y = 0; y < HEIGHT; y++)
        board[y][0] = board[y][WIDTH - 1] = WALL;

    length = 4;
    heading = moved = RIGHT_DIR;
    for (int i = 0; i < length; i++) {
        body_x[i] = WIDTH / 2 - i;
        body_y[i] = HEIGHT / 2;
        board[body_y[i]][body_x[i]] = SNAKE;
    }
    score = paused = over = 0;

    for (int y = 0; y < vga_rows(); y++)            // clear the whole screen
        for (int x = 0; x < vga_cols(); x++)
            vga_put_at(x, y, ' ', VGA_COLOR_BLACK, VGA_COLOR_BLACK);
    draw_board();
    place_food();
    draw_status();
}

static void game_over(void) {
    over = 1;
    if (score > best)
        best = score;
    draw_message("=== GAME OVER ===", "[R] restart   [Q] quit", VGA_COLOR_LIGHT_RED);
    speaker_beep(400, 40);
    speaker_beep(300, 40);
    speaker_beep(200, 60);
}

static void step(void) {
    if (gold_ticks && --gold_ticks == 0) {          // gold food ran out
        board[food_y][food_x] = EMPTY;
        draw_cell(food_x, food_y);
        place_food();
    }

    int x = body_x[0] + (heading == RIGHT_DIR) - (heading == LEFT_DIR);
    int y = body_y[0] + (heading == DOWN) - (heading == UP);
    moved = heading;

    if (board[y][x] == WALL || board[y][x] == SNAKE) {
        game_over();
        return;
    }

    int ate = board[y][x] == FOOD || board[y][x] == GOLD;
    int grows = ate && length < MAX_LENGTH;
    if (ate)
        score += board[y][x] == GOLD ? 30 : 10;

    if (!grows) {                                   // the tail moves up one cell
        board[body_y[length - 1]][body_x[length - 1]] = EMPTY;
        draw_cell(body_x[length - 1], body_y[length - 1]);
    } else {
        length++;
    }
    for (int i = length - 1; i > 0; i--) {          // every part follows the one in front
        body_x[i] = body_x[i - 1];
        body_y[i] = body_y[i - 1];
    }

    body_x[0] = x;
    body_y[0] = y;
    board[y][x] = SNAKE;
    draw_cell(x, y);
    draw_cell(body_x[1], body_y[1]);                // old head becomes body

    if (ate) {
        speaker_beep(score % 30 ? 523 : 784, 20);
        place_food();
        draw_status();
    }
}

/* ---------- Input and main loop ---------- */

// Turn, unless that would go straight back into the snake's own neck.
static void turn(enum dir want) {
    static const enum dir opposite[] = { DOWN, UP, RIGHT_DIR, LEFT_DIR };
    if (!paused && want != opposite[moved])         // 'moved', not 'heading':
        heading = want;                             // two quick keys cannot reverse
}

// Handle one key. Returns 0 when the player quits.
static int handle_key(char c) {
    unsigned char k = (unsigned char)c;
    if (k == KEY_UP || c == 'w' || c == 'W')         turn(UP);
    else if (k == KEY_DOWN || c == 's' || c == 'S')  turn(DOWN);
    else if (k == KEY_LEFT || c == 'a' || c == 'A')  turn(LEFT_DIR);
    else if (k == KEY_RIGHT || c == 'd' || c == 'D') turn(RIGHT_DIR);
    else if (c == 'r' || c == 'R')                   new_game();
    else if (c == 'q' || c == 'Q')                   return 0;
    else if ((c == 'p' || c == 'P' || c == ' ') && !over) {
        paused = !paused;
        if (paused)
            draw_message("=== PAUSED ===", "[P] continue", VGA_COLOR_LIGHT_BROWN);
        else
            draw_board();                           // paint over the message
    }
    return 1;
}

void cmd_snake(const char *args) {
    (void)args;
    left = (vga_cols() - WIDTH) / 2;                // centred on any screen size
    top = (vga_rows() - HEIGHT - TEXT_ROWS) / 2 + 3;
    vga_fullscreen(1);
    new_game();
    uint32_t last_step = timer_ticks;
    int playing = 1;

    while (playing) {
        char c;
        while (playing && (c = keyboard_pop_key()))
            playing = handle_key(c);

        // Faster as the score grows: 14 ticks per step, down to 4.
        int level = score / 50;
        uint32_t delay = level < 10 ? 14 - level : 4;
        if (!paused && !over && timer_ticks - last_step >= delay) {
            last_step = timer_ticks;
            step();
        }
        asm volatile("hlt");                        // sleep until the next tick or key
    }

    vga_fullscreen(0);                              // bring the shell screen back
    vga_print_str("Snake: score ");
    vga_print_int(score);
    vga_print_str(", best ");
    vga_print_int(best);
    vga_print_str("\n");
}
