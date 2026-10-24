/*
 * TREASURE HOUNDS - drawing the world.
 *
 *   1. ground tiles, wall tops and faces, shore foam and contact shadows
 *   2. everything that stands up (props, kids, creatures), sorted by their feet
 *   3. the light map: a screen-space mesh drawn with "modulate 2x" blending, so warm
 *      lantern and torch light can brighten the painted pixels as well as darken them
 *   4. emissive things on top: flames, glows, flashlight beams, sparks, the lighthouse beam
 *   5. the rain and the lightning
 */
#include "game_int.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

int spike_state(const Game *g, int tx, int ty);
int jet_state(const Ent *e, float time);

static float CX, CY;               /* camera (texels, rounded) */

/* ------------------------------------------------------------------ */
static void spr(const ArtRect *r, float x, float y, int flip, uint32_t c)
{
    float left = flip ? x - (float)(r->w - r->ax) : x - (float)r->ax;
    gfx_img(left, y - (float)r->ay, r->w, r->h, r->x, r->y, r->w, r->h, c, flip);
}
/* sprite with its top pushed sideways (wind) */
static void spr_sway(const ArtRect *r, float x, float y, int flip, uint32_t c, float sway)
{
    float left = flip ? x - (float)(r->w - r->ax) : x - (float)r->ax;
    float top = y - (float)r->ay;
    float p[8] = { left + sway, top, left + r->w + sway, top, left + r->w, top + r->h, left, top + r->h };
    uint32_t c4[4] = { c, c, c, c };
    gfx_quad_uv(p, r->x, r->y, r->w, r->h, c4, flip);
}
static void spr_scaled(const ArtRect *r, float x, float y, float sx, float sy, uint32_t c, int flip)
{
    float w = (float)r->w * sx, h = (float)r->h * sy;
    gfx_img(x - (float)r->ax * sx, y - (float)r->ay * sy, w, h, r->x, r->y, r->w, r->h, c, flip);
}
static void spr_rot(const ArtRect *r, float cx, float cy, float ang, uint32_t c)
{
    float hw = (float)r->w * 0.5f, hh = (float)r->h * 0.5f, cs = cosf(ang), sn = sinf(ang);
    float p[8];
    float lx[4] = { -hw, hw, hw, -hw }, ly[4] = { -hh, -hh, hh, hh };
    for (int i = 0; i < 4; i++) { p[i * 2] = cx + lx[i] * cs - ly[i] * sn; p[i * 2 + 1] = cy + lx[i] * sn + ly[i] * cs; }
    uint32_t c4[4] = { c, c, c, c };
    gfx_quad_uv(p, r->x, r->y, r->w, r->h, c4, 0);
}
static void shadow(float x, float y, float rx, float ry, float a)
{
    gfx_ellipse(x, y, rx, ry, 12, RGBA(6, 4, 14, (int)(a * 255)), RGBA(6, 4, 14, 0));
}

/* ------------------------------------------------------------------ */
/* 1. ground                                                           */
/* ------------------------------------------------------------------ */
static int is_wall(const Game *g, int x, int y) { return KINFO[tile_at(g, x, y)].wall; }

static void draw_tiles(const Game *g, int x0, int y0, int x1, int y1)
{
    int wf = ((int)(g->time * 4.0f)) & 3;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            int k = tile_at(g, x, y);
            const KindInfo *ki = &KINFO[k];
            int var = (x >= 0 && y >= 0 && x < g->lv.w && y < g->lv.h) ? g->lv.v[y][x] : 0;
            int art = ki->floor_art, fr = var;
            float px = (float)(x * TS), py = (float)(y * TS);
            if (ki->wall) {
                /* walls 3+ tiles thick show a two-tile-tall face under their top */
                int b1 = is_wall(g, x, y + 1), b2 = is_wall(g, x, y + 2), a1 = is_wall(g, x, y - 1);
                if (!b1) art = (a1 && is_wall(g, x, y - 2)) ? ki->face2_art : ki->face_art;
                else if (!b2 && a1) art = ki->face_art;
                else art = ki->top_art;
            } else switch (k) {
                case K_VOID: art = TA_PIT; fr = 0; break;
                case K_SEA: case K_POOL: case K_SHALLOW: fr = (wf + var) & 3; break;
                case K_SPIKES: fr = spike_state(g, x, y); break;
                case K_CRUMBLE: fr = g->lv.st[y][x] > 2 ? 2 : g->lv.st[y][x]; break;
                case K_HOLE: art = TA_CRUMBLE; fr = 3; break;
                case K_PLATE: fr = g->lv.st[y][x] ? 1 : 0; break;
                case K_GRATE: fr = g->crane_done ? 1 : 0; break;
                case K_RUG: fr = var & 1; break;
            }
            const ArtRect *r = &art_tile[art][fr];
            gfx_img(px, py, 16, 16, r->x, r->y, 16, 16, 0xFFFFFFFFu, 0);
        }
    /* soft edges: grass and dry sand spill over onto paths, dirt, cobbles and wet sand */
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            int k = tile_at(g, x, y);
            if (KINFO[k].wall || k == K_VOID || k == K_GRASS || k == K_SAND || (KINFO[k].solid && !KINFO[k].water)) continue;
            static const int dx[4] = { 0, 0, -1, 1 }, dy[4] = { -1, 1, 0, 0 };
            for (int d = 0; d < 4; d++) {
                int nk = tile_at(g, x + dx[d], y + dy[d]);
                int art = -1;
                if (nk == K_GRASS && !KINFO[k].water) art = TA_FRINGE_GRASS;
                else if (nk == K_SAND && (k == K_WETSAND || k == K_SHALLOW || k == K_DIRT)) art = TA_FRINGE_SAND;
                if (art < 0) continue;
                const ArtRect *r = &art_tile[art][d];
                gfx_img((float)(x * TS), (float)(y * TS), 16, 16, r->x, r->y, 16, 16, 0xFFFFFFFFu, 0);
            }
        }
    /* depth: shade under wall faces, and down the sides of walls */
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            int k = tile_at(g, x, y);
            if (KINFO[k].wall || k == K_VOID) continue;
            float px = (float)(x * TS), py = (float)(y * TS);
            if (KINFO[k].water) {
                /* shore foam on deep water next to land */
                if (KINFO[k].solid) {
                    float w = 0.55f + 0.45f * sinf(g->time * 2.0f + (float)(x * 3 + y * 5));
                    uint32_t foam = RGBA(200, 230, 240, (int)(110 * w));
                    int up = tile_at(g, x, y - 1);
                    if (!KINFO[up].water && up != K_VOID) gfx_rect_v(px, py, 16, 5 + w * 2, foam, foam & 0xFFFFFF00u);
                    int dn = tile_at(g, x, y + 1);
                    if (!KINFO[dn].water && dn != K_VOID && !KINFO[dn].wall) gfx_rect_v(px, py + 11 - w * 2, 16, 5 + w * 2, foam & 0xFFFFFF00u, foam);
                    int lf = tile_at(g, x - 1, y);
                    if (!KINFO[lf].water && lf != K_VOID) gfx_rect_h(px, py, 4 + w * 2, 16, foam, foam & 0xFFFFFF00u);
                    int rt = tile_at(g, x + 1, y);
                    if (!KINFO[rt].water && rt != K_VOID) gfx_rect_h(px + 12 - w * 2, py, 4 + w * 2, 16, foam & 0xFFFFFF00u, foam);
                }
                continue;
            }
            if (is_wall(g, x, y - 1)) gfx_rect_v(px, py, 16, 7, RGBA(4, 2, 10, 120), RGBA(4, 2, 10, 0));
            if (is_wall(g, x - 1, y)) gfx_rect_h(px, py, 5, 16, RGBA(4, 2, 10, 80), RGBA(4, 2, 10, 0));
            if (is_wall(g, x + 1, y)) gfx_rect_h(px + 11, py, 5, 16, RGBA(4, 2, 10, 0), RGBA(4, 2, 10, 80));
            if (tile_at(g, x, y - 1) == K_VOID || tile_at(g, x, y - 1) == K_HOLE) gfx_rect_v(px, py, 16, 4, RGBA(0, 0, 0, 160), RGBA(0, 0, 0, 0));
        }
}

/* ------------------------------------------------------------------ */
/* 2. sorted sprites                                                   */
/* ------------------------------------------------------------------ */
typedef struct { float y; int16_t kind, idx; } DItem;
static DItem dl[1400];
static int ndl;

static int cmp_d(const void *a, const void *b)
{
    float ya = ((const DItem *)a)->y, yb = ((const DItem *)b)->y;
    return ya < yb ? -1 : (ya > yb ? 1 : 0);
}

static void draw_whip(const Game *g, const Kid *k, float hx, float hy)
{
    float age = 0.34f - g->whip_t;
    float ext = age < 0.09f ? 0.12f : (age < 0.22f ? 1.0f : 1.0f - (age - 0.22f) / 0.12f);
    ext = clampf(ext, 0.05f, 1.0f);
    float fx = 0, fy = 0;
    if (k->dir == DIR_SIDE) fx = k->flip ? -1.0f : 1.0f; else fy = k->dir == DIR_UP ? -1.0f : 1.0f;
    float len = 44 * ext;
    enum { NP = 12 };
    float px[NP + 1], py[NP + 1];
    for (int i = 0; i <= NP; i++) {
        float t = (float)i / (float)NP;
        /* a travelling wave along the lash, flattening out as it snaps straight */
        float wave = sinf(t * 4.0f - age * 46.0f) * (1.1f - ext) * 7.0f * t;
        float arc = (fx != 0 ? -sinf(t * 3.14159f) * 6.0f * ext : 0);
        px[i] = hx + fx * len * t - fy * wave;
        py[i] = hy + fy * len * t * 0.92f + fx * wave + arc;
    }
    /* dark outline, leather, then a thin highlight */
    for (int pass = 0; pass < 3; pass++)
        for (int i = 0; i < NP; i++) {
            float t = (float)i / (float)NP;
            float w = (2.4f - t * 1.5f) + (pass == 0 ? 1.4f : (pass == 2 ? -1.3f : 0));
            if (w < 0.5f) w = 0.5f;
            uint32_t c = pass == 0 ? 0x1A0E08FFu : (pass == 1 ? RGBA(132 - (int)(t * 30), 82 - (int)(t * 20), 40, 255) : RGBA(196, 140, 84, 200));
            gfx_line(px[i], py[i], px[i + 1], py[i + 1], w, c, c);
        }
    /* the crack! */
    if (age > 0.09f && age < 0.17f) {
        float a = 1.0f - (age - 0.09f) / 0.08f;
        float tx = px[NP], ty = py[NP];
        gfx_blend(BL_ADD);
        gfx_glow(tx, ty, 9, RGBA(255, 250, 220, (int)(170 * a)));
        for (int r = 0; r < 4; r++) {
            float ang = (float)r * 0.785f + 0.39f;
            gfx_line(tx - cosf(ang) * 7 * a, ty - sinf(ang) * 7 * a, tx + cosf(ang) * 7 * a, ty + sinf(ang) * 7 * a, 1.0f, RGBA(255, 255, 255, (int)(230 * a)), RGBA(255, 255, 255, (int)(230 * a)));
        }
        gfx_blend(BL_ALPHA);
    }
}

static void draw_kid(const Game *g, int id, int pos)
{
    const Kid *k = &g->kid[id];
    if (k->inv > 0 && k->inv < 1.2f && ((int)(k->inv * 16) & 1) && pos == 0) return;
    float x = floorf(k->x + 0.5f), y = floorf(k->y + 0.5f) - k->z;
    if (k->state == KS_FALL) {
        float t = clampf(k->mt / k->mdur, 0, 1);
        const ArtRect *r = &art_char[id][k->dir][FR_HURT];
        spr_scaled(r, x, y + t * 6, 1 - t * 0.8f, 1 - t * 0.8f, RGBA(255, 255, 255, (int)(255 * (1 - t))), k->flip);
        return;
    }
    const ArtRect *r = &art_char[id][k->dir][k->frame];
    int lantern = pos == 0;
    /* lantern held on the far side is drawn first */
    float lx = 0, ly = 0;
    int behind = 0;
    if (lantern) {
        float swing = sinf(g->time * 3.0f + k->anim * 1.5f) * 1.5f;
        if (k->dir == DIR_DOWN) { lx = x + 7; ly = y - 7 + swing * 0.3f; }
        else if (k->dir == DIR_UP) { lx = x - 7; ly = y - 9; behind = 1; }
        else { lx = x + (k->flip ? -6 : 6); ly = y - 8; }
        if (g->whip_t > 0) { if (k->dir == DIR_DOWN) lx = x + 8; }
        lx += swing * 0.5f;
        if (behind) spr(&art_item[IT_LANTERN], lx, ly + 6, 0, 0xFFFFFFFFu);
    }
    uint32_t c = 0xFFFFFFFFu;
    if (k->hp == 0) c = 0xC8C8E8FFu;
    spr(r, x, y, k->flip, c);
    if (lantern && !behind) spr(&art_item[IT_LANTERN], lx, ly + 6, 0, 0xFFFFFFFFu);

    /* tired kids get little dizzy stars */
    if (k->hp == 0) {
        for (int i = 0; i < 3; i++) {
            float a = g->time * 4 + (float)i * 2.09f;
            spr(&art_item[IT_STAR], x + cosf(a) * 7, y - 26 + sinf(a) * 2.5f, 0, 0xFFFFFFC0u);
        }
    }
}

static int thug_ch(int type) { return type == E_VIC ? CH_VIC : (type == E_LENNY ? CH_LENNY : (type == E_MA ? CH_MA : CH_TINY)); }

static void draw_ent(const Game *g, const Ent *e)
{
    const EntInfo *in = &EINFO[e->type];
    float x = floorf(e->x + 0.5f), y = floorf(e->y + 0.5f);
    uint32_t white = 0xFFFFFFFFu;
    switch (e->type) {
    case E_COIN: {
        int f = ((int)(g->time * 8 + e->x * 0.1f)) & 3;
        float bob = e->z > 0 ? e->z : 2 + sinf(g->time * 3 + e->x) * 1.5f;
        spr(&art_item[IT_COIN + f], x, y - bob, 0, white);
        return;
    }
    case E_GEM: {
        float bob = e->z > 0 ? e->z : 3 + sinf(g->time * 3 + e->x) * 2;
        spr(&art_item[IT_GEM_R + (e->a % 3)], x, y - bob, 0, white);
        return;
    }
    case E_GEAR: spr(&art_item[IT_GEAR], x, y - 2 - sinf(g->time * 2.5f) * 2, 0, white); return;
    case E_DOUBLOON: spr(&art_item[IT_DOUBLOON], x, y - 3 - sinf(g->time * 2.5f + x) * 2, 0, white); return;
    case E_FOOD: spr(&art_item[e->a ? IT_PIZZA : IT_CANDY], x, y - 2 - (e->z > 0 ? e->z : sinf(g->time * 2.5f + x) * 1.5f + 1.5f), 0, white); return;
    case E_SKULLKEY: spr(&art_item[IT_SKULLKEY], x, y - 6 - sinf(g->time * 2) * 3, 0, white); return;
    case E_BAT: {
        int f = e->state == 0 ? 1 : (((int)e->t3) & 3);
        uint32_t c = e->flash > 0 ? 0xFF8080FFu : white;
        spr(&art_crit[CR_BAT][0][f], x, y - e->z, 0, c);
        return;
    }
    case E_CRAB: spr(&art_crit[CR_CRAB][0][((int)e->t3) % 3], x, y, 0, e->flash > 0 ? 0xFFA0A0FFu : white); return;
    case E_RAT: spr(&art_crit[CR_RAT][0][((int)e->t3) & 1], x, y, e->flip, e->flash > 0 ? 0xFFA0A0FFu : white); return;
    case E_SKEL: {
        if (e->state == 5) {
            float jig = e->t < 1.0f ? sinf(e->t3) * 1.5f : 0;
            spr(&art_spr[SP_BONES], x + jig, y, 0, white);
            return;
        }
        int f = 0;
        if (e->state == 1 || e->state == 0) f = 1 + (((int)e->t3) & 3);
        if (e->state == 2) f = 0;
        if (e->state == 3) f = 5;
        if (e->state == 0 && fmodf(e->t3, 4.0f) < 2.0f) f = 0;
        uint32_t c = e->flash > 0 ? 0xFF9090FFu : (e->state == 2 ? 0xFFE0C0FFu : white);
        float shake = e->state == 2 ? sinf(g->time * 60) * 1.0f : 0;
        spr(&art_crit[CR_SKEL][e->dir][f], x + shake, y, e->flip, c);
        return;
    }
    case E_VIC: case E_LENNY: case E_MA: case E_TINY: {
        int ch = thug_ch(e->type);
        int f = FR_IDLE + (((int)(e->t3 * 0.5f)) & 1);
        int moving = (e->type == E_MA) ? (e->state == 1 || e->state == 2) : (e->state == 0 || e->state == 2 || e->state == 3);
        if (e->type == E_TINY) moving = e->state == 2;
        if (e->type == E_MA && g->boss_phase < 1) moving = 0;
        if (moving) f = FR_WALK0 + (((int)e->t3) & 3);
        if ((e->type == E_VIC || e->type == E_LENNY) && e->state == 1) f = FR_HURT;
        if ((e->type == E_VIC || e->type == E_LENNY) && e->state == 4) f = FR_DIZZY;
        if (e->type == E_MA && e->state == 0 && g->boss_phase >= 1 && e->t > 0.8f) f = FR_WHIP0;
        if (e->type == E_MA && g->boss_down) f = FR_DIZZY;
        if (e->type == E_TINY && e->t > 0) f = ((int)(g->time * 4) & 1) ? FR_CHEER0 : FR_CHEER1;
        if (e->type == E_TINY && e->state == 3) f = ((int)(g->time * 2) & 1) ? FR_IDLE : FR_IDLE2;
        uint32_t c = e->flash > 0 ? 0xFF9090FFu : white;
        int dir = e->type == E_TINY && e->state < 2 ? DIR_DOWN : e->dir;
        spr(&art_char[ch][dir][f], x, y - e->z, e->flip && dir == DIR_SIDE, c);
        if ((e->type == E_VIC || e->type == E_LENNY) && e->state == 4) {
            for (int i = 0; i < 3; i++) {
                float a = g->time * 5 + (float)i * 2.09f;
                spr(&art_item[IT_STAR], x + cosf(a) * 8, y - 31 + sinf(a) * 2.5f, 0, white);
            }
        }
        if ((e->type == E_VIC || e->type == E_LENNY) && e->state == 1) {
            gfx_text("!", x - 1, y - 44 + sinf(g->time * 20) * 1.5f, 1, 0xFF3020FFu);
        }
        if (e->type == E_MA && g->boss_down)
            for (int i = 0; i < 3; i++) {
                float a = g->time * 5 + (float)i * 2.09f;
                spr(&art_item[IT_STAR], x + cosf(a) * 8, y - 33 + sinf(a) * 2.5f, 0, white);
            }
        return;
    }
    case E_BOTTLE:
        spr_rot(&art_item[IT_BOTTLE], x, y - e->z - 6, e->t3, white);
        return;
    case E_DART:
        spr_rot(&art_item[IT_DART], x, y - 10, e->vy > 0 ? 1.5708f : -1.5708f, white);
        return;
    case E_BOULDER: {
        if (e->state == 2) { spr(&art_spr[SP_ROCK_BIG], x, y + 2, 0, white); return; }
        const ArtRect *r = &art_spr[SP_BOULDER];
        spr_rot(r, x, y - 13, e->t2, white);
        return;
    }
    case E_GATE: {
        if (e->state && e->t >= 1) { spr(&art_spr[SP_GATE_OPEN], x, y, 0, white); return; }
        if (e->state) {
            spr(&art_spr[SP_GATE_OPEN], x, y, 0, white);
            spr(&art_spr[SP_GATE], x, y - e->t * 26, 0, RGBA(255, 255, 255, (int)(255 * (1 - e->t))));
            return;
        }
        spr(&art_spr[SP_GATE], x, y, 0, white);
        return;
    }
    case E_BLOCK: spr(&art_spr[SP_BLOCK], x, y, 0, white); return;
    case E_PIPE: {
        const ArtRect *r = &art_spr[SP_BONEPIPE];
        float sy = 0.6f + (float)e->a * 0.2f;
        float vib = e->flash > 0 ? sinf(g->time * 70) * e->flash * 2 : 0;
        spr_scaled(r, x + vib, y, 1.0f, sy, e->flash > 0 ? RGBA(255, 255, (int)(200 + 55 * (1 - e->flash)), 255) : white, 0);
        return;
    }
    case E_BRAZIER: spr(&art_spr[e->state ? SP_BRAZIER_LIT : SP_BRAZIER], x, y, 0, white); return;
    case E_CHEST: spr(&art_spr[e->state ? SP_CHEST_OPEN : SP_CHEST], x, y, 0, white); return;
    case E_BIGCHEST: spr(&art_spr[e->state ? SP_BIGCHEST_OPEN : SP_BIGCHEST], x, y, 0, white); return;
    case E_CAGE:
        if (e->state) return;
        spr(&art_spr[SP_CAGE], x, y, 0, white);
        return;
    case E_DARTER: spr(&art_spr[SP_DARTER], x, y - (e->dir == DIR_DOWN ? 0 : 0), 0, e->flash > 0 ? 0xFFC0A0FFu : white); return;
    case E_HOOK: spr(&art_spr[SP_HOOK], x, y, 0, white); return;
    case E_LEVER: spr(&art_spr[e->state ? SP_LEVER_R : SP_LEVER_L], x, y, 0, white); return;
    }
    if (in->spr < 0) return;
    const ArtRect *r = &art_spr[in->spr];
    if ((in->flags & EF_BREAK) && e->state) {
        int br = e->type == E_CRATE || e->type == E_BARREL ? SP_CRATE_BROKEN : (e->type == E_POT ? SP_POT_BROKEN : SP_BUSH_CUT);
        spr(&art_spr[br], x, y, e->flip, white);
        return;
    }
    if (in->flags & EF_SWAY) {
        float amp = (e->type == E_PINE ? 2.2f : (e->type == E_PALM ? 3.0f : 1.0f));
        if (g->lv.theme == THEME_CAVE || g->lv.theme == THEME_TEMPLE) amp *= 0.15f;
        float sway = (sinf(g->time * 1.6f + e->t) * 0.6f + g->wind * 0.9f + sinf(g->time * 4.1f + e->t * 2) * 0.25f) * amp;
        spr_sway(r, x, y, e->flip, white, sway);
        return;
    }
    if (e->type == E_PUDDLE) {
        spr(r, x, y, e->flip, white);
        return;
    }
    spr(r, x, y, e->flip, white);
}

static void collect_and_draw(const Game *g, float vx0, float vy0, float vx1, float vy1)
{
    ndl = 0;
    /* flat things and shadows first */
    for (int i = 0; i < g->nent; i++) {
        const Ent *e = &g->ent[i];
        if (!e->on) continue;
        float m = 70;
        if (e->type == E_LIGHTHOUSE || e->type == E_SHACK || e->type == E_MAST || e->type == E_SKULLWALL) m = 140;
        if (e->x < vx0 - m || e->x > vx1 + m || e->y < vy0 - 10 || e->y > vy1 + m + 20) continue;
        const EntInfo *in = &EINFO[e->type];
        if (in->flags & EF_FLAT) {
            if (e->type == E_FLAMEJET) spr(&art_spr[SP_FLAMEJET], e->x, e->y, 0, 0xFFFFFFFFu);
            else draw_ent(g, e);
            continue;
        }
        /* shadows */
        int t = e->type;
        if (t == E_PINE) shadow(e->x + 3, e->y, 11, 4, 0.35f);
        else if (t == E_PALM) shadow(e->x + 4, e->y, 12, 4, 0.3f);
        else if (in->flags & EF_ENEMY || t == E_TINY) {
            float rr = (t == E_BAT ? 5 : (t == E_MA || t == E_TINY ? 9 : 7));
            shadow(e->x, e->y, rr, rr * 0.45f, t == E_BAT ? 0.25f : 0.4f);
        } else if (in->flags & EF_PICKUP) shadow(e->x, e->y, 4, 1.6f, 0.3f);
        else if (t == E_BOULDER) shadow(e->x, e->y, 13, 4, 0.45f);
        else if (t == E_BOTTLE) {
            float k = clampf(e->t / e->t2, 0, 1);
            gfx_ring(e->vx, e->vy, 10 - k * 3, 1.5f, 16, 0, RGBA(255, 60, 40, (int)(120 + 100 * k)));
            shadow(e->x, e->y, 4, 1.5f, 0.3f);
        } else if (in->fw > 0 && t != E_SKULLWALL && t != E_GATE) shadow(e->x, e->y, (float)in->fw + 2, 3, 0.25f);
        if (ndl < 1390) { dl[ndl].y = e->y; dl[ndl].kind = 0; dl[ndl].idx = (int16_t)i; ndl++; }
    }
    for (int i = 0; i < NKIDS; i++) {
        int id = g->order[i];
        const Kid *k = &g->kid[id];
        shadow(k->x, k->y, 6 - k->z * 0.05f, 2.5f, 0.42f);
        dl[ndl].y = k->y + 0.01f * (float)(NKIDS - i); dl[ndl].kind = 1; dl[ndl].idx = (int16_t)i; ndl++;
    }
    qsort(dl, (size_t)ndl, sizeof(DItem), cmp_d);
    for (int i = 0; i < ndl; i++) {
        if (dl[i].kind == 0) draw_ent(g, &g->ent[dl[i].idx]);
        else draw_kid(g, g->order[dl[i].idx], dl[i].idx);
    }
    if (g->whip_t > 0) {
        const Kid *k = &g->kid[g->order[0]];
        float x = floorf(k->x + 0.5f), y = floorf(k->y + 0.5f) - k->z;
        float hx = x, hy = y - 13;
        if (k->dir == DIR_SIDE) hx += k->flip ? -7 : 7;
        if (k->dir == DIR_DOWN) { hx -= 6; hy = y - 9; }
        if (k->dir == DIR_UP) { hx += 6; hy = y - 17; }
        draw_whip(g, k, hx, hy);
    }
}

/* ------------------------------------------------------------------ */
/* particles                                                           */
/* ------------------------------------------------------------------ */
static void draw_parts(const Game *g, int additive)
{
    for (int i = 0; i < g->npart; i++) {
        const Part *p = &g->part[i];
        if ((p->add != 0) != additive) continue;
        float a = clampf(p->life / p->max, 0, 1);
        float x = p->x, y = p->y - p->z;
        switch (p->kind) {
        case PK_SPARK: case PK_EMBER: case PK_STAR:
            gfx_rect(x - p->size * 0.5f, y - p->size * 0.5f, p->size, p->size, col_a(p->c, a));
            if (p->kind != PK_EMBER) gfx_glow(x, y, 3 + p->size, col_a(p->c, a * 0.35f));
            break;
        case PK_MOTE:
            gfx_glow(x, y, 2.5f, col_a(p->c, a * sinf(a * PI_F) * 0.9f));
            gfx_rect(x - 0.5f, y - 0.5f, 1, 1, col_a(p->c, a * sinf(a * PI_F)));
            break;
        case PK_FLAME: {
            float t = 1 - a;
            uint32_t c = t < 0.3f ? 0xFFF0B0FFu : (t < 0.6f ? 0xFFA040FFu : 0xE04010FFu);
            gfx_glow(x, y, p->size * (0.6f + t), col_a(c, a * 0.9f));
            break;
        }
        case PK_DUST: case PK_SMOKE:
            gfx_glow(x, y, p->size, col_a(p->c, a * 0.6f));
            break;
        case PK_SPLASH:
            gfx_ring(x, y, p->size * (1.6f - a) + 1, 1, 10, 0, RGBA(190, 220, 240, (int)(150 * a)));
            break;
        case PK_RING:
            gfx_ring(x, y, p->size * (1.3f - a) + 0.5f, 0.7f, 8, 0, RGBA(170, 200, 230, (int)(110 * a)));
            break;
        case PK_FOAM:
            gfx_glow(x, y, p->size, RGBA(210, 235, 245, (int)(120 * sinf(a * PI_F))));
            break;
        case PK_DEBRIS: case PK_SHARD:
            gfx_rect(x - 1, y - 1, p->kind == PK_SHARD ? 1.5f : 2, 2, col_a(p->c, fminf(1, a * 3)));
            break;
        case PK_BONE:
            gfx_line(x - cosf(p->rot) * 2, y - sinf(p->rot) * 2, x + cosf(p->rot) * 2, y + sinf(p->rot) * 2, 1.4f, 0xE0D8C0FFu, 0xE0D8C0FFu);
            break;
        case PK_LEAF:
            gfx_tri1(x, y - 1.5f, x + 2 * cosf(p->rot), y + 1, x - 2 * cosf(p->rot), y + 1, col_a(p->c, fminf(1, a * 3)));
            break;
        case PK_GOLD: {
            int f = ((int)(p->life * 12 + p->rot * 3)) & 3;
            spr(&art_item[IT_COIN + f], x, y + 4, 0, col_a(0xFFFFFFFFu, fminf(1, a * 4)));
            break;
        }
        case PK_HEART:
            spr(&art_item[IT_HEART], x, y, 0, col_a(0xFFFFFFFFu, a));
            break;
        case PK_DRIP:
            gfx_rect(x - 0.5f, y - 3, 1, 3, RGBA(160, 210, 230, 200));
            break;
        case PK_TEXT: {
            char buf[8];
            if (p->val >= 10) { buf[0] = '+'; int v = p->val, n = 1; char t[6]; int tn = 0; while (v && tn < 5) { t[tn++] = (char)('0' + v % 10); v /= 10; } while (tn) buf[n++] = t[--tn]; buf[n] = 0; }
            else { buf[0] = (char)('0' + p->val); buf[1] = 0; }
            gfx_text_c(buf, x, y - 4, 1, col_a(p->c, a));
            break;
        }
        }
    }
}

/* ------------------------------------------------------------------ */
/* 3. light map                                                        */
/* ------------------------------------------------------------------ */
#define LGX 55
#define LGY 31
static float lmap[LGY][LGX][3];

static void light_mesh(const Game *g, float sx, float sy)
{
    /* screen grid vertex -> world point */
    for (int j = 0; j < LGY; j++)
        for (int i = 0; i < LGX; i++) {
            float wx = CX + (float)i * 8.0f, wy = CY + (float)j * 8.0f;
            float r = g->ambient[0], gg = g->ambient[1], b = g->ambient[2];
            for (int k = 0; k < g->nlight; k++) {
                const Light *L = &g->light[k];
                float dx = wx - L->x, dy = (wy - L->y) * 1.15f;
                float d2 = dx * dx + dy * dy, r2 = L->r * L->r;
                if (d2 >= r2) continue;
                float f = 1.0f - d2 / r2;
                f = f * f * L->i;
                r += L->cr * f; gg += L->cg * f; b += L->cb * f;
            }
            /* vignette */
            float nx = ((float)i / (LGX - 1) - 0.5f) * 2.0f, ny = ((float)j / (LGY - 1) - 0.5f) * 2.0f;
            float v = 1.0f - 0.28f * (nx * nx * 0.7f + ny * ny);
            lmap[j][i][0] = r * v; lmap[j][i][1] = gg * v; lmap[j][i][2] = b * v;
        }
    gfx_blend(BL_MUL2);
    for (int j = 0; j < LGY - 1; j++)
        for (int i = 0; i < LGX - 1; i++) {
            uint32_t c[4];
            for (int k = 0; k < 4; k++) {
                int ii = i + (k == 1 || k == 2), jj = j + (k >= 2);
                c[k] = RGBA((int)(lmap[jj][ii][0] * 127.5f), (int)(lmap[jj][ii][1] * 127.5f), (int)(lmap[jj][ii][2] * 127.5f), 255);
            }
            float x0 = (float)i * 16.0f + sx, y0 = (float)j * 16.0f + sy;
            gfx_quad4(x0, y0, c[0], x0 + 16, y0, c[1], x0 + 16, y0 + 16, c[2], x0, y0 + 16, c[3]);
        }
}

/* light level (0..2) at a screen position, from the last light map */
float light_at_screen(float x, float y)
{
    int i = (int)(x / 16.0f), j = (int)(y / 16.0f);
    if (i < 0) i = 0; if (j < 0) j = 0; if (i >= LGX) i = LGX - 1; if (j >= LGY) j = LGY - 1;
    return (lmap[j][i][0] + lmap[j][i][1] + lmap[j][i][2]) * 0.333f;
}

/* ------------------------------------------------------------------ */
/* 4. emissive                                                         */
/* ------------------------------------------------------------------ */
static void flame(const Game *g, float x, float y, float s, float seed)
{
    float t = g->time * 9 + seed;
    float h = (8 + sinf(t) * 1.5f + sinf(t * 2.3f) * 1.0f) * s;
    float w = 3.2f * s;
    float lean = (sinf(t * 0.7f) * 1.2f + g->wind * 1.5f) * s * (g->lv.theme == THEME_TOWN || g->lv.theme == THEME_COVE ? 1.0f : 0.3f);
    gfx_glow(x, y - h * 0.3f, 14 * s, RGBA(255, 120, 40, 60));
    gfx_tri(x - w, y, RGBA(255, 90, 20, 220), x + w, y, RGBA(255, 90, 20, 220), x + lean, y - h, RGBA(255, 60, 10, 0));
    gfx_tri(x - w * 0.6f, y, RGBA(255, 200, 80, 240), x + w * 0.6f, y, RGBA(255, 200, 80, 240), x + lean * 0.7f, y - h * 0.7f, RGBA(255, 160, 40, 0));
    gfx_tri(x - w * 0.3f, y, RGBA(255, 255, 220, 255), x + w * 0.3f, y, RGBA(255, 255, 220, 255), x + lean * 0.4f, y - h * 0.4f, RGBA(255, 240, 160, 0));
}

static void emissive(const Game *g, float vx0, float vy0, float vx1, float vy1)
{
    gfx_blend(BL_ADD);
    const Kid *l = &g->kid[g->order[0]];
    /* lantern halo */
    {
        float lx = l->x + (l->dir == DIR_SIDE ? (l->flip ? -6 : 6) : (l->dir == DIR_UP ? -7 : 7)), ly = l->y - 4 - l->z;
        float f = 0.85f + 0.1f * sinf(g->time * 13) + 0.05f * sinf(g->time * 31);
        gfx_glow(lx, ly, 26 * f, RGBA(255, 190, 90, 70));
        gfx_glow(lx, ly, 7, RGBA(255, 230, 160, 120));
    }
    for (int i = 0; i < g->nent; i++) {
        const Ent *e = &g->ent[i];
        if (!e->on) continue;
        if (e->x < vx0 - 160 || e->x > vx1 + 160 || e->y < vy0 - 40 || e->y > vy1 + 160) continue;
        float x = e->x, y = e->y;
        switch (e->type) {
        case E_TORCH: flame(g, x, y - 12, 0.85f, x); break;
        case E_FIRE: flame(g, x - 3, y - 4, 1.2f, x); flame(g, x + 3, y - 3, 1.0f, x + 3); break;
        case E_BRAZIER: if (e->state) { flame(g, x - 2, y - 9, 1.0f, x); flame(g, x + 3, y - 9, 0.8f, y); } break;
        case E_LAMP: gfx_glow(x, y - 30, 16, RGBA(255, 220, 140, 90)); gfx_glow(x, y - 30, 5, RGBA(255, 240, 200, 140)); break;
        case E_HLANTERN: gfx_glow(x, y - 9, 14, RGBA(255, 210, 120, 90)); break;
        case E_MUSHROOM: gfx_glow(x, y - 5, 10, RGBA(80, 240, 220, 70)); break;
        case E_CRYSTAL: gfx_glow(x, y - 7, 13 + sinf(g->time * 2 + x) * 2, RGBA(170, 110, 255, 80)); break;
        case E_SHACK:
            gfx_glow(x - 20, y - 18, 14, RGBA(255, 190, 90, 70)); gfx_glow(x + 18, y - 18, 14, RGBA(255, 190, 90, 70));
            /* chimney smoke is drawn as normal particles; a light trail here */
            break;
        case E_LIGHTHOUSE: {
            float ly = y - 104;
            gfx_glow(x, ly, 26, RGBA(255, 240, 190, 160));
            /* the turning beam */
            float a = g->time * 0.9f;
            float len = 420;
            for (int s = 0; s < 2; s++) {
                float aa = a + (float)s * PI_F;
                float ca = cosf(aa), sa = sinf(aa) * 0.55f;
                float spread = 0.16f;
                float c1 = cosf(aa - spread), s1 = sinf(aa - spread) * 0.55f, c2 = cosf(aa + spread), s2 = sinf(aa + spread) * 0.55f;
                float k = 0.45f + 0.55f * fabsf(ca);
                gfx_tri(x, ly, RGBA(255, 245, 200, (int)(70 * k)), x + c1 * len, ly + s1 * len, RGBA(255, 245, 200, 0), x + c2 * len, ly + s2 * len, RGBA(255, 245, 200, 0));
                (void)sa;
            }
            break;
        }
        case E_FLAMEJET: {
            int s = jet_state(e, g->time);
            if (s == 1) gfx_glow(x, y - 3, 8 + sinf(g->time * 30) * 2, RGBA(255, 80, 20, 140));
            if (s == 2) {
                for (int k = 0; k < 4; k++) flame(g, x + sinf(g->time * 20 + (float)k) * 1.5f, y - 2 - (float)k * 6, 1.3f - (float)k * 0.15f, x + (float)k);
            }
            break;
        }
        case E_VIC: case E_LENNY: {
            if (e->state == 4) break;
            /* flashlight cone */
            float fx = e->dir == DIR_SIDE ? (e->flip ? -1.0f : 1.0f) : 0, fy = e->dir == DIR_SIDE ? 0 : (e->dir == DIR_UP ? -1.0f : 1.0f);
            float bx = x + fx * 6, by = y - 10 + fy * 4;
            float len = 96, sp = 0.5f;
            float a0 = atan2f(fy, fx);
            float flick = e->state == 1 || e->state == 2 ? 1.0f : 0.85f;
            gfx_tri(bx, by, RGBA(255, 250, 200, (int)(70 * flick)),
                    bx + cosf(a0 - sp) * len, by + sinf(a0 - sp) * len * 0.8f, RGBA(255, 250, 200, 0),
                    bx + cosf(a0 + sp) * len, by + sinf(a0 + sp) * len * 0.8f, RGBA(255, 250, 200, 0));
            gfx_glow(bx + fx * len * 0.55f, by + fy * len * 0.45f, 24, RGBA(255, 250, 210, 30));
            break;
        }
        case E_BAT: if (e->state) { gfx_glow(x - 1, y - e->z - 7, 3, RGBA(255, 40, 30, 120)); gfx_glow(x + 1, y - e->z - 7, 3, RGBA(255, 40, 30, 120)); } break;
        case E_SKEL: if (e->state != 5) gfx_glow(x, y - 25, 4, RGBA(255, 80, 30, e->state == 2 ? 180 : 70)); break;
        case E_SKULLWALL: {
            float f = 0.6f + 0.4f * sinf(g->time * 1.7f);
            gfx_glow(x - 8, y - 31, 7 * f + 3, RGBA(255, 90, 20, 120)); gfx_glow(x + 8, y - 31, 7 * f + 3, RGBA(255, 90, 20, 120));
            break;
        }
        case E_SKULLKEY: gfx_glow(x, y - 9, 16 + sinf(g->time * 3) * 3, RGBA(255, 210, 90, 90)); break;
        case E_DOUBLOON: case E_GEAR: case E_GEM: {
            float tw = sinf(g->time * 4 + x * 0.3f);
            if (tw > 0.85f) { float k = (tw - 0.85f) / 0.15f; gfx_rect(x - 4 * k, y - 7, 8 * k, 1, RGBA(255, 255, 230, (int)(200 * k))); gfx_rect(x, y - 11 * k - 3, 1, 8 * k, RGBA(255, 255, 230, (int)(200 * k))); }
            gfx_glow(x, y - 6, 8, RGBA(255, 220, 120, 40));
            break;
        }
        case E_BIGCHEST: if (e->state) { gfx_glow(x, y - 14, 50 + sinf(g->time * 3) * 6, RGBA(255, 200, 80, 90)); gfx_glow(x, y - 12, 20, RGBA(255, 240, 180, 120)); } break;
        case E_GOLDPILE: { float tw = sinf(g->time * 3 + x); if (tw > 0.9f) gfx_glow(x + sinf(x) * 6, y - 6, 4, RGBA(255, 255, 220, 200)); } break;
        case E_PIPE: if (e->flash > 0) gfx_glow(x, y - 16, 16, RGBA(255, 240, 200, (int)(120 * e->flash))); break;
        }
    }
    draw_parts(g, 1);
}

/* ------------------------------------------------------------------ */
/* 5. rain and lightning (screen space)                                */
/* ------------------------------------------------------------------ */
static void bolt(const Game *g)
{
    if (g->bolt_t <= 0) return;
    float a = clampf(g->bolt_t / 0.32f, 0, 1);
    uint32_t s = g->bolt_seed;
    float x = g->bolt_x, y = -10;
    gfx_blend(BL_ADD);
    for (int i = 0; i < 9 && y < g->bolt_y; i++) {
        s = s * 1664525u + 1013904223u;
        float nx = x + ((float)((s >> 8) & 255) / 255.0f - 0.5f) * 60, ny = y + 18 + (float)((s >> 16) & 31);
        gfx_glowline(x, y, nx, ny, 14, RGBA(160, 180, 255, (int)(110 * a)));
        gfx_line(x, y, nx, ny, 2.2f, RGBA(240, 245, 255, (int)(255 * a)), RGBA(240, 245, 255, (int)(255 * a)));
        if (((s >> 24) & 3) == 0) {
            float bx = nx + ((float)((s >> 4) & 63) - 32), by = ny + 24;
            gfx_line(nx, ny, bx, by, 1.2f, RGBA(220, 230, 255, (int)(200 * a)), RGBA(220, 230, 255, 0));
        }
        x = nx; y = ny;
    }
}

static void rain(const Game *g)
{
    if (!g->ndrop) return;
    gfx_blend(BL_ALPHA);
    float slant = 0.18f + g->wind * 0.22f;
    for (int i = 0; i < g->ndrop; i++) {
        const Drop *d = &g->drop[i];
        float lit = light_at_screen(d->x, d->y);
        int a = (int)(40 + 70 * clampf(lit, 0, 1.6f)) + (int)(g->flash * 90);
        if (a > 200) a = 200;
        float dx = d->len * slant, dy = d->len;
        gfx_line(d->x - dx, d->y - dy, d->x, d->y, 1.3f, RGBA(170, 190, 230, 0), RGBA(200, 215, 245, a));
    }
}

void render_world(const Game *g)
{
    CX = floorf(g->camx + 0.5f); CY = floorf(g->camy + 0.5f);
    float shx = 0, shy = 0;
    if (g->shake > 0.05f) { shx = sinf(g->time * 73.0f) * g->shake; shy = cosf(g->time * 61.0f) * g->shake * 0.7f; }
    float sx = floorf(-CX * ZOOM + shx), sy = floorf(-CY * ZOOM + shy);
    gfx_xf(ZOOM, sx, sy);
    int x0 = (int)floorf(CX / TS) - 1, y0 = (int)floorf(CY / TS) - 1;
    int x1 = x0 + (int)(VIEW_W / TS) + 3, y1 = y0 + (int)(VIEW_H / TS) + 3;
    gfx_blend(BL_ALPHA);
    draw_tiles(g, x0, y0, x1, y1);
    float vx0 = CX, vy0 = CY, vx1 = CX + VIEW_W, vy1 = CY + VIEW_H;
    collect_and_draw(g, vx0, vy0, vx1, vy1);
    draw_parts(g, 0);
    /* light */
    gfx_xf_reset();
    light_mesh(g, fmodf(shx, 1.0f), fmodf(shy, 1.0f));
    /* glow */
    gfx_xf(ZOOM, sx, sy);
    emissive(g, vx0, vy0, vx1, vy1);
    gfx_xf_reset();
    bolt(g);
    rain(g);
    if (g->flash > 0) { gfx_blend(BL_ADD); gfx_rect(0, 0, VW, VH, RGBA(180, 200, 255, (int)(g->flash * g->flash * 70))); }
    gfx_blend(BL_ALPHA);
}
