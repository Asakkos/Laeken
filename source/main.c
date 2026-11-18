#include <stdint.h>
#include <stdlib.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define REG_DISPCNT   (*(volatile u16*)0x04000000)
#define REG_VCOUNT    (*(volatile u16*)0x04000006)
#define REG_KEYINPUT  (*(volatile u16*)0x04000130)
#define MEM_PAL       ((volatile u16*)0x05000000)

#define REG_SOUNDCNT_L (*(volatile u16*)0x04000080)
#define REG_SOUNDCNT_H (*(volatile u16*)0x04000082)
#define REG_SOUNDCNT_X (*(volatile u16*)0x04000084)
#define REG_SOUND1_L   (*(volatile u16*)0x04000060)
#define REG_SOUND1_H   (*(volatile u16*)0x04000062)
#define REG_SOUND1_X   (*(volatile u16*)0x04000064)

#define KEY_A      (1<<0)
#define KEY_B      (1<<1)
#define KEY_SELECT (1<<2)
#define KEY_START  (1<<3)
#define KEY_RIGHT  (1<<4)
#define KEY_LEFT   (1<<5)
#define KEY_UP     (1<<6)
#define KEY_DOWN   (1<<7)
#define KEY_R      (1<<8)
#define KEY_L      (1<<9)

#define RGB15(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))

#define SW 240
#define SH 160
#define GROUND_Y 138
#define LEVEL_W 1280
#define CAGE_X 1160

// Palette indices
enum {
    C_BLACK = 0, C_OUTLINE, C_SKY1, C_SKY2, C_SKY3, C_BLDG, C_WIN_B, C_WIN_Y,
    C_TEAL1, C_TEAL2, C_ASPHALT, C_CURB, C_SHOP1, C_SHOP2, C_NEON_P, C_NEON_C,
    C_WHITE, C_SKIN, C_SKIN_D, C_HAIR, C_ANORAK, C_ANORAK_D, C_JEANS, C_NAVY,
    C_NAVY_L, C_RED, C_YELLOW, C_SWORD, C_DOUDOU_G, C_DOUDOU_C, C_DOUDOU_T,
    C_HOODIE, C_STRIPES, C_BEARD, C_SHIRT_B, C_POLO, C_IRON, C_GREEN, C_ORANGE
};

static volatile u16 *vid_page = (volatile u16*)0x0600A000;
static u16 key_cur = 0, key_prev = 0;
static u32 frame = 0;

static void init_palette(void) {
    MEM_PAL[C_BLACK]    = RGB15(1, 1, 3);
    MEM_PAL[C_OUTLINE]  = RGB15(2, 1, 4);
    MEM_PAL[C_SKY1]     = RGB15(1, 2, 7);
    MEM_PAL[C_SKY2]     = RGB15(3, 4, 12);
    MEM_PAL[C_SKY3]     = RGB15(6, 7, 17);
    MEM_PAL[C_BLDG]     = RGB15(3, 5, 11);
    MEM_PAL[C_WIN_B]    = RGB15(10, 24, 31);
    MEM_PAL[C_WIN_Y]    = RGB15(31, 26, 10);
    MEM_PAL[C_TEAL1]    = RGB15(4, 15, 16);
    MEM_PAL[C_TEAL2]    = RGB15(6, 20, 21);
    MEM_PAL[C_ASPHALT]  = RGB15(3, 4, 8);
    MEM_PAL[C_CURB]     = RGB15(8, 22, 23);
    MEM_PAL[C_SHOP1]    = RGB15(5, 12, 11);
    MEM_PAL[C_SHOP2]    = RGB15(10, 6, 11);
    MEM_PAL[C_NEON_P]   = RGB15(31, 9, 17);
    MEM_PAL[C_NEON_C]   = RGB15(9, 29, 31);
    MEM_PAL[C_WHITE]    = RGB15(31, 31, 31);
    MEM_PAL[C_SKIN]     = RGB15(29, 21, 16);
    MEM_PAL[C_SKIN_D]   = RGB15(23, 15, 11);
    MEM_PAL[C_HAIR]     = RGB15(4, 3, 3);
    MEM_PAL[C_ANORAK]   = RGB15(9, 10, 12);
    MEM_PAL[C_ANORAK_D] = RGB15(5, 6, 8);
    MEM_PAL[C_JEANS]    = RGB15(12, 14, 18);
    MEM_PAL[C_NAVY]     = RGB15(4, 7, 15);
    MEM_PAL[C_NAVY_L]   = RGB15(7, 12, 24);
    MEM_PAL[C_RED]      = RGB15(29, 5, 6);
    MEM_PAL[C_YELLOW]   = RGB15(31, 27, 6);
    MEM_PAL[C_SWORD]    = RGB15(20, 28, 31);
    MEM_PAL[C_DOUDOU_G] = RGB15(20, 21, 20);
    MEM_PAL[C_DOUDOU_C] = RGB15(30, 29, 26);
    MEM_PAL[C_DOUDOU_T] = RGB15(8, 25, 23);
    MEM_PAL[C_HOODIE]   = RGB15(17, 18, 21);
    MEM_PAL[C_STRIPES]  = RGB15(11, 16, 26);
    MEM_PAL[C_BEARD]    = RGB15(3, 3, 3);
    MEM_PAL[C_SHIRT_B]  = RGB15(12, 18, 28);
    MEM_PAL[C_POLO]     = RGB15(29, 28, 25);
    MEM_PAL[C_IRON]     = RGB15(15, 17, 22);
    MEM_PAL[C_GREEN]    = RGB15(10, 30, 14);
    MEM_PAL[C_ORANGE]   = RGB15(31, 17, 4);
}

static inline void m4_plot(int x, int y, u8 c) {
    if ((unsigned)x >= SW || (unsigned)y >= SH) return;
    volatile u16 *dst = &vid_page[(y * SW + x) >> 1];
    if (x & 1) *dst = (*dst & 0x00FF) | (c << 8);
    else       *dst = (*dst & 0xFF00) | c;
}

static void m4_rect(int x, int y, int w, int h, u8 c) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SW) w = SW - x;
    if (y + h > SH) h = SH - y;
    if (w <= 0 || h <= 0) return;

    for (int row = y; row < y + h; row++) {
        int cx = x, cw = w;
        if (cx & 1) { m4_plot(cx, row, c); cx++; cw--; }
        if (cw & 1) { m4_plot(cx + cw - 1, row, c); cw--; }
        if (cw > 0) {
            volatile u16 *dst = &vid_page[(row * SW + cx) >> 1];
            u16 pair = (u16)(c | (c << 8));
            int pairs = cw >> 1;
            while (pairs--) *dst++ = pair;
        }
    }
}

static void vsync_flip(void) {
    while (REG_VCOUNT >= 160);
    while (REG_VCOUNT < 160);
    REG_DISPCNT ^= 0x0010;
    vid_page = (volatile u16*)((uintptr_t)vid_page ^ 0xA000);
}

static void sfx(int freq, int len, int dir) {
    REG_SOUNDCNT_X = 0x0080;
    REG_SOUNDCNT_L = 0xFF77;
    REG_SOUNDCNT_H = 0x0002;
    REG_SOUND1_L   = (dir ? 0x001C : 0x0014);
    REG_SOUND1_H   = 0xF100 | (len & 0x3F);
    REG_SOUND1_X   = 0x8000 | (freq & 0x7FF);
}

// Compact 3x5 pixel font for crisp GBA text
static const u16 FONT3X5[] = {
    0x7B6F,0x2492,0x73E7,0x73CF,0x5BC9,0x79CF,0x79EF,0x7249,0x7BEF,0x7BCF, // 0-9
    0x2BED,0x7AEF,0x7927,0x6B6E,0x79E7,0x79E4,0x796F,0x5BED,0x7497,0x12DF, // A-J
    0x5AED,0x4927,0x5F6D,0x5F7D,0x7B6F,0x7BE4,0x7B6B,0x7AED,0x79CF,0x7492, // K-T
    0x5B6F,0x5B6A,0x5B7D,0x5AAD,0x5ACA,0x72A7,                              // U-Z
    0x0000,0x2482,0x0020,0x01C0,0x0080,0x5B7F                              // sp, !, ., -, :, heart
};
static int font_idx(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'Z') return 10 + (ch - 'A');
    if (ch >= 'a' && ch <= 'z') return 10 + (ch - 'a');
    if (ch == '!') return 37; if (ch == '.') return 38;
    if (ch == '-') return 39; if (ch == ':') return 40;
    if (ch == '*') return 41;
    return 36;
}
static void draw_text(int x, int y, const char *s, u8 col) {
    for (int i = 0; s[i]; i++) {
        u16 g = FONT3X5[font_idx(s[i])];
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++)
                if (g & (1 << (14 - (r * 3 + c))))
                    m4_plot(x + i * 4 + c, y + r, col);
    }
}

// ================== GAME ENTITIES ==================
enum { ST_SELECT, ST_PLAY, ST_PAUSE, ST_RESCUE, ST_OVER };
static int state = ST_SELECT, sel_hero = 0, score = 0, cam_x = 0, hit_stop = 0, shake = 0;
static int rescue_t = 0, cage_open = 0, hugging = 0;
static int yousra_x, abdel_x, sib_x;

typedef struct {
    int id, x, y, vx, vy, w, h, dir, ground, jumps, hp, max_hp, inv, atk_t, atk_cd, spc_t, spc_cd;
} Player;
static Player pl;

typedef struct { int x, y, vx, active, timer; } Boomerang;
static Boomerang doudou_proj;

typedef struct { int x, y, w, h, kind, drop, broken; } Prop;
static Prop props[6];

typedef struct { int x, y, type, got; } Pickup;
static Pickup pickups[12];
static int num_pickups = 0;

typedef struct { int x, y, w, h, kind, minx, maxx, hp, max_hp, dir, flash, stun, act; } Enemy;
static Enemy enemies[10];
static int last_enemy_idx = -1, last_enemy_t = 0;

typedef struct { int x, y, w, h; } Plat;
static const Plat plats[] = {
    {120,104,52,5},{220,82,56,5},{340,104,56,5},{460,80,52,5},
    {600,102,64,5},{730,82,66,5},{860,102,64,5},{990,100,50,5}
};

static int overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void start_game(int hero) {
    sel_hero = hero;
    pl.id = hero;
    pl.w = 12; pl.h = hero == 0 ? 26 : 21;
    pl.x = 30; pl.y = GROUND_Y - pl.h;
    pl.vx = 0; pl.vy = 0; pl.dir = 1; pl.ground = 1; pl.jumps = 2;
    pl.max_hp = 6; pl.hp = 6; pl.inv = 0;
    pl.atk_t = 0; pl.atk_cd = 0; pl.spc_t = 0; pl.spc_cd = 0;
    doudou_proj.active = 0;
    score = 0; cam_x = 0; hit_stop = 0; shake = 0;
    last_enemy_idx = -1; last_enemy_t = 0;
    cage_open = 0; hugging = 0; rescue_t = 0;
    yousra_x = CAGE_X + 12; abdel_x = CAGE_X + 34;

    int px[6] = {150, 360, 540, 750, 880, 1020};
    int py[6] = {GROUND_Y-14, 104-14, GROUND_Y-14, 82-14, GROUND_Y-14, GROUND_Y-14};
    int pd[6] = {1, 0, 2, 0, 1, 2};
    for (int i = 0; i < 6; i++) {
        props[i] = (Prop){px[i], py[i], 13, 14, i & 1, pd[i], 0};
    }

    pickups[0] = (Pickup){145, 92, 0, 0};
    pickups[1] = (Pickup){245, 70, 1, 0};
    pickups[2] = (Pickup){485, 68, 2, 0};
    pickups[3] = (Pickup){630, 90, 0, 0};
    pickups[4] = (Pickup){1010, 88, 1, 0};
    num_pickups = 5;

    enemies[0] = (Enemy){190, GROUND_Y-30, 13, 30, 0, 140, 280, 3, 3, -1, 0, 0, 0};
    enemies[1] = (Enemy){235, 82-30,       13, 30, 1, 220, 270, 3, 3, -1, 0, 0, 0};
    enemies[2] = (Enemy){370, GROUND_Y-30, 13, 30, 1, 310, 460, 4, 4, -1, 0, 0, 0};
    enemies[3] = (Enemy){510, GROUND_Y-30, 13, 30, 0, 470, 620, 3, 3, -1, 0, 0, 0};
    enemies[4] = (Enemy){615, 102-30,      13, 30, 1, 600, 655, 3, 3, -1, 0, 0, 0};
    enemies[5] = (Enemy){750, GROUND_Y-30, 13, 30, 0, 670, 840, 4, 4, -1, 0, 0, 0};
    enemies[6] = (Enemy){875, 102-30,      13, 30, 1, 860, 915, 3, 3, -1, 0, 0, 0};
    enemies[7] = (Enemy){950, GROUND_Y-30, 13, 30, 1, 880, 1030,4, 4, -1, 0, 0, 0};
    enemies[8] = (Enemy){1100,GROUND_Y-34, 16, 34, 2, 1025,1145,14,14,-1, 0, 0, 60};
    state = ST_PLAY;
}

// ================== SPRITE RENDERING (NON-CHIBI GBA SCALE) ==================
static void draw_doudou_gba(int x, int y) {
    m4_rect(x-3, y-4, 6, 8, C_OUTLINE);
    m4_rect(x-2, y-3, 4, 3, C_DOUDOU_C);
    m4_plot(x-1, y-4, C_YELLOW); m4_plot(x, y-5, C_DOUDOU_T);
    m4_plot(x, y-2, C_DOUDOU_T); // teal nose
    m4_rect(x-2, y, 4, 3, C_DOUDOU_C);
}

static void draw_liam_gba(int x, int y, int dir, int atk, int flash) {
    if (flash && (frame & 2)) return;
    int cx = x + 6, by = y + 26;
    u8 anorak = C_ANORAK, jeans = C_JEANS, skin = C_SKIN;
    m4_rect(cx-5, by-26, 10, 26, C_OUTLINE);
    // Realistic non-chibi legs
    m4_rect(cx-4, by-11, 3, 9, jeans);
    m4_rect(cx+1, by-11, 3, 9, jeans);
    m4_rect(cx-4, by-2, 4, 2, C_BLACK);
    m4_rect(cx+1, by-2, 4, 2, C_BLACK);
    // Dark grey anorak
    m4_rect(cx-4, by-19, 8, 8, anorak);
    m4_rect(cx-3, by-15, 6, 3, C_ANORAK_D);
    // Proportional head
    m4_rect(cx-2, by-24, 5, 5, skin);
    m4_rect(cx-3, by-26, 6, 2, C_HAIR);
    m4_plot(cx + (dir > 0 ? 1 : -1), by-22, C_BLACK);
    // Silver Sword
    int sx = dir > 0 ? cx + 4 : cx - (atk ? 16 : 11);
    int sy = atk ? by - 16 : by - 18;
    m4_rect(sx, sy, atk ? 13 : 8, 2, C_SWORD);
    if (atk) m4_rect(sx, sy-1, atk ? 11 : 6, 1, C_WHITE);
}

static void draw_mael_gba(int x, int y, int dir, int atk, int flash) {
    if (flash && (frame & 2)) return;
    int cx = x + 6, by = y + 21;
    m4_rect(cx-4, by-21, 9, 21, C_OUTLINE);
    m4_rect(cx-3, by-8, 3, 6, C_ANORAK_D);
    m4_rect(cx+1, by-8, 3, 6, C_ANORAK_D);
    m4_rect(cx-3, by-2, 3, 2, C_NAVY_L);
    m4_rect(cx+1, by-2, 3, 2, C_NAVY_L);
    // Navy Marvel sweater
    m4_rect(cx-4, by-15, 8, 7, C_NAVY);
    m4_plot(cx-2, by-14, C_RED); m4_plot(cx+1, by-14, C_WHITE);
    // Proportional kid head
    m4_rect(cx-2, by-20, 5, 5, C_SKIN);
    m4_rect(cx-3, by-21, 6, 2, C_HAIR);
    m4_plot(cx + (dir > 0 ? 1 : -1), by-18, C_BLACK);
    if (!doudou_proj.active) {
        int dx = dir > 0 ? cx + (atk ? 9 : 6) : cx - (atk ? 9 : 6);
        draw_doudou_gba(dx, by - 13);
    }
}

static void draw_youth_gba(const Enemy *e) {
    int cx = e->x + e->w / 2, by = e->y + e->h;
    u8 top = e->flash ? C_WHITE : (e->kind == 2 ? C_NAVY : (e->kind == 1 ? C_ANORAK : C_NAVY_L));
    u8 leg = e->flash ? C_WHITE : (e->kind == 0 ? C_STRIPES : C_ANORAK_D);
    m4_rect(cx-6, by-e->h, 12, e->h, C_OUTLINE);
    m4_rect(cx-4, by-13, 3, 11, leg);
    m4_rect(cx+1, by-13, 3, 11, leg);
    m4_rect(cx-4, by-2, 4, 2, C_WHITE);
    m4_rect(cx+1, by-2, 4, 2, C_WHITE);
    // Torso (Puffer Vest or Tracksuit or Chef Parka)
    m4_rect(cx-5, by-24, 10, 11, top);
    if (!e->flash && e->kind == 1) {
        m4_rect(cx-5, by-23, 2, 7, C_HOODIE);
        m4_rect(cx+3, by-23, 2, 7, C_HOODIE);
    }
    if (!e->flash && e->kind == 0) {
        m4_rect(cx-5, by-24, 10, 2, C_WHITE);
    }
    // Head
    m4_rect(cx-3, by-30, 6, 6, e->flash ? C_WHITE : C_SKIN_D);
    if (!e->flash) {
        m4_rect(cx-3, by-e->h, 6, 2, e->kind == 2 ? C_NAVY : C_HAIR);
        if (e->kind == 2) m4_rect(cx-3, by-27, 6, 3, C_BEARD);
    }
}

static void draw_yousra_gba(int x, int hug) {
    int by = GROUND_Y;
    m4_rect(x-5, by-31, 10, 31, C_OUTLINE);
    m4_rect(x-4, by-30, 8, 12, C_HAIR); // long curly dark hair
    m4_rect(x-3, by-14, 3, 12, C_JEANS);
    m4_rect(x+1, by-14, 3, 12, C_JEANS);
    m4_rect(x-4, by-2, 4, 2, C_WHITE); m4_rect(x+1, by-2, 4, 2, C_WHITE);
    // Blue-and-white striped shirt
    m4_rect(x-4, by-24, 8, 10, C_WHITE);
    m4_rect(x-3, by-24, 1, 10, C_SHIRT_B);
    m4_rect(x,   by-24, 1, 10, C_SHIRT_B);
    m4_rect(x+3, by-24, 1, 10, C_SHIRT_B);
    m4_rect(x-2, by-30, 5, 6, C_SKIN);
    m4_rect(x-3, by-31, 6, 2, C_HAIR);
    if (hug) m4_rect(x-7, by-22, 5, 2, C_SHIRT_B);
}

static void draw_abdel_gba(int x, int hug) {
    int by = GROUND_Y;
    m4_rect(x-5, by-33, 10, 33, C_OUTLINE);
    m4_rect(x-4, by-15, 3, 13, C_JEANS);
    m4_rect(x+1, by-15, 3, 13, C_JEANS);
    m4_rect(x-4, by-2, 4, 2, C_BLACK); m4_rect(x+1, by-2, 4, 2, C_BLACK);
    // Cream knit polo
    m4_rect(x-4, by-26, 8, 11, C_POLO);
    // Head + beard + round glasses
    m4_rect(x-2, by-32, 5, 6, C_SKIN_D);
    m4_rect(x-3, by-33, 6, 2, C_HAIR);
    m4_rect(x-2, by-29, 5, 3, C_BEARD);
    m4_plot(x-1, by-30, C_NEON_C); m4_plot(x+1, by-30, C_NEON_C);
    if (hug) m4_rect(x+3, by-24, 5, 2, C_POLO);
}

// ================== STAGE & RENDER ==================
static void draw_stage(void) {
    // Night sky bands
    m4_rect(0, 0, SW, 45, C_SKY1);
    m4_rect(0, 45, SW, 40, C_SKY2);
    m4_rect(0, 85, SW, 34, C_SKY3);

    // Distant skyline + Atomium
    for (int i = 0; i < 12; i++) {
        int bx = (i * 28 - (cam_x >> 2)) % 336;
        if (bx < -24) bx += 336;
        int bh = 40 + ((i * 19) % 34);
        m4_rect(bx, GROUND_Y - 22 - bh, 22, bh, C_BLDG);
        for (int wy = GROUND_Y - 18 - bh; wy < GROUND_Y - 28; wy += 6)
            m4_plot(bx + 5, wy, (i & 1) ? C_WIN_B : C_WIN_Y);
    }
    int ax = 185 - (cam_x >> 3);
    m4_rect(ax-1, 40, 2, 44, C_IRON);
    m4_rect(ax-10, 46, 20, 2, C_IRON);
    m4_rect(ax-2, 34, 4, 4, C_NEON_C);
    m4_rect(ax-12, 45, 4, 4, C_NEON_C);
    m4_rect(ax+8, 45, 4, 4, C_NEON_C);

    // Streets of Rage Shopfronts (x=20..700) & Bridge Railing (x=720..1040)
    for (int s = 0; s < 5; s++) {
        int sx = 20 + s * 140 - cam_x;
        if (sx + 110 < 0 || sx > SW) continue;
        m4_rect(sx, 50, 108, 69, (s & 1) ? C_SHOP2 : C_SHOP1);
        m4_rect(sx + 8, 56, 92, 14, C_BLACK);
        m4_rect(sx + 8, 56, 92, 2, (s & 1) ? C_NEON_C : C_NEON_P);
        draw_text(sx + 18, 61, (s & 1) ? "PINE POT" : "LAEKEN", (s & 1) ? C_NEON_C : C_NEON_P);
        m4_rect(sx + 12, 76, 84, 38, C_BLDG);
        m4_rect(sx + 16, 80, 76, 30, C_SKIN_D);
    }
    int brx = 720 - cam_x;
    if (brx < SW && brx + 320 > 0) {
        m4_rect(brx, 102, 320, 3, C_IRON);
        for (int p = brx; p < brx + 320; p += 12) m4_rect(p, 104, 2, 15, C_IRON);
    }

    // Turquoise Checkered Streets of Rage Sidewalk + Hydrants
    m4_rect(0, GROUND_Y - 19, SW, 19, C_TEAL1);
    for (int r = 0; r < 3; r++) {
        for (int c = (cam_x / 10) - 1; c < (cam_x / 10) + 26; c++) {
            if ((c + r) & 1) m4_rect(c * 10 - cam_x, GROUND_Y - 19 + r * 6, 9, 5, C_TEAL2);
        }
    }
    m4_rect(0, GROUND_Y - 2, SW, 2, C_CURB);
    m4_rect(0, GROUND_Y, SW, SH - GROUND_Y, C_ASPHALT);

    for (int lx = 60; lx < LEVEL_W - 120; lx += 130) {
        int sx = lx - cam_x;
        if (sx < -20 || sx > SW + 20) continue;
        m4_rect(sx, 58, 2, 62, C_IRON);
        m4_rect(sx, 56, 10, 2, C_IRON);
        m4_rect(sx + 6, 58, 5, 3, C_NEON_C);
        m4_rect(sx + 28, GROUND_Y - 12, 5, 10, C_RED);
        m4_rect(sx + 27, GROUND_Y - 10, 7, 2, C_ORANGE);
    }

    // Platforms
    for (int i = 0; i < 8; i++) {
        int px = plats[i].x - cam_x;
        if (px + plats[i].w < 0 || px > SW) continue;
        m4_rect(px, plats[i].y, plats[i].w, plats[i].h, (i & 1) ? C_IRON : C_RED);
        m4_rect(px, plats[i].y, plats[i].w, 1, C_WHITE);
    }

    // Smashable Crates / Trash cans
    for (int i = 0; i < 6; i++) {
        if (props[i].broken) continue;
        int px = props[i].x - cam_x;
        if (px + 14 < 0 || px > SW) continue;
        m4_rect(px, props[i].y, props[i].w, props[i].h, props[i].kind ? C_IRON : C_ORANGE);
        m4_rect(px + 2, props[i].y + 2, props[i].w - 4, 2, C_WHITE);
    }

    // Parents & Cage
    int cx = CAGE_X - cam_x;
    if (cx < SW + 20 && cx + 60 > -20) {
        draw_yousra_gba(yousra_x - cam_x, hugging);
        draw_abdel_gba(abdel_x - cam_x, hugging);
        if (!cage_open) {
            m4_rect(cx, GROUND_Y - 46, 56, 3, C_IRON);
            for (int b = cx; b <= cx + 56; b += 8) m4_rect(b, GROUND_Y - 46, 2, 46, C_IRON);
            if ((frame >> 4) & 1) draw_text(cx + 4, GROUND_Y - 54, "AIDEZ-NOUS!", C_YELLOW);
        }
    }

    // Pickups
    for (int i = 0; i < num_pickups; i++) {
        if (pickups[i].got) continue;
        int ix = pickups[i].x - cam_x;
        if (ix < -10 || ix > SW + 10) continue;
        u8 col = pickups[i].type == 0 ? C_YELLOW : (pickups[i].type == 1 ? C_RED : C_ORANGE);
        m4_rect(ix - 3, pickups[i].y - 3, 6, 6, col);
        m4_plot(ix - 1, pickups[i].y - 1, C_WHITE);
    }

    // Enemies
    for (int i = 0; i < 9; i++) {
        if (enemies[i].hp <= 0) continue;
        int ex = enemies[i].x - cam_x;
        if (ex < -24 || ex > SW + 24) continue;
        Enemy tmp = enemies[i]; tmp.x = ex;
        draw_youth_gba(&tmp);
    }

    // Doudou boomerang
    if (doudou_proj.active) draw_doudou_gba(doudou_proj.x - cam_x, doudou_proj.y);

    // Sibling + Player
    if (state == ST_RESCUE) {
        if (pl.id == 0) draw_mael_gba(sib_x - cam_x, GROUND_Y - 21, 1, 0, 0);
        else            draw_liam_gba(sib_x - cam_x, GROUND_Y - 26, 1, 0, 0);
    }
    if (pl.id == 0) draw_liam_gba(pl.x - cam_x, pl.y, pl.dir, pl.atk_t > 0, pl.inv > 0);
    else            draw_mael_gba(pl.x - cam_x, pl.y, pl.dir, pl.atk_t > 0, pl.inv > 0);

    // Floating hearts on hug
    if (hugging) {
        for (int h = 0; h < 4; h++) {
            int hy = GROUND_Y - 40 - ((frame + h * 11) % 30);
            draw_text(CAGE_X - 42 - cam_x + h * 12, hy, "*", C_NEON_P);
        }
    }
}

static void draw_hud(void) {
    draw_text(6, 6, pl.id == 0 ? "1P LIAM" : "1P MAEL", C_YELLOW);
    for (int i = 0; i < pl.max_hp; i++) {
        m4_rect(6 + i * 8, 13, 6, 4, i < pl.hp ? C_YELLOW : C_RED);
    }
    draw_text(SW / 2 - 24, 6, "LAEKEN", C_NEON_P);
    if (last_enemy_idx >= 0 && last_enemy_t > 0 && enemies[last_enemy_idx].hp > 0) {
        Enemy *e = &enemies[last_enemy_idx];
        draw_text(SW - 60, 6, e->kind == 2 ? "CHEF BOSS" : "QUARTIER", C_RED);
        int w = (48 * e->hp) / e->max_hp;
        m4_rect(SW - 56, 13, 48, 4, C_OUTLINE);
        m4_rect(SW - 56, 13, w, 4, C_ORANGE);
    }
}

// ================== UPDATE ==================
static void hit_enemy(int idx, int dmg, int dir) {
    Enemy *e = &enemies[idx];
    e->hp -= dmg; e->flash = 6; e->stun = 10;
    e->x += dir * (e->kind == 2 ? 3 : 6);
    hit_stop = 2; shake = 2;
    last_enemy_idx = idx; last_enemy_t = 120;
    sfx(600, 6, 0);
    if (e->hp <= 0) {
        score += e->kind == 2 ? 1000 : 100;
        if (e->kind == 2) {
            for (int i = 0; i < 8; i++) enemies[i].hp = 0;
            state = ST_RESCUE;
            rescue_t = 0;
            sib_x = pl.x - 45;
            doudou_proj.active = 0;
        }
    }
}

static void update_play(u16 just) {
    if (just & KEY_START) { state = ST_PAUSE; return; }
    if (hit_stop > 0) { hit_stop--; return; }

    int spd = pl.id == 0 ? 2 : 2;
    if (pl.spc_t <= 0) {
        if (key_cur & KEY_LEFT)       { pl.vx = -spd; pl.dir = -1; }
        else if (key_cur & KEY_RIGHT) { pl.vx = spd;  pl.dir = 1; }
        else pl.vx = 0;
    }
    if ((just & KEY_A) && pl.jumps > 0) {
        pl.vy = -5; pl.ground = 0; pl.jumps--; sfx(1100, 5, 1);
    }
    if (pl.atk_cd > 0) pl.atk_cd--;
    if (pl.atk_t > 0) pl.atk_t--;
    if ((just & KEY_B) && pl.atk_cd == 0) {
        pl.atk_t = 10; pl.atk_cd = 14; sfx(900, 4, 0);
    }
    if (pl.spc_cd > 0) pl.spc_cd--;
    if (pl.spc_t > 0) pl.spc_t--;
    if ((just & KEY_R) && pl.spc_cd == 0) {
        if (pl.id == 0) { pl.spc_t = 10; pl.spc_cd = 40; pl.vx = pl.dir * 4; sfx(1200, 6, 0); }
        else if (!doudou_proj.active) {
            pl.spc_cd = 36;
            doudou_proj = (Boomerang){pl.x + 6, pl.y + 8, pl.dir * 4, 1, 0};
            sfx(1000, 5, 1);
        }
    }

    int prev_b = pl.y + pl.h;
    pl.x += pl.vx;
    if ((frame & 1) == 0 && pl.vy < 4) pl.vy++;
    pl.y += pl.vy;
    if (pl.x < 4) pl.x = 4;
    if (pl.x > CAGE_X - pl.w - 4) pl.x = CAGE_X - pl.w - 4;
    pl.ground = 0;
    if (pl.y + pl.h >= GROUND_Y) { pl.y = GROUND_Y - pl.h; pl.vy = 0; pl.ground = 1; pl.jumps = 2; }
    if (pl.vy >= 0) {
        for (int i = 0; i < 8; i++) {
            if (pl.x + pl.w > plats[i].x && pl.x < plats[i].x + plats[i].w &&
                prev_b <= plats[i].y + 2 && pl.y + pl.h >= plats[i].y) {
                pl.y = plats[i].y - pl.h; pl.vy = 0; pl.ground = 1; pl.jumps = 2;
            }
        }
    }
    if (pl.inv > 0) pl.inv--;

    if (pl.atk_t == 8 || (pl.spc_t > 0 && (pl.spc_t & 3) == 0)) {
        int reach = pl.id == 0 ? 18 : 15;
        int ax = pl.dir > 0 ? pl.x + pl.w - 2 : pl.x - reach + 2;
        for (int i = 0; i < 9; i++) {
            if (enemies[i].hp > 0 && overlap(ax, pl.y, reach, pl.h, enemies[i].x, enemies[i].y, enemies[i].w, enemies[i].h)) {
                hit_enemy(i, pl.spc_t > 0 ? 2 : 1, pl.dir);
            }
        }
        for (int i = 0; i < 6; i++) {
            if (!props[i].broken && overlap(ax, pl.y, reach, pl.h, props[i].x, props[i].y, props[i].w, props[i].h)) {
                props[i].broken = 1;
                pickups[num_pickups++] = (Pickup){props[i].x + 5, props[i].y + 6, props[i].drop, 0};
                sfx(500, 6, 0);
            }
        }
    }

    if (doudou_proj.active) {
        doudou_proj.timer++;
        if (doudou_proj.timer < 18) doudou_proj.x += doudou_proj.vx;
        else {
            int dx = (pl.x + 6) - doudou_proj.x;
            doudou_proj.x += (dx > 0 ? 4 : -4);
            if (abs(dx) < 6 || doudou_proj.timer > 50) doudou_proj.active = 0;
        }
        for (int i = 0; i < 9; i++) {
            if (enemies[i].hp > 0 && (doudou_proj.timer % 6 == 0) &&
                overlap(doudou_proj.x - 5, doudou_proj.y - 5, 10, 10, enemies[i].x, enemies[i].y, enemies[i].w, enemies[i].h)) {
                hit_enemy(i, 2, doudou_proj.vx > 0 ? 1 : -1);
            }
        }
    }

    for (int i = 0; i < 9; i++) {
        Enemy *e = &enemies[i];
        if (e->hp <= 0) continue;
        if (e->flash > 0) e->flash--;
        if (e->stun > 0) { e->stun--; continue; }
        if ((frame & 1) == 0) {
            if (e->kind == 2) {
                e->dir = e->x > pl.x ? -1 : 1;
                e->x += e->dir;
            } else {
                e->x += e->dir;
                if (e->x <= e->minx) { e->x = e->minx; e->dir = 1; }
                if (e->x >= e->maxx) { e->x = e->maxx; e->dir = -1; }
            }
            if (e->x < e->minx) e->x = e->minx;
            if (e->x > e->maxx) e->x = e->maxx;
        }
        if (pl.inv == 0 && pl.spc_t == 0 && overlap(pl.x, pl.y, pl.w, pl.h, e->x, e->y, e->w, e->h)) {
            pl.hp--; pl.inv = 45; pl.vy = -3; sfx(400, 10, 0);
            if (pl.hp <= 0) state = ST_OVER;
        }
    }

    for (int i = 0; i < num_pickups; i++) {
        if (!pickups[i].got && overlap(pl.x, pl.y, pl.w, pl.h, pickups[i].x - 5, pickups[i].y - 5, 10, 10)) {
            pickups[i].got = 1;
            if (pickups[i].type == 0) score += 100;
            else if (pl.hp < pl.max_hp) pl.hp++;
            sfx(1500, 4, 1);
        }
    }

    int target_cam = pl.x - 80;
    if (target_cam < 0) target_cam = 0;
    if (target_cam > LEVEL_W - SW) target_cam = LEVEL_W - SW;
    cam_x += (target_cam - cam_x) >> 2;
    if (last_enemy_t > 0) last_enemy_t--;
    if (shake > 0) shake--;
}

static void update_rescue(u16 just) {
    rescue_t++;
    int tc = CAGE_X - 135;
    if (cam_x < tc) cam_x += 2;
    int meet = CAGE_X - 28;
    pl.y = GROUND_Y - pl.h; pl.dir = 1;
    if (pl.x < meet) pl.x++;
    if (sib_x < meet - 14) sib_x += 2;
    if (rescue_t == 15) { cage_open = 1; sfx(700, 8, 1); }
    if (cage_open && !hugging) {
        if (yousra_x > meet + 10) yousra_x--;
        if (abdel_x > meet - 26) abdel_x--;
        if (yousra_x <= meet + 10 && abdel_x <= meet - 26) { hugging = 1; sfx(1600, 12, 1); }
    }
    if (hugging && rescue_t > 120 && (just & (KEY_A | KEY_B | KEY_START))) state = ST_SELECT;
}

int main(void) {
    REG_DISPCNT = 0x0404; // Mode 4 + BG2
    init_palette();

    while (1) {
        frame++;
        key_prev = key_cur;
        key_cur = (~REG_KEYINPUT) & 0x03FF;
        u16 just = key_cur & ~key_prev;

        if (state == ST_SELECT) {
            if (just & (KEY_LEFT | KEY_RIGHT)) { sel_hero ^= 1; sfx(1200, 3, 1); }
            if (just & (KEY_A | KEY_B | KEY_START)) { start_game(sel_hero); }
            m4_rect(0, 0, SW, SH, C_SKY1);
            m4_rect(0, 32, SW, 128, C_SKY2);
            draw_text(44, 12, "STREETS OF LAEKEN", C_YELLOW);
            draw_text(34, 22, "SAUVEZ YOUSRA ET ABDEL !", C_NEON_C);
            // Cards
            m4_rect(26, 40, 82, 82, sel_hero == 0 ? C_NAVY : C_BLDG);
            m4_rect(132, 40, 82, 82, sel_hero == 1 ? C_NAVY : C_BLDG);
            draw_liam_gba(61, 64, 1, 0, 0);
            draw_mael_gba(167, 69, 1, 0, 0);
            draw_text(44, 98, "LIAM (9)", C_WHITE);
            draw_text(46, 108, "SWORD", C_NEON_C);
            draw_text(150, 98, "MAEL (5)", C_WHITE);
            draw_text(150, 108, "DOUDOU", C_DOUDOU_T);
            if ((frame >> 4) & 1) draw_text(54, 138, "PRESS A OR START!", C_GREEN);
        } else if (state == ST_PLAY || state == ST_PAUSE || state == ST_RESCUE || state == ST_OVER) {
            if (state == ST_PLAY) update_play(just);
            else if (state == ST_RESCUE) update_rescue(just);
            else if (state == ST_PAUSE) {
                if (just & KEY_START) state = ST_PLAY;
                if (just & KEY_SELECT) state = ST_SELECT;
            } else if (state == ST_OVER) {
                if (just & (KEY_A | KEY_B | KEY_START)) state = ST_SELECT;
            }

            draw_stage();
            if (state != ST_RESCUE) draw_hud();
            if (state == ST_PAUSE) {
                m4_rect(60, 56, 120, 44, C_BLACK);
                draw_text(104, 66, "PAUSE", C_YELLOW);
                draw_text(74, 78, "START: RESUME", C_WHITE);
                draw_text(72, 88, "SELECT: HEROES", C_NEON_C);
            } else if (state == ST_RESCUE && hugging) {
                m4_rect(14, 16, 212, 46, C_BLACK);
                draw_text(62, 22, "BRAVO LIAM & MAEL !", C_GREEN);
                draw_text(22, 32, "VOUS AVEZ SAUVE MAMAN YOUSRA", C_WHITE);
                draw_text(30, 40, "ET PAPA ABDEL A LAEKEN ! *", C_NEON_P);
                draw_text(48, 52, "PRESS START TO REPLAY", C_YELLOW);
            } else if (state == ST_OVER) {
                m4_rect(54, 58, 132, 36, C_BLACK);
                draw_text(94, 66, "K.O. !", C_RED);
                draw_text(64, 80, "PRESS START / A", C_YELLOW);
            }
        }
        vsync_flip();
    }
    return 0;
}
