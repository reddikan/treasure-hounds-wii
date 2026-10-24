/*
 * TREASURE HOUNDS - particles, the storm (rain, wind, lightning, surf), cave drips, and gathering
 * the lights that the light map is built from each frame.
 */
#include "game_int.h"
#include <string.h>
#include <math.h>

Part *part_new(Game *g, int kind, float x, float y)
{
    Part *p;
    if (g->npart < MAX_PART) p = &g->part[g->npart++];
    else {                                             /* recycle the oldest-ish */
        int i = (int)(frand(g) * MAX_PART) % MAX_PART;
        p = &g->part[i];
    }
    memset(p, 0, sizeof(*p));
    p->kind = (uint8_t)kind; p->x = x; p->y = y;
    p->life = p->max = 0.6f; p->size = 2; p->c = 0xFFFFFFFFu;
    return p;
}

void burst(Game *g, int kind, float x, float y, int n, float spd, uint32_t c)
{
    for (int i = 0; i < n; i++) {
        Part *p = part_new(g, kind, x, y);
        float a = frand(g) * TAU_F, s = spd * (0.35f + frand(g) * 0.65f);
        p->vx = cosf(a) * s; p->vy = sinf(a) * s * 0.6f;
        p->c = c;
        p->rot = frand(g) * TAU_F;
        switch (kind) {
        case PK_SPARK: p->life = p->max = 0.25f + frand(g) * 0.3f; p->add = 1; p->size = 1.5f; break;
        case PK_STAR:  p->life = p->max = 0.5f; p->add = 1; p->size = 2; break;
        case PK_DUST:  p->life = p->max = 0.5f + frand(g) * 0.4f; p->size = 3 + frand(g) * 3; p->vy -= 8; break;
        case PK_SMOKE: p->life = p->max = 0.7f + frand(g) * 0.4f; p->size = 4 + frand(g) * 3; p->vy -= 15; break;
        case PK_EMBER: p->life = p->max = 0.5f + frand(g) * 0.6f; p->add = 1; p->vy -= 30; p->size = 1; break;
        case PK_DEBRIS: case PK_BONE: case PK_SHARD:
            p->life = p->max = 0.9f + frand(g) * 0.4f; p->vz = 70 + frand(g) * 70; p->z = 6; p->size = kind == PK_BONE ? 3 : 2; break;
        case PK_LEAF:  p->life = p->max = 1.0f + frand(g) * 0.6f; p->vz = 50 + frand(g) * 50; p->z = 8; p->size = 2; break;
        case PK_GOLD:  p->life = p->max = 1.6f + frand(g) * 0.8f; p->vz = 120 + frand(g) * 140; p->z = 10; break;
        case PK_HEART: p->life = p->max = 0.9f; p->vy = -20 - frand(g) * 10; p->vx *= 0.3f; break;
        case PK_MOTE:  p->life = p->max = 1.0f; p->add = 1; p->vy -= 10; break;
        }
    }
}

void float_text(Game *g, float x, float y, const char *s, uint32_t c)
{
    Part *p = part_new(g, PK_TEXT, x, y);
    int v = 0;
    for (; *s; s++) if (*s >= '0' && *s <= '9') v = v * 10 + (*s - '0');
    p->val = (int16_t)v; p->c = c; p->vy = -24; p->life = p->max = 1.0f;
}

void parts_update(Game *g, float dt)
{
    int w = 0;
    for (int i = 0; i < g->npart; i++) {
        Part *p = &g->part[i];
        p->life -= dt;
        if (p->life <= 0) continue;
        p->x += p->vx * dt; p->y += p->vy * dt;
        switch (p->kind) {
        case PK_DEBRIS: case PK_BONE: case PK_SHARD: case PK_LEAF: case PK_GOLD:
            p->z += p->vz * dt; p->vz -= (p->kind == PK_LEAF ? 120 : 360) * dt;
            if (p->kind == PK_LEAF) { p->vx += sinf(p->life * 9 + p->rot) * 60 * dt + g->wind * 20 * dt; }
            if (p->z < 0) { p->z = 0; p->vz *= -0.35f; p->vx *= 0.5f; p->vy *= 0.5f; }
            p->rot += dt * 8;
            break;
        case PK_FLAME:
            p->z += p->vz * dt; p->size += dt * 6;
            break;
        case PK_DUST: case PK_SMOKE:
            p->vx *= 0.95f; p->vy *= 0.95f; p->size += dt * 6;
            break;
        case PK_SPARK: case PK_STAR:
            p->vx *= 0.9f; p->vy *= 0.9f;
            break;
        case PK_EMBER: case PK_MOTE:
            p->vx += sinf(p->life * 5 + p->rot) * 10 * dt;
            break;
        case PK_DRIP:
            p->z -= 160 * dt;
            if (p->z <= 0) {
                p->kind = PK_SPLASH; p->life = p->max = 0.3f; p->size = 2; p->z = 0;
            }
            break;
        case PK_FOAM:
            p->vx *= 0.97f; p->vy *= 0.97f; p->size += dt * 2;
            break;
        }
        g->part[w++] = *p;
    }
    g->npart = w;
}

/* ------------------------------------------------------------------ */
/* weather                                                             */
/* ------------------------------------------------------------------ */
static int outdoors(const Game *g) { return g->lv.theme == THEME_TOWN || g->lv.theme == THEME_COVE; }

static int near_sea(const Game *g)
{
    const Kid *l = &g->kid[g->order[0]];
    int tx = (int)(l->x / TS), ty = (int)(l->y / TS);
    for (int dy = -8; dy <= 8; dy += 2)
        for (int dx = -12; dx <= 12; dx += 2)
            if (tile_at(g, tx + dx, ty + dy) == K_SEA) return 1;
    return 0;
}

void weather_update(Game *g, float dt)
{
    g->wind = sinf(g->time * 0.31f) * 0.45f + sinf(g->time * 1.7f) * 0.15f + 0.35f;
    if (g->flash > 0) g->flash -= dt * 3.2f;
    if (g->flash < 0) g->flash = 0;
    if (g->bolt_t > 0) g->bolt_t -= dt;

    if (outdoors(g)) {
        if (g->ndrop == 0) {
            g->ndrop = 260;
            for (int i = 0; i < g->ndrop; i++) {
                g->drop[i].x = frand(g) * (VW + 200) - 100; g->drop[i].y = frand(g) * VH;
                g->drop[i].len = 10 + frand(g) * 18; g->drop[i].spd = 520 + frand(g) * 260;
            }
        }
        for (int i = 0; i < g->ndrop; i++) {
            Drop *d = &g->drop[i];
            d->y += d->spd * dt;
            d->x += d->spd * dt * (0.18f + g->wind * 0.22f);
            if (d->y > VH + 20) { d->y -= VH + 40; d->x = frand(g) * (VW + 200) - 160; }
            if (d->x > VW + 60) d->x -= VW + 200;
        }
        /* splashes on the ground around the view */
        int n = (int)(dt * 70.0f + frand(g));
        for (int i = 0; i < n; i++) {
            float x = g->camx + frand(g) * VIEW_W, y = g->camy + frand(g) * VIEW_H;
            int k = tile_at(g, (int)(x / TS), (int)(y / TS));
            if (KINFO[k].wall) continue;
            Part *p = part_new(g, PK_RING, x, y);
            p->life = p->max = 0.25f + frand(g) * 0.1f;
            p->size = KINFO[k].water ? 3.5f : 2.0f;
        }
        /* rain rings on the puddles */
        for (int i = 0; i < g->nent; i++) {
            const Ent *e = &g->ent[i];
            if (!e->on || e->type != E_PUDDLE || frand(g) > dt * 9) continue;
            if (e->x < g->camx - 20 || e->x > g->camx + VIEW_W + 20 || e->y < g->camy - 20 || e->y > g->camy + VIEW_H + 20) continue;
            Part *p = part_new(g, PK_RING, e->x + frand2(g) * 7, e->y + frand2(g) * 2.5f);
            p->life = p->max = 0.4f; p->size = 3;
        }
        /* lightning */
        g->flash_t -= dt;
        if (g->flash_t <= 0) {
            g->flash_t = 7.0f + frand(g) * 9.0f;
            g->flash = 1.0f;
            g->bolt_t = 0.32f;
            g->bolt_x = 80 + frand(g) * (VW - 160);
            g->bolt_y = 120 + frand(g) * 140;
            g->bolt_seed = (uint32_t)(frand(g) * 1e6f);
            g->thunder_t = 0.25f + frand(g) * 1.0f;
            sfx(g, SFX_CRACK, 128 + (int)(frand2(g) * 15), (g->bolt_x / VW - 0.5f) * 1.2f, 150);
        }
        if (g->thunder_t > 0) {
            g->thunder_t -= dt;
            if (g->thunder_t <= 0) {
                sfx(g, SFX_THUNDER, 110 + (int)(frand(g) * 30), frand2(g) * 0.5f, 255);
                g->shake = fmaxf(g->shake, 3.0f);
                if (g->state == ST_PLAY && frand(g) < 0.3f) {
                    int who = (int)(frand(g) * 4) & 3;
                    kid_say(g, who, SAY_THUNDER[who][(int)(frand(g) * 3) % 3], 1.8f);
                }
            }
            /* the second flicker */
            if (g->thunder_t > 0 && g->bolt_t < 0.18f && g->bolt_t > 0.12f) g->flash = fmaxf(g->flash, 0.7f);
        }
        /* surf */
        g->wave_t -= dt;
        if (g->wave_t <= 0) {
            g->wave_t = 4.0f + frand(g) * 4.0f;
            if (near_sea(g)) sfx(g, SFX_WAVE, 110 + (int)(frand(g) * 30), frand2(g) * 0.4f, 160);
        }
        /* foam drifting where sea meets land */
        for (int i = 0; i < 3; i++) {
            float x = g->camx + frand(g) * VIEW_W, y = g->camy + frand(g) * VIEW_H;
            int tx = (int)(x / TS), ty = (int)(y / TS);
            if (tile_at(g, tx, ty) != K_SEA) continue;
            int shore = 0;
            for (int k = 0; k < 4; k++) {
                int nk = tile_at(g, tx + (k == 0) - (k == 1), ty + (k == 2) - (k == 3));
                if (!KINFO[nk].water && !KINFO[nk].wall && nk != K_VOID) shore = 1;
            }
            if (!shore) continue;
            Part *p = part_new(g, PK_FOAM, x, y);
            p->life = p->max = 1.4f + frand(g); p->size = 2 + frand(g) * 2; p->vx = frand2(g) * 6; p->vy = frand2(g) * 6;
        }
    } else {
        g->ndrop = 0;
        /* drips from the cave roof, dust in the torchlight */
        if (frand(g) < dt * 1.5f) {
            float x = g->camx + frand(g) * VIEW_W, y = g->camy + frand(g) * VIEW_H;
            if (!solid_at(g, x, y)) {
                Part *p = part_new(g, PK_DRIP, x, y);
                p->z = 120; p->life = p->max = 2.0f;
                Kid *l = leader(g);
                if (fabsf(x - l->x) < 140 && fabsf(y - l->y) < 90) sfx_at(g, SFX_DRIP, x, y, 110 + (int)(frand(g) * 60));
            }
        }
        if (frand(g) < dt * 5) {
            Part *p = part_new(g, PK_MOTE, g->camx + frand(g) * VIEW_W, g->camy + frand(g) * VIEW_H);
            p->c = g->lv.theme == THEME_CAVE ? 0x90C8D8C0u : 0xF0D8A8C0u;
            p->life = p->max = 2.5f; p->vy = -3; p->vx = frand2(g) * 4;
        }
    }
}

/* ------------------------------------------------------------------ */
/* lights                                                              */
/* ------------------------------------------------------------------ */
static void add_light(Game *g, float x, float y, float r, float i, float cr, float cg, float cb)
{
    if (g->nlight >= MAX_LIGHT) return;
    if (x + r < g->camx - 20 || x - r > g->camx + VIEW_W + 20 || y + r < g->camy - 20 || y - r > g->camy + VIEW_H + 20) return;
    Light *L = &g->light[g->nlight++];
    L->x = x; L->y = y; L->r = r; L->i = i; L->cr = cr; L->cg = cg; L->cb = cb;
}

static float flick(const Game *g, float seed) { return 0.88f + 0.08f * sinf(g->time * 13.0f + seed) + 0.05f * sinf(g->time * 29.0f + seed * 3.0f); }

void lights_collect(Game *g)
{
    g->nlight = 0;
    switch (g->lv.theme) {
    case THEME_TOWN:   g->ambient[0] = 0.22f; g->ambient[1] = 0.26f; g->ambient[2] = 0.42f; break;
    case THEME_COVE:   g->ambient[0] = 0.21f; g->ambient[1] = 0.26f; g->ambient[2] = 0.41f; break;
    case THEME_CAVE:   g->ambient[0] = 0.09f; g->ambient[1] = 0.10f; g->ambient[2] = 0.15f; break;
    default:           g->ambient[0] = 0.15f; g->ambient[1] = 0.12f; g->ambient[2] = 0.13f; break;
    }
    if (g->flash > 0) {
        float f = g->flash * g->flash;
        g->ambient[0] += f * 0.9f; g->ambient[1] += f * 0.95f; g->ambient[2] += f * 1.1f;
    }
    /* the lantern */
    const Kid *l = &g->kid[g->order[0]];
    int dark = g->lv.theme == THEME_CAVE || g->lv.theme == THEME_TEMPLE;
    add_light(g, l->x, l->y - 10 - l->z, dark ? 112 : 84, (dark ? 1.15f : 0.85f) * flick(g, 1.0f), 1.0f, 0.74f, 0.42f);
    for (int i = 0; i < g->nent; i++) {
        const Ent *e = &g->ent[i];
        if (!e->on) continue;
        switch (e->type) {
        case E_LAMP:      add_light(g, e->x, e->y - 30, 84, 0.9f * flick(g, e->x), 1.0f, 0.82f, 0.52f); break;
        case E_TORCH:     add_light(g, e->x, e->y - 4, 96, 1.0f * flick(g, e->x * 0.3f), 1.0f, 0.6f, 0.3f); break;
        case E_FIRE:      add_light(g, e->x, e->y - 6, 118, 1.15f * flick(g, e->y), 1.0f, 0.55f, 0.25f); break;
        case E_BRAZIER:   if (e->state) add_light(g, e->x, e->y - 10, 100, 1.05f * flick(g, e->x), 1.0f, 0.6f, 0.3f); break;
        case E_MUSHROOM:  add_light(g, e->x, e->y - 4, 44, 0.55f, 0.3f, 0.95f, 0.85f); break;
        case E_CRYSTAL:   add_light(g, e->x, e->y - 6, 54, 0.6f + 0.1f * sinf(g->time * 2 + e->x), 0.65f, 0.4f, 1.0f); break;
        case E_HLANTERN:  add_light(g, e->x, e->y - 10, 76, 0.9f * flick(g, e->y), 1.0f, 0.8f, 0.45f); break;
        case E_LIGHTHOUSE:add_light(g, e->x, e->y - 104, 120, 1.0f, 1.0f, 0.92f, 0.65f); break;
        case E_SHACK:
            add_light(g, e->x - 18, e->y - 14, 52, 0.8f, 1.0f, 0.75f, 0.4f);
            add_light(g, e->x + 20, e->y - 14, 52, 0.8f, 1.0f, 0.75f, 0.4f);
            break;
        case E_GOLDPILE:  add_light(g, e->x, e->y - 6, 44, 0.45f, 1.0f, 0.8f, 0.35f); break;
        case E_FLAMEJET:  if (e->state == 2) add_light(g, e->x, e->y - 14, 80, 1.1f, 1.0f, 0.55f, 0.2f);
                          else if (e->state == 1) add_light(g, e->x, e->y - 2, 30, 0.6f, 1.0f, 0.4f, 0.1f); break;
        case E_SKULLKEY:  add_light(g, e->x, e->y - 8, 60, 0.8f, 1.0f, 0.85f, 0.45f); break;
        case E_DOUBLOON:  add_light(g, e->x, e->y - 6, 26, 0.5f, 1.0f, 0.85f, 0.4f); break;
        case E_GEAR:      add_light(g, e->x, e->y - 6, 22, 0.35f, 1.0f, 0.85f, 0.6f); break;
        case E_BIGCHEST:  if (e->state) add_light(g, e->x, e->y - 14, 150, 1.3f, 1.0f, 0.82f, 0.35f);
                          else add_light(g, e->x, e->y - 8, 40, 0.4f, 1.0f, 0.8f, 0.4f); break;
        case E_SKULLWALL: add_light(g, e->x, e->y - 30, 70, 0.35f, 1.0f, 0.5f, 0.2f); break;
        case E_PIPE:      if (e->flash > 0) add_light(g, e->x, e->y - 14, 50, e->flash * 1.6f, 1.0f, 0.95f, 0.7f); break;
        }
    }
}
