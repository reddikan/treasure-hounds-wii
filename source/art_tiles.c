/*
 * TREASURE HOUNDS - ground and wall tiles (16x16, seamless, 4 variants or frames each).
 */
#include "pix.h"

void pix_commit_tile(ArtRect *r);

static float wn(int x, int y, int cell, uint32_t s) { return pnoise((float)x, (float)y, cell, 16, s); }
static inline int wrap(int v) { return v & 15; }
static void PW(int x, int y, uint32_t c) { P(wrap(x), wrap(y), c); }

/* ---- floors ---- */
static void grass(int v)
{
    uint32_t s = 100 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 8, s) * 0.55f + wn(x, y, 4, s + 9) * 0.3f + hn(x, y, s) * 0.15f;
            uint32_t c = n < 0.38f ? C(0x2B4A2A) : (n < 0.55f ? C(0x355A30) : (n < 0.7f ? C(0x406A36) : C(0x4C7A3C)));
            P(x, y, c);
        }
    /* blades: short light strokes with a dark root */
    for (int i = 0; i < 22; i++) {
        int x = (int)(hn(i, 1, s) * 16), y = (int)(hn(i, 2, s) * 16);
        PW(x, y, C(0x5E9048)); PW(x, y - 1, C(0x76A858)); PW(x, y + 1, C(0x22402A));
    }
    if (v == 3) {                                  /* a few wildflowers */
        PW(4, 5, C(0xE8E0C8)); PW(3, 5, C(0xC8C0A8)); PW(4, 6, C(0xE8C860));
        PW(11, 12, C(0xD88AA8)); PW(12, 12, C(0xB86A88));
    }
    if (v == 2) { PW(9, 3, C(0x6A6458)); PW(10, 3, C(0x7E786A)); PW(10, 4, C(0x58524A)); }
}
static void dirt(int v)
{
    uint32_t s = 200 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 8, s) * 0.5f + wn(x, y, 4, s + 3) * 0.3f + hn(x, y, s) * 0.2f;
            uint32_t c = n < 0.35f ? C(0x4E3828) : (n < 0.55f ? C(0x5E4430) : (n < 0.72f ? C(0x6C5038) : C(0x7A5C40)));
            P(x, y, c);
        }
    for (int i = 0; i < 4; i++) {                  /* pebbles */
        int x = (int)(hn(i, 7, s) * 16), y = (int)(hn(i, 8, s) * 16);
        PW(x, y, C(0x8A8070)); PW(x + 1, y, C(0x6E665A)); PW(x, y + 1, C(0x3A2C20)); PW(x + 1, y + 1, C(0x3A2C20));
    }
    if (v == 1) for (int x = 3; x < 13; x++) PW(x, 9 + (x > 7), C(0x46301E));   /* a wheel rut */
    if (v == 3) { PW(6, 6, C(0x3C5A8A)); PW(7, 6, C(0x5A7AAA)); PW(8, 6, C(0x3C5A8A)); PW(7, 7, C(0x2E4870)); }  /* puddle */
}
/* jittered-grid voronoi stones (wrapping) */
static void stones(uint32_t s, int cell, const uint32_t *pal, int npal, uint32_t mortar, float moss)
{
    int g = 16 / cell;
    float px[8][8], py[8][8];
    for (int j = 0; j < g; j++)
        for (int i = 0; i < g; i++) {
            px[j][i] = (float)(i * cell) + 1.0f + hn(i, j, s) * (float)(cell - 2);
            py[j][i] = (float)(j * cell) + 1.0f + hn(i, j, s + 1) * (float)(cell - 2);
        }
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float d1 = 1e9f, d2 = 1e9f;
            int bi = 0, bj = 0;
            for (int j = -1; j <= g; j++)
                for (int i = -1; i <= g; i++) {
                    int ii = (i + g) % g, jj = (j + g) % g;
                    float cx = px[jj][ii] + (float)((i < 0) ? -16 : (i >= g ? 16 : 0));
                    float cy = py[jj][ii] + (float)((j < 0) ? -16 : (j >= g ? 16 : 0));
                    float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
                    float d = dx * dx + dy * dy;
                    if (d < d1) { d2 = d1; d1 = d; bi = ii; bj = jj; }
                    else if (d < d2) d2 = d;
                }
            float edge = sqrtf(d2) - sqrtf(d1);
            uint32_t base = pal[hash2(bi, bj, s + 5) % (uint32_t)npal];
            uint32_t c = base;
            /* light from the top-left: stone pixels near the upper-left of their cell are lighter */
            float lx = (float)x - px[bj][bi], ly = (float)y - py[bj][bi];
            if (lx < -16 + 8) lx += 16;
            if (ly < -16 + 8) ly += 16;
            float shade = -(lx + ly) * 0.04f + (hn(x, y, s + 7) - 0.5f) * 0.14f;
            c = pdark(c, 1.0f + shade);
            if (moss > 0 && wn(x, y, 8, s + 11) > 1.0f - moss) c = pmix(c, C(0x4E6A30), 0.65f);
            if (edge < 1.0f) c = mortar;
            else if (edge < 1.8f) c = pmix(c, mortar, 0.45f);
            P(x, y, c);
        }
}
static void cobble(int v)
{
    static const uint32_t pal[] = { 0x686460FF, 0x74706AFF, 0x5E5A58FF, 0x6E6862FF };
    stones(300 + v, 5, pal, 4, C(0x2E2C2C), v == 2 ? 0.25f : 0.0f);
    (void)pal;
}
static void plank(int v, int deck)
{
    uint32_t s = 400 + v + deck * 50;
    uint32_t base = deck ? C(0x6A4A30) : C(0x7E5A3A), lite = deck ? C(0x7E5A3A) : C(0x946C46), dk = deck ? C(0x3C2618) : C(0x4A3020);
    for (int y = 0; y < 16; y++) {
        int board = y / 4, yy = y % 4;
        int seam = (int)(hn(board, v, s) * 16);
        for (int x = 0; x < 16; x++) {
            float g = wn(x * 1, y * 4, 8, s + board) * 0.6f + hn(x, y, s) * 0.4f;
            uint32_t c = pmix(base, lite, g * 0.8f);
            if (yy == 0) c = pdark(lite, 1.08f);
            if (yy == 3) c = dk;
            if (x == seam && yy != 3) c = dk;
            if ((x == seam + 1 || x == seam - 1) && yy == 1 && v != 3) c = C(0xA0A0A8);   /* nails */
            if (hn(x / 3, y, s + 9) > 0.93f && yy == 2) c = pdark(base, 0.8f);           /* knots / grain */
            P(x, y, c);
        }
    }
}
static void sand(int v, int wet)
{
    uint32_t s = 500 + v + wet * 7;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float rip = sinf(((float)y + wn(x, y, 8, s) * 6.0f) * 0.8f);
            float n = hn(x, y, s);
            uint32_t c = wet ? C(0x8A7656) : C(0xC0A676);
            if (rip > 0.6f) c = wet ? C(0x7A6648) : C(0xA88E62);
            else if (rip < -0.7f) c = wet ? C(0x9A8662) : C(0xD0B888);
            if (n > 0.92f) c = wet ? C(0xB0A080) : C(0xE6D2A4);
            if (n < 0.05f) c = wet ? C(0x5E4E38) : C(0x9A8058);
            P(x, y, c);
        }
    if (wet) {                                     /* glassy sheen streaks */
        for (int x = 2; x < 9; x++) PW(x + v * 3, 4 + v, C(0xB6B0A0));
        for (int x = 9; x < 13; x++) PW(x + v * 2, 11, C(0xA8A090));
    }
    if (!wet && v == 2) { PW(5, 8, C(0xF0E0D0)); PW(6, 8, C(0xE8C8B8)); PW(5, 9, C(0xD0A898)); PW(6, 9, C(0xF0E0D0)); }
}
static void cave(int v, int mud)
{
    uint32_t s = 600 + v + mud * 9;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 8, s) * 0.5f + wn(x, y, 4, s + 2) * 0.3f + hn(x, y, s) * 0.2f;
            uint32_t c;
            if (mud) c = n < 0.4f ? C(0x3A2E26) : (n < 0.6f ? C(0x46382C) : C(0x524234));
            else c = n < 0.38f ? C(0x3A3640) : (n < 0.55f ? C(0x46424C) : (n < 0.72f ? C(0x524C56) : C(0x5E5862)));
            P(x, y, c);
        }
    /* a crack */
    int x = (int)(hn(1, 1, s) * 16), y = (int)(hn(2, 1, s) * 16);
    for (int i = 0; i < 7; i++) {
        PW(x, y, mud ? C(0x2A201A) : C(0x26222C));
        x += hn(i, 3, s) > 0.5f ? 1 : 0; y += 1;
        if (hn(i, 4, s) > 0.6f) x--;
    }
    for (int i = 0; i < 3; i++) {
        int px = (int)(hn(i, 9, s) * 16), py = (int)(hn(i, 10, s) * 16);
        PW(px, py, mud ? C(0x6A5A48) : C(0x726C78)); PW(px, py + 1, C(0x221E26));
    }
    if (mud && v == 1) { PW(6, 7, C(0x5A6A7A)); PW(7, 7, C(0x7A8A9A)); PW(8, 8, C(0x4A5A6A)); }
}
static void slab(int v, int moss)
{
    uint32_t s = 700 + v + moss * 13;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            /* worn flagstones: 2x2 per tile, or one long slab across two rows */
            int bx = x % 8, by = y % 8, blk = (x / 8) + (y / 8) * 2, bw = 8;
            if (v == 1) { bx = x; by = y % 8; blk = y / 8; bw = 16; }
            float tone = 0.84f + 0.3f * hn(blk, v, s);
            float n = wn(x, y, 4, s) * 0.5f + hn(x, y, s) * 0.5f;
            uint32_t c = pdark(pmix(C(0x6E6A58), C(0x827C68), n), tone);
            if (bx == 0 || by == 0) c = pdark(c, 1.18f);                       /* lit upper-left bevel */
            if (by == 7 || bx == bw - 1) c = C(0x34302A);                     /* joint */
            if ((by == 6 || bx == bw - 2) && by != 7 && bx != bw - 1) c = pdark(c, 0.86f);
            if (moss && wn(x, y, 8, s + 5) > 0.55f) c = pmix(c, (hn(x, y, s + 6) > 0.5f) ? C(0x4E6A2E) : C(0x3E5824), 0.75f);
            if (!moss && (by == 7 || bx == bw - 1) && hn(x, y, s + 9) > 0.7f) c = C(0x3A4A26);   /* moss in the joints */
            P(x, y, c);
        }
    if (v == 2) { PW(3, 9, C(0x2A2620)); PW(4, 10, C(0x2A2620)); PW(5, 10, C(0x2A2620)); PW(6, 11, C(0x2A2620)); PW(5, 11, C(0x2A2620)); }
    if (v == 3 && !moss) {                                                   /* a carved spiral */
        for (int k = 0; k < 18; k++) { float a = (float)k * 0.6f, r = 0.5f + (float)k * 0.17f; PW(4 + (int)(cosf(a) * r), 4 + (int)(sinf(a) * r), C(0x4A463A)); }
    }
}

/* ---- liquids (4 animation frames) ---- */
static void water(int f, int kind)          /* 0 sea, 1 cave pool, 2 shallow */
{
    uint32_t deep = kind == 0 ? C(0x15324A) : (kind == 1 ? C(0x0D3640) : C(0x2E5A60));
    uint32_t mid  = kind == 0 ? C(0x1A3C58) : (kind == 1 ? C(0x114650) : C(0x386A6C));
    uint32_t lite = kind == 0 ? C(0x2E6482) : (kind == 1 ? C(0x237E80) : C(0x5A8E88));
    uint32_t foam = kind == 0 ? C(0x7AAEC4) : (kind == 1 ? C(0x6ED0C8) : C(0x9CC4B4));
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 8, 900 + kind) * 0.7f + wn(x, y, 4, 950) * 0.3f;
            uint32_t c = pmix(deep, mid, n);
            if (kind == 2 && hn(x, y, 905) > 0.9f) c = pmix(c, C(0xB0A070), 0.35f);
            P(x, y, c);
        }
    /* little wave crests that drift sideways frame by frame */
    static const int cx[6] = { 1, 9, 5, 12, 3, 10 }, cy[6] = { 2, 4, 8, 10, 13, 15 };
    for (int i = 0; i < 6; i++) {
        int x = cx[i] + f * 2 * ((i & 1) ? 1 : -1) + (i * 3), y = cy[i];
        int len = 3 + (i % 3);
        for (int k = 0; k < len; k++) PW(x + k, y, (k == 0 || k == len - 1) ? lite : foam);
        for (int k = 0; k < len; k++) PW(x + k, y + 1, pmix(deep, C(0x0A1A28), 0.4f));
    }
}

/* ---- walls ---- */
static void rock_top(int v, int cavek)
{
    uint32_t s = 1000 + v + cavek * 20;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 8, s) * 0.6f + wn(x, y, 4, s + 1) * 0.25f + hn(x, y, s) * 0.15f;
            uint32_t c;
            if (cavek) c = n < 0.4f ? C(0x1E1A22) : (n < 0.6f ? C(0x26222A) : C(0x302A34));
            else {
                c = n < 0.4f ? C(0x3A3836) : (n < 0.6f ? C(0x464240) : C(0x52504A));
                if (wn(x, y, 8, s + 30) > 0.6f) c = pmix(c, C(0x2E4A28), 0.7f);      /* moss on cliff tops */
            }
            P(x, y, c);
        }
}
static void rock_face(int v, int cavek, int lower)
{
    uint32_t s = 1100 + v + cavek * 20;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            /* vertical strata with horizontal ledges */
            float col = wn(x * 2, y / 3, 8, s) * 0.6f + hn(x, y / 2, s) * 0.4f;
            float t = lower ? 0.5f + (float)y / 30.0f : (float)y / 15.0f * 0.75f;
            uint32_t top = cavek ? C(0x5A5060) : C(0x7A746C), bot = cavek ? C(0x2E2834) : C(0x3E3A38);
            uint32_t c = pmix(top, bot, t * 0.85f + col * 0.25f);
            if (hn(x / 3, y / 5, s + 3) > 0.7f && (y % 5) == 0) c = pdark(c, 1.25f);   /* ledge highlights */
            if (col < 0.25f) c = pdark(c, 0.75f);
            P(x, y, c);
        }
    if (!lower) for (int x = 0; x < 16; x++) {      /* bright rim where the top meets the face */
        P(x, 0, cavek ? C(0x6E6478) : C(0x948C80));
        if (hn(x, 0, s) > 0.5f) P(x, 1, cavek ? C(0x625870) : C(0x88827A));
    }
    if (lower) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) P(x, y, pdark(G(x, y), 0.86f - (float)y * 0.012f));
    if (!cavek && v == 2) for (int y = 2; y < 9; y++) { P(5, y, C(0x2E4A28)); if (y < 6) P(6, y, C(0x3A5A30)); }   /* hanging moss */
    if (cavek && v == 3) for (int y = 3; y < 12; y++) P(9 + (y > 7), y, C(0x2A6060));                    /* wet seep */
}
static void brick_top(int v)
{
    uint32_t s = 1200 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 8, s) * 0.5f + hn(x, y, s) * 0.5f;
            uint32_t c = pmix(C(0x3E3C32), C(0x4C4A3C), n);
            if (wn(x, y, 8, s + 3) > 0.62f) c = pmix(c, C(0x34502A), 0.7f);
            P(x, y, c);
        }
}
static void brick_face(int v, int lower)
{
    uint32_t s = 1300 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int row = y / 5, yy = y % 5;
            int off = (row & 1) ? 4 : 0;
            int xx = (x + off) % 8;
            float n = hn((x + off) / 8, row, s) * 0.5f + hn(x, y, s) * 0.3f;
            uint32_t c = pmix(C(0x8A8266), C(0x6E684E), n);
            if (yy == 0 || xx == 0) c = C(0xA49C7E);
            if (yy == 4 || xx == 7) c = C(0x34302A);
            if (y >= 15) c = C(0x2A2620);
            P(x, y, c);
        }
    if (!lower) for (int x = 0; x < 16; x++) P(x, 0, C(0xB0A888));
    if (lower) for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) P(x, y, pdark(G(x, y), 0.88f - (float)y * 0.01f));
    if (v == 1) { /* carved glyph */
        R(5, 6, 6, 1, C(0x3A3628)); R(5, 6, 1, 4, C(0x3A3628)); R(10, 6, 1, 4, C(0x3A3628)); R(7, 8, 2, 2, C(0x3A3628));
    }
    if (v == 2) for (int y = 1; y < 10; y++) { P(3, y, C(0x3E5A28)); if (y < 7) P(4, y, C(0x4E6A30)); if (y < 4) P(12, y, C(0x3E5A28)); }
}
static void wood_face(int v, int window)
{
    uint32_t s = 1400 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int bx = x % 4;
            float n = hn(x / 4, v, s) * 0.5f + hn(x, y, s) * 0.3f;
            uint32_t c = pmix(C(0x5E4432), C(0x4A3426), n);
            if (bx == 0) c = C(0x2E2018);
            if (bx == 1) c = pdark(c, 1.15f);
            if (y == 0 || y == 1) c = C(0x3A2A20);
            if (y == 15) c = C(0x22180E);
            P(x, y, c);
        }
    if (window) {
        R(3, 4, 10, 9, C(0x2A1A10));
        R(4, 5, 8, 7, C(0xFFD27A));
        R(4, 5, 8, 2, C(0xFFE6A8));
        R(7, 5, 2, 7, C(0x3A2416)); R(4, 8, 8, 1, C(0x3A2416));
        R(3, 13, 10, 1, C(0x6A4A30));
    }
    if (v == 3 && !window) {                       /* a door */
        R(4, 3, 8, 13, C(0x3A2416)); R(5, 4, 6, 12, C(0x6A4428));
        for (int y = 4; y < 16; y += 3) R(5, y, 6, 1, C(0x52341E));
        P(10, 10, C(0xE0B040));
    }
}
static void roof(int v)
{
    uint32_t s = 1500 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int row = y / 4, yy = y % 4, off = (row & 1) * 3;
            int xx = (x + off) % 6;
            float n = hn((x + off) / 6, row, s);
            uint32_t c = pmix(C(0x4A3E46), C(0x3A3038), n);
            if (yy == 3) c = C(0x221A20);
            if (yy == 0) c = pdark(c, 1.2f);
            if (xx == 0) c = C(0x2A2028);
            if (wn(x, y, 8, s + 2) > 0.7f) c = pmix(c, C(0x3A4A2A), 0.4f);
            P(x, y, c);
        }
}
static void hull(int v)
{
    uint32_t s = 1600 + v;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            int yy = y % 4;
            float n = wn(x, y * 3, 8, s) * 0.5f + hn(x, y, s) * 0.3f;
            uint32_t c = pmix(C(0x4A3424), C(0x3A281A), n);
            if (yy == 0) c = C(0x5E4430);
            if (yy == 3) c = C(0x1E140C);
            if (hn(x / 5, y / 4, s + 1) > 0.85f && yy == 1) c = C(0x2E3A30);      /* barnacles / slime */
            P(x, y, c);
        }
    for (int x = 0; x < 16; x++) P(x, 0, C(0x7A5A3A));
}
static void hedge(int v, int face)
{
    uint32_t s = 1700 + v + face * 5;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float n = wn(x, y, 4, s) * 0.6f + hn(x, y, s) * 0.4f;
            uint32_t c = n < 0.35f ? C(0x1A3018) : (n < 0.55f ? C(0x24401E) : (n < 0.75f ? C(0x2E5026) : C(0x3C6230)));
            if (face) c = pdark(c, 1.0f - (float)y * 0.025f);
            P(x, y, c);
        }
    if (face) for (int x = 0; x < 16; x++) { P(x, 0, C(0x4E7A3A)); if (hn(x, 1, s) > 0.4f) P(x, 1, C(0x467034)); }
    if (face) for (int x = 0; x < 16; x++) if (hn(x, 2, s) > 0.5f) P(x, 15, C(0x10200E));
}

/* ---- traps and gadgets ---- */
static void spikes(int f)
{
    slab(3, 0);
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++) {
            int x = 2 + i * 5, y = 3 + j * 5;
            R(x, y, 3, 2, C(0x1A1814));
            if (f == 1) { P(x + 1, y - 1, C(0xA0A0A8)); P(x + 1, y, C(0x707078)); }
            if (f == 2) {
                L(x + 1, y - 4, x + 1, y + 1, C(0xC8C8D0));
                P(x, y, C(0x7A7A84)); P(x + 2, y, C(0x5A5A64)); P(x + 1, y - 4, C(0xFFFFFF)); P(x, y - 1, C(0x8A8A94));
            }
        }
}
static void crumble(int f)
{
    slab(0, 0);
    if (f >= 1) {
        L(2, 3, 7, 8, C(0x2A2620)); L(7, 8, 13, 5, C(0x2A2620)); L(7, 8, 8, 14, C(0x2A2620));
    }
    if (f >= 2) {
        L(1, 11, 7, 8, C(0x2A2620)); L(10, 12, 14, 14, C(0x2A2620));
        R(5, 6, 4, 4, C(0x1A1612));
    }
    if (f == 3) {
        R(0, 0, 16, 16, C(0x050407));
        R(1, 1, 14, 14, C(0x000000));
        for (int x = 0; x < 16; x++) { P(x, 0, C(0x2A2620)); P(x, 1, C(0x161410)); }
    }
}
static void plate(int f)
{
    slab(1, 0);
    uint32_t top = f ? C(0x6A6050) : C(0xA49470), side = C(0x4A4236);
    R(2, 3, 12, 11, C(0x2A2620));
    R(3, 3 + f, 10, 9 - f, top);
    if (!f) R(3, 12, 10, 2, side);
    R(5, 5 + f, 6, 1, f ? C(0x7A7060) : C(0xC8B890));
    R(7, 7 + f, 2, 2, f ? C(0x8A7020) : C(0xE0B040));
}
static void grate(int f)
{
    cave(0, 0);
    if (!f) {
        R(1, 1, 14, 14, C(0x1A1614));
        for (int i = 2; i < 14; i += 3) { R(i, 1, 1, 14, C(0x6A6058)); R(1, i, 14, 1, C(0x5A5048)); }
        for (int x = 1; x < 15; x++) P(x, 1, C(0x8A8070));
    } else {
        R(1, 1, 14, 14, C(0x000000));
        for (int x = 1; x < 15; x++) { P(x, 1, C(0x2A2420)); P(x, 2, C(0x14100E)); }
        for (int y = 3; y < 15; y += 3) { P(3, y, C(0x4A4038)); P(12, y + 1, C(0x4A4038)); }   /* ladder rungs */
        for (int y = 2; y < 15; y++) { P(2, y, C(0x3A3028)); P(13, y, C(0x3A3028)); }
    }
}
static void rug(int v)
{
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            uint32_t c = C(0x7A2420);
            if (((x + y) & 7) == 0 || ((x - y) & 7) == 0) c = C(0xA88030);
            if (v == 1 && (y < 2)) c = C(0xC8A048);
            c = pdark(c, 0.92f + hn(x, y, 66) * 0.12f);
            P(x, y, c);
        }
}

/* a ragged edge of grass (or dry sand) hanging over the next tile; dir 0 = it comes from above */
static void fringe(int dir, int sandy)
{
    uint32_t s = 1900 + (uint32_t)dir * 7 + (uint32_t)sandy * 31;
    for (int i = 0; i < 16; i++) {
        int depth = 1 + (int)(hn(i, 0, s) * 3.2f) + ((i % 5) == 2);
        for (int d = 0; d < depth + 1; d++) {
            int x, y;
            switch (dir) {
            case 0: x = i; y = d; break;
            case 1: x = i; y = 15 - d; break;
            case 2: x = d; y = i; break;
            default: x = 15 - d; y = i; break;
            }
            uint32_t c;
            if (d == depth) c = CA(0x0A0A08, 90);                /* soft shadow under the overhang */
            else if (sandy) c = (hn(x, y, s) > 0.6f) ? C(0xD0B888) : C(0xC0A676);
            else c = (d == depth - 1) ? C(0x22402A) : ((hn(x, y, s) > 0.55f) ? C(0x4C7A3C) : C(0x355A30));
            P(x, y, c);
        }
        if (!sandy && hn(i, 3, s) > 0.6f) {                       /* a few taller blades */
            int x = dir == 2 ? depth : (dir == 3 ? 15 - depth : i), y = dir == 0 ? depth : (dir == 1 ? 15 - depth : i);
            P(x, y, C(0x5E9048));
        }
    }
}

void art_tiles(void)
{
    for (int v = 0; v < 4; v++) {
        pix_clear(16, 16); grass(v);        pix_commit_tile(&art_tile[TA_GRASS][v]);
        pix_clear(16, 16); dirt(v);         pix_commit_tile(&art_tile[TA_DIRT][v]);
        pix_clear(16, 16); cobble(v);       pix_commit_tile(&art_tile[TA_COBBLE][v]);
        pix_clear(16, 16); plank(v, 0);     pix_commit_tile(&art_tile[TA_PLANK][v]);
        pix_clear(16, 16); sand(v, 0);      pix_commit_tile(&art_tile[TA_SAND][v]);
        pix_clear(16, 16); sand(v, 1);      pix_commit_tile(&art_tile[TA_WETSAND][v]);
        pix_clear(16, 16); cave(v, 0);      pix_commit_tile(&art_tile[TA_CAVE][v]);
        pix_clear(16, 16); cave(v, 1);      pix_commit_tile(&art_tile[TA_MUD][v]);
        pix_clear(16, 16); slab(v, 0);      pix_commit_tile(&art_tile[TA_SLAB][v]);
        pix_clear(16, 16); slab(v, 1);      pix_commit_tile(&art_tile[TA_MOSS][v]);
        pix_clear(16, 16); plank(v, 1);     pix_commit_tile(&art_tile[TA_DECK][v]);
        pix_clear(16, 16); water(v, 0);     pix_commit_tile(&art_tile[TA_WATER][v]);
        pix_clear(16, 16); water(v, 1);     pix_commit_tile(&art_tile[TA_CAVEWATER][v]);
        pix_clear(16, 16); water(v, 2);     pix_commit_tile(&art_tile[TA_SHALLOW][v]);
        pix_clear(16, 16); rock_top(v, 0);  pix_commit_tile(&art_tile[TA_ROCK_TOP][v]);
        pix_clear(16, 16); rock_face(v, 0, 0); pix_commit_tile(&art_tile[TA_ROCK_FACE][v]);
        pix_clear(16, 16); rock_face(v, 0, 1); pix_commit_tile(&art_tile[TA_ROCK_FACEL][v]);
        pix_clear(16, 16); rock_top(v, 1);  pix_commit_tile(&art_tile[TA_CAVE_TOP][v]);
        pix_clear(16, 16); rock_face(v, 1, 0); pix_commit_tile(&art_tile[TA_CAVE_FACE][v]);
        pix_clear(16, 16); rock_face(v, 1, 1); pix_commit_tile(&art_tile[TA_CAVE_FACEL][v]);
        pix_clear(16, 16); brick_top(v);    pix_commit_tile(&art_tile[TA_BRICK_TOP][v]);
        pix_clear(16, 16); brick_face(v, 0); pix_commit_tile(&art_tile[TA_BRICK_FACE][v]);
        pix_clear(16, 16); brick_face(v, 1); pix_commit_tile(&art_tile[TA_BRICK_FACEL][v]);
        pix_clear(16, 16); wood_face(v, 0); pix_commit_tile(&art_tile[TA_WOOD_FACE][v]);
        pix_clear(16, 16); wood_face(v, 1); pix_commit_tile(&art_tile[TA_WINDOW][v]);
        pix_clear(16, 16); roof(v);         pix_commit_tile(&art_tile[TA_ROOF][v]);
        pix_clear(16, 16); hull(v);         pix_commit_tile(&art_tile[TA_HULL][v]);
        pix_clear(16, 16); hedge(v, 0);     pix_commit_tile(&art_tile[TA_HEDGE_TOP][v]);
        pix_clear(16, 16); hedge(v, 1);     pix_commit_tile(&art_tile[TA_HEDGE_FACE][v]);
        pix_clear(16, 16); spikes(v > 2 ? 2 : v);  pix_commit_tile(&art_tile[TA_SPIKES][v]);
        pix_clear(16, 16); crumble(v);      pix_commit_tile(&art_tile[TA_CRUMBLE][v]);
        pix_clear(16, 16); plate(v & 1);    pix_commit_tile(&art_tile[TA_PLATE][v]);
        pix_clear(16, 16); grate(v & 1);    pix_commit_tile(&art_tile[TA_GRATE][v]);
        pix_clear(16, 16); rug(v & 1);      pix_commit_tile(&art_tile[TA_RUG][v]);
        pix_clear(16, 16); fringe(v, 0);    pix_commit_tile(&art_tile[TA_FRINGE_GRASS][v]);
        pix_clear(16, 16); fringe(v, 1);    pix_commit_tile(&art_tile[TA_FRINGE_SAND][v]);
    }
    pix_clear(16, 16);
    R(0, 0, 16, 16, C(0x000000));
    for (int x = 0; x < 16; x++) { P(x, 0, C(0x2A2420)); P(x, 1, C(0x161210)); P(x, 2, C(0x0A0806)); }
    for (int v = 0; v < 4; v++) pix_commit_tile(&art_tile[TA_PIT][v]);
}
