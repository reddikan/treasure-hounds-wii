/*
 * TREASURE HOUNDS - HUD, speech bubbles, the dialog box, menus and the story screens.
 */
#include "game_int.h"
#include "music.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

void render_world(const Game *g);
float light_at_screen(float x, float y);

static const uint32_t KID_COL[CH_COUNT] = { 0xE84848FFu, 0xF2C232FFu, 0x52C45EFFu, 0x4C8CF4FFu, 0x9A9AA8FFu, 0xC07A40FFu, 0xB050A0FFu, 0xE0C890FFu };

/* 9-slice panel from a 24x24 atlas tile with 8 px corners */
static void panel9(const ArtRect *r, float x, float y, float w, float h, float s, uint32_t c)
{
    float cs = 8 * s;
    float xs[4] = { x, x + cs, x + w - cs, x + w }, ys[4] = { y, y + cs, y + h - cs, y + h };
    float us[4] = { 0, 8, 16, 24 };
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++)
            gfx_img(xs[i], ys[j], xs[i + 1] - xs[i], ys[j + 1] - ys[j], (float)r->x + us[i], (float)r->y + us[j], 8, 8, c, 0);
}
static void parch(float x, float y, float w, float h, float a)
{
    /* smooth paper in the middle, the torn painted edge from the atlas round it */
    panel9(&art_parch, x, y, w, h, 2, col_a(0xFFFFFFFFu, a));
    gfx_rect_v(x + 9, y + 9, w - 18, h - 18, RGBA(234, 214, 168, (int)(255 * a)), RGBA(218, 190, 140, (int)(255 * a)));
}
static void woodp(float x, float y, float w, float h, float a) { panel9(&art_frame, x, y, w, h, 2, col_a(0xFFFFFFFFu, a)); }

static void icon(int it, float x, float y, float s, uint32_t c)
{
    const ArtRect *r = &art_item[it];
    gfx_img(x - (float)r->w * s * 0.5f, y - (float)r->h * s * 0.5f, r->w * s, r->h * s, r->x, r->y, r->w, r->h, c, 0);
}
static void face(int who, int mood, float x, float y, float s, uint32_t c)
{
    const ArtRect *r = &art_face[who][mood];
    gfx_img(x, y, 32 * s, 32 * s, r->x, r->y, 32, 32, c, 0);
}

/* ------------------------------------------------------------------ */
/* HUD                                                                 */
/* ------------------------------------------------------------------ */
static void minimap(const Game *g, float x, float y, float w, float h, int full)
{
    const Kid *l = &g->kid[g->order[0]];
    float ts = full ? fminf(w / (float)g->lv.w, h / (float)g->lv.h) : 4.0f;
    int cx = (int)(l->x / TS), cy = (int)(l->y / TS);
    int nx = (int)(w / ts), ny = (int)(h / ts);
    int x0 = full ? 0 : cx - nx / 2, y0 = full ? 0 : cy - ny / 2;
    if (full) { nx = g->lv.w; ny = g->lv.h; x += (w - ts * (float)nx) * 0.5f; y += (h - ts * (float)ny) * 0.5f; }
    for (int j = 0; j < ny; j++) {
        int run = 0; uint32_t rc = 0;
        for (int i = 0; i <= nx; i++) {
            uint32_t c = 0;
            int tx = x0 + i, ty = y0 + j;
            if (i < nx && tx >= 0 && ty >= 0 && tx < g->lv.w && ty < g->lv.h && g->lv.seen[ty][tx]) {
                int k = g->lv.k[ty][tx];
                if (KINFO[k].water == 1) c = 0x3E6E92FFu;
                else if (KINFO[k].water == 2) c = 0x6A92A0FFu;
                else if (KINFO[k].wall) c = 0x5A3E24FFu;
                else if (k == K_VOID || k == K_HOLE) c = 0x2A1E14FFu;
                else if (k == K_PLANK || k == K_DECK) c = 0x9A6C3CFFu;
                else if (k == K_GRASS) c = 0x8A9A5AFFu;
                else c = 0xB09468FFu;
            }
            if (i < nx && c == rc) { run++; continue; }
            if (run > 0 && rc) gfx_rect(x + (float)(i - run) * ts, y + (float)j * ts, (float)run * ts, ts, rc);
            rc = c; run = 1;
        }
    }
    /* objective markers */
    for (int i = 0; i < g->nent; i++) {
        const Ent *e = &g->ent[i];
        if (!e->on) continue;
        int t = e->type;
        int mark = t == E_GEAR || t == E_SKULLKEY || t == E_BIGCHEST || (t == E_CRANE && !g->crane_done) || t == E_DOUBLOON || (t == E_GATE && !e->state);
        if (!mark) continue;
        int tx = (int)(e->x / TS), ty = (int)((e->y - 2) / TS);
        if (tx < 0 || ty < 0 || tx >= g->lv.w || ty >= g->lv.h || !g->lv.seen[ty][tx]) continue;
        float mx = x + ((float)(tx - x0) + 0.5f) * ts, my = y + ((float)(ty - y0) + 0.5f) * ts;
        if (mx < x || my < y || mx > x + (float)nx * ts || my > y + (float)ny * ts) continue;
        uint32_t c = t == E_GATE ? 0x6A3A20FFu : 0xC82020FFu;
        float s = full ? 3 : 2;
        gfx_rect(mx - s, my - 0.5f, s * 2, 1.2f, c); gfx_rect(mx - 0.5f, my - s, 1.2f, s * 2, c);
    }
    float px = x + ((l->x / TS) - (float)x0) * ts, py = y + ((l->y / TS) - (float)y0) * ts;
    float a = l->dir == DIR_UP ? -1.5708f : (l->dir == DIR_DOWN ? 1.5708f : (l->flip ? 3.14159f : 0));
    float s = full ? 6 : 4.5f;
    float pulse = 0.8f + 0.2f * sinf(g->time * 6);
    gfx_tri(px + cosf(a) * s * pulse, py + sinf(a) * s * pulse, 0xE83020FFu,
            px + cosf(a + 2.4f) * s, py + sinf(a + 2.4f) * s, 0xA01810FFu,
            px + cosf(a - 2.4f) * s, py + sinf(a - 2.4f) * s, 0xA01810FFu);
}

static void hud(const Game *g)
{
    /* the four Hounds */
    for (int i = 0; i < NKIDS; i++) {
        int lead = g->order[0] == i;
        float x = 14 + (float)i * 54, y = 10;
        const Kid *k = &g->kid[i];
        uint32_t ring = lead ? 0xFFD860FFu : 0x2A1C14FFu;
        gfx_ellipse(x + 24, y + 24, 25, 25, 20, ring, ring);
        gfx_ellipse(x + 24, y + 24, 22, 22, 20, 0x3A2A22FFu, 0x1A120EFFu);
        int mood = k->hurt > 0 ? MOOD_SHOCK : (k->state == KS_CHEER ? MOOD_HAPPY : MOOD_NORMAL);
        face(i, mood, x + 0, y + 0, 1.5f, k->hp == 0 ? 0x8888A0FFu : 0xFFFFFFFFu);
        /* breath bar */
        float bw = 42, bx = x + 3, by = y + 52;
        gfx_rect(bx - 1, by - 1, bw + 2, 7, 0x140C08FFu);
        gfx_rect(bx, by, bw, 5, 0x3A2A24FFu);
        float f = (float)k->hp / (float)KID_HP;
        uint32_t c = KID_COL[i];
        if (k->hp <= 1 && ((int)(g->time * 4) & 1)) c = 0xFFFFFFFFu;
        gfx_rect(bx, by, bw * f, 5, c);
        gfx_rect(bx, by, bw * f, 1.5f, col_mix(c, 0xFFFFFFFFu, 0.5f));
        if (lead) gfx_text_sc(CHAR_NAME[i], x + 24, y + 61, 1, 0xFFE8A0FFu);
    }
    /* coins + doubloons */
    {
        float x = 16, y = 86;
        float bounce = g->hud_coin_t > 0 ? sinf(g->hud_coin_t * 20) * 2 : 0;
        icon(IT_COIN, x + 8, y + 8 - bounce, 2, 0xFFFFFFFFu);
        char buf[32];
        snprintf(buf, sizeof buf, "%d", g->coins);
        gfx_text_s(buf, x + 20, y + 1, 2, 0xFFE070FFu);
        icon(IT_DOUBLOON, x + 98, y + 8, 2, 0xFFFFFFFFu);
        snprintf(buf, sizeof buf, "%d", g->doubloons);
        gfx_text_s(buf, x + 112, y + 1, 2, 0xFFE070FFu);
    }
    /* minimap, top right */
    {
        float w = 136, h = 88, x = VW - w - 14, y = 12;
        parch(x - 6, y - 6, w + 12, h + 12, 0.95f);
        minimap(g, x, y, w, h, 0);
        gfx_text_s(ICON_X, x + w - 8, y + h - 6, 1, 0xFFFFFFFFu);
    }
    /* item slots, bottom left */
    {
        int items[3] = { IT_LANTERN, IT_WHIP, IT_MAP };
        if (g->level == 0 && !g->crane_done) items[2] = IT_GEAR;
        if (g->has_key) items[2] = IT_SKULLKEY;
        for (int i = 0; i < 3; i++) {
            float x = 14 + (float)i * 46, y = VH - 54;
            woodp(x, y, 40, 40, 0.95f);
            icon(items[i], x + 20, y + 20, items[i] == IT_SKULLKEY ? 1.5f : 2, 0xFFFFFFFFu);
        }
        if (items[2] == IT_GEAR) {
            char buf[8];
            snprintf(buf, sizeof buf, "%d/3", g->gears);
            gfx_text_s(buf, 14 + 2 * 46 + 18, VH - 24, 1, 0xFFFFFFFFu);
        }
        gfx_text_s(ICON_B, 14 + 46 + 30, VH - 24, 1, 0xFFFFFFFFu);
    }
    /* toast */
    if (g->toast_t > 0 && g->toast[0]) {
        float a = clampf(g->toast_t * 2, 0, 1);
        float w = gfx_text_w(g->toast, 2) + 36;
        float x = (VW - w) * 0.5f, y = 116;
        parch(x, y, w, 38, a);
        gfx_text_c(g->toast, VW * 0.5f, y + 11, 2, RGBA(58, 30, 12, (int)(255 * a)));
    }
}

/* speech bubbles over the kids' heads */
static void bubbles(const Game *g)
{
    float cx = floorf(g->camx + 0.5f), cy = floorf(g->camy + 0.5f);
    for (int i = 0; i < NKIDS; i++) {
        const Kid *k = &g->kid[i];
        if (k->bubble_t <= 0 || !k->bubble[0]) continue;
        char buf[200];
        int lines = gfx_wrap(k->bubble, buf, sizeof buf, 2, 230);
        float tw = gfx_text_w(buf, 2), th = (float)lines * 24 - 6;
        float a = clampf(k->bubble_t * 3, 0, 1) * clampf((k->bubble_max - k->bubble_t) * 8, 0, 1);
        float sx = (k->x - cx) * ZOOM, sy = (k->y - k->z - cy) * ZOOM - 62;
        float w = tw + 22, h = th + 20;
        float bx = clampf(sx - w * 0.5f, 8, VW - w - 8), by = clampf(sy - h, 106, VH - h - 70);
        uint32_t edge = col_a(KID_COL[i], a);
        gfx_rect(bx - 2, by - 2, w + 4, h + 4, RGBA(30, 16, 8, (int)(220 * a)));
        gfx_rect(bx, by, w, h, RGBA(250, 238, 210, (int)(245 * a)));
        gfx_rect(bx, by, 4, h, edge);
        float tx = clampf(sx, bx + 10, bx + w - 10);
        gfx_tri(tx - 7, by + h, RGBA(250, 238, 210, (int)(245 * a)), tx + 7, by + h, RGBA(250, 238, 210, (int)(245 * a)), tx, by + h + 10, RGBA(250, 238, 210, (int)(245 * a)));
        gfx_text(buf, bx + 13, by + 10, 2, RGBA(46, 26, 14, (int)(255 * a)));
    }
}

/* ------------------------------------------------------------------ */
/* the dialog box                                                      */
/* ------------------------------------------------------------------ */
void render_dialog(const Game *g)
{
    const Line *ln = SCENES[g->scene] ? &SCENES[g->scene][g->line] : 0;
    if (!ln || !ln->text) return;
    float x = 26, w = VW - 52, h = 124, y = VH - h - 14;
    float in = clampf(g->line_t * 6, 0, 1);
    y += (1 - in) * 20;
    woodp(x, y, w, h, 0.97f);
    gfx_rect(x + 8, y + 8, w - 16, h - 16, RGBA(24, 16, 12, 200));
    float tx = x + 26;
    if (ln->who >= 0 || ln->who == SPK_SIGN || ln->who == SPK_ALL) {
        float px = x + 18, py = y + 16;
        gfx_rect(px - 3, py - 3, 70, 70, 0x140C08FFu);
        gfx_rect(px, py, 64, 64, ln->who >= 0 ? col_mix(KID_COL[ln->who], 0x1A1010FFu, 0.65f) : 0x3A2A1EFFu);
        if (ln->who == SPK_ALL) {
            for (int i = 0; i < 4; i++) face(i, MOOD_HAPPY, px + (float)(i & 1) * 32, py + (float)(i >> 1) * 32, 1, 0xFFFFFFFFu);
        } else {
            int who = ln->who == SPK_SIGN ? FACE_SKULL : ln->who;
            int mood = ln->mood;
            /* talking: flick between the line's mood and a neutral face */
            if (g->chars < (float)strlen(g->wrap) && ((int)(g->line_t * 9) & 1) && mood == MOOD_NORMAL && ln->who >= 0) mood = MOOD_HAPPY;
            face(who, mood, px, py, 2, 0xFFFFFFFFu);
        }
        tx = px + 84;
        const char *name = ln->who >= 0 ? CHAR_NAME[ln->who] : (ln->who == SPK_ALL ? "Everybody" : "Carved words");
        uint32_t nc = ln->who >= 0 ? KID_COL[ln->who] : 0xE8C890FFu;
        gfx_text_s(name, tx, y + 15, 2, nc);
    }
    float ty = (ln->who == SPK_NARR) ? y + 22 : y + 42;
    gfx_text_n(g->wrap, tx, ty, 2, ln->who == SPK_NARR ? 0xE8DCC8FFu : 0xFFF6E6FFu, (int)g->chars, 0);
    if (g->chars >= (float)strlen(g->wrap) && ((int)(g->time * 3) & 1))
        gfx_text(ICON_A, x + w - 34, y + h - 30 + sinf(g->time * 6) * 2, 2, 0xFFFFFFFFu);
}

/* ------------------------------------------------------------------ */
/* screens                                                             */
/* ------------------------------------------------------------------ */
static void dim(float a) { gfx_rect(0, 0, VW, VH, RGBA(6, 4, 12, (int)(255 * a))); }

void render_loading(float p)
{
    gfx_begin();
    gfx_rect_v(0, 0, VW, VH, 0x0C0A18FFu, 0x020208FFu);
    const ArtRect *r = &art_logo;
    gfx_img(VW * 0.5f - r->w, 110, r->w * 2, r->h * 2, r->x, r->y, r->w, r->h, 0xFFFFFFFFu, 0);
    float w = 360, x = (VW - w) * 0.5f, y = 330;
    gfx_rect(x - 3, y - 3, w + 6, 20, 0x5A3A1CFFu);
    gfx_rect(x, y, w, 14, 0x1A120CFFu);
    gfx_rect_v(x, y, w * clampf(p, 0, 1), 14, 0xFFE070FFu, 0xC07818FFu);
    static const char *msg[] = { "Tuning the bone organ...", "Teaching Rocco manners... (failed)", "Hiding Ollie's snacks...", "Polishing Gus's glasses...", "Unfolding Grandpa's map..." };
    int i = (int)(p * 4.99f);
    gfx_text_c(msg[i < 0 ? 0 : (i > 4 ? 4 : i)], VW * 0.5f, y + 32, 2, 0xC8B898FFu);
}

void render_title(const Game *g)
{
    render_world(g);
    dim(0.25f);
    float t = g->title_t;
    /* logo */
    const ArtRect *r = &art_logo;
    float s = 3.0f;
    float lw = (float)r->w * s, lh = (float)r->h * s;
    float ly = 46 + sinf(t * 1.2f) * 4;
    gfx_img(VW * 0.5f - lw * 0.5f, ly, lw, lh, r->x, r->y, r->w, r->h, 0xFFFFFFFFu, 0);
    /* a glint sweeping across the gold */
    gfx_blend(BL_ADD);
    float gx = fmodf(t * 260.0f, VW + 600) - 300;
    gfx_quad4(gx, ly, RGBA(255, 255, 220, 0), gx + 40, ly, RGBA(255, 255, 220, 90), gx + 10, ly + lh, RGBA(255, 255, 220, 90), gx - 30, ly + lh, RGBA(255, 255, 220, 0));
    gfx_blend(BL_ALPHA);
    gfx_text_sc("~ The Black Lantern's Gold ~", VW * 0.5f, ly + lh + 4, 2, 0xE8D8B0FFu);
    /* menu */
    char buf[64];
    const char *items[4];
    static char ch[64], mu[32], so[32];
    items[0] = "START ADVENTURE";
    snprintf(ch, sizeof ch, "CHAPTER   <  %d. %s  >", g->chapter_pick + 1, LEVELS[g->chapter_pick].name);
    snprintf(mu, sizeof mu, "MUSIC:  %s", g->music_on ? "ON" : "OFF");
    snprintf(so, sizeof so, "SOUND:  %s", g->sound_on ? "ON" : "OFF");
    items[1] = ch; items[2] = mu; items[3] = so;
    float my = 300;
    parch(VW * 0.5f - 200, my - 16, 400, 150, 0.92f);
    for (int i = 0; i < 4; i++) {
        int sel = g->menu == i;
        uint32_t c = sel ? 0x8A2010FFu : 0x4A3420FFu;
        if (sel) {
            gfx_text_wave(items[i], VW * 0.5f, my + (float)i * 30, 2, c, t, 1.5f);
            icon(IT_SKULLKEY, VW * 0.5f - gfx_text_w(items[i], 2) * 0.5f - 22, my + (float)i * 30 + 8, 1.5f, 0xFFFFFFFFu);
        } else gfx_text_c(items[i], VW * 0.5f, my + (float)i * 30, 2, c);
    }
    snprintf(buf, sizeof buf, ICON_A " select    " ICON_L ICON_R " / < > change");
    gfx_text_sc(buf, VW * 0.5f, VH - 24, 1, 0xC8C0B0FFu);
    gfx_text_s("v1.0", 8, VH - 14, 1, 0x8A8070FFu);
}

void render_story(const Game *g)
{
    gfx_rect_v(0, 0, VW, VH, 0x0A0814FFu, 0x020206FFu);
    gfx_blend(BL_ADD);
    for (int i = 0; i < 40; i++) {
        float x = fmodf((float)(i * 97) + g->time * 6.0f * (float)(1 + i % 3), VW), y = fmodf((float)(i * 53) + sinf((float)i) * 40, VH);
        gfx_glow(x, y, 2 + (float)(i % 3), RGBA(255, 220, 160, 40));
    }
    gfx_blend(BL_ALPHA);
    float a = clampf(g->st_t * 2.0f, 0, 1);
    float w = 640, h = 220, x = (VW - w) * 0.5f, y = (VH - h) * 0.5f - 10;
    parch(x, y, w, h, a);
    const char *s = STORY_PAGES[g->story_page];
    int lines = 1;
    for (const char *p = s; *p; p++) if (*p == '\n') lines++;
    gfx_text(s, x + 40, y + h * 0.5f - (float)lines * 12, 2, RGBA(58, 30, 12, (int)(255 * a)));
    /* page dots */
    for (int i = 0; i < NSTORY; i++) gfx_ellipse(VW * 0.5f - (float)(NSTORY - 1) * 9 + (float)i * 18, y + h + 22, 4, 4, 8, i == g->story_page ? 0xFFD860FFu : 0x5A4A3AFFu, i == g->story_page ? 0xFFD860FFu : 0x5A4A3AFFu);
    gfx_text_sc(ICON_A " next    " ICON_B " skip", VW * 0.5f, VH - 30, 1, 0xA89880FFu);
}

void render_chapter(const Game *g)
{
    render_world(g);
    float t = g->st_t;
    float a = t < 0.4f ? t / 0.4f : (t > 2.6f ? clampf((3.2f - t) / 0.6f, 0, 1) : 1);
    dim(0.55f * a + 0.2f);
    const LevelData *ld = &LEVELS[g->level];
    gfx_text_sc(ld->subtitle, VW * 0.5f, 170, 2, RGBA(232, 216, 176, (int)(255 * a)));
    gfx_text_wave(ld->name, VW * 0.5f, 200, 5, RGBA(255, 214, 90, (int)(255 * a)), t, 2);
    gfx_rect(VW * 0.5f - 160 * a, 262, 320 * a, 2, RGBA(255, 214, 90, (int)(200 * a)));
}

void render_pause(const Game *g)
{
    dim(0.6f);
    float w = 360, h = 230, x = (VW - w) * 0.5f, y = (VH - h) * 0.5f;
    parch(x, y, w, h, 1);
    gfx_text_c("PAUSED", VW * 0.5f, y + 22, 3, 0x6A2010FFu);
    static char mu[32], so[32];
    snprintf(mu, sizeof mu, "MUSIC:  %s", g->music_on ? "ON" : "OFF");
    snprintf(so, sizeof so, "SOUND:  %s", g->sound_on ? "ON" : "OFF");
    const char *items[5] = { "RESUME", mu, so, "BACK TO CHECKPOINT", "QUIT TO TITLE" };
    for (int i = 0; i < 5; i++) {
        int sel = g->menu == i;
        gfx_text_c(items[i], VW * 0.5f, y + 70 + (float)i * 28, 2, sel ? 0x8A2010FFu : 0x4A3420FFu);
        if (sel) icon(IT_SKULLKEY, VW * 0.5f - gfx_text_w(items[i], 2) * 0.5f - 22, y + 78 + (float)i * 28, 1.5f, 0xFFFFFFFFu);
    }
}

void render_gameover(const Game *g)
{
    float a = clampf(g->st_t * 1.5f, 0, 1);
    dim(0.7f * a);
    gfx_text_wave("THE HOUNDS ARE POOPED!", VW * 0.5f, 170, 3, RGBA(255, 120, 90, (int)(255 * a)), g->time, 2);
    for (int i = 0; i < 4; i++) face(i, MOOD_SHOCK, VW * 0.5f - 136 + (float)i * 70, 230, 2, col_a(0xFFFFFFFFu, a));
    if (g->st_t > 1.2f) gfx_text_sc("Press " ICON_A " to try again from the last checkpoint", VW * 0.5f, 330, 2, 0xE8DCC8FFu);
}

void render_leveldone(const Game *g)
{
    dim(0.65f);
    float w = 460, h = 250, x = (VW - w) * 0.5f, y = (VH - h) * 0.5f;
    parch(x, y, w, h, 1);
    char buf[80];
    snprintf(buf, sizeof buf, "%s complete!", LEVELS[g->level].name);
    gfx_text_c(buf, VW * 0.5f, y + 24, 2, 0x6A2010FFu);
    icon(IT_COIN, x + 120, y + 84, 3, 0xFFFFFFFFu);
    snprintf(buf, sizeof buf, "Treasure:  %d", g->lvl_coins);
    gfx_text(buf, x + 146, y + 76, 2, 0x3A2010FFu);
    icon(IT_DOUBLOON, x + 120, y + 124, 3, 0xFFFFFFFFu);
    snprintf(buf, sizeof buf, "Ironjaw's doubloons:  %d / 3", g->lvl_doubloons);
    gfx_text(buf, x + 146, y + 116, 2, 0x3A2010FFu);
    for (int i = 0; i < 4; i++) face(i, MOOD_HAPPY, x + 82 + (float)i * 76, y + 150, 1.5f, 0xFFFFFFFFu);
    if (g->st_t > 1.0f && ((int)(g->time * 3) & 1)) gfx_text_c(ICON_A " onward!", VW * 0.5f, y + h - 30, 2, 0x4A3420FFu);
}

void render_ending(const Game *g)
{
    float t = g->end_t;
    gfx_rect_v(0, 0, VW, VH, 0x1A1008FFu, 0x060402FFu);
    gfx_blend(BL_ADD);
    for (int i = 0; i < 60; i++) {
        float x = fmodf((float)(i * 131) + sinf(t + (float)i) * 20, VW), y = fmodf((float)(i * 71) + t * (30 + (float)(i % 5) * 12), VH + 20) - 10;
        gfx_glow(x, y, 3 + (float)(i % 3), RGBA(255, 210, 90, 60));
    }
    gfx_blend(BL_ALPHA);
    /* falling coins */
    for (int i = 0; i < 40; i++) {
        float x = fmodf((float)(i * 211), VW), y = fmodf((float)(i * 37) + t * (60 + (float)(i % 7) * 10), VH + 30) - 15;
        icon(IT_COIN + (((int)(t * 8) + i) & 3), x, y, 2, 0xFFFFFFFFu);
    }
    int page = g->end_stage;
    if (page < NENDING) {
        float a = clampf(g->st_t * 1.5f, 0, 1);
        float w = 660, h = 180, x = (VW - w) * 0.5f, y = 150;
        parch(x, y, w, h, a);
        const char *s = ENDING_PAGES[page];
        int lines = 1;
        for (const char *p = s; *p; p++) if (*p == '\n') lines++;
        gfx_text(s, x + 36, y + h * 0.5f - (float)lines * 12, 2, RGBA(58, 30, 12, (int)(255 * a)));
        for (int i = 0; i < 4; i++) face(i, MOOD_HAPPY, VW * 0.5f - 136 + (float)i * 70, 50, 2, 0xFFFFFFFFu);
    } else {
        const ArtRect *r = &art_logo;
        gfx_img(VW * 0.5f - r->w * 1.5f, 40, r->w * 3, r->h * 3, r->x, r->y, r->w, r->h, 0xFFFFFFFFu, 0);
        gfx_text_wave("THE END", VW * 0.5f, 250, 4, 0xFFD860FFu, g->time, 2);
        char buf[96];
        snprintf(buf, sizeof buf, "Treasure found: %d      Doubloons: %d / 12", g->coins, g->doubloons);
        gfx_text_sc(buf, VW * 0.5f, 320, 2, 0xE8DCC8FFu);
        gfx_text_sc("Starring Sam, Rocco, Gus, Ollie... and Tiny", VW * 0.5f, 360, 2, 0xC8B898FFu);
        gfx_text_sc("Hounds stick together!", VW * 0.5f, 392, 2, 0xFFE8A0FFu);
        if (g->st_t > 2 && ((int)(g->time * 2) & 1)) gfx_text_sc("Press " ICON_A " to return to the title", VW * 0.5f, VH - 34, 1, 0xA89880FFu);
    }
}

void render_map(const Game *g)
{
    dim(0.55f);
    float w = 720, h = 410, x = (VW - w) * 0.5f, y = (VH - h) * 0.5f + 6;
    parch(x, y, w, h, 1);
    gfx_text_c(LEVELS[g->level].name, VW * 0.5f, y + 14, 2, 0x6A2010FFu);
    minimap(g, x + 24, y + 40, w - 48, h - 64, 1);
    gfx_text_c(ICON_X " / " ICON_B " close", VW * 0.5f, y + h - 18, 1, 0x4A3420FFu);
}

void render_ui(const Game *g)
{
    if (g->state == ST_PLAY || g->state == ST_DIALOG || g->state == ST_PAUSE || g->state == ST_MAP) {
        bubbles(g);
        hud(g);
    }
    if (g->state == ST_DIALOG) render_dialog(g);
}
