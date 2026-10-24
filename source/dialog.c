/*
 * TREASURE HOUNDS - running the script: the dialog box, scene actions, and banter bubbles.
 */
#include "game_int.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static void prepare_line(Game *g)
{
    const Line *ln = &SCENES[g->scene][g->line];
    char tmp[400];
    const char *src = ln->text ? ln->text : "";
    /* substitutions: %G gears found, %S the sign being read */
    int n = 0;
    for (const char *p = src; *p && n < (int)sizeof(tmp) - 1; p++) {
        if (p[0] == '%' && p[1] == 'G') { n += snprintf(tmp + n, sizeof(tmp) - (size_t)n, "%d", g->gears); p++; continue; }
        if (p[0] == '%' && p[1] == 'S') {
            const char *s = (g->scene_after >= 0 && g->scene_after < g->ld->nsign) ? g->ld->sign[g->scene_after] : "";
            n += snprintf(tmp + n, sizeof(tmp) - (size_t)n, "%s", s);
            p++; continue;
        }
        tmp[n++] = *p;
    }
    tmp[n < (int)sizeof(tmp) ? n : (int)sizeof(tmp) - 1] = 0;
    float width = (ln->who == SPK_NARR) ? VW - 52 - 52 : VW - 52 - 26 - 84 - 20;
    gfx_wrap(tmp, g->wrap, sizeof(g->wrap), 2, width);
    g->line_t = 0;
    g->chars = 0;
    if (ln->act != ACT_NONE) scene_action(g, ln->act);
    if (ln->who >= 0 && ln->who < NKIDS) g->kid[ln->who].bubble_t = 0;
}

void start_scene(Game *g, int scene)
{
    if (scene <= SC_NONE || scene >= SC_COUNT || !SCENES[scene]) return;
    if (scene == SC_L1_TINY) g->trig_done[63] = 1;
    g->scene = scene;
    g->line = 0;
    g->prev_state = g->state == ST_DIALOG ? g->prev_state : g->state;
    g->state = ST_DIALOG;
    g->whip_t = 0;
    for (int i = 0; i < NKIDS; i++) g->kid[i].bubble_t = 0;
    g->bubble_seq = -1;
    prepare_line(g);
}

static void end_scene(Game *g)
{
    int sc = g->scene;
    g->scene = SC_NONE;
    g->state = ST_PLAY;
    party_regroup(g);
    int act = g->after_act;
    g->after_act = 0;
    if (sc == SC_L1_INTRO) { start_scene(g, SC_L1_TUTORIAL); return; }
    if (act == ACT_LEVEL_DONE) { g->state = ST_LEVELDONE; g->st_t = 0; sfx(g, SFX_FANFARE, 128, 0, 230); return; }
    if (act == ACT_ENDING) { g->fade_to = ST_ENDING; return; }
    g->banter_t = fmaxf(g->banter_t, 12.0f);
}

void dialog_update(Game *g, const Input *in, float dt)
{
    const Line *ln = &SCENES[g->scene][g->line];
    g->line_t += dt;
    int len = (int)strlen(g->wrap);
    if (len == 0) {                     /* an action-only line: move straight on */
        g->line++;
        if (!SCENES[g->scene][g->line].text) end_scene(g); else prepare_line(g);
        return;
    }
    float before = g->chars;
    if (g->chars < (float)len) {
        /* typewriter, with little pauses after punctuation */
        int c = (int)g->chars;
        char prev = c > 0 ? g->wrap[c - 1] : 0;
        float spd = (prev == '.' || prev == '!' || prev == '?') ? 9.0f : (prev == ',' ? 18.0f : 52.0f);
        g->chars += dt * spd;
        if (g->chars > (float)len) g->chars = (float)len;
        /* voice blips, pitched per speaker */
        if ((int)(g->chars / 2.0f) != (int)(before / 2.0f) && g->wrap[(int)g->chars - 1 > 0 ? (int)g->chars - 1 : 0] != ' ') {
            static const int vp[8] = { 150, 175, 200, 125, 105, 95, 115, 70 };
            int p = ln->who >= 0 ? vp[ln->who] : (ln->who == SPK_SIGN ? 80 : 135);
            sfx(g, SFX_BLIP, p + (int)(frand(g) * 12), 0, ln->who == SPK_NARR ? 50 : 80);
        }
    }
    if (in->pressed & (BTN_A | BTN_B)) {
        if (g->chars < (float)len) g->chars = (float)len;
        else {
            g->line++;
            sfx(g, SFX_MENU, 140, 0, 70);
            if (!SCENES[g->scene][g->line].text) { end_scene(g); return; }
            prepare_line(g);
        }
    }
}

void scene_action(Game *g, int act)
{
    switch (act) {
    case ACT_SHAKE: g->shake = 7; break;
    case ACT_THUNDER:
        g->flash = 1.0f; g->bolt_t = 0.32f; g->bolt_x = VW * 0.5f + frand2(g) * 200; g->bolt_y = 200; g->bolt_seed = (uint32_t)(frand(g) * 1e6f);
        sfx(g, SFX_CRACK, 128, 0, 200); g->thunder_t = 0.3f;
        break;
    case ACT_CHEER:
        for (int i = 0; i < NKIDS; i++) { g->kid[i].state = KS_CHEER; g->kid[i].anim = (float)i; }
        sfx(g, SFX_CHEER, 128, 0, 200);
        break;
    case ACT_OPEN_GRATE:
        g->crane_done = 1;
        g->shake = 6;
        sfx(g, SFX_CRANE, 128, 0, 255);
        for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_CRANE) burst(g, PK_DUST, g->ent[i].x + 30, g->ent[i].y - 4, 20, 50, 0x8A8070C0u);
        break;
    case ACT_TINY_LOVE:
        for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_TINY) g->ent[i].t = 5.0f;
        sfx(g, SFX_ROAR, 150, 0, 160);
        break;
    case ACT_TINY_ENTER: {
        Ent *ma = 0;
        for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_MA) ma = &g->ent[i];
        Ent *t = ent_new(g, E_TINY, (float)(58 * TS + 4), (float)(20 * TS + 14));
        if (t) {
            t->state = 2; t->dir = DIR_SIDE;
            t->hx = ma ? ma->x - 24 : t->x + 120; t->hy = ma ? ma->y : t->y;
        }
        g->shake = 8;
        sfx(g, SFX_ROAR, 100, 0, 255);
        break;
    }
    case ACT_BOSS_START: {
        g->boss_phase = 1;
        Ent *ma = 0;
        for (int i = 0; i < g->nent; i++) {
            Ent *e = &g->ent[i];
            if (!e->on) continue;
            if (e->type == E_MA) ma = e;
            if (e->type == E_VIC || e->type == E_LENNY) { e->on = 0; e->type = E_NONE; }   /* they run up the gangway */
        }
        if (ma) {
            ma->state = 0; ma->t = 1.0f; ma->hp = 5;
            Ent *v = ent_new(g, E_VIC, ma->x + 10, ma->y - 40);
            Ent *l = ent_new(g, E_LENNY, ma->x + 10, ma->y + 40);
            if (v) { v->state = 2; v->hp = 3; v->hx = v->x; v->hy = v->y; }
            if (l) { l->state = 2; l->hp = 3; l->hx = l->x; l->hy = l->y; }
        }
        sfx(g, SFX_HEY, 90, 0, 220);
        break;
    }
    case ACT_DUNK:
        for (int i = 0; i < g->nent; i++) {
            Ent *e = &g->ent[i];
            if (!e->on || (e->type != E_MA && e->type != E_VIC && e->type != E_LENNY && e->type != E_BOTTLE)) continue;
            burst(g, PK_SMOKE, e->x, e->y - 10, 8, 40, 0xC8C0D0B0u);
            for (int k = 0; k < 3; k++) {
                Part *p = part_new(g, PK_SPLASH, e->x + 40 + frand2(g) * 30, (float)(28 * TS + 8) + frand2(g) * 8);
                p->size = 8; p->life = p->max = 0.9f;
            }
            e->on = 0; e->type = E_NONE;
        }
        g->shake = 9;
        sfx(g, SFX_SPLASH, 80, 0, 255);
        sfx(g, SFX_SPLASH, 60, 0.3f, 255);
        for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_TINY) { g->ent[i].t = 6.0f; g->ent[i].state = 3; }
        break;
    case ACT_CHEST_OPEN:
        for (int i = 0; i < g->nent; i++) {
            Ent *e = &g->ent[i];
            if (!e->on || e->type != E_BIGCHEST) continue;
            e->state = 1;
            for (int k = 0; k < 90; k++) {
                Part *p = part_new(g, PK_GOLD, e->x + frand2(g) * 6, e->y - 14);
                float a = frand(g) * TAU_F, s = 20 + frand(g) * 60;
                p->vx = cosf(a) * s; p->vy = sinf(a) * s * 0.5f; p->vz = 120 + frand(g) * 160; p->z = 8;
                p->life = p->max = 1.6f + frand(g) * 1.4f; p->rot = frand(g) * 6;
            }
            burst(g, PK_SPARK, e->x, e->y - 16, 40, 120, 0xFFE070FFu);
        }
        g->coins += 1000; g->lvl_coins += 1000;
        g->flash = 0.6f;
        g->shake = 6;
        sfx(g, SFX_CHEST, 90, 0, 255);
        sfx(g, SFX_GOLD, 128, 0, 255);
        for (int i = 0; i < NKIDS; i++) { g->kid[i].state = KS_CHEER; g->kid[i].anim = (float)i * 0.7f; }
        break;
    case ACT_COLLAPSE:
        g->collapse_t = 60.0f;
        g->shake = 10;
        sfx(g, SFX_RUMBLE, 80, 0, 255);
        break;
    case ACT_RUMBLE:
        g->shake = 8;
        sfx(g, SFX_RUMBLE, 100, 0, 230);
        break;
    case ACT_FANFARE: sfx(g, SFX_FANFARE, 128, 0, 230); break;
    case ACT_ENDING: g->after_act = ACT_ENDING; sfx(g, SFX_CHEER, 128, 0, 230); break;
    case ACT_LEVEL_DONE: g->after_act = ACT_LEVEL_DONE; break;
    }
}

/* ------------------------------------------------------------------ */
/* banter                                                              */
/* ------------------------------------------------------------------ */
static const Banter *banter_list(const Game *g, int *n)
{
    switch (g->lv.theme) {
    case THEME_TOWN: *n = NBANTER_TOWN; return BANTER_TOWN;
    case THEME_CAVE: *n = NBANTER_CAVE; return BANTER_CAVE;
    case THEME_TEMPLE: *n = NBANTER_TEMPLE; return BANTER_TEMPLE;
    default: *n = NBANTER_COVE; return BANTER_COVE;
    }
}

void banter_update(Game *g, float dt)
{
    if (g->state != ST_PLAY) return;
    int n;
    const Banter *list = banter_list(g, &n);
    if (g->bubble_seq >= 0) {
        const Banter *b = &list[g->bubble_seq % n];
        int who = b->who[g->bubble_pos];
        if (g->kid[who].bubble_t <= 0) {
            g->bubble_gap -= dt;
            if (g->bubble_gap <= 0) {
                g->bubble_pos++;
                if (g->bubble_pos >= 3 || b->who[g->bubble_pos] < 0 || !b->text[g->bubble_pos]) { g->bubble_seq = -1; return; }
                kid_say(g, b->who[g->bubble_pos], b->text[g->bubble_pos], 0);
                g->bubble_gap = 0.35f;
            }
        }
        return;
    }
    for (int i = 0; i < NKIDS; i++) if (g->kid[i].bubble_t > 0) return;
    g->banter_t -= dt;
    if (g->banter_t > 0) return;
    g->banter_t = 24.0f + frand(g) * 14.0f;
    g->bubble_seq = g->banter_i++ % n;
    g->bubble_pos = 0;
    g->bubble_gap = 0.35f;
    kid_say(g, list[g->bubble_seq].who[0], list[g->bubble_seq].text[0], 0);
}
