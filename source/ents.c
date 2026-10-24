/*
 * TREASURE HOUNDS - everything in the world that isn't a kid: pickups, creatures, the Crowleys,
 * traps, puzzles, gates, chests and the final showdown on the Black Gull.
 */
#include "game_int.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static float dist2(float ax, float ay, float bx, float by) { return (ax - bx) * (ax - bx) + (ay - by) * (ay - by); }
static int ent_index(const Game *g, const Ent *e) { return (int)(e - g->ent); }

/* spike timing: down, rising, up */
int spike_state(const Game *g, int tx, int ty)
{
    float ph = fmodf(g->time + (float)((tx + ty) % 2) * 1.5f, 3.0f);
    if (ph < 1.6f) return 0;
    if (ph < 1.95f) return 1;
    return 2;
}

/* flame jets: idle, glowing, blasting */
int jet_state(const Ent *e, float time)
{
    float ph = fmodf(time + e->t, 3.2f);
    if (ph < 1.5f) return 0;
    if (ph < 2.0f) return 1;
    return 2;
}

static void drop_loot(Game *g, float x, float y, int coins, int food)
{
    for (int i = 0; i < coins; i++) {
        Ent *c = ent_new(g, E_COIN, x, y);
        if (!c) break;
        float a = frand(g) * TAU_F;
        c->vx = cosf(a) * (20 + frand(g) * 30); c->vy = sinf(a) * (14 + frand(g) * 20);
        c->vz = 90 + frand(g) * 60; c->z = 2; c->t2 = 0.35f;     /* can't be picked up until it lands */
    }
    if (food) {
        Ent *f = ent_new(g, E_FOOD, x, y);
        if (f) { f->vz = 100; f->z = 2; f->t2 = 0.35f; f->a = (int16_t)(frand(g) < 0.5f); }
    }
}

void open_gate(Game *g, int idx)
{
    if (idx < 0 || idx >= 8 || g->gate_open[idx]) return;
    g->gate_open[idx] = 1;
    for (int i = 0; i < g->nent; i++) {
        Ent *e = &g->ent[i];
        if (e->on && e->type == E_GATE && e->a == idx) {
            e->state = 1; e->t = 0;
            burst(g, PK_DUST, e->x, e->y - 4, 14, 50, 0x8A8070C0u);
            sfx_at(g, SFX_DOOR, e->x, e->y, 128);
        }
    }
    g->shake = fmaxf(g->shake, 5.0f);
    sfx(g, SFX_SOLVED, 128, 0, 200);
}

/* ------------------------------------------------------------------ */
/* puzzles                                                             */
/* ------------------------------------------------------------------ */
static int plates_in(Game *g, const MapGroup *gr, int *pressed)
{
    int n = 0, p = 0;
    for (int y = gr->y0; y <= gr->y1; y++)
        for (int x = gr->x0; x <= gr->x1; x++)
            if (tile_at(g, x, y) == K_PLATE) { n++; if (g->lv.st[y][x]) p++; }
    if (pressed) *pressed = p;
    return n;
}

static void update_plates(Game *g)
{
    /* clear unlatched plates, then press the ones with a kid or a block on them */
    for (int y = 0; y < g->lv.h; y++)
        for (int x = 0; x < g->lv.w; x++)
            if (g->lv.k[y][x] == K_PLATE && g->lv.st[y][x] == 1) g->lv.st[y][x] = 0;
    for (int i = 0; i < NKIDS; i++) {
        const Kid *k = &g->kid[i];
        if (k->z > 2) continue;
        int tx = (int)(k->x / TS), ty = (int)((k->y - 2) / TS);
        if (tile_at(g, tx, ty) == K_PLATE && g->lv.st[ty][tx] == 0) {
            g->lv.st[ty][tx] = 1;
        }
    }
    for (int i = 0; i < g->nent; i++) {
        Ent *e = &g->ent[i];
        if (!e->on || e->type != E_BLOCK || e->t2 > 0) continue;
        int tx = (int)(e->x / TS), ty = (int)((e->y - 2) / TS);
        if (tile_at(g, tx, ty) == K_PLATE && g->lv.st[ty][tx] == 0) g->lv.st[ty][tx] = 1;
    }
}

void puzzles_check(Game *g)
{
    static uint8_t prev[MAXH][MAXW];
    update_plates(g);
    /* click when a plate goes down */
    for (int y = 0; y < g->lv.h; y++)
        for (int x = 0; x < g->lv.w; x++)
            if (g->lv.k[y][x] == K_PLATE) {
                if (g->lv.st[y][x] && !prev[y][x]) sfx_at(g, SFX_CLICK, (float)(x * TS + 8), (float)(y * TS + 8), 128);
                prev[y][x] = g->lv.st[y][x];
            }
    const LevelData *ld = g->ld;
    for (int i = 0; i < ld->ngroup; i++) {
        const MapGroup *gr = &ld->group[i];
        if (gr->gate < 0 || g->gate_open[gr->gate]) continue;
        int ok = 0;
        switch (gr->cond) {
        case COND_PLATES: {
            int p, n = plates_in(g, gr, &p);
            ok = n > 0 && p == n;
            if (ok) {
                for (int y = gr->y0; y <= gr->y1; y++)
                    for (int x = gr->x0; x <= gr->x1; x++) if (tile_at(g, x, y) == K_PLATE) g->lv.st[y][x] = 2;   /* latch */
            }
            break;
        }
        case COND_BRAZIERS: {
            int n = 0, lit = 0;
            for (int k = 0; k < g->nent; k++) {
                Ent *e = &g->ent[k];
                if (!e->on || e->type != E_BRAZIER) continue;
                int tx = (int)(e->x / TS), ty = (int)((e->y - 2) / TS);
                if (tx < gr->x0 || tx > gr->x1 || ty < gr->y0 || ty > gr->y1) continue;
                n++; if (e->state) lit++;
            }
            ok = n > 0 && lit == n;
            break;
        }
        case COND_LEVER:
            for (int k = 0; k < g->nent; k++) {
                Ent *e = &g->ent[k];
                if (!e->on || e->type != E_LEVER || !e->state) continue;
                int tx = (int)(e->x / TS), ty = (int)((e->y - 2) / TS);
                if (tx >= gr->x0 && tx <= gr->x1 && ty >= gr->y0 && ty <= gr->y1) ok = 1;
            }
            break;
        case COND_ORGAN: ok = g->organ_pos >= ld->norgan && ld->norgan > 0; break;
        case COND_KEY: ok = g->has_key; break;
        case COND_BOSS: ok = 0; break;
        }
        if (!ok) continue;
        open_gate(g, gr->gate);
        party_regroup(g);
        if (gr->cond == COND_PLATES) start_scene(g, g->level == 1 ? SC_L2_PLATES_DONE : SC_L3_BLOCKS_DONE);
        if (gr->cond == COND_BRAZIERS) start_scene(g, SC_L2_BRAZIERS_DONE);
        if (gr->cond == COND_ORGAN) start_scene(g, SC_L3_ORGAN_DONE);
        if (gr->cond == COND_LEVER) kid_say(g, CH_GUS, "I heard a door open... back in the room with the blocks!", 2.6f);
    }
}

/* ------------------------------------------------------------------ */
/* hurting things                                                      */
/* ------------------------------------------------------------------ */
static int enemy_r(int type) { return type == E_BAT ? 7 : (type == E_RAT ? 6 : (type == E_CRAB ? 8 : 9)); }

void enemy_hurt(Game *g, Ent *e, int dmg, float kx, float ky)
{
    if (e->type == E_SKEL && e->state == 5) return;
    if ((e->type == E_VIC || e->type == E_LENNY) && e->state == 4) return;
    if (e->type == E_MA && (g->boss_phase < 1 || g->boss_down)) return;
    if (e->flash > 0.12f) return;
    e->hp -= dmg;
    e->flash = 0.25f;
    e->vx += kx * 130; e->vy += ky * 130;
    sfx_at(g, SFX_ENEMYHIT, e->x, e->y, 110 + (int)(frand(g) * 30));
    burst(g, PK_SPARK, e->x, e->y - 10 - e->z, 6, 70, 0xFFF0C0FFu);
    g->shake = fmaxf(g->shake, 2.0f);
    if (e->hp > 0) {
        if (e->type == E_VIC || e->type == E_LENNY) { e->state = 2; e->t = 0; }
        return;
    }
    switch (e->type) {
    case E_BAT: case E_CRAB: case E_RAT:
        burst(g, PK_SMOKE, e->x, e->y - 6 - e->z, 8, 30, 0xC8C0D0B0u);
        if (e->type == E_BAT) burst(g, PK_LEAF, e->x, e->y - e->z - 8, 5, 40, 0x3A2A44FFu);
        sfx_at(g, SFX_POOF, e->x, e->y, 128);
        if (frand(g) < 0.45f) drop_loot(g, e->x, e->y, 1, 0);
        e->on = 0; e->type = E_NONE;
        break;
    case E_SKEL:
        burst(g, PK_BONE, e->x, e->y - 12, 9, 70, 0xE0D8C0FFu);
        sfx_at(g, SFX_RATTLE, e->x, e->y, 100);
        if (e->b) {                        /* already got back up once: stays down for good */
            e->type = E_BONES; e->state = 0;
            if (frand(g) < 0.6f) drop_loot(g, e->x, e->y, 2, 0);
        } else { e->state = 5; e->t = 6.0f; e->b = 1; }
        break;
    case E_VIC: case E_LENNY:
        e->state = 4; e->t = 6.0f; e->hp = 3;
        sfx_at(g, SFX_DIZZY, e->x, e->y, 128);
        if (frand(g) < 0.7f) {
            const char *s = e->type == E_VIC ? SAY_DIZZY_VIC[(int)(frand(g) * 3) % 3] : SAY_DIZZY_LENNY[(int)(frand(g) * 3) % 3];
            snprintf(g->toast, sizeof(g->toast), "%s: %s", e->type == E_VIC ? "Vic" : "Lenny", s);
            g->toast_t = 2.0f;
        }
        break;
    case E_MA:
        e->hp = 0;
        g->boss_down = 1;
        e->state = 9;
        sfx(g, SFX_DIZZY, 100, 0, 200);
        start_scene(g, SC_L4_BOSS_DOWN);
        break;
    }
}

/* break crates, pots and bushes, hurt creatures, play bone pipes, grab hooks */
void ents_whip(Game *g, float x0, float y0, float x1, float y1)
{
    Kid *l = leader(g);
    float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy;
    float nx = dx / sqrtf(len2), ny = dy / sqrtf(len2);
    int hit_any = 0;
    for (int i = 0; i < g->nent; i++) {
        Ent *e = &g->ent[i];
        if (!e->on) continue;
        const EntInfo *in = &EINFO[e->type];
        int enemy = in->flags & EF_ENEMY;
        int brk = (in->flags & EF_BREAK) && !e->state;
        int pipe = e->type == E_PIPE, hook = e->type == E_HOOK, lever = e->type == E_LEVER && !e->state;
        if (!enemy && !brk && !pipe && !hook && !lever) continue;
        float cy = e->y - (enemy ? 8 + e->z : (hook ? 18 : 6));
        float t = ((e->x - x0) * dx + (cy - y0) * dy) / len2;
        t = clampf(t, 0, 1);
        float px = x0 + dx * t, py = y0 + dy * t;
        float rad = enemy ? (float)enemy_r(e->type) + 4 : (hook ? 12 : (float)(in->fw > 4 ? in->fw : 6) + 3);
        if (e->type == E_MA) rad = 13;
        if (dist2(px, py, e->x, cy) > rad * rad) continue;
        if (enemy) {
            if (e->type == E_TINY) continue;
            enemy_hurt(g, e, 1, nx, ny);
            hit_any = 1;
        } else if (brk) {
            e->state = 1;
            hit_any = 1;
            uint32_t h = (uint32_t)(e->x * 13 + e->y * 7) * 2654435761u;
            if (e->type == E_BUSH) {
                burst(g, PK_LEAF, e->x, e->y - 6, 12, 60, 0x3E6A30FFu);
                sfx_at(g, SFX_RUSTLE, e->x, e->y, 128);
                if ((h >> 7) % 6 == 0) drop_loot(g, e->x, e->y - 2, 1, 0);
            } else {
                burst(g, PK_DEBRIS, e->x, e->y - 8, 12, 80, e->type == E_POT ? 0x9A5A3AFFu : 0x8A6438FFu);
                sfx_at(g, SFX_BREAK, e->x, e->y, e->type == E_POT ? 150 : 110);
                int coins = (h >> 5) % 10 < 5 ? 1 + (int)((h >> 9) % 3) : 0;
                int food = (h >> 13) % 9 == 0;
                drop_loot(g, e->x, e->y - 2, coins, food);
            }
        } else if (lever) {
            hit_any = 1;
            e->state = 1;
            sfx_at(g, SFX_LEVER, e->x, e->y, 128);
            burst(g, PK_SPARK, e->x, e->y - 10, 8, 50, 0xFFE8A0FFu);
        } else if (pipe) {
            hit_any = 1;
            e->flash = 0.5f;
            static const int note_pitch[5] = { 96, 108, 121, 136, 152 };
            sfx_at(g, SFX_BONE, e->x, e->y, note_pitch[e->a % 5]);
            burst(g, PK_MOTE, e->x, e->y - 18, 6, 30, 0xFFF0C0FFu);
            const LevelData *ld = g->ld;
            if (ld->norgan > 0 && g->organ_pos < ld->norgan) {
                if (e->a == ld->organ[g->organ_pos]) {
                    g->organ_pos++;
                    Part *p = part_new(g, PK_TEXT, e->x, e->y - 30);
                    if (p) { p->val = (int16_t)g->organ_pos; p->c = 0xFFE890FFu; p->life = p->max = 1.0f; p->vy = -20; }
                    if (g->organ_pos >= ld->norgan) sfx(g, SFX_FANFARE, 128, 0, 160);
                } else {
                    g->organ_pos = 0;
                    g->organ_fail_t = 0.4f;          /* the darters fire in a moment */
                    sfx(g, SFX_WRONG, 128, 0, 200);
                    kid_say(g, CH_ROCCO, "Wrong note! DARTS! I TOLD you it'd be darts!", 2.0f);
                }
            }
        } else if (hook) {
            const MapHook *mh = &g->ld->hook[e->a];
            float d = sqrtf(dist2(l->x, l->y, e->x, e->y));
            if (d < 60) {
                hit_any = 1;
                burst(g, PK_SPARK, e->x, e->y - 18, 8, 60, 0xFFFFFFFFu);
                party_swing(g, (float)(mh->lx * TS + 8), (float)(mh->ly * TS + 10), e->x, e->y - 18);
                g->whip_t = 0;
                return;
            }
        }
    }
    (void)hit_any;
}

/* ------------------------------------------------------------------ */
/* A: talk, read, open, light                                          */
/* ------------------------------------------------------------------ */
static void spread_on_plates(Game *g)
{
    Kid *l = leader(g);
    int ltx = (int)(l->x / TS), lty = (int)((l->y - 2) / TS);
    int found = 0, px[8], py[8], n = 0;
    for (int i = 0; i < g->ld->ngroup; i++) {
        const MapGroup *gr = &g->ld->group[i];
        if (gr->cond != COND_PLATES || g->gate_open[gr->gate]) continue;
        if (ltx < gr->x0 || ltx > gr->x1 || lty < gr->y0 || lty > gr->y1) continue;
        for (int y = gr->y0; y <= gr->y1; y++)
            for (int x = gr->x0; x <= gr->x1; x++)
                if (tile_at(g, x, y) == K_PLATE && !(x == ltx && y == lty) && n < 8) { px[n] = x; py[n] = y; n++; }
        found = 1;
    }
    if (!found) return;
    for (int i = 1; i < NKIDS && i - 1 < n; i++) {
        int k = g->order[i];
        float tx = (float)(px[i - 1] * TS + 8), ty = (float)(py[i - 1] * TS + 12);
        float d = sqrtf(dist2(g->kid[k].x, g->kid[k].y, tx, ty));
        party_walk_to(g, k, tx, ty, d / 70.0f + 0.1f, 1);
    }
    kid_say(g, CH_GUS, "Everybody grab a plate!", 1.6f);
    sfx(g, SFX_SELECT, 128, 0, 140);
}

int ents_use(Game *g, float x, float y, int dir)
{
    Kid *l = leader(g);
    /* anybody holding a plate? call them back */
    for (int i = 1; i < NKIDS; i++)
        if (g->kid[g->order[i]].state == KS_HOLD) { party_regroup(g); trail_rebuild(g); kid_say(g, CH_SAM, "Hounds, back in line!", 1.2f); return 1; }
    int ltx = (int)(l->x / TS), lty = (int)((l->y - 2) / TS);
    if (tile_at(g, ltx, lty) == K_PLATE && g->lv.st[lty][ltx] != 2) { spread_on_plates(g); return 1; }

    float fx = 0, fy = 0;
    if (dir == DIR_SIDE) fx = l->flip ? -1.0f : 1.0f; else fy = dir == DIR_UP ? -1.0f : 1.0f;
    float px = x + fx * 12, py = y + fy * 12 - 3;
    Ent *best = 0;
    float bd = 26 * 26;
    for (int i = 0; i < g->nent; i++) {
        Ent *e = &g->ent[i];
        if (!e->on) continue;
        int t = e->type;
        if (!(EINFO[t].flags & EF_USE) && t != E_TINY) continue;
        float ey = e->y - (t == E_GATE ? 6 : 4);
        float d = dist2(px, py, e->x, ey);
        if (t == E_SHACK || t == E_CARVING || t == E_BIGCHEST || t == E_CAGE) d = dist2(px, py, e->x, e->y - 4) * 0.55f;
        if (d < bd) { bd = d; best = e; }
    }
    if (!best) return 0;
    Ent *e = best;
    switch (e->type) {
    case E_SIGN: case E_CARVING:
        g->scene_after = e->a;
        start_scene(g, e->type == E_CARVING ? SC_CARVING : SC_SIGN);
        return 1;
    case E_CHEST:
        if (e->state) return 0;
        e->state = 1;
        sfx_at(g, SFX_CHEST, e->x, e->y, 128);
        burst(g, PK_SPARK, e->x, e->y - 10, 16, 60, 0xFFE070FFu);
        drop_loot(g, e->x, e->y - 4, 6 + (int)(frand(g) * 4), 0);
        {
            Ent *gm = ent_new(g, E_GEM, e->x, e->y - 4);
            if (gm) { gm->vz = 120; gm->z = 4; gm->t2 = 0.4f; gm->vy = 20; gm->a = (int16_t)((int)(frand(g) * 3) % 3); }
        }
        return 1;
    case E_BIGCHEST:
        if (e->state) return 0;
        if (!g->boss_down) { kid_say(g, leader_id(g), "Ma Crowley's not gonna let us near it!", 1.8f); return 1; }
        start_scene(g, SC_L4_CHEST);
        return 1;
    case E_CRANE:
        if (g->crane_done) return 0;
        start_scene(g, g->gears >= 3 ? SC_CRANE_DONE : SC_CRANE_NEED);
        return 1;
    case E_BRAZIER:
        if (e->state) return 0;
        e->state = 1;
        sfx_at(g, SFX_IGNITE, e->x, e->y, 128);
        burst(g, PK_EMBER, e->x, e->y - 12, 14, 50, 0xFFA040FFu);
        g->braziers_lit++;
        {
            char buf[48];
            int n = 0, lit = 0;
            for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_BRAZIER) { n++; if (g->ent[i].state) lit++; }
            if (lit < n) { snprintf(buf, sizeof buf, "%d down, %d to go!", lit, n - lit); kid_say(g, leader_id(g), buf, 1.4f); }
        }
        return 1;
    case E_CAGE: case E_TINY:
        if (g->level == 0) {
            if (!g->trig_done[63]) { g->trig_done[63] = 1; start_scene(g, SC_L1_TINY); }
            else kid_say(g, CH_OLLIE, "Hang in there, Tiny! We'll be back!", 1.8f);
            return 1;
        }
        if (e->type == E_TINY) { kid_say(g, CH_OLLIE, "Tiny! Best buddy!", 1.4f); return 1; }
        return 0;
    case E_GATE:
        if (e->state) return 0;
        start_scene(g, SC_GATE_LOCKED);
        return 1;
    case E_LEVER:
        if (e->state) return 0;
        e->state = 1;
        sfx_at(g, SFX_LEVER, e->x, e->y, 128);
        burst(g, PK_SPARK, e->x, e->y - 10, 8, 50, 0xFFE8A0FFu);
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* per-type updates                                                    */
/* ------------------------------------------------------------------ */
static void pickup(Game *g, Ent *e)
{
    char buf[80];
    switch (e->type) {
    case E_COIN:
        g->coins++; g->lvl_coins++; g->hud_coin_t = 0.4f;
        sfx_at(g, SFX_COIN, e->x, e->y, 128 + (g->coins % 5) * 6);
        burst(g, PK_SPARK, e->x, e->y - 6, 5, 40, 0xFFE070FFu);
        break;
    case E_GEM:
        g->coins += 10; g->lvl_coins += 10; g->gems++; g->hud_coin_t = 0.6f;
        sfx_at(g, SFX_GEM, e->x, e->y, 128);
        burst(g, PK_SPARK, e->x, e->y - 6, 10, 60, 0xFFFFFFFFu);
        float_text(g, e->x, e->y - 16, "+10", 0xFFE070FFu);
        break;
    case E_GEAR:
        g->gears++;
        sfx(g, SFX_ITEM, 128, 0, 220);
        burst(g, PK_SPARK, e->x, e->y - 6, 16, 70, 0xFFD080FFu);
        if (g->gears >= 3) snprintf(buf, sizeof buf, "That's all three gears! To the winch by the lighthouse!");
        else snprintf(buf, sizeof buf, "Gear number %d! %d more to go!", g->gears, 3 - g->gears);
        kid_say(g, CH_GUS, buf, 2.4f);
        snprintf(g->toast, sizeof g->toast, "Found a rusty gear  (%d / 3)", g->gears);
        g->toast_t = 2.5f;
        break;
    case E_DOUBLOON:
        g->doubloons++; g->lvl_doubloons++; g->coins += 25; g->lvl_coins += 25;
        sfx(g, SFX_ITEM, 140, 0, 220);
        burst(g, PK_SPARK, e->x, e->y - 6, 20, 80, 0xFFE070FFu);
        kid_say(g, leader_id(g), SAY_DOUBLOON[leader_id(g)], 2.0f);
        snprintf(g->toast, sizeof g->toast, "Ironjaw's doubloon!  (%d / 3 in this chapter)", g->lvl_doubloons);
        g->toast_t = 2.5f;
        break;
    case E_FOOD:
        party_heal(g, 2);
        sfx(g, SFX_EAT, 128, 0, 200);
        burst(g, PK_HEART, e->x, e->y - 10, 6, 30, 0xFF5060FFu);
        if (frand(g) < 0.35f) { kid_say(g, CH_OLLIE, "*BUUURP* ...Excuse me.", 1.6f); sfx(g, SFX_BURP, 110 + (int)(frand(g) * 30), 0, 200); }
        else kid_say(g, CH_OLLIE, SAY_FOOD[(int)(frand(g) * 4) & 3], 1.4f);
        break;
    case E_SKULLKEY:
        g->has_key = 1;
        sfx(g, SFX_ITEM, 110, 0, 255);
        burst(g, PK_SPARK, e->x, e->y - 6, 30, 90, 0xFFE070FFu);
        start_scene(g, SC_L3_KEY);
        break;
    }
    e->on = 0; e->type = E_NONE;
}

static int line_clear(const Game *g, float ax, float ay, float bx, float by)
{
    for (int i = 1; i < 8; i++) {
        float t = (float)i / 8.0f;
        if (solid_at(g, lerpf(ax, bx, t), lerpf(ay, by, t))) return 0;
    }
    return 1;
}

static void seek(Game *g, Ent *e, float tx, float ty, float spd, float dt)
{
    float dx = tx - e->x, dy = ty - e->y, d = sqrtf(dx * dx + dy * dy);
    if (d < 0.5f) return;
    float mx = dx / d * spd * dt, my = dy / d * spd * dt;
    move_circle(g, &e->x, &e->y, mx, my, 5, ent_index(g, e));
    if (fabsf(dx) > fabsf(dy)) { e->dir = DIR_SIDE; e->flip = dx < 0; }
    else e->dir = dy < 0 ? DIR_UP : DIR_DOWN;
}

static void touch_damage(Game *g, Ent *e, float r, int dmg)
{
    Kid *l = leader(g);
    if (l->z > 6) return;
    if (dist2(l->x, l->y - 6, e->x, e->y - 6 - (e->type == E_BAT ? 0 : 0)) < r * r) hurt_leader(g, dmg, e->x, e->y);
}

static void thug_update(Game *g, Ent *e, float dt)
{
    Kid *l = leader(g);
    float d = sqrtf(dist2(e->x, e->y, l->x, l->y));
    float spd = 0;
    int boss = g->boss_phase >= 1 && g->level == 3;
    switch (e->state) {
    case 0: {                               /* patrol back and forth along the home row */
        if (e->vx == 0 && e->vy == 0 && e->c == 0) e->c = 1;
        float tx = e->hx + (e->c > 0 ? 48.0f : -48.0f);
        float ox = e->x;
        seek(g, e, tx, e->hy, 26, dt);
        spd = 26;
        if (fabsf(e->x - tx) < 2 || fabsf(e->x - ox) < 0.05f) { e->c = (int16_t)-e->c; e->t2 = 0.8f; }
        /* spotting the kids in the flashlight */
        float fx = e->dir == DIR_SIDE ? (e->flip ? -1.0f : 1.0f) : 0, fy = e->dir == DIR_SIDE ? 0 : (e->dir == DIR_UP ? -1.0f : 1.0f);
        float vx = l->x - e->x, vy = l->y - e->y;
        float dot = (vx * fx + vy * fy) / (d + 0.01f);
        if ((d < 96 && dot > 0.82f && line_clear(g, e->x, e->y - 8, l->x, l->y - 8)) || d < 26 || boss) {
            e->state = 1; e->t = 0.55f;
            sfx_at(g, SFX_HEY, e->x, e->y, e->type == E_VIC ? 120 : 100);
            const char *s = e->type == E_VIC ? SAY_SPOT_VIC[(int)(frand(g) * 3) % 3] : SAY_SPOT_LENNY[(int)(frand(g) * 3) % 3];
            snprintf(g->toast, sizeof(g->toast), "%s: %s", e->type == E_VIC ? "Vic" : "Lenny", s);
            g->toast_t = 1.8f;
        }
        break;
    }
    case 1:                                  /* "!" */
        e->t -= dt;
        if (e->t <= 0) { e->state = 2; e->t = 0; }
        break;
    case 2:                                  /* chase */
        spd = boss ? 66 : 76;
        seek(g, e, l->x, l->y, spd, dt);
        if (d > 170 && !boss) { e->t += dt; if (e->t > 2.5f) { e->state = 3; } } else e->t = 0;
        touch_damage(g, e, 12, 1);
        break;
    case 3:                                  /* walk home */
        seek(g, e, e->hx, e->hy, 34, dt);
        spd = 34;
        if (dist2(e->x, e->y, e->hx, e->hy) < 9) { e->state = 0; }
        if (d < 40 && line_clear(g, e->x, e->y - 8, l->x, l->y - 8)) { e->state = 1; e->t = 0.4f; }
        break;
    case 4:                                  /* seeing stars */
        e->t -= dt;
        if (e->t <= 0) { e->state = boss ? 2 : 3; e->t = 0; }
        break;
    }
    e->t3 += dt * (spd > 0 ? spd / 9.0f : 1.6f);
}

static void ma_update(Game *g, Ent *e, float dt)
{
    Kid *l = leader(g);
    e->t3 += dt * 1.6f;
    if (g->boss_phase < 1 || g->boss_down) {
        e->dir = DIR_SIDE; e->flip = l->x < e->x;
        return;
    }
    e->t -= dt;
    switch (e->state) {
    case 0:                                  /* lob bottles */
        e->dir = DIR_SIDE; e->flip = l->x < e->x;
        if (e->t <= 0) {
            Ent *b = ent_new(g, E_BOTTLE, e->x, e->y - 4);
            if (b) {
                b->hx = e->x; b->hy = e->y - 4;
                b->vx = l->x + g->lead_vx * 0.1f + frand2(g) * 10; b->vy = l->y + frand2(g) * 8;
                b->t = 0; b->t2 = 0.95f;
            }
            sfx_at(g, SFX_THROW, e->x, e->y, 120);
            e->c++;
            e->t = 1.15f;
            if (e->c >= 3) { e->state = 1; e->t = 3.6f; e->c = 0; snprintf(g->toast, sizeof g->toast, "Ma: Outta bottles! C'mere, you!"); g->toast_t = 1.6f; }
        }
        break;
    case 1:                                  /* stomps after you: whip her now! */
        seek(g, e, l->x, l->y, 64, dt);
        e->t3 += dt * 4;
        touch_damage(g, e, 13, 1);
        if (e->t <= 0) { e->state = 2; e->t = 0; }
        break;
    case 2:                                  /* back to her spot */
        seek(g, e, e->hx, e->hy, 70, dt);
        if (dist2(e->x, e->y, e->hx, e->hy) < 16) { e->state = 0; e->t = 0.8f; }
        break;
    }
}

static void fire_dart(Game *g, Ent *e)
{
    Ent *d = ent_new(g, E_DART, e->x, e->y + (e->dir == DIR_DOWN ? 2 : -14));
    if (!d) return;
    d->vy = e->dir == DIR_DOWN ? 150 : -150;
    d->dir = e->dir;
    sfx_at(g, SFX_DART, e->x, e->y, 128);
    e->flash = 0.3f;
}

void ents_update(Game *g, float dt)
{
    Kid *l = leader(g);
    int leader_tx = (int)(l->x / TS), leader_ty = (int)((l->y - 2) / TS);

    /* crumbling floors */
    if (g->state == ST_PLAY) for (int y = 0; y < g->lv.h; y++)
        for (int x = 0; x < g->lv.w; x++) {
            int k = g->lv.k[y][x];
            if (k == K_CRUMBLE) {
                if (g->lv.st[y][x] == 0 && x == leader_tx && y == leader_ty && l->z < 1 && l->state == KS_FOLLOW) {
                    g->lv.st[y][x] = 1; g->lv.tt[y][x] = 0;
                    sfx_at(g, SFX_CRUMBLE, (float)(x * TS + 8), (float)(y * TS + 8), 170);
                }
                if (g->lv.st[y][x] > 0) {
                    g->lv.tt[y][x] += dt;
                    if (g->lv.tt[y][x] > 0.3f) g->lv.st[y][x] = 2;
                    if (g->lv.tt[y][x] > 0.62f) {
                        g->lv.k[y][x] = K_HOLE; g->lv.st[y][x] = 3; g->lv.tt[y][x] = 0;
                        burst(g, PK_DEBRIS, (float)(x * TS + 8), (float)(y * TS + 10), 8, 40, 0x6A6656FFu);
                        sfx_at(g, SFX_CRUMBLE, (float)(x * TS + 8), (float)(y * TS + 8), 100);
                        if (x == leader_tx && y == leader_ty && l->z < 1 && l->state == KS_FOLLOW) party_fall(g);
                    }
                }
            } else if (k == K_HOLE) {
                g->lv.tt[y][x] += dt;                     /* the old boards settle back into place */
                int near = abs(x - leader_tx) < 2 && abs(y - leader_ty) < 2;
                if (g->lv.tt[y][x] > 4.0f && !near) { g->lv.k[y][x] = K_CRUMBLE; g->lv.st[y][x] = 0; g->lv.tt[y][x] = 0; }
            }
        }
    /* spikes */
    if (tile_at(g, leader_tx, leader_ty) == K_SPIKES && spike_state(g, leader_tx, leader_ty) == 2 && l->z < 2) hurt_leader(g, 1, l->x - 3, l->y - 6);
    {
        static int prev_up;
        int any = 0;
        for (int dy = -6; dy <= 6 && !any; dy++)
            for (int dx = -9; dx <= 9; dx++)
                if (tile_at(g, leader_tx + dx, leader_ty + dy) == K_SPIKES && spike_state(g, leader_tx + dx, leader_ty + dy) == 2) { any = 1; break; }
        if (any && !prev_up) sfx(g, SFX_SPIKES, 128, 0, 110);
        prev_up = any;
    }
    /* the organ's punishment */
    if (g->organ_fail_t > 0) {
        g->organ_fail_t -= dt;
        if (g->organ_fail_t <= 0)
            for (int i = 0; i < g->nent; i++) if (g->ent[i].on && g->ent[i].type == E_DARTER && g->ent[i].c) fire_dart(g, &g->ent[i]);
    }

    int cutscene = g->state != ST_PLAY;
    for (int i = 0; i < g->nent; i++) {
        Ent *e = &g->ent[i];
        if (!e->on) continue;
        if (e->flash > 0) e->flash -= dt;
        /* during conversations the bad guys and the traps hold still */
        if (cutscene && ((EINFO[e->type].flags & EF_ENEMY) || e->type == E_DARTER || e->type == E_DART || e->type == E_FLAMEJET ||
                         e->type == E_BOULDER || e->type == E_BOTTLE)) continue;
        float near2 = dist2(e->x, e->y, l->x, l->y);
        int active = near2 < 330.0f * 330.0f;
        switch (e->type) {
        case E_COIN: case E_GEM: case E_GEAR: case E_DOUBLOON: case E_FOOD: case E_SKULLKEY:
            e->t += dt;
            if (e->vz != 0 || e->z > 0) {          /* popped out of a crate */
                e->z += e->vz * dt; e->vz -= 380 * dt;
                float nx = e->x + e->vx * dt, ny = e->y + e->vy * dt;
                if (!solid_at(g, nx, ny)) { e->x = nx; e->y = ny; }
                if (e->z <= 0) {
                    e->z = 0;
                    if (e->vz < -60) { e->vz = -e->vz * 0.4f; sfx_at(g, SFX_COIN, e->x, e->y, 200); }
                    else { e->vz = 0; e->vx = e->vy = 0; }
                    e->vx *= 0.6f; e->vy *= 0.6f;
                }
            }
            if (e->t2 > 0) { e->t2 -= dt; break; }
            for (int k = 0; k < NKIDS; k++) {
                const Kid *kd = &g->kid[k];
                float r = e->type == E_COIN ? 11 : 12;
                if (dist2(kd->x, kd->y - 4, e->x, e->y - 4) < r * r && kd->state != KS_FALL) { pickup(g, e); break; }
            }
            break;
        case E_BAT: {
            if (!active) break;
            e->t3 += dt * (e->state ? 14 : 3);
            float d = sqrtf(near2);
            if (e->state == 0) {
                e->z = 20;
                if (d < 72) { e->state = 1; sfx_at(g, SFX_BAT, e->x, e->y, 128 + (int)(frand(g) * 30)); }
            } else if (e->state == 1) {
                e->t += dt;
                float wob = sinf(e->t * 6) * 30;
                float tx = l->x + cosf(e->t * 2.3f) * 4 + wob * 0.4f, ty = l->y - 4;
                float dx = tx - e->x, dy = ty - e->y, dd = sqrtf(dx * dx + dy * dy) + 0.01f;
                e->x += dx / dd * 62 * dt; e->y += dy / dd * 62 * dt + sinf(e->t * 9) * 0.6f;
                e->z = 10 + sinf(e->t * 5) * 3;
                if (dd < 10) { touch_damage(g, e, 13, 1); e->state = 2; e->t = 1.2f; }
            } else {
                e->t -= dt;
                float dx = e->x - l->x, dy = e->y - l->y, dd = sqrtf(dx * dx + dy * dy) + 0.01f;
                e->x += dx / dd * 50 * dt; e->y += dy / dd * 50 * dt;
                e->z = 14;
                if (e->t <= 0) e->state = 1;
            }
            e->x += e->vx * dt; e->y += e->vy * dt; e->vx *= 0.9f; e->vy *= 0.9f;
            if (solid_at(g, e->x, e->y)) { e->x -= e->vx * dt * 2; e->y -= e->vy * dt * 2; }
            break;
        }
        case E_CRAB: case E_RAT: {
            if (!active) break;
            float d = sqrtf(near2);
            float spd = e->type == E_CRAB ? 26 : 34;
            e->t += dt;
            if (d < (e->type == E_CRAB ? 64 : 70)) {
                seek(g, e, l->x, l->y, e->type == E_CRAB ? 36 : 54, dt);
                spd = 40;
                if (e->type == E_CRAB && fabsf(l->x - e->x) > 4) { e->dir = DIR_SIDE; }
            } else {
                if (e->t > 1.6f) { e->t = 0; e->c = (int16_t)(frand(g) * 4); }
                static const float wx[4] = { 1, -1, 0, 0 }, wy[4] = { 0, 0, 1, -1 };
                float mx = e->type == E_CRAB ? wx[e->c & 1] : wx[e->c & 3], my = e->type == E_CRAB ? 0 : wy[e->c & 3];
                move_circle(g, &e->x, &e->y, mx * spd * dt * 0.6f, my * spd * dt * 0.6f, 5, i);
                if (mx != 0) { e->dir = DIR_SIDE; e->flip = mx < 0; }
            }
            move_circle(g, &e->x, &e->y, e->vx * dt, e->vy * dt, 5, i);
            e->vx *= 0.85f; e->vy *= 0.85f;
            e->t3 += dt * (spd / 6.0f);
            touch_damage(g, e, 10, 1);
            if (e->type == E_CRAB && frand(g) < dt * 0.3f && d < 120) sfx_at(g, SFX_CRAB, e->x, e->y, 128);
            if (e->type == E_RAT && frand(g) < dt * 0.25f && d < 120) sfx_at(g, SFX_SQUEAK, e->x, e->y, 128 + (int)(frand(g) * 40));
            break;
        }
        case E_SKEL: {
            if (!active) break;
            float d = sqrtf(near2);
            move_circle(g, &e->x, &e->y, e->vx * dt, e->vy * dt, 5, i);
            e->vx *= 0.85f; e->vy *= 0.85f;
            if (e->state == 5) {                       /* a pile of bones... for now */
                e->t -= dt;
                if (e->t < 1.0f) e->t3 += dt * 20;
                if (e->t <= 0) { e->state = 1; e->hp = 3; sfx_at(g, SFX_RATTLE, e->x, e->y, 140); burst(g, PK_DUST, e->x, e->y, 8, 30, 0xA09880A0u); }
                break;
            }
            if (e->state == 0) {
                e->t += dt;
                e->t3 += dt * 1.5f;
                if (d < 100 && line_clear(g, e->x, e->y - 8, l->x, l->y - 8)) { e->state = 1; sfx_at(g, SFX_RATTLE, e->x, e->y, 128); }
            } else if (e->state == 1) {               /* shamble toward */
                seek(g, e, l->x, l->y, 38, dt);
                e->t3 += dt * 5;
                if (d < 22) { e->state = 2; e->t = 0.45f; }
                if (d > 180) e->state = 0;
                touch_damage(g, e, 9, 1);
            } else if (e->state == 2) {               /* wind-up */
                e->t -= dt;
                if (e->t <= 0) {
                    e->state = 3; e->t = 0.25f;
                    float dx = l->x - e->x, dy = l->y - e->y, dd = sqrtf(dx * dx + dy * dy) + 0.01f;
                    e->vx = dx / dd * 120; e->vy = dy / dd * 120;
                    sfx_at(g, SFX_WHIP, e->x, e->y, 80);
                }
            } else if (e->state == 3) {               /* the swing */
                e->t -= dt;
                if (sqrtf(dist2(e->x, e->y, l->x, l->y)) < 20) hurt_leader(g, 1, e->x, e->y);
                if (e->t <= 0) { e->state = 4; e->t = 0.8f; }
            } else if (e->state == 4) {
                e->t -= dt;
                if (e->t <= 0) e->state = 1;
            }
            break;
        }
        case E_VIC: case E_LENNY:
            if (!active && e->state == 0) break;
            thug_update(g, e, dt);
            move_circle(g, &e->x, &e->y, e->vx * dt, e->vy * dt, 5, i);
            e->vx *= 0.85f; e->vy *= 0.85f;
            break;
        case E_MA:
            ma_update(g, e, dt);
            move_circle(g, &e->x, &e->y, e->vx * dt, e->vy * dt, 6, i);
            e->vx *= 0.85f; e->vy *= 0.85f;
            break;
        case E_TINY:
            e->t3 += dt;
            if (e->state == 2) {                       /* storming across the deck */
                seek(g, e, e->hx, e->hy, 80, dt);
                e->t3 += dt * 6;
                if (dist2(e->x, e->y, e->hx, e->hy) < 9) e->state = 3;
                if (frand(g) < dt * 6) g->shake = fmaxf(g->shake, 2.0f);
            }
            if (e->t > 0) {                            /* hearts */
                e->t -= dt;
                if (frand(g) < dt * 5) { Part *p = part_new(g, PK_HEART, e->x + frand2(g) * 10, e->y - 40); if (p) { p->vy = -20; p->life = p->max = 1.2f; } }
            }
            break;
        case E_BOTTLE: {
            e->t += dt;
            float t = e->t / e->t2;
            if (t >= 1) {
                burst(g, PK_SHARD, e->vx, e->vy, 12, 60, 0x60C070E0u);
                sfx_at(g, SFX_BOTTLE, e->vx, e->vy, 128 + (int)(frand(g) * 20));
                if (dist2(l->x, l->y, e->vx, e->vy) < 15 * 15) hurt_leader(g, 1, e->vx, e->vy);
                e->on = 0; e->type = E_NONE;
                break;
            }
            e->x = lerpf(e->hx, e->vx, t); e->y = lerpf(e->hy, e->vy, t);
            e->z = sinf(t * PI_F) * 60 + 10 * (1 - t);
            e->t3 += dt * 14;
            break;
        }
        case E_DARTER:
            if (e->c) break;                          /* organ darters wait for a wrong note */
            if (!active) break;
            e->t += dt;
            if (e->t > 2.4f) { e->t = 0; fire_dart(g, e); }
            break;
        case E_DART: {
            e->y += e->vy * dt;
            e->t += dt;
            if (dist2(e->x, e->y, l->x, l->y - 8) < 9 * 9) { hurt_leader(g, 1, e->x, e->y - 8); e->on = 0; e->type = E_NONE; break; }
            if ((e->t > 0.08f && solid_at(g, e->x, e->y + (e->vy > 0 ? 2 : -2))) || e->t > 3) {
                burst(g, PK_SPARK, e->x, e->y, 4, 30, 0xC0C0C0FFu);
                e->on = 0; e->type = E_NONE;
            }
            break;
        }
        case E_FLAMEJET: {
            if (!active) break;
            int s = jet_state(e, g->time);
            if (s == 2) {
                if (e->state != 2) sfx_at(g, SFX_FLAME, e->x, e->y, 128);
                if (fabsf(l->x - e->x) < 9 && l->y > e->y - 6 && l->y < e->y + 12) hurt_leader(g, 1, e->x, e->y + 12);
                if (frand(g) < dt * 30) {
                    Part *p = part_new(g, PK_FLAME, e->x + frand2(g) * 3, e->y - 2);
                    if (p) { p->vz = 70 + frand(g) * 40; p->life = p->max = 0.45f; p->size = 5; p->add = 1; }
                }
            }
            e->state = (uint8_t)s;
            break;
        }
        case E_BOULDER: {
            const MapBoulder *mb = &g->ld->boul[e->a];
            if (e->state == 0) {
                if (leader_tx >= mb->tx0 && leader_tx <= mb->tx1 && leader_ty >= mb->ty0 && leader_ty <= mb->ty1) {
                    e->state = 1; e->vx = 0; e->vy = 0;
                    g->shake = 6;
                    sfx(g, SFX_RUMBLE, 100, 0, 255);
                    kid_say(g, CH_ROCCO, "BOULDER!!! RUN!!!", 2.0f);
                    burst(g, PK_DUST, e->x, e->y - 10, 20, 60, 0x9A9080C0u);
                }
            } else if (e->state == 1) {
                float sp = fminf(96.0f, sqrtf(e->vx * e->vx + e->vy * e->vy) + 160 * dt);
                e->vx = (float)mb->dx * sp; e->vy = (float)mb->dy * sp;
                float nx = e->x + e->vx * dt, ny = e->y + e->vy * dt;
                e->t2 += sp * dt / 14.0f;            /* rolling angle */
                g->shake = fmaxf(g->shake, 2.5f);
                e->t += dt;
                if (e->t > 0.45f) { e->t = 0; sfx_at(g, SFX_RUMBLE, e->x, e->y, 90); }
                if (frand(g) < dt * 20) burst(g, PK_DUST, e->x + frand2(g) * 10, e->y, 1, 20, 0x9A9080A0u);
                if (solid_at(g, nx + (float)mb->dx * 13, ny + (float)mb->dy * 2)) {
                    e->state = 2;
                    g->shake = 10;
                    sfx_at(g, SFX_CRUMBLE, e->x, e->y, 70);
                    burst(g, PK_DEBRIS, e->x, e->y - 10, 20, 90, 0x6E6862FFu);
                } else { e->x = nx; e->y = ny; }
                if (dist2(l->x, l->y, e->x, e->y) < 15 * 15) hurt_leader(g, 2, e->x, e->y - 20);
            }
            break;
        }
        case E_GATE:
            if (e->state && e->t < 1) { e->t += dt * 0.8f; if (frand(g) < dt * 20) burst(g, PK_DUST, e->x + frand2(g) * 14, e->y, 1, 20, 0x8A8070A0u); }
            break;
        case E_BLOCK:
            if (e->t2 > 0) {
                e->t2 -= dt;
                float t = 1 - clampf(e->t2 / 0.28f, 0, 1);
                e->x = lerpf(e->hx, e->vx, t); e->y = lerpf(e->hy, e->vy, t);
                if (e->t2 <= 0) { e->x = e->vx; e->y = e->vy; e->hx = e->x; e->hy = e->y; }
            }
            break;
        case E_PIPE:
            break;
        case E_TORCH: case E_FIRE: case E_BRAZIER:
            if (!active) break;
            if ((e->type != E_BRAZIER || e->state) && frand(g) < dt * 3) {
                Part *p = part_new(g, PK_EMBER, e->x + frand2(g) * 3, e->y - (e->type == E_TORCH ? 20 : 12));
                if (p) { p->vy = -18 - frand(g) * 14; p->vx = frand2(g) * 8 + g->wind * 6; p->life = p->max = 0.9f + frand(g) * 0.7f; p->add = 1; }
            }
            break;
        case E_SHACK:
            if (active && frand(g) < dt * 4) {                 /* chimney smoke, bent by the wind */
                Part *p = part_new(g, PK_SMOKE, e->x + 21 + frand2(g) * 2, e->y - 60);
                p->c = 0x50505870u; p->vy = -10 - frand(g) * 6; p->vx = 8 + g->wind * 14;
                p->size = 3; p->life = p->max = 2.0f + frand(g);
            }
            break;
        case E_CRYSTAL: case E_MUSHROOM: case E_GOLDPILE:
            if (active && frand(g) < dt * 1.2f) {
                Part *p = part_new(g, PK_MOTE, e->x + frand2(g) * 8, e->y - 6 - frand(g) * 8);
                if (p) {
                    p->c = e->type == E_CRYSTAL ? 0xC8A0FFFFu : (e->type == E_GOLDPILE ? 0xFFF0A0FFu : 0x80FFF0FFu);
                    p->vy = -6; p->life = p->max = 1.2f; p->add = 1;
                }
            }
            break;
        case E_CHEST: case E_BIGCHEST:
            if (e->state == 0 && active && frand(g) < dt * 0.8f) {
                Part *p = part_new(g, PK_MOTE, e->x + frand2(g) * 8, e->y - 10 - frand(g) * 6);
                if (p) { p->c = 0xFFF0A0FFu; p->vy = -6; p->life = p->max = 1.0f; p->add = 1; }
            }
            break;
        }
    }

    /* pushing stone blocks */
    if (g->push_t > 0.28f && l->state == KS_FOLLOW) {
        g->push_t = 0;
        int dx = 0, dy = 0;
        if (l->dir == DIR_SIDE) dx = l->flip ? -1 : 1; else dy = l->dir == DIR_UP ? -1 : 1;
        float px = l->x + (float)dx * 10, py = l->y + (float)dy * 9 - 3;
        for (int i = 0; i < g->nent; i++) {
            Ent *e = &g->ent[i];
            if (!e->on || e->type != E_BLOCK || e->t2 > 0) continue;
            if (px < e->x - 9 || px > e->x + 9 || py < e->y - 18 || py > e->y + 2) continue;
            int tx = (int)(e->x / TS) + dx, ty = (int)((e->y - 2) / TS) + dy;
            int k = tile_at(g, tx, ty);
            int free = !KINFO[k].solid && k != K_HOLE;
            float nx = (float)(tx * TS + 8), ny = (float)(ty * TS + 15);
            for (int j = 0; j < g->nent && free; j++) {
                Ent *o = &g->ent[j];
                if (j == i || !o->on) continue;
                if ((EINFO[o->type].flags & EF_SOLID) || o->type == E_BLOCK)
                    if (fabsf(o->x - nx) < 10 && fabsf(o->y - ny) < 10 && !((EINFO[o->type].flags & EF_BREAK) && o->state)) free = 0;
            }
            if (free) {
                e->hx = e->x; e->hy = e->y; e->vx = nx; e->vy = ny; e->t2 = 0.28f;
                sfx_at(g, SFX_PUSH, e->x, e->y, 128);
                burst(g, PK_DUST, e->x, e->y, 5, 20, 0x8A8070A0u);
            }
            break;
        }
    }
}
