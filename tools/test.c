/*
 * TREASURE HOUNDS - native test harness (no Wii toolchain needed).
 *
 *  - software-rasterizes the exact triangle list the Wii's GPU gets (textured, alpha / add /
 *    modulate-2x blending) and saves screenshots
 *  - checks every level: start, exits, items and puzzle pieces reachable, hook landings walkable
 *  - plays scripted runs through puzzles and fights, and measures CPU time per frame
 *  - writes the music and sound effects to WAV files
 *
 *  usage: test <outdir> [all|shots|maps|perf|audio|reel N|walk]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "../source/game.h"
#include "../source/game_int.h"
#include "../source/gfx.h"
#include "../source/music.h"
#include "../source/audio.h"

#define OW 1280
#define OH 720
static float fb[OH][OW][3];
static const char *outdir = ".";
static int fails;
static Game *G;

#define CHECK(c, ...) do { if (!(c)) { printf("  FAIL: " __VA_ARGS__); printf("\n"); fails++; } } while (0)

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}

/* ------------------------------------------------------------------ */
static void raster(void)
{
    for (int y = 0; y < OH; y++) for (int x = 0; x < OW; x++) fb[y][x][0] = fb[y][x][1] = fb[y][x][2] = 0;
    const float S = (float)OW / VW;
    for (int s = 0; s < gfx_nseg; s++) {
        int mode = gfx_seg[s].mode;
        for (int i = gfx_seg[s].start; i + 2 < gfx_seg[s].start + gfx_seg[s].count; i += 3) {
            const GVtx *v = &gfx_v[i];
            float x0 = v[0].x * S, y0 = v[0].y * S, x1 = v[1].x * S, y1 = v[1].y * S, x2 = v[2].x * S, y2 = v[2].y * S;
            float area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);
            if (fabsf(area) < 1e-6f) continue;
            int minx = (int)floorf(fminf(x0, fminf(x1, x2))), maxx = (int)ceilf(fmaxf(x0, fmaxf(x1, x2)));
            int miny = (int)floorf(fminf(y0, fminf(y1, y2))), maxy = (int)ceilf(fmaxf(y0, fmaxf(y1, y2)));
            if (minx < 0) minx = 0; if (miny < 0) miny = 0;
            if (maxx > OW - 1) maxx = OW - 1; if (maxy > OH - 1) maxy = OH - 1;
            float c[3][4];
            for (int k = 0; k < 3; k++) {
                uint32_t col = v[k].c;
                c[k][0] = (float)(col >> 24) / 255.0f; c[k][1] = (float)((col >> 16) & 255) / 255.0f;
                c[k][2] = (float)((col >> 8) & 255) / 255.0f; c[k][3] = (float)(col & 255) / 255.0f;
            }
            float inv = 1.0f / area;
            /* top-left rule: pixels exactly on an edge belong to one triangle only */
            float sgn = area > 0 ? 1.0f : -1.0f;
            float e0x = x2 - x1, e0y = y2 - y1, e1x = x0 - x2, e1y = y0 - y2, e2x = x1 - x0, e2y = y1 - y0;
            float b0 = ((e0y * sgn < 0) || (e0y == 0 && e0x * sgn > 0)) ? 0.0f : 1e-6f;
            float b1 = ((e1y * sgn < 0) || (e1y == 0 && e1x * sgn > 0)) ? 0.0f : 1e-6f;
            float b2 = ((e2y * sgn < 0) || (e2y == 0 && e2x * sgn > 0)) ? 0.0f : 1e-6f;
            for (int py = miny; py <= maxy; py++) {
                float fy = (float)py + 0.5f;
                for (int px = minx; px <= maxx; px++) {
                    float fx = (float)px + 0.5f;
                    float w0 = ((x1 - fx) * (y2 - fy) - (x2 - fx) * (y1 - fy)) * inv;
                    float w1 = ((x2 - fx) * (y0 - fy) - (x0 - fx) * (y2 - fy)) * inv;
                    float w2 = 1.0f - w0 - w1;
                    if (w0 < b0 || w1 < b1 || w2 < b2) continue;
                    float u = v[0].u * w0 + v[1].u * w1 + v[2].u * w2, vv = v[0].v * w0 + v[1].v * w1 + v[2].v * w2;
                    int tx = (int)(u * ATLAS_W), ty = (int)(vv * ATLAS_H);
                    if (tx < 0) tx = 0; if (ty < 0) ty = 0; if (tx >= ATLAS_W) tx = ATLAS_W - 1; if (ty >= ATLAS_H) ty = ATLAS_H - 1;
                    uint32_t t = gfx_atlas[ty * ATLAS_W + tx];
                    float tc[4] = { (float)(t >> 24) / 255.0f, (float)((t >> 16) & 255) / 255.0f, (float)((t >> 8) & 255) / 255.0f, (float)(t & 255) / 255.0f };
                    float a = (c[0][3] * w0 + c[1][3] * w1 + c[2][3] * w2) * tc[3];
                    for (int ch = 0; ch < 3; ch++) {
                        float src = (c[0][ch] * w0 + c[1][ch] * w1 + c[2][ch] * w2) * tc[ch];
                        float d = fb[py][px][ch];
                        if (mode == BL_ADD) d = d + src * a;
                        else if (mode == BL_MUL2) d = d * src * 2.0f;
                        else d = d * (1 - a) + src * a;
                        fb[py][px][ch] = d > 1 ? 1 : (d < 0 ? 0 : d);
                    }
                }
            }
        }
    }
}

static void save_ppm(const char *name)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6 %d %d 255\n", OW, OH);
    for (int y = 0; y < OH; y++)
        for (int x = 0; x < OW; x++)
            for (int c = 0; c < 3; c++) fputc((int)(fb[y][x][c] * 255.0f + 0.5f), f);
    fclose(f);
}
static void shot(const char *name)
{
    game_render(G);
    raster();
    save_ppm(name);
    printf("  shot %s  (%d verts, %d segs)\n", name, gfx_nv, gfx_nseg);
}

static Input none;
static void step(int n, const Input *in)
{
    for (int i = 0; i < n; i++) {
        Input k = in ? *in : none;
        if (i > 0) k.pressed = 0;
        game_update(G, &k, 1.0f / 60.0f);
        int p, v; float pan;
        while (game_pop_sound(G, &p, &pan, &v) >= 0) {}
    }
}
static void press(uint32_t b) { Input in = none; in.held = in.pressed = b; step(1, &in); step(1, 0); }
static void skip_dialog(void)
{
    for (int i = 0; i < 400 && G->state == ST_DIALOG; i++) { press(BTN_A); step(2, 0); }
}
static void place(float tx, float ty)
{
    Game *g = G;
    party_reset(g, tx * TS + 8, ty * TS + 12, DIR_DOWN);
    for (int i = 0; i < 60; i++) { game_update(g, &none, 1.0f / 60.0f); int p, v; float pan; while (game_pop_sound(g, &p, &pan, &v) >= 0) {} }
}
static void start_level(int lvl)
{
    game_init(G);
    G->state = ST_TITLE;
    G->chapter_pick = lvl;
    G->menu = 1;
    press(BTN_A);
    for (int i = 0; i < 8 && G->state == ST_STORY; i++) press(BTN_A);
    step(200, 0);                 /* chapter card */
    press(BTN_A);
    step(5, 0);
    skip_dialog();
    step(30, 0);
}

/* walk the leader toward a point with the stick (simple steering) */
static int walk_to(float x, float y, int max_frames)
{
    for (int f = 0; f < max_frames; f++) {
        Kid *l = &G->kid[G->order[0]];
        float dx = x - l->x, dy = y - l->y, d = sqrtf(dx * dx + dy * dy);
        if (d < 4) return 1;
        Input in = none;
        in.sx = dx / d; in.sy = dy / d;
        in.held = BTN_Y;
        step(1, &in);
        if (G->state == ST_DIALOG) skip_dialog();
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* level checks                                                        */
/* ------------------------------------------------------------------ */
static int reach[MAXH][MAXW];
static void flood(Game *g, int sx, int sy)
{
    memset(reach, 0, sizeof reach);
    static int qx[MAXW * MAXH], qy[MAXW * MAXH];
    int h = 0, t = 0;
    qx[t] = sx; qy[t] = sy; t++; reach[sy][sx] = 1;
    while (h < t) {
        int x = qx[h], y = qy[h]; h++;
        for (int k = 0; k < 4; k++) {
            int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
            if (nx < 0 || ny < 0 || nx >= g->lv.w || ny >= g->lv.h || reach[ny][nx]) continue;
            int kk = g->lv.k[ny][nx];
            if (KINFO[kk].solid && kk != K_VOID) continue;
            if (kk == K_VOID) continue;
            reach[ny][nx] = 1;
            qx[t] = nx; qy[t] = ny; t++;
        }
    }
}

static void check_level(int lvl)
{
    Game *g = G;
    world_load(g, lvl);
    const LevelData *ld = &LEVELS[lvl];
    printf("level %d: %s (%dx%d)\n", lvl + 1, ld->name, ld->w, ld->h);
    for (int y = 0; y < ld->h; y++) CHECK((int)strlen(ld->rows[y]) == ld->w, "row %d length", y);
    /* hooks: the swing joins two areas; treat every landing as reachable from its hook */
    const Kid *l = &g->kid[g->order[0]];
    int sx = (int)(l->x / TS), sy = (int)((l->y - 2) / TS);
    flood(g, sx, sy);
    for (int pass = 0; pass < 4; pass++) {
        for (int i = 0; i < ld->nhook; i++) {
            const MapHook *h = &ld->hook[i];
            CHECK(walkable_tile(g, h->lx, h->ly), "hook %d landing (%d,%d) not walkable", i, h->lx, h->ly);
            int near = 0;
            for (int dy = -2; dy <= 2; dy++) for (int dx = -2; dx <= 2; dx++) {
                int x = h->x + dx, y = h->y + dy;
                if (x >= 0 && y >= 0 && x < g->lv.w && y < g->lv.h && reach[y][x]) near = 1;
            }
            if (near && !reach[h->ly][h->lx]) {
                int save[MAXH][MAXW];
                memcpy(save, reach, sizeof save);
                flood(g, h->lx, h->ly);
                for (int y = 0; y < MAXH; y++) for (int x = 0; x < MAXW; x++) reach[y][x] |= save[y][x];
            }
        }
    }
    int counts[E_COUNT] = { 0 }, unreach[E_COUNT] = { 0 };
    for (int i = 0; i < g->nent; i++) {
        Ent *e = &g->ent[i];
        if (!e->on) continue;
        counts[e->type]++;
        int important = e->type == E_GEAR || e->type == E_DOUBLOON || e->type == E_SKULLKEY || e->type == E_BRAZIER ||
                        e->type == E_PIPE || e->type == E_CRANE || e->type == E_BIGCHEST || e->type == E_CHEST || e->type == E_BLOCK ||
                        e->type == E_HOOK || e->type == E_SIGN || e->type == E_CARVING || e->type == E_FOOD || e->type == E_COIN;
        if (!important) continue;
        int tx = (int)(e->x / TS), ty = (int)((e->y - 2) / TS), ok = 0;
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
            int x = tx + dx, y = ty + dy;
            if (x >= 0 && y >= 0 && x < g->lv.w && y < g->lv.h && reach[y][x]) ok = 1;
        }
        if (!ok) { unreach[e->type]++; printf("    unreachable: type %d at tile (%d,%d)\n", e->type, tx, ty); }
    }
    for (int t = 0; t < E_COUNT; t++)
        if (unreach[t] && (t == E_GEAR || t == E_SKULLKEY || t == E_BRAZIER || t == E_PIPE || t == E_CRANE || t == E_BIGCHEST || t == E_DOUBLOON || t == E_BLOCK))
            CHECK(0, "level %d: %d of type %d unreachable", lvl + 1, unreach[t], t);
    /* exits */
    int exits = 0;
    for (int i = 0; i < ld->ntrig; i++) {
        const MapTrig *t = &ld->trig[i];
        if (!(t->flags & TRIG_EXIT)) continue;
        exits++;
        int ok = 0;
        for (int y = t->y0; y <= t->y1; y++) for (int x = t->x0; x <= t->x1; x++) if (reach[y][x]) ok = 1;
        CHECK(ok, "level %d exit not reachable", lvl + 1);
    }
    CHECK(exits == 1 || lvl == 3, "level %d has %d exits", lvl + 1, exits);
    printf("  gears %d, doubloons %d, coins %d, chests %d, enemies: bat %d crab %d rat %d skel %d vic %d lenny %d\n",
           counts[E_GEAR], counts[E_DOUBLOON], counts[E_COIN], counts[E_CHEST], counts[E_BAT], counts[E_CRAB], counts[E_RAT],
           counts[E_SKEL], counts[E_VIC], counts[E_LENNY]);
    if (lvl == 0) CHECK(counts[E_GEAR] == 3, "level 1 needs 3 gears");
    CHECK(counts[E_DOUBLOON] == 3, "level %d should have 3 doubloons (has %d)", lvl + 1, counts[E_DOUBLOON]);
    if (lvl == 2) CHECK(counts[E_SKULLKEY] == 1 && counts[E_PIPE] == ld->norgan, "level 3 key / organ");
    if (lvl == 3) CHECK(counts[E_BIGCHEST] == 1 && counts[E_MA] == 1, "level 4 chest / Ma");
    /* every trigger lies on reachable ground */
    for (int i = 0; i < ld->ntrig; i++) {
        const MapTrig *t = &ld->trig[i];
        int ok = 0;
        for (int y = t->y0; y <= t->y1; y++) for (int x = t->x0; x <= t->x1; x++) if (x >= 0 && y >= 0 && reach[y][x]) ok = 1;
        CHECK(ok, "level %d trigger %d (scene %d) unreachable", lvl + 1, i, t->scene);
    }
}

/* ------------------------------------------------------------------ */
static void write_wav(const char *name, const int16_t *d, int n, int rate)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s.wav", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    int bytes = n * 2;
    fwrite("RIFF", 1, 4, f); int v = 36 + bytes; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f); short s = 1; fwrite(&s, 2, 1, f); fwrite(&s, 2, 1, f);
    fwrite(&rate, 4, 1, f); v = rate * 2; fwrite(&v, 4, 1, f); s = 2; fwrite(&s, 2, 1, f); s = 16; fwrite(&s, 2, 1, f);
    fwrite("data", 1, 4, f); fwrite(&bytes, 4, 1, f); fwrite(d, 2, (size_t)n, f);
    fclose(f);
}

static void audio_checks(int write)
{
    double t0 = now();
    for (int i = 0; i < MUS_TRACKS; i++) music_init_step(i);
    double t1 = now();
    printf("music rendered in %.2f s (native); Wii estimate ~%.0f s\n", t1 - t0, (t1 - t0) * 14);
    long total = 0;
    for (int i = 0; i < MUS_TRACKS; i++) {
        const int16_t *d = music_data(i);
        int n = music_len(i), clip = 0;
        double rms = 0;
        for (int k = 0; k < n; k++) { if (abs(d[k]) > 32000) clip++; rms += (double)d[k] * d[k]; }
        rms = sqrt(rms / n);
        printf("  track %d: %.1f s, rms %.0f, clipped %d, loop seam jump %d\n", i, (double)n / MUS_RATE, rms, clip, abs(d[0] - d[n - 1]));
        total += n;
        CHECK(n > MUS_RATE * 10, "track %d too short", i);
        CHECK(rms > 1500, "track %d too quiet", i);
        if (write) { char nm[32]; snprintf(nm, sizeof nm, "music_%d", i); write_wav(nm, d, n, MUS_RATE); }
    }
    printf("  music memory: %.1f MB\n", (double)total * 2 / 1048576.0);
    long sfx = 0;
    for (int i = 0; i < SFX_COUNT; i++) {
        sfx += audio_len(i);
        CHECK(audio_len(i) >= 16, "sfx %d empty", i);
        if (write) { char nm[32]; snprintf(nm, sizeof nm, "sfx_%02d", i); write_wav(nm, audio_data(i), audio_len(i), SFX_RATE); }
    }
    printf("  sfx: %d effects, %.1f MB\n", SFX_COUNT, (double)sfx * 2 / 1048576.0);
    if (write) { write_wav("amb_rain", amb_data(AMB_RAIN), amb_len(AMB_RAIN), AMB_RATE); write_wav("amb_cave", amb_data(AMB_CAVE), amb_len(AMB_CAVE), AMB_RATE); }
}

/* ------------------------------------------------------------------ */
static void perf(void)
{
    printf("performance (native; the Wii's CPU is very roughly 10-15x slower):\n");
    for (int lvl = 0; lvl < 4; lvl++) {
        start_level(lvl);
        double tu = 0, tr = 0;
        int maxv = 0;
        for (int f = 0; f < 240; f++) {
            Input in = none;
            in.sx = sinf((float)f * 0.02f); in.sy = cosf((float)f * 0.013f);
            double a = now();
            step(1, &in);
            double b = now();
            game_render(G);
            double c = now();
            tu += b - a; tr += c - b;
            if (gfx_nv > maxv) maxv = gfx_nv;
            if (G->state == ST_DIALOG) skip_dialog();
        }
        printf("  level %d: update %.3f ms, render %.3f ms, max verts %d\n", lvl + 1, tu / 240 * 1000, tr / 240 * 1000, maxv);
        CHECK(maxv < GFX_MAX_VTX - 2000, "vertex budget");
    }
}

/* ------------------------------------------------------------------ */
static void shots(void)
{
    game_init(G);
    step(30, 0);
    shot("title");
    /* the story */
    press(BTN_A); step(30, 0);
    shot("story");
    for (int lvl = 0; lvl < 4; lvl++) {
        start_level(lvl);
        char nm[64];
        snprintf(nm, sizeof nm, "l%d_start", lvl + 1);
        step(40, 0);
        shot(nm);
    }
    /* L1 views */
    start_level(0);
    place(34, 30); step(10, 0); skip_dialog(); step(20, 0); shot("l1_square");
    place(55, 32); skip_dialog(); step(20, 0); shot("l1_cannery");
    place(56, 14); skip_dialog(); step(20, 0); shot("l1_lighthouse");
    place(12, 33); skip_dialog(); step(20, 0); shot("l1_garden");
    place(70, 23); skip_dialog(); step(20, 0); shot("l1_pier");
    G->flash = 1.0f; G->bolt_t = 0.3f; G->bolt_x = 400; G->bolt_y = 200; G->bolt_seed = 77; shot("l1_lightning");
    /* dialog */
    start_scene(G, SC_L1_TINY); step(5, 0); press(BTN_A); step(60, 0); shot("dialog");
    skip_dialog();
    /* L2 */
    start_level(1);
    place(27, 9); skip_dialog(); step(20, 0); shot("l2_well");
    place(55, 31); skip_dialog(); step(20, 0); shot("l2_braziers");
    place(55, 18); skip_dialog(); step(20, 0); shot("l2_chasm");
    place(74, 39); skip_dialog(); step(20, 0); shot("l2_hooks");
    /* L3 */
    start_level(2);
    place(42, 39); skip_dialog(); step(20, 0); shot("l3_court");
    place(42, 18); skip_dialog(); step(20, 0); shot("l3_organ");
    G->gate_open[1] = 1; place(41, 7); skip_dialog(); step(20, 0); shot("l3_vault");
    place(66, 33); skip_dialog(); step(80, 0); shot("l3_flames");
    /* L4 */
    start_level(3);
    place(22, 33); skip_dialog(); step(20, 0); shot("l4_wreck");
    place(40, 26); skip_dialog(); step(20, 0); shot("l4_camp");
    place(66, 21); skip_dialog(); step(20, 0); shot("l4_ship");
    /* pause + map + game over + ending */
    G->state = ST_PAUSE; shot("pause");
    G->state = ST_MAP; shot("map");
    G->state = ST_PLAY;
    G->state = ST_GAMEOVER; G->st_t = 2; shot("gameover");
    G->state = ST_ENDING; G->end_stage = 0; G->st_t = 2; G->end_t = 3; shot("ending");
    G->end_stage = NENDING; shot("the_end");
}

/* ------------------------------------------------------------------ */
/* scripted runs: solve each puzzle the way a player would             */
/* ------------------------------------------------------------------ */
static Ent *find_ent(int type, int nth)
{
    for (int i = 0; i < G->nent; i++) if (G->ent[i].on && G->ent[i].type == type && nth-- == 0) return &G->ent[i];
    return 0;
}

static int gate_for(int cond)
{
    const LevelData *ld = G->ld;
    for (int i = 0; i < ld->ngroup; i++) if (ld->group[i].cond == cond) return G->gate_open[ld->group[i].gate];
    return -1;
}
static int gate_ent_open(int cond)
{
    const LevelData *ld = G->ld;
    for (int i = 0; i < ld->ngroup; i++)
        if (ld->group[i].cond == cond)
            for (int k = 0; k < G->nent; k++) if (G->ent[k].on && G->ent[k].type == E_GATE && G->ent[k].a == ld->group[i].gate) return G->ent[k].state;
    return -1;
}

/* stand behind a block and walk into it until it has slid n tiles */
static void push_block(Ent *b, int dx, int dy, int n)
{
    for (int k = 0; k < n; k++) {
        float tx = b->x - (float)dx * 15.0f, ty = b->y - (float)dy * 15.0f + (dy < 0 ? 6.0f : 0.0f) - (dx != 0 ? 5.0f : 0.0f);
        party_reset(G, tx, ty, DIR_DOWN);
        float ox = b->x, oy = b->y;
        Input in = none;
        if (dx > 0) in.held = BTN_RIGHT; if (dx < 0) in.held = BTN_LEFT; if (dy > 0) in.held = BTN_DOWN; if (dy < 0) in.held = BTN_UP;
        for (int i = 0; i < 90 && fabsf(b->x - ox) < 15 && fabsf(b->y - oy) < 15; i++) { step(1, &in); if (G->state == ST_DIALOG) skip_dialog(); }
        step(20, 0);
    }
}

static void walk_tests(void)
{
    printf("scripted runs:\n");
    /* L1: grab all three gears, use the crane, exit */
    start_level(0);
    {
        Ent *b = find_ent(E_BARREL, 0);
        party_reset(G, b->x - 22, b->y, DIR_SIDE);
        G->kid[G->order[0]].dir = DIR_SIDE; G->kid[G->order[0]].flip = 0; G->lead_dir = DIR_SIDE; G->lead_flip = 0;
        step(2, 0); skip_dialog();
        press(BTN_B); step(20, 0);
        CHECK(b->state == 1, "whip breaks a barrel");
    }
    for (int k = 0; k < 3; k++) {
        Ent *gear = find_ent(E_GEAR, 0);
        if (!gear) break;
        place(gear->x / TS - 0.5f, gear->y / TS - 1.1f);
        step(30, 0);
        skip_dialog();
    }
    CHECK(G->gears == 3, "L1 gears collected: %d", G->gears);
    Ent *crane = find_ent(E_CRANE, 0);
    if (crane) {
        place(crane->x / TS - 0.5f, crane->y / TS - 0.2f);
        G->kid[G->order[0]].dir = DIR_UP; G->lead_dir = DIR_UP;
        Input in = none; in.pressed = in.held = BTN_A;
        step(1, &in);
        CHECK(G->state == ST_DIALOG && G->scene == SC_CRANE_DONE, "L1 crane scene (state %d scene %d)", G->state, G->scene);
        skip_dialog();
        CHECK(G->crane_done, "L1 grate open");
        place(63, 11.3f);
        walk_to(63 * TS + 8, 12 * TS + 10, 120);
        step(5, 0);
        CHECK(G->state == ST_DIALOG || G->state == ST_LEVELDONE, "L1 exit (state %d)", G->state);
        skip_dialog();
        CHECK(G->state == ST_LEVELDONE, "L1 level done (state %d)", G->state);
    }
    /* L2: braziers, four plates, hooks */
    start_level(1);
    for (int k = 0; k < 4; k++) {
        Ent *b = find_ent(E_BRAZIER, k);
        place(b->x / TS - 0.5f, b->y / TS + 0.3f);
        G->kid[G->order[0]].dir = DIR_UP;
        press(BTN_A);
        step(3, 0);
    }
    step(5, 0);
    CHECK(gate_for(COND_BRAZIERS) == 1 && gate_ent_open(COND_BRAZIERS) == 1, "L2 braziers open the gate");
    skip_dialog();
    place(75, 6); skip_dialog();
    G->lv.st[6][71] = 0;
    place(71, 5.5f);
    press(BTN_A);
    step(240, 0);
    skip_dialog();
    CHECK(gate_for(COND_PLATES) == 1 && gate_ent_open(COND_PLATES) == 1, "L2 four plates open the gate");
    /* hook swing */
    Ent *h = find_ent(E_HOOK, 0);
    place(h->x / TS - 0.5f, h->y / TS - 1.6f);
    skip_dialog();
    G->kid[G->order[0]].dir = DIR_DOWN; G->lead_dir = DIR_DOWN;
    press(BTN_B);
    step(200, 0);
    const Kid *l = &G->kid[G->order[0]];
    CHECK(l->y > 46 * TS, "L2 swung across the chasm (y=%.0f)", l->y / TS);
    /* L3: blocks, organ, key */
    start_level(2);
    {
        /* push block 0 (13,30) onto plate (12,36): left 1, down 6 */
        Ent *b = find_ent(E_BLOCK, 0);
        float bx = b->x;
        push_block(b, -1, 0, 1);
        CHECK(b->x < bx - 8, "L3 block pushed left (%.0f -> %.0f)", bx, b->x);
        /* and on down to its plate (12,36); the third block (21,37) goes right then up to (22,30) */
        push_block(b, 0, 1, 6);
        Ent *b3 = find_ent(E_BLOCK, 2);
        push_block(b3, 1, 0, 1);
        push_block(b3, 0, -1, 7);
        step(5, 0);
        Ent *b2 = b3;
        CHECK(gate_for(COND_PLATES) == 1, "L3 blocks on plates (block1 %.1f,%.1f block2 %.1f,%.1f)", b->x / TS, b->y / TS, b2->x / TS, b2->y / TS);
        skip_dialog();
    }
    {
        Game *g = G;
        /* the organ: whip the pipes in tune order */
        for (int n = 0; n < LEVELS[2].norgan; n++) {
            int want = LEVELS[2].organ[n];
            Ent *p = 0;
            for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_PIPE && g->ent[i].a == want) p = &g->ent[i];
            place(p->x / TS - 0.5f, p->y / TS + 0.4f);
            skip_dialog();
            g->kid[g->order[0]].dir = DIR_UP; g->lead_dir = DIR_UP;
            g->whip_cool = 0;
            press(BTN_B);
            step(30, 0);
        }
        skip_dialog();
        CHECK(gate_for(COND_ORGAN) == 1 && gate_ent_open(COND_ORGAN) == 1, "L3 organ solved (pos %d)", g->organ_pos);
        Ent *key = find_ent(E_SKULLKEY, 0);
        place(key->x / TS - 0.5f, key->y / TS - 1.2f);
        step(20, 0);
        CHECK(g->has_key, "L3 got the key");
        skip_dialog();
        step(5, 0);
        CHECK(gate_for(COND_KEY) == 1 && gate_ent_open(COND_KEY) == 1, "L3 escape gate open");
        Ent *lv = find_ent(E_LEVER, 0);
        place(lv->x / TS - 1.6f, lv->y / TS - 1.0f); skip_dialog();
        g->kid[g->order[0]].dir = DIR_SIDE; g->kid[g->order[0]].flip = 0; g->lead_dir = DIR_SIDE; g->whip_cool = 0;
        press(BTN_B); step(20, 0); skip_dialog();
        CHECK(gate_for(COND_LEVER) == 1 && gate_ent_open(COND_LEVER) == 1, "L3 lever opens the nook");
    }
    /* L4: board the ship, beat Ma, open the chest */
    start_level(3);
    {
        Game *g = G;
        place(52, 19.5f);
        skip_dialog();
        Ent *h4 = find_ent(E_HOOK, 0);
        g->kid[g->order[0]].x = h4->x - 14; g->kid[g->order[0]].y = h4->y + 1;
        g->kid[g->order[0]].dir = DIR_SIDE; g->kid[g->order[0]].flip = 0; g->lead_dir = DIR_SIDE; g->lead_flip = 0;
        press(BTN_B);
        step(220, 0);
        skip_dialog();
        CHECK(g->boss_phase == 1, "L4 boss started (phase %d, x=%.1f)", g->boss_phase, g->kid[g->order[0]].x / TS);
        Ent *ma = find_ent(E_MA, 0);
        for (int f = 0; f < 60 * 90 && ma && !g->boss_down; f++) {
            Kid *k = &g->kid[g->order[0]];
            for (int i = 0; i < NKIDS; i++) g->kid[i].hp = KID_HP;
            if (ma->state == 1) {
                float dx = ma->x - k->x, dy = ma->y - k->y;
                k->dir = fabsf(dx) > fabsf(dy) ? DIR_SIDE : (dy < 0 ? DIR_UP : DIR_DOWN); k->flip = dx < 0;
                g->lead_dir = k->dir; g->lead_flip = k->flip;
                Input in = none;
                float d = sqrtf(dx * dx + dy * dy);
                if (d < 28 && g->whip_cool <= 0) { in.pressed = in.held = BTN_B; }
                else if (d > 22) { in.sx = dx / d; in.sy = dy / d; }
                step(1, &in);
                if (d > 22) { k->dir = fabsf(dx) > fabsf(dy) ? DIR_SIDE : (dy < 0 ? DIR_UP : DIR_DOWN); k->flip = dx < 0; g->lead_dir = k->dir; g->lead_flip = k->flip; }
            } else step(1, 0);
            if (g->state == ST_GAMEOVER) { press(BTN_A); }
            if (getenv("DBG") && f % 30 == 0) printf("    f%d ma st%d hp%d t%.2f pos %.0f,%.0f  kid %.0f,%.0f state %d dir %d\n", f, ma->state, ma->hp, ma->t, ma->x, ma->y, g->kid[g->order[0]].x, g->kid[g->order[0]].y, g->state, g->kid[g->order[0]].dir);
        }
        CHECK(g->boss_down, "L4 Ma beaten");
        skip_dialog();
        Ent *ch = find_ent(E_BIGCHEST, 0);
        place(ch->x / TS - 1.6f, ch->y / TS - 1.0f);
        g->kid[g->order[0]].dir = DIR_SIDE; g->kid[g->order[0]].flip = 0;
        press(BTN_A);
        CHECK(g->scene == SC_L4_CHEST, "L4 chest scene (%d)", g->scene);
        step(5, 0);
        for (int i = 0; i < 100 && g->state == ST_DIALOG; i++) { press(BTN_A); step(4, 0); }
        step(120, 0);
        CHECK(g->state == ST_ENDING, "L4 ending (state %d)", g->state);
        for (int i = 0; i < 8; i++) { step(80, 0); press(BTN_A); }
        step(20, 0);
        CHECK(g->state == ST_TITLE || g->state == ST_STORY, "back to title (state %d)", g->state);
    }
}

static void anim(void)
{
    start_level(0);
    place(20, 44);
    skip_dialog();
    Input in = none;
    in.sx = 1;
    for (int i = 0; i < 40; i++) step(1, &in);
    char nm[32];
    for (int f = 0; f < 8; f++) {
        if (f == 2) { Input w = in; w.pressed = w.held = BTN_B; step(1, &w); } else step(3, &in);
        printf("f%d whip_t %.3f state %d frame %d busy %d\n", f, G->whip_t, G->state, G->kid[G->order[0]].frame, party_busy(G));
        snprintf(nm, sizeof nm, "anim%d", f);
        shot(nm);
    }
    G->kid[G->order[0]].dir = DIR_DOWN; G->lead_dir = DIR_DOWN;
    press(BTN_B); step(5, 0); printf("whip_t %.3f state %d cool %.2f kstate %d\n", G->whip_t, G->state, G->whip_cool, G->kid[G->order[0]].state); shot("anim_whipdown");
    G->kid[G->order[0]].dir = DIR_UP; G->lead_dir = DIR_UP; G->whip_cool = 0;
    press(BTN_B); step(5, 0); shot("anim_whipup");
    kid_say(G, CH_ROCCO, "Hey Ollie, why'd the pirate go to school?", 0); step(10, 0); shot("anim_bubble");
}

static void reel(int lvl)
{
    char dir[512];
    snprintf(dir, sizeof dir, "%s/reel", outdir);
    start_level(lvl);
    const char *save = outdir;
    outdir = dir;
    for (int f = 0; f < 30 * 14; f++) {
        Input in = none;
        float t = (float)f / 30.0f;
        in.sx = sinf(t * 0.5f) * 0.9f; in.sy = sinf(t * 0.33f + 1) * 0.9f;
        if (f % 50 == 25) in.pressed = in.held = BTN_B;
        game_update(G, &in, 1.0f / 60.0f);
        in.pressed = 0;
        game_update(G, &in, 1.0f / 60.0f);
        int p, v; float pan; while (game_pop_sound(G, &p, &pan, &v) >= 0) {}
        if (G->state == ST_DIALOG) skip_dialog();
        char nm[32];
        snprintf(nm, sizeof nm, "f%04d", f);
        game_render(G); raster(); save_ppm(nm);
    }
    outdir = save;
}

int main(int argc, char **argv)
{
    outdir = argc > 1 ? argv[1] : ".";
    const char *what = argc > 2 ? argv[2] : "all";
    G = (Game *)calloc(1, (size_t)GAME_SIZE);
    srand(1);
    double t0 = now();
    game_init(G);
    printf("init (art + sfx) %.2f s native\n", now() - t0);
    int all = !strcmp(what, "all");
    if (all || !strcmp(what, "maps")) for (int i = 0; i < 4; i++) check_level(i);
    if (all || !strcmp(what, "audio")) audio_checks(!strcmp(what, "audio"));
    if (all || !strcmp(what, "shots")) shots();
    if (all || !strcmp(what, "walk")) walk_tests();
    if (all || !strcmp(what, "perf")) perf();
    if (!strcmp(what, "anim")) anim();
    if (!strcmp(what, "reel")) reel(argc > 3 ? atoi(argv[3]) - 1 : 0);
    printf(fails ? "\n%d FAILURES\n" : "\nall checks passed\n", fails);
    return fails ? 1 : 0;
}
