/*
 * TREASURE HOUNDS - the game's state machine: title, story, chapters, play, dialog, pause,
 * game over, chapter tally and the ending. Plus the camera, triggers and the sound queue.
 */
#include "game_int.h"
#include "music.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

const int GAME_SIZE = sizeof(Game);

float frand(Game *g)
{
    g->rng ^= g->rng << 13; g->rng ^= g->rng >> 17; g->rng ^= g->rng << 5;
    return (float)(g->rng >> 8) / 16777216.0f;
}
float frand2(Game *g) { return frand(g) * 2.0f - 1.0f; }

void sfx(Game *g, int id, int pitch, float pan, int vol)
{
    if (!g->sound_on || g->nsnd >= MAX_SND || id < 0) return;
    Snd *s = &g->snd[g->nsnd++];
    s->id = (int16_t)id;
    s->pitch = (uint8_t)(pitch < 20 ? 20 : (pitch > 255 ? 255 : pitch));
    s->pan = (int8_t)(clampf(pan, -1, 1) * 100);
    s->vol = (uint8_t)(vol < 0 ? 0 : (vol > 255 ? 255 : vol));
}

void sfx_at(Game *g, int id, float x, float y, int pitch)
{
    float cx = g->camx + VIEW_W * 0.5f, cy = g->camy + VIEW_H * 0.5f;
    float dx = x - cx, dy = y - cy, d = sqrtf(dx * dx + dy * dy);
    if (d > 320) return;
    float v = 1.0f - d / 320.0f;
    sfx(g, id, pitch, dx / (VIEW_W * 0.5f) * 0.8f, (int)(255 * v * v));
}

int game_pop_sound(Game *g, int *pitch, float *pan, int *vol)
{
    if (g->nsnd <= 0) return -1;
    Snd s = g->snd[0];
    memmove(&g->snd[0], &g->snd[1], sizeof(Snd) * (size_t)(g->nsnd - 1));
    g->nsnd--;
    *pitch = s.pitch; *pan = (float)s.pan / 100.0f; *vol = s.vol;
    return s.id;
}

void toast(Game *g, const char *s)
{
    snprintf(g->toast, sizeof g->toast, "%s", s);
    g->toast_t = 2.5f;
}

void checkpoint(Game *g)
{
    const Kid *l = &g->kid[g->order[0]];
    g->chk_x = l->x; g->chk_y = l->y;
    for (int i = 0; i < NKIDS; i++) g->chk_hp[i] = g->kid[i].hp;
}

/* ------------------------------------------------------------------ */
static void title_setup(Game *g)
{
    world_load(g, 0);
    g->state = ST_TITLE;
    g->menu = 0;
    g->title_t = 0;
    /* hide the kids off by the edge; frame the lighthouse */
    for (int i = 0; i < NKIDS; i++) { g->kid[i].x = -200; g->kid[i].y = -200; }
    g->camx = 49 * TS - VIEW_W * 0.5f; g->camy = 6 * TS - VIEW_H * 0.5f + 40;
}

static void begin_level(Game *g, int lvl)
{
    world_load(g, lvl);
    g->state = ST_CHAPTER;
    g->st_t = 0;
    g->lvl_coins = 0; g->lvl_doubloons = 0;
    g->gears = 0; g->crane_done = 0; g->organ_pos = 0; g->braziers_lit = 0;
    g->boss_phase = 0; g->boss_down = 0; g->collapse_t = 0;
    memset(g->gate_open, 0, sizeof(g->gate_open));
    memset(g->trig_done, 0, sizeof(g->trig_done));
    if (lvl == 3) g->has_key = 1;
    if (lvl < 3 && lvl != 2) g->has_key = 0;
    if (lvl == 2) g->has_key = 0;
    for (int i = 0; i < NKIDS; i++) g->kid[i].hp = KID_HP;
    g->order[0] = CH_SAM; g->order[1] = CH_ROCCO; g->order[2] = CH_GUS; g->order[3] = CH_OLLIE;
    const Kid *l = &g->kid[g->order[0]];
    party_reset(g, l->x, l->y, DIR_DOWN);
    checkpoint(g);
    g->banter_t = 20.0f;
    g->bubble_seq = -1;
    g->ndrop = 0;
    g->flash_t = 4.0f;
    g->scene = SC_NONE;
}

void game_init(Game *g)
{
    memset(g, 0, sizeof(*g));
    g->rng = 0x9E3779B9u;
    g->music_on = 1; g->sound_on = 1;
    g->fade_to = -1;
    for (int i = 0; i < NKIDS; i++) { g->order[i] = i; g->kid[i].hp = KID_HP; }
    art_init();
    audio_init();
    title_setup(g);
}

int game_quit_requested(const Game *g) { return g->quit; }

int game_music(const Game *g)
{
    if (!g->music_on) return -1;
    switch (g->state) {
    case ST_TITLE: case ST_STORY: return MUS_TITLE;
    case ST_ENDING: return MUS_VICTORY;
    default: break;
    }
    if (g->level == 3 && g->scene == SC_L4_CHEST && g->line >= 3) return MUS_VICTORY;
    if (g->level == 3 && g->after_act == ACT_ENDING) return MUS_VICTORY;
    static const int tr[4] = { MUS_TOWN, MUS_CAVES, MUS_TEMPLE, MUS_COVE };
    return tr[g->level & 3];
}

int game_ambience(const Game *g)
{
    if (!g->sound_on || g->state == ST_STORY || g->state == ST_ENDING) return -1;
    if (g->lv.theme == THEME_TOWN || g->lv.theme == THEME_COVE) return AMB_RAIN;
    return AMB_CAVE;
}

/* ------------------------------------------------------------------ */
static void camera(Game *g, float dt, int snap)
{
    const Kid *l = &g->kid[g->order[0]];
    float lx = 0, ly = 0;
    if (l->dir == DIR_SIDE) lx = l->flip ? -24.0f : 24.0f; else ly = l->dir == DIR_UP ? -16.0f : 16.0f;
    g->cam_tx = lerpf(g->cam_tx, lx, 1 - expf(-dt * 1.5f));
    g->cam_ty = lerpf(g->cam_ty, ly, 1 - expf(-dt * 1.5f));
    float tx = l->x + g->cam_tx - VIEW_W * 0.5f, ty = l->y - 12 + g->cam_ty - VIEW_H * 0.5f;
    float mw = (float)(g->lv.w * TS) - VIEW_W, mh = (float)(g->lv.h * TS) - VIEW_H;
    tx = clampf(tx, 0, mw > 0 ? mw : 0); ty = clampf(ty, 0, mh > 0 ? mh : 0);
    if (snap) { g->camx = tx; g->camy = ty; return; }
    float k = 1 - expf(-dt * 6.0f);
    g->camx = lerpf(g->camx, tx, k); g->camy = lerpf(g->camy, ty, k);
}

static void explore(Game *g)
{
    const Kid *l = &g->kid[g->order[0]];
    int cx = (int)(l->x / TS), cy = (int)(l->y / TS);
    for (int y = cy - 9; y <= cy + 9; y++)
        for (int x = cx - 14; x <= cx + 14; x++)
            if (x >= 0 && y >= 0 && x < g->lv.w && y < g->lv.h) g->lv.seen[y][x] = 1;
}

static void triggers(Game *g)
{
    const Kid *l = &g->kid[g->order[0]];
    if (l->state != KS_FOLLOW) return;
    int tx = (int)(l->x / TS), ty = (int)((l->y - 2) / TS);
    const LevelData *ld = g->ld;
    for (int i = 0; i < ld->ntrig && i < 60; i++) {
        const MapTrig *t = &ld->trig[i];
        if (tx < t->x0 || tx > t->x1 || ty < t->y0 || ty > t->y1) continue;
        if (t->flags & TRIG_EXIT) {
            if (g->level == 0 && !g->crane_done) continue;
            static const int ex[3] = { SC_EXIT_L1, SC_EXIT_L2, SC_EXIT_L3 };
            if (g->level < 3) start_scene(g, ex[g->level]);
            return;
        }
        if ((t->flags & TRIG_ONCE) && g->trig_done[i]) continue;
        if (t->scene == SC_L1_TINY && g->trig_done[63]) { g->trig_done[i] = 1; continue; }
        g->trig_done[i] = 1;
        if (t->flags & TRIG_CHECKPOINT) checkpoint(g);
        start_scene(g, t->scene);
        return;
    }
}

static void respawn(Game *g)
{
    for (int i = 0; i < NKIDS; i++) g->kid[i].hp = g->chk_hp[i] > 3 ? g->chk_hp[i] : 4;
    int lead = g->order[0];
    if (g->kid[lead].hp <= 0) g->kid[lead].hp = 4;
    party_reset(g, g->chk_x, g->chk_y, DIR_DOWN);
    for (int i = 0; i < g->nent; i++) {          /* the Crowleys lose track of you */
        Ent *e = &g->ent[i];
        if (!e->on) continue;
        if (e->type == E_VIC || e->type == E_LENNY) { if (g->boss_phase < 1) { e->x = e->hx; e->y = e->hy; e->state = 0; } }
        if (e->type == E_BOTTLE || e->type == E_DART) { e->on = 0; e->type = E_NONE; }
        if (e->type == E_MA && g->boss_phase >= 1) { e->x = e->hx; e->y = e->hy; e->state = 0; e->t = 1.5f; }
        if (e->type == E_SKEL && e->state != 5) { e->state = 0; }
    }
    g->deaths++;
    camera(g, 0, 1);
    g->state = ST_PLAY;
    g->st_t = 0;
}

/* ------------------------------------------------------------------ */
static void menu_move(Game *g, const Input *in, int n)
{
    int up = (in->pressed & BTN_UP) || 0, dn = (in->pressed & BTN_DOWN) != 0;
    static float rep;
    if (in->sy < -0.6f || in->sy > 0.6f) {
        rep -= 1.0f / 60.0f;
        if (rep <= 0) { if (in->sy < 0) up = 1; else dn = 1; rep = 0.22f; }
    } else rep = 0;
    if (up) { g->menu = (g->menu + n - 1) % n; sfx(g, SFX_MENU, 128, 0, 120); }
    if (dn) { g->menu = (g->menu + 1) % n; sfx(g, SFX_MENU, 128, 0, 120); }
}
static int menu_lr(const Input *in)
{
    static float rep;
    int v = 0;
    if (in->pressed & (BTN_LEFT | BTN_L)) v = -1;
    if (in->pressed & (BTN_RIGHT | BTN_R)) v = 1;
    if (in->sx < -0.6f || in->sx > 0.6f) {
        rep -= 1.0f / 60.0f;
        if (rep <= 0) { v = in->sx < 0 ? -1 : 1; rep = 0.28f; }
    } else rep = 0;
    return v;
}

static void world_tick(Game *g, const Input *in, float dt, int full)
{
    Input none;
    memset(&none, 0, sizeof none);
    party_update(g, full ? in : &none, dt);
    if (full || g->state == ST_DIALOG) ents_update(g, dt);
    if (full) { puzzles_check(g); if (g->state == ST_PLAY) triggers(g); }
    weather_update(g, dt);
    parts_update(g, dt);
    camera(g, dt, 0);
    explore(g);
    banter_update(g, dt);
    lights_collect(g);
    if (g->shake > 0) { g->shake *= expf(-dt * 7.0f); if (g->shake < 0.05f) g->shake = 0; }
    if (g->toast_t > 0) g->toast_t -= dt;
    if (g->hud_coin_t > 0) g->hud_coin_t -= dt;
    /* the temple falling apart behind you */
    if (g->collapse_t > 0 && g->level == 2) {
        g->collapse_t -= dt;
        g->shake = fmaxf(g->shake, 1.2f + 0.8f * sinf(g->time * 3));
        if (frand(g) < dt * 6) {
            Part *p = part_new(g, PK_DEBRIS, g->camx + frand(g) * VIEW_W, g->camy + frand(g) * VIEW_H);
            p->z = 90 + frand(g) * 40; p->vz = -40; p->c = 0x6E6A56FFu; p->life = p->max = 1.4f;
        }
        if (fmodf(g->time, 3.3f) < dt) sfx(g, SFX_RUMBLE, 80 + (int)(frand(g) * 30), frand2(g) * 0.5f, 160);
    }
}

void game_update(Game *g, const Input *in, float dt)
{
    g->time += dt;
    g->st_t += dt;
    g->nsnd = g->nsnd > MAX_SND - 8 ? MAX_SND - 8 : g->nsnd;

    /* fade-to requests (the ending) */
    if (g->fade_to >= 0) {
        g->fade += dt * 1.2f;
        if (g->fade >= 1) {
            g->state = g->fade_to; g->fade_to = -1; g->st_t = 0;
            if (g->state == ST_ENDING) { g->end_stage = 0; g->end_t = 0; }
        }
    } else if (g->fade > 0) g->fade -= dt * 1.5f;

    switch (g->state) {
    case ST_TITLE: {
        g->title_t += dt;
        weather_update(g, dt);
        parts_update(g, dt);
        lights_collect(g);
        g->camx = 49 * TS - VIEW_W * 0.5f + sinf(g->title_t * 0.12f) * 40;
        menu_move(g, in, 4);
        int lr = menu_lr(in);
        if (g->menu == 1 && lr) { g->chapter_pick = (g->chapter_pick + NUM_LEVELS + lr) % NUM_LEVELS; sfx(g, SFX_MENU, 150, 0, 120); }
        if ((g->menu == 2 || g->menu == 3) && lr) {
            if (g->menu == 2) g->music_on = !g->music_on; else g->sound_on = !g->sound_on;
            sfx(g, SFX_SELECT, 128, 0, 160);
        }
        if (in->pressed & (BTN_A | BTN_PLUS)) {
            sfx(g, SFX_SELECT, 128, 0, 200);
            if (g->menu == 0 || g->menu == 1) {
                g->coins = 0; g->doubloons = 0; g->gems = 0; g->deaths = 0;
                int lvl = g->menu == 0 ? 0 : g->chapter_pick;
                if (lvl == 0) { g->state = ST_STORY; g->story_page = 0; g->st_t = 0; }
                else begin_level(g, lvl);
            } else if (g->menu == 2) g->music_on = !g->music_on;
            else if (g->menu == 3) g->sound_on = !g->sound_on;
        }
        break;
    }
    case ST_STORY:
        if (in->pressed & BTN_A) {
            sfx(g, SFX_MENU, 128, 0, 120);
            g->story_page++; g->st_t = 0;
            if (g->story_page >= NSTORY) begin_level(g, 0);
        }
        if (in->pressed & BTN_B) begin_level(g, 0);
        break;
    case ST_CHAPTER:
        world_tick(g, in, dt, 0);
        camera(g, dt, g->st_t < 0.05f);
        if (g->st_t > 3.2f || (g->st_t > 0.8f && (in->pressed & BTN_A))) {
            g->state = ST_PLAY; g->st_t = 0;
            if (g->level == 0) start_scene(g, SC_L1_INTRO);
        }
        break;
    case ST_PLAY:
        if (in->pressed & BTN_PLUS) { g->state = ST_PAUSE; g->menu = 0; sfx(g, SFX_PAUSE, 128, 0, 160); break; }
        if (in->pressed & BTN_X) { g->state = ST_MAP; sfx(g, SFX_MENU, 110, 0, 140); break; }
        world_tick(g, in, dt, 1);
        break;
    case ST_DIALOG:
        dialog_update(g, in, dt);
        world_tick(g, in, dt, 0);
        break;
    case ST_MAP:
        if (in->pressed & (BTN_X | BTN_B | BTN_A | BTN_PLUS)) { g->state = ST_PLAY; sfx(g, SFX_MENU, 140, 0, 120); }
        weather_update(g, dt);
        break;
    case ST_PAUSE:
        menu_move(g, in, 5);
        if (in->pressed & BTN_PLUS) { g->state = ST_PLAY; break; }
        if (g->menu == 1 && menu_lr(in)) g->music_on = !g->music_on;
        if (g->menu == 2 && menu_lr(in)) g->sound_on = !g->sound_on;
        if (in->pressed & BTN_A) {
            sfx(g, SFX_SELECT, 128, 0, 160);
            switch (g->menu) {
            case 0: g->state = ST_PLAY; break;
            case 1: g->music_on = !g->music_on; break;
            case 2: g->sound_on = !g->sound_on; break;
            case 3: respawn(g); break;
            case 4: title_setup(g); break;
            }
        }
        if (in->pressed & BTN_B) g->state = ST_PLAY;
        break;
    case ST_GAMEOVER:
        weather_update(g, dt);
        parts_update(g, dt);
        lights_collect(g);
        if (g->st_t > 1.2f && (in->pressed & BTN_A)) respawn(g);
        break;
    case ST_LEVELDONE:
        if (g->st_t > 1.0f && (in->pressed & BTN_A)) {
            if (g->level + 1 < NUM_LEVELS) begin_level(g, g->level + 1);
            else title_setup(g);
        }
        break;
    case ST_ENDING:
        g->end_t += dt;
        if (g->end_stage < NENDING) {
            if ((g->st_t > 1.0f && (in->pressed & BTN_A)) || g->st_t > 9.0f) { g->end_stage++; g->st_t = 0; sfx(g, SFX_MENU, 128, 0, 100); }
        } else if (g->st_t > 2.0f && (in->pressed & BTN_A)) title_setup(g);
        break;
    }
}

void game_render(const Game *g)
{
    gfx_begin();
    switch (g->state) {
    case ST_TITLE: render_title(g); break;
    case ST_STORY: render_story(g); break;
    case ST_CHAPTER: render_chapter(g); break;
    case ST_ENDING: render_ending(g); break;
    default:
        render_world(g);
        render_ui(g);
        if (g->state == ST_PAUSE) render_pause(g);
        if (g->state == ST_MAP) render_map(g);
        if (g->state == ST_GAMEOVER) render_gameover(g);
        if (g->state == ST_LEVELDONE) render_leveldone(g);
        break;
    }
    if (g->fade > 0) { gfx_xf_reset(); gfx_blend(BL_ALPHA); gfx_rect(0, 0, VW, VH, RGBA(0, 0, 0, (int)(clampf(g->fade, 0, 1) * 255))); }
}
