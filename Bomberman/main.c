
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <windows.h>
#include <time.h>
#include <stdbool.h>

#define TILE_SIZE 32
#define MAP_W 25
#define MAP_H 13
#define SCREEN_W (MAP_W * TILE_SIZE)
#define SCREEN_H (MAP_H * TILE_SIZE)
#define BOMB_TIME 120
#define EXPLOSION_TIME 30

int map[MAP_H][MAP_W];
int explosion_map[MAP_H][MAP_W] = { 0 };
int player_x, player_y;

typedef struct {
    int x, y;
    bool active;
    int timer;
} Bomb;

Bomb bomb;

bool is_protected_zone(int x, int y) {
    return abs(x - player_x) <= 1 && abs(y - player_y) <= 1;
}

void generate_map() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1)
                map[y][x] = 1;
            else if (x % 2 == 0 && y % 2 == 0)
                map[y][x] = 1;
            else
                map[y][x] = 0;
        }
    }

    // Собираем доступные клетки для разрушимых блоков
    int positions[MAP_H * MAP_W][2];
    int count = 0;
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (map[y][x] == 0 && !is_protected_zone(x, y)) {
                positions[count][0] = x;
                positions[count][1] = y;
                count++;
            }
        }
    }

    if (count == 0) return;

    int min_blocks = count / 3;
    int max_blocks = count / 2;
    int num_blocks = min_blocks + rand() % (max_blocks - min_blocks + 1);

    for (int i = 0; i < num_blocks; i++) {
        int r = rand() % count;
        int bx = positions[r][0];
        int by = positions[r][1];
        map[by][bx] = 2; // разрушимый блок
        positions[r][0] = positions[count - 1][0];
        positions[r][1] = positions[count - 1][1];
        count--;
    }
}

void respawn_player() {
    int x, y;
    do {
        x = rand() % MAP_W;
        y = rand() % MAP_H;
    } while (map[y][x] != 0);
    player_x = x;
    player_y = y;
    generate_map();
}

void draw_map() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (map[y][x] == 1)
                al_draw_filled_rectangle(x * TILE_SIZE, y * TILE_SIZE,
                    (x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE, al_map_rgb(100, 100, 100));
            else if (map[y][x] == 2)
                al_draw_filled_rectangle(x * TILE_SIZE, y * TILE_SIZE,
                    (x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE, al_map_rgb(139, 69, 19));
        }
    }
}

void draw_player() {
    al_draw_filled_rectangle(
        player_x * TILE_SIZE + 4, player_y * TILE_SIZE + 4,
        (player_x + 1) * TILE_SIZE - 4, (player_y + 1) * TILE_SIZE - 4,
        al_map_rgb(0, 200, 255)
    );
}

void draw_bomb() {
    if (bomb.active) {
        al_draw_filled_circle(
            bomb.x * TILE_SIZE + TILE_SIZE / 2,
            bomb.y * TILE_SIZE + TILE_SIZE / 2,
            TILE_SIZE / 3,
            al_map_rgb(0, 0, 0)
        );
    }
}

void draw_explosions() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (explosion_map[y][x] > 0) {
                al_draw_filled_rectangle(
                    x * TILE_SIZE, y * TILE_SIZE,
                    (x + 1) * TILE_SIZE, (y + 1) * TILE_SIZE,
                    al_map_rgb(255, 140, 0)
                );
            }
        }
    }
}

bool is_player_hit(int x, int y) {
    return player_x == x && player_y == y;
}

void mark_explosion(int x, int y) {
    if (x >= 0 && x < MAP_W && y >= 0 && y < MAP_H && map[y][x] != 1) {
        explosion_map[y][x] = EXPLOSION_TIME;
        if (map[y][x] == 2) map[y][x] = 0;
    }
}

void explode_bomb() {
    int x = bomb.x, y = bomb.y;
    bool hit = false;

    mark_explosion(x, y);
    hit |= is_player_hit(x, y);

    if (x - 1 >= 0 && map[y][x - 1] != 1) { mark_explosion(x - 1, y); hit |= is_player_hit(x - 1, y); }
    if (x + 1 < MAP_W && map[y][x + 1] != 1) { mark_explosion(x + 1, y); hit |= is_player_hit(x + 1, y); }
    if (y - 1 >= 0 && map[y - 1][x] != 1) { mark_explosion(x, y - 1); hit |= is_player_hit(x, y - 1); }
    if (y + 1 < MAP_H && map[y + 1][x] != 1) { mark_explosion(x, y + 1); hit |= is_player_hit(x, y + 1); }

    if (hit)
        respawn_player();

    bomb.active = false;
}

void update_explosions() {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            if (explosion_map[y][x] > 0)
                explosion_map[y][x]--;
        }
    }
}

void place_bomb() {
    if (!bomb.active) {
        bomb.x = player_x;
        bomb.y = player_y;
        bomb.timer = BOMB_TIME;
        bomb.active = true;
    }
}

int main() {
    srand(time(NULL));
    al_init();
    al_init_primitives_addon();
    ALLEGRO_DISPLAY* display = al_create_display(SCREEN_W, SCREEN_H);

    generate_map();
    respawn_player();
    bomb.active = false;

    while (true) {
        if (GetAsyncKeyState(VK_UP) & 0x8000 && map[player_y - 1][player_x] == 0) {
            player_y--; Sleep(120);
        }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000 && map[player_y + 1][player_x] == 0) {
            player_y++; Sleep(120);
        }
        else if (GetAsyncKeyState(VK_LEFT) & 0x8000 && map[player_y][player_x - 1] == 0) {
            player_x--; Sleep(120);
        }
        else if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && map[player_y][player_x + 1] == 0) {
            player_x++; Sleep(120);
        }
        else if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
            place_bomb(); Sleep(120);
        }
        else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }

        if (bomb.active) {
            bomb.timer--;
            if (bomb.timer <= 0)
                explode_bomb();
        }

        update_explosions();

        al_clear_to_color(al_map_rgb(30, 30, 30));
        draw_map();
        draw_explosions();
        draw_bomb();
        draw_player();
        al_flip_display();
        al_rest(1.0 / 60.0);
    }

    al_destroy_display(display);
    return 0;
}
