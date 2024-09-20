#include <stdlib.h>
#include <unistd.h>
#include <curses.h>
#include <time.h>
#include <string.h>
#include <stdio.h>  // For file handling

#define MAX_SCORE 256
#define FRAME_TIME 120000

typedef struct {
    int x;
    int y;
} vec2;

int score = 0;
int high_score = 0;
char score_message[32];

bool skip = false;
bool is_running = true;

int screen_width = 40;
int screen_height = 30;

// initialize screen
WINDOW *win;

// snake
vec2 head = { 0, 0 };
vec2 segments[MAX_SCORE + 1];
vec2 dir = { 1, 0 };
// berry
vec2 berry;

bool collide(vec2 a, vec2 b) {
    if (a.x == b.x && a.y == b.y) {
        return true;
    }
    return false;
}

bool collide_snake_body(vec2 point) {
    for (int i = 0; i < score; i++) {
        if (collide(point, segments[i])) {
            return true;
        }
    }
    return false;
}

vec2 spawn_berry() {
    vec2 berry = { 1 + rand() % (screen_width - 2), 1 + rand() % (screen_height - 2) };
    while (collide(head, berry) || collide_snake_body(berry)) {
        berry.x = 1 + rand() % (screen_width - 2);
        berry.y = 1 + rand() % (screen_height - 2);
    }
    return berry;
}

void draw_border(int y, int x, int width, int height) {
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + width * 2 + 1, ACS_URCORNER);
    for (int i = 1; i < width * 2 + 1; i++) {
        mvaddch(y, x + i, ACS_HLINE);
    }
    for (int i = 1; i < height + 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width * 2 + 1, ACS_VLINE);
    }
    mvaddch(y + height + 1, x, ACS_LLCORNER);
    mvaddch(y + height + 1, x + width * 2 + 1, ACS_LRCORNER);
    for (int i = 1; i < width * 2 + 1; i++) {
        mvaddch(y + height + 1, x + i, ACS_HLINE);
    }
}

void quit_game() {
    endwin();
    printf("\e[1;1H\e[2J");
    printf("\e[?25h");

    exit(0);
}

void restart_game() {
    head.x = 0;
    head.y = 0;
    dir.x = 1;
    dir.y = 0;
    score = 0;
    sprintf(score_message, "[ Score: %d ] [ High Score: %d ]", score, high_score);
    is_running = true;
}

void save_high_score(int high_score) {
    FILE *file = fopen("high_score.txt", "w");
    if (file != NULL) {
        fprintf(file, "%d", high_score);
        fclose(file);
    }
}

int load_high_score() {
    int saved_high_score = 0;
    FILE *file = fopen("high_score.txt", "r");
    if (file != NULL) {
        fscanf(file, "%d", &saved_high_score);
        fclose(file);
    }
    return saved_high_score;
}

void init() {
    srand(time(NULL));
    win = initscr();
    keypad(win, true);
    noecho();
    nodelay(win, true);
    curs_set(0);

    if (has_colors() == FALSE) {
        endwin();
        fprintf(stderr, "Your terminal does not support color\n");
        exit(1);
    }
    start_color();
    use_default_colors();
    init_pair(1, COLOR_RED, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);

    berry.x = rand() % screen_width;
    berry.y = rand() % screen_height;

    high_score = load_high_score();
    sprintf(score_message, "[ Score: %d ] [ High Score: %d ]", score, high_score);
}

void process_input() {
    int pressed = wgetch(win);
    if (pressed == KEY_LEFT && dir.x != 1) {
        dir.x = -1;
        dir.y = 0;
    }
    if (pressed == KEY_RIGHT && dir.x != -1) {
        dir.x = 1;
        dir.y = 0;
    }
    if (pressed == KEY_UP && dir.y != 1) {
        dir.x = 0;
        dir.y = -1;
    }
    if (pressed == KEY_DOWN && dir.y != -1) {
        dir.x = 0;
        dir.y = 1;
    }
    if (pressed == ' ') {
        if (!is_running) restart_game();
    }
    if (pressed == '\e') {
        is_running = false;
        quit_game();
    }
}

void game_over() {
    if (score > high_score) {
        high_score = score;
        save_high_score(high_score);
    }
    while (!is_running) {
        process_input();
        mvaddstr(screen_height / 2, screen_width - 16, "              Game Over          ");
        mvaddstr(screen_height / 2 + 1, screen_width - 16, "[SPACE] to restart, [ESC] to quit ");
        attron(COLOR_PAIR(3));
        draw_border(screen_height / 2 - 1, screen_width - 17, 17, 2);
        attroff(COLOR_PAIR(3));
        usleep(FRAME_TIME);
    }
}

void update() {
    for (int i = score; i > 0; i--) {
        segments[i] = segments[i - 1];
    }
    segments[0] = head;
    head.x += dir.x;
    head.y += dir.y;

    if (collide_snake_body(head) || head.x < 0 || head.y < 0 || head.x >= screen_width || head.y >= screen_height) {
        is_running = false;
        game_over();
    }

    if (collide(head, berry)) {
        if (score < MAX_SCORE) {
            score += 1;
            sprintf(score_message, "[ Score: %d ] [ High Score: %d ]", score, high_score);
        }
        berry = spawn_berry();
    }

    usleep(FRAME_TIME);
}

void draw() {
    erase();

    // Draw the berry
    attron(COLOR_PAIR(1));
    mvaddch(berry.y+1, berry.x * 2+1, '@');
    attroff(COLOR_PAIR(1));

    // Draw the snake
    attron(COLOR_PAIR(2));
    for (int i = 0; i < score; i++) {
        mvaddch(segments[i].y+1, segments[i].x * 2 + 1, ACS_DIAMOND);
    }
    mvaddch(head.y+1, head.x * 2+1, 'O');
    attroff(COLOR_PAIR(2));

    // Draw the border
    attron(COLOR_PAIR(3));
    draw_border(0, 0, screen_width, screen_height);
    attroff(COLOR_PAIR(3));

    // Center the score and high score message
    int msg_length = strlen(score_message);
    int center_x = (screen_width * 2 - msg_length) / 2;  // Calculate center position
    mvaddstr(0, center_x, score_message);  // Print the message at the center
}


int main(int argc, char *argv[]) {
    if (argc == 1) {}
    else if (argc == 3) {
        if (!strcmp(argv[1], "-d")) {
            if (sscanf(argv[2], "%dx%d", &screen_width, &screen_height) != 2) {
                printf("Usage: snake [options]\nOptions:\n -d [width]x[height]"
                       "\tdefine dimensions of the screen\n\nDefault dimensions are 25x20\n");
                exit(1);
            }
        }
    } else {
        printf("Usage: snake [options]\nOptions:\n -d [width]x[height]"
               "\tdefine dimensions of the screen\n\nDefault dimensions are 25x20\n");
        exit(1);
    }

    init();
    while(is_running) {
        process_input();
        if (skip == true) {
            skip = false;
            continue;
        }

        update();
        draw();
    }
    quit_game();
    return 0;
}
