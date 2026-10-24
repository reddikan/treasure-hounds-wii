/*
 * TREASURE HOUNDS - pixel painter, atlas packer, font and UI art. See art.h / pix.h.
 */
#include "pix.h"
#include <string.h>

uint32_t cv[CVW * CVH];
int cvw, cvh = CVH;

ArtRect  art_tile[TA_COUNT][4];
ArtRect  art_char[CH_COUNT][3][FR_COUNT];
ArtRect  art_face[CH_COUNT + 1][MOOD_COUNT];
ArtRect  art_crit[CR_COUNT][3][6];
ArtRect  art_spr[SP_COUNT];
ArtRect  art_item[IT_COUNT];
ArtRect  art_icon[8];
ArtGlyph art_font[128];
ArtRect  art_logo;
ArtRect  art_parch;
ArtRect  art_frame;

void art_tiles(void);
void art_chars(void);
void art_props(void);
void art_items(void);

/* ------------------------------------------------------------------ */
/* painter                                                             */
/* ------------------------------------------------------------------ */
float pnoise(float x, float y, int cell, int per, uint32_t seed)
{
    float fx = x / (float)cell, fy = y / (float)cell;
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    float tx = fx - (float)ix, ty = fy - (float)iy;
    tx = tx * tx * (3 - 2 * tx); ty = ty * ty * (3 - 2 * ty);
    int pc = per > 0 ? per / cell : 0;
    float v[4];
    for (int k = 0; k < 4; k++) {
        int gx = ix + (k & 1), gy = iy + (k >> 1);
        if (pc > 0) { gx = ((gx % pc) + pc) % pc; gy = ((gy % pc) + pc) % pc; }
        v[k] = hn(gx, gy, seed);
    }
    float a = v[0] + (v[1] - v[0]) * tx, b = v[2] + (v[3] - v[2]) * tx;
    return a + (b - a) * ty;
}

void pix_clear(int w, int h)
{
    /* only the rows the last drawing and this one use */
    int rows = (h > cvh ? h : cvh) + 2;
    if (rows > CVH) rows = CVH;
    memset(cv, 0, sizeof(uint32_t) * CVW * (size_t)rows);
    cvw = w; cvh = h;
}
uint32_t G(int x, int y)
{
    if (x < 0 || y < 0 || x >= cvw || y >= cvh) return 0;
    return cv[y * CVW + x];
}
void P(int x, int y, uint32_t c)
{
    if (x < 0 || y < 0 || x >= cvw || y >= cvh) return;
    int a = (int)(c & 255);
    if (a == 255) { cv[y * CVW + x] = c; return; }
    if (a == 0) return;
    uint32_t d = cv[y * CVW + x];
    if ((d & 255) == 0) { cv[y * CVW + x] = c; return; }
    uint32_t m = col_mix(d, c | 0xFFu, (float)a / 255.0f);
    int na = (int)(d & 255) + a * (255 - (int)(d & 255)) / 255;
    cv[y * CVW + x] = (m & 0xFFFFFF00u) | (uint32_t)(na > 255 ? 255 : na);
}
void R(int x, int y, int w, int h, uint32_t c)
{
    for (int j = y; j < y + h; j++) for (int i = x; i < x + w; i++) P(i, j, c);
}
void E(float cx, float cy, float rx, float ry, uint32_t c)
{
    int x0 = (int)floorf(cx - rx), x1 = (int)ceilf(cx + rx), y0 = (int)floorf(cy - ry), y1 = (int)ceilf(cy + ry);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float dx = ((float)x + 0.5f - cx) / rx, dy = ((float)y + 0.5f - cy) / ry;
            if (dx * dx + dy * dy <= 1.0f) P(x, y, c);
        }
}
void L(int x0, int y0, int x1, int y1, uint32_t c)
{
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx + dy;
    for (;;) {
        P(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
void Ld(float x0, float y0, float x1, float y1, float w, uint32_t c)
{
    float len = sqrtf((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
    int n = (int)(len * 2) + 1;
    for (int i = 0; i <= n; i++) {
        float t = (float)i / (float)n;
        E(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, w * 0.5f, w * 0.5f, c);
    }
}
static uint32_t tmp[CVW * CVH];
static void snapshot(void) { memcpy(tmp, cv, sizeof(uint32_t) * CVW * (size_t)(cvh < CVH ? cvh + 1 : CVH)); }
void pix_outline(uint32_t c)
{
    snapshot();
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw; x++) {
            if ((tmp[y * CVW + x] & 255) > 40) continue;
            int hit = 0;
            for (int k = 0; k < 4 && !hit; k++) {
                int nx = x + (k == 0) - (k == 1), ny = y + (k == 2) - (k == 3);
                if (nx < 0 || ny < 0 || nx >= cvw || ny >= cvh) continue;
                if ((tmp[ny * CVW + nx] & 255) > 120) hit = 1;
            }
            if (hit) cv[y * CVW + x] = c;
        }
}
void pix_outline_soft(uint32_t c)
{
    snapshot();
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw; x++) {
            if ((tmp[y * CVW + x] & 255) > 40) continue;
            int hit = 0;
            for (int k = 0; k < 3 && !hit; k++) {
                int nx = x + (k == 0) - (k == 1), ny = y - (k == 2);
                if (nx < 0 || ny < 0 || nx >= cvw || ny >= cvh) continue;
                if ((tmp[ny * CVW + nx] & 255) > 120) hit = 1;
            }
            if (hit) cv[y * CVW + x] = c;
        }
}
void pix_shade_rim(uint32_t hi, int dx, int dy)
{
    snapshot();
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw; x++) {
            uint32_t p = tmp[y * CVW + x];
            if ((p & 255) < 200) continue;
            int nx = x + dx, ny = y + dy;
            uint32_t q = (nx < 0 || ny < 0 || nx >= cvw || ny >= cvh) ? 0 : tmp[ny * CVW + nx];
            if ((q & 255) < 100) cv[y * CVW + x] = (col_mix(p, hi | 0xFF, (float)(hi & 255) / 255.0f) & 0xFFFFFF00u) | (p & 255);
        }
}
void pix_flip(void)
{
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw / 2; x++) {
            uint32_t t = cv[y * CVW + x];
            cv[y * CVW + x] = cv[y * CVW + cvw - 1 - x];
            cv[y * CVW + cvw - 1 - x] = t;
        }
}

/* shelf packer with one texel of padding round everything */
static int sh_x = 0, sh_y = 0, sh_h = 0;
void pix_alloc(int w, int h, int *x, int *y)
{
    if (sh_x + w + 2 > ATLAS_W - 8) { sh_x = 0; sh_y += sh_h; sh_h = 0; }
    *x = sh_x + 1; *y = sh_y + 1;
    sh_x += w + 2;
    if (h + 2 > sh_h) sh_h = h + 2;
    if (sh_y + sh_h > ATLAS_H - 8) { *x = 0; *y = 0; }          /* out of room (the tests catch this) */
}
int art_atlas_used(void) { return sh_y + sh_h; }

void pix_commit_at(ArtRect *r, int sx, int sy, int w, int h, int ax, int ay)
{
    int x, y;
    pix_alloc(w, h, &x, &y);
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) gfx_atlas[(y + j) * ATLAS_W + x + i] = G(sx + i, sy + j);
    r->x = (uint16_t)x; r->y = (uint16_t)y; r->w = (uint8_t)w; r->h = (uint8_t)h; r->ax = (int8_t)ax; r->ay = (int8_t)ay;
}
void pix_commit(ArtRect *r, int ax, int ay) { pix_commit_at(r, 0, 0, cvw, cvh, ax, ay); }

/* a tile: copied with its edge texels extruded one texel out so nearest sampling never bleeds */
void pix_commit_tile(ArtRect *r)
{
    int x, y;
    pix_alloc(16, 16, &x, &y);
    for (int j = -1; j <= 16; j++)
        for (int i = -1; i <= 16; i++) {
            int si = i < 0 ? 0 : (i > 15 ? 15 : i), sj = j < 0 ? 0 : (j > 15 ? 15 : j);
            gfx_atlas[(y + j) * ATLAS_W + x + i] = G(si, sj);
        }
    r->x = (uint16_t)x; r->y = (uint16_t)y; r->w = 16; r->h = 16; r->ax = 0; r->ay = 0;
}

/* ------------------------------------------------------------------ */
/* font: 7 px caps, 2 px descenders, proportional                      */
/* ------------------------------------------------------------------ */
typedef struct { char ch; const char *rows; } GlyphDef;   /* rows separated by '/', '#' = ink */
static const GlyphDef FONT[] = {
    {'A', ".##./#..#/#..#/####/#..#/#..#/#..#"}, {'B', "###./#..#/#..#/###./#..#/#..#/###."},
    {'C', ".##./#..#/#.../#.../#.../#..#/.##."}, {'D', "###./#..#/#..#/#..#/#..#/#..#/###."},
    {'E', "####/#.../#.../###./#.../#.../####"}, {'F', "####/#.../#.../###./#.../#.../#..."},
    {'G', ".##./#..#/#.../#.##/#..#/#..#/.###"}, {'H', "#..#/#..#/#..#/####/#..#/#..#/#..#"},
    {'I', "###/.#./.#./.#./.#./.#./###"},        {'J', "..##/...#/...#/...#/#..#/#..#/.##."},
    {'K', "#..#/#..#/#.#./##../#.#./#..#/#..#"}, {'L', "#.../#.../#.../#.../#.../#.../####"},
    {'M', "#...#/##.##/#.#.#/#.#.#/#...#/#...#/#...#"}, {'N', "#..#/##.#/##.#/#.##/#.##/#..#/#..#"},
    {'O', ".##./#..#/#..#/#..#/#..#/#..#/.##."}, {'P', "###./#..#/#..#/###./#.../#.../#..."},
    {'Q', ".##./#..#/#..#/#..#/#..#/#.#./.#.#"}, {'R', "###./#..#/#..#/###./#.#./#..#/#..#"},
    {'S', ".###/#.../#.../.##./...#/...#/###."}, {'T', "#####/..#../..#../..#../..#../..#../..#.."},
    {'U', "#..#/#..#/#..#/#..#/#..#/#..#/.##."}, {'V', "#...#/#...#/#...#/.#.#./.#.#./..#../..#.."},
    {'W', "#...#/#...#/#...#/#.#.#/#.#.#/##.##/#...#"}, {'X', "#...#/#...#/.#.#./..#../.#.#./#...#/#...#"},
    {'Y', "#...#/#...#/.#.#./..#../..#../..#../..#.."}, {'Z', "####/...#/...#/..#./.#../#.../####"},
    {'a', "..../..../.##./...#/.###/#..#/.###"}, {'b', "#.../#.../###./#..#/#..#/#..#/###."},
    {'c', "..../..../.##./#..#/#.../#..#/.##."}, {'d', "...#/...#/.###/#..#/#..#/#..#/.###"},
    {'e', "..../..../.##./#..#/####/#.../.##."}, {'f', "..##/.#../.#../###./.#../.#../.#.."},
    {'g', "..../..../.###/#..#/#..#/#..#/.###/...#/.##."}, {'h', "#.../#.../###./#..#/#..#/#..#/#..#"},
    {'i', "#/./#/#/#/#/#"},                    {'j', ".#/../.#/.#/.#/.#/.#/.#/#."},
    {'k', "#.../#.../#..#/#.#./##../#.#./#..#"}, {'l', "#./#./#./#./#./#./.#"},
    {'m', "...../...../##.#./#.#.#/#.#.#/#.#.#/#.#.#"}, {'n', "..../..../###./#..#/#..#/#..#/#..#"},
    {'o', "..../..../.##./#..#/#..#/#..#/.##."}, {'p', "..../..../###./#..#/#..#/#..#/###./#.../#..."},
    {'q', "..../..../.###/#..#/#..#/#..#/.###/...#/...#"}, {'r', "..../..../#.##/##../#.../#.../#..."},
    {'s', "..../..../.###/#.../.##./...#/###."}, {'t', ".#../.#../###./.#../.#../.#../..#."},
    {'u', "..../..../#..#/#..#/#..#/#..#/.###"}, {'v', "...../...../#...#/#...#/.#.#./.#.#./..#.."},
    {'w', "...../...../#...#/#...#/#.#.#/#.#.#/.#.#."}, {'x', "...../...../#...#/.#.#./..#../.#.#./#...#"},
    {'y', "..../..../#..#/#..#/#..#/#..#/.###/...#/.##."}, {'z', "..../..../####/...#/..#./.#../####"},
    {'0', ".##./#..#/#.##/##.#/#..#/#..#/.##."}, {'1', ".#./##./.#./.#./.#./.#./###"},
    {'2', ".##./#..#/...#/..#./.#../#.../####"}, {'3', "###./...#/...#/.##./...#/...#/###."},
    {'4', "#..#/#..#/#..#/####/...#/...#/...#"}, {'5', "####/#.../#.../###./...#/...#/###."},
    {'6', ".##./#.../#.../###./#..#/#..#/.##."}, {'7', "####/...#/...#/..#./.#../.#../.#.."},
    {'8', ".##./#..#/#..#/.##./#..#/#..#/.##."}, {'9', ".##./#..#/#..#/.###/...#/...#/.##."},
    {'.', "./././././#/#"} , {',', "./././././#/#/#"},
    {'!', "#/#/#/#/#/./#"}, {'?', ".##./#..#/...#/..#./.#../..../.#.."},
    {'\'', "#/#"}, {'"', "#.#/#.#"}, {'-', ".../.../.../###"}, {':', "./././#/./#/#"},
    {';', "../../../.#/../.#/#."}, {'(', ".#/#./#./#./#./#./.#"}, {')', "#./.#/.#/.#/.#/.#/#."},
    {'/', "...#/...#/..#./.##./.#../#.../#..."}, {'+', ".../.../.#./###/.#."},
    {'*', "...../#.#.#/.###./#.#.#"}, {'=', "..../..../####/..../####"}, {'_', "..../..../..../..../..../..../####"},
    {'<', "../../.#/#./.#"}, {'>', "../../#./.#/#."}, {'~', "..../..../.#.#/#.#."},
    {'%', "#..#/...#/..#./.#../#.../#..#"}, {'&', ".#../#.#./#.#./.#../#.##/#..#/.##."},
    {'$', "..#../.####/#.#../.###./..#.#/####./..#.."}, {'#', ".#.#./#####/.#.#./#####/.#.#."},
    {'[', "##/#./#./#./#./#./##"}, {']', "##/.#/.#/.#/.#/.#/##"},
};

static void make_font(void)
{
    memset(art_font, 0, sizeof(art_font));
    /* paint every glyph into one strip, then commit each as a sub-rect */
    for (unsigned g = 0; g < sizeof(FONT) / sizeof(FONT[0]); g++) {
        const char *s = FONT[g].rows;
        int w = 0, row = 0, col = 0;
        pix_clear(8, 9);
        for (; *s; s++) {
            if (*s == '/') { row++; col = 0; continue; }
            if (*s == '#') P(col, row, 0xFFFFFFFFu);
            col++;
            if (col > w) w = col;
        }
        ArtRect r;
        pix_commit_at(&r, 0, 0, w, 9, 0, 0);
        art_font[(unsigned char)FONT[g].ch].x = r.x;
        art_font[(unsigned char)FONT[g].ch].y = r.y;
        art_font[(unsigned char)FONT[g].ch].w = (uint8_t)w;
    }
}

/* ------------------------------------------------------------------ */
/* text icons: classic controller buttons + a coin                      */
/* ------------------------------------------------------------------ */
static void make_icons(void)
{
    const char *lab = "ABXYLR+";
    for (int i = 0; i < 7; i++) {
        pix_clear(9, 9);
        uint32_t base = C(0xE8E4D8), dark = C(0x6C6458);
        if (i >= 4) {                                   /* shoulder buttons / plus: rounded rectangle */
            R(0, 1, 9, 7, dark); R(1, 0, 7, 9, dark); R(1, 1, 7, 7, base);
        } else {
            E(4.5f, 4.5f, 4.5f, 4.5f, dark); E(4.5f, 4.3f, 3.6f, 3.6f, base);
        }
        if (lab[i] == '+') { P(4, 3, dark); R(3, 4, 3, 1, dark); P(4, 5, dark); }
        else {
            /* a 4x5 mini version of the letter */
            static const char *mini[] = { ".##./#..#/####/#..#/#..#", "###./#..#/###./#..#/###.", "#..#/.##./.##./.##./#..#",
                                          "#..#/#..#/.##./.#../.#..", "#.../#.../#.../#.../####", "###./#..#/###./#.#./#..#" };
            const char *s = mini[i];
            int row = 0, col = 0;
            for (; *s; s++) {
                if (*s == '/') { row++; col = 0; continue; }
                if (*s == '#') P(2 + col, 2 + row, C(0x2A2420));
                col++;
            }
        }
        pix_commit(&art_icon[i], 0, 0);
    }
    /* coin */
    pix_clear(9, 9);
    E(4.5f, 4.5f, 4.2f, 4.2f, C(0x8A5A10));
    E(4.5f, 4.2f, 3.6f, 3.6f, C(0xF2C230));
    E(4.0f, 3.6f, 2.0f, 2.0f, C(0xFFE890));
    R(4, 2, 1, 5, C(0xC88E18));
    pix_commit(&art_icon[7], 0, 0);
}

/* ------------------------------------------------------------------ */
/* UI: parchment and wood 9-slices, title lettering                     */
/* ------------------------------------------------------------------ */
static void make_ui(void)
{
    pix_clear(24, 24);
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 24; x++) {
            float n = pnoise((float)x, (float)y, 4, 0, 77) * 0.6f + hn(x, y, 3) * 0.4f;
            uint32_t c = pmix(C(0xE9D3A4), C(0xC9A66C), n);
            int e = x < 2 || y < 2 || x > 21 || y > 21;
            int e2 = x < 4 || y < 4 || x > 19 || y > 19;
            if (e2) c = pmix(c, C(0x9C7440), 0.45f);
            if (e) c = C(0x5A3A1C);
            P(x, y, c);
        }
    /* torn corners */
    P(0, 0, 0); P(1, 0, 0); P(0, 1, 0); P(23, 0, 0); P(22, 0, 0); P(23, 1, 0);
    P(0, 23, 0); P(1, 23, 0); P(0, 22, 0); P(23, 23, 0); P(22, 23, 0); P(23, 22, 0);
    pix_commit(&art_parch, 0, 0);

    pix_clear(24, 24);
    for (int y = 0; y < 24; y++)
        for (int x = 0; x < 24; x++) {
            float n = hn(x / 3, y, 9);
            uint32_t c = pmix(C(0x2A1C14), C(0x3A2818), n);
            int e = x < 3 || y < 3 || x > 20 || y > 20;
            if (e) c = pmix(C(0x6A4424), C(0x8A5C30), hn(x, y / 4, 5));
            if (x == 0 || y == 0 || x == 23 || y == 23) c = C(0x140C08);
            if ((x == 3 || y == 3) && !(x < 3 || y < 3) && x < 21 && y < 21) c = C(0x140C08);
            if ((x == 20 || y == 20) && x > 2 && y > 2 && x < 21 && y < 21) c = C(0x140C08);
            P(x, y, c);
        }
    /* brass rivets */
    int rv[4][2] = { {1, 1}, {21, 1}, {1, 21}, {21, 21} };
    for (int i = 0; i < 4; i++) { R(rv[i][0], rv[i][1], 2, 2, C(0xD8A840)); P(rv[i][0], rv[i][1], C(0xFFE8A0)); }
    pix_commit(&art_frame, 0, 0);

    /* logo: "TREASURE" over "HOUNDS", chunky bevelled gold letters */
    const char *l1 = "TREASURE", *l2 = "HOUNDS";
    int sc = 3;
    pix_clear(232, 64);
    for (int line = 0; line < 2; line++) {
        const char *s = line ? l2 : l1;
        int n = (int)strlen(s), tw = 0;
        for (int i = 0; i < n; i++) tw += (s[i] == 'M' || s[i] == 'W' || s[i] == 'T' || s[i] == 'V' || s[i] == 'X' || s[i] == 'Y' ? 5 : 4) * sc + sc;
        int x = (232 - tw) / 2, y = 4 + line * 30;
        for (int i = 0; i < n; i++) {
            int gw = (s[i] == 'M' || s[i] == 'W' || s[i] == 'T' || s[i] == 'V' || s[i] == 'X' || s[i] == 'Y') ? 5 : 4;
            for (unsigned g = 0; g < sizeof(FONT) / sizeof(FONT[0]); g++) {
                if (FONT[g].ch != s[i]) continue;
                const char *q = FONT[g].rows;
                int row = 0, col = 0;
                for (; *q; q++) {
                    if (*q == '/') { row++; col = 0; continue; }
                    if (*q == '#')
                        for (int yy = 0; yy < sc; yy++)
                            for (int xx = 0; xx < sc; xx++) {
                                float t = (float)(row * sc + yy) / (7.0f * (float)sc);
                                uint32_t c = t < 0.45f ? pmix(C(0xFFF0A0), C(0xFFC830), t / 0.45f) : pmix(C(0xFFC830), C(0xC86410), (t - 0.45f) / 0.55f);
                                P(x + col * sc + xx, y + row * sc + yy, c);
                            }
                    col++;
                }
            }
            x += gw * sc + sc;
        }
    }
    pix_shade_rim(CA(0xFFFFFF, 200), 0, -1);
    pix_shade_rim(CA(0x8A3C08, 160), 0, 1);
    pix_outline(C(0x3C1A08));
    pix_outline(C(0x1A0A04));
    /* drop shadow: shift everything down-right into a dark copy behind */
    static uint32_t keep[CVW * CVH];
    memcpy(keep, cv, sizeof(cv));
    for (int y = 0; y < cvh; y++)
        for (int x = 0; x < cvw; x++)
            if ((keep[y * CVW + x] & 255) == 0) {
                uint32_t s = (x >= 2 && y >= 3) ? keep[(y - 3) * CVW + x - 2] : 0;
                if ((s & 255) > 0) cv[y * CVW + x] = CA(0x0A0408, 170);
            }
    pix_commit(&art_logo, 0, 0);
}

void art_init(void)
{
    static int done;
    if (done) return;
    done = 1;
    memset(gfx_atlas, 0, sizeof(gfx_atlas));
    for (int y = 1016; y < 1024; y++) for (int x = 1016; x < 1024; x++) gfx_atlas[y * ATLAS_W + x] = 0xFFFFFFFFu;
    make_font();
    make_icons();
    make_ui();
    art_tiles();
    art_items();
    art_chars();
    art_props();
}
