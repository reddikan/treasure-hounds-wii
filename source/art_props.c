/*
 * TREASURE HOUNDS - props: trees, barrels, chests, the lighthouse, wrecks, temple pieces and traps.
 * Anchors (ax, ay) are the point that sits on the ground.
 */
#include "pix.h"
#include <stdlib.h>

#define OUT C(0x140E10)

static void pine(void)
{
    pix_clear(28, 44);
    R(12, 34, 4, 9, C(0x4A3020)); R(12, 34, 1, 9, C(0x5E4028));
    for (int layer = 0; layer < 5; layer++) {
        float y = 36.0f - (float)layer * 7.0f, w = 13.0f - (float)layer * 2.2f;
        for (int j = 0; j < 9; j++)
            for (int x = -(int)w; x <= (int)w; x++) {
                float t = (float)j / 9.0f;
                if (fabsf((float)x) > w * (1.0f - t * 0.85f)) continue;
                float n = hn(x + 14, (int)y - j, 33);
                uint32_t c = (x < -2) ? C(0x2C4A30) : (x > 3 ? C(0x1A3022) : C(0x223E28));
                if (n > 0.8f) c = C(0x3A6040);
                if (j == 0 && n > 0.4f) c = C(0x14261A);
                P(14 + x, (int)y - j, c);
            }
    }
    pix_outline(OUT);
    pix_commit(&art_spr[SP_PINE], 14, 42);
}
static void palm(void)
{
    pix_clear(40, 50);
    for (int i = 0; i < 30; i++) {
        float t = (float)i / 30.0f;
        float x = 20.0f + sinf(t * 1.6f) * 5.0f, y = 48.0f - t * 32.0f;
        uint32_t c = (i % 3 == 0) ? C(0x6A4A2A) : C(0x8A6438);
        R((int)x - 2, (int)y, 4, 2, c); P((int)x - 2, (int)y, C(0x4A3018));
    }
    float hx = 20 + sinf(1.6f) * 5.0f, hy = 16;
    for (int f = 0; f < 7; f++) {
        float a = -3.0f + (float)f * 0.85f;
        for (int k = 0; k < 16; k++) {
            float t = (float)k / 16.0f;
            float x = hx + cosf(a) * t * 17.0f, y = hy + sinf(a) * t * 9.0f + t * t * 9.0f;
            uint32_t c = (k & 1) ? C(0x2E6A30) : C(0x3E8A3C);
            E(x, y, 1.8f - t, 1.4f, c);
            if (k % 3 == 0) { P((int)x, (int)y + 2, C(0x245A28)); }
        }
    }
    E(hx, hy + 1, 3, 2, C(0x5A3A1E)); P((int)hx - 1, (int)hy + 2, C(0x7A5A2E)); P((int)hx + 2, (int)hy + 2, C(0x7A5A2E));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_PALM], 20, 48);
}
static void bush(int cut)
{
    pix_clear(20, 16);
    if (cut) {
        for (int i = 0; i < 8; i++) R(3 + i * 2, 10 - (i & 1), 1, 5, C(0x3A5A28));
        R(2, 13, 16, 2, C(0x2A3A1E));
        pix_outline(OUT);
        pix_commit(&art_spr[SP_BUSH_CUT], 10, 15);
        return;
    }
    E(10, 10, 9, 5.5f, C(0x24401E));
    E(7, 8, 5, 4, C(0x2E5026)); E(13, 7, 5, 4.5f, C(0x2E5026));
    E(9, 6, 3.5f, 3, C(0x3E6A30)); E(14, 5, 2.5f, 2, C(0x4A7A38));
    for (int i = 0; i < 10; i++) P((int)(hn(i, 1, 5) * 18) + 1, (int)(hn(i, 2, 5) * 8) + 5, C(0x56883E));
    P(6, 9, C(0xC83040)); P(12, 10, C(0xC83040)); P(15, 8, C(0xC83040));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BUSH], 10, 15);
}
static void rock(int big)
{
    int w = big ? 28 : 16, h = big ? 22 : 12;
    pix_clear(w, h);
    float cx = (float)w * 0.5f, cy = (float)h * 0.58f;
    E(cx, cy, cx - 1, cy - 1, C(0x5A5654));
    E(cx - 2, cy - 2, cx - 4, cy - 3, C(0x6E6A66));
    E(cx - 3, cy - 3, cx - 7 > 2 ? cx - 7 : 2, cy - 5 > 1.5f ? cy - 5 : 1.5f, C(0x86827C));
    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) if ((G(x, y) & 255) && hn(x, y, 8) > 0.88f) P(x, y, C(0x48444A));
    if (big) { L(8, 6, 12, 12, C(0x3A3638)); L(12, 12, 18, 13, C(0x3A3638)); E(20, 6, 3, 1.5f, C(0x3A5A30)); }
    for (int x = 2; x < w - 2; x++) if (G(x, h - 3) & 255) P(x, h - 3, C(0x3E3A3C));
    pix_outline(OUT);
    pix_commit(&art_spr[big ? SP_ROCK_BIG : SP_ROCK], w / 2, h - 2);
}
static void barrel(void)
{
    pix_clear(14, 18);
    for (int y = 3; y < 17; y++) {
        float bulge = sinf((float)(y - 3) / 13.0f * 3.14159f);
        int hw = 4 + (int)(bulge * 1.6f);
        for (int x = 7 - hw; x < 7 + hw; x++) {
            uint32_t c = x < 5 ? C(0x9A6A3C) : (x > 9 ? C(0x5E3E22) : C(0x7E5430));
            if ((x - 1) % 3 == 0) c = pdark(c, 0.85f);
            P(x, y, c);
        }
    }
    E(7, 3.5f, 5, 2.2f, C(0x6A4A2A)); E(7, 3.5f, 3.8f, 1.4f, C(0x8A6438));
    for (int x = 1; x < 13; x++) { if (G(x, 6) & 255) P(x, 6, C(0x4A4A54)); if (G(x, 14) & 255) P(x, 14, C(0x4A4A54)); }
    P(3, 6, C(0x8A8A94)); P(3, 14, C(0x8A8A94));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BARREL], 7, 16);
}
static void crate(int broken)
{
    pix_clear(16, 18);
    if (broken) {
        L(1, 15, 6, 12, C(0x8A6438)); L(2, 16, 7, 13, C(0x6A4A2A)); L(8, 16, 14, 13, C(0x8A6438)); L(9, 14, 13, 16, C(0x6A4A2A));
        R(4, 15, 3, 2, C(0x7A5430)); R(11, 15, 3, 1, C(0x9A7444));
        pix_outline(OUT);
        pix_commit(&art_spr[SP_CRATE_BROKEN], 8, 16);
        return;
    }
    R(1, 1, 14, 5, C(0x9A7444)); R(1, 1, 14, 1, C(0xB89058));
    R(1, 6, 14, 10, C(0x7A5430));
    for (int y = 7; y < 16; y += 3) R(1, y, 14, 1, C(0x5E3E22));
    L(2, 7, 13, 15, C(0x9A7444)); L(2, 8, 12, 15, C(0x5E3E22));
    R(1, 6, 1, 10, C(0x9A7444)); R(14, 6, 1, 10, C(0x4A3018));
    R(3, 2, 10, 3, C(0x8A6438));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_CRATE], 8, 16);
}
static void lamppost(void)
{
    pix_clear(12, 38);
    R(5, 10, 2, 26, C(0x2A2A30)); R(5, 10, 1, 26, C(0x4A4A54));
    R(3, 34, 6, 3, C(0x2A2A30));
    R(2, 2, 8, 2, C(0x2A2A30)); R(3, 4, 6, 6, C(0xFFE0A0)); R(4, 5, 4, 4, C(0xFFF8E0));
    R(3, 4, 1, 6, C(0x2A2A30)); R(8, 4, 1, 6, C(0x2A2A30)); R(2, 10, 8, 1, C(0x2A2A30)); R(5, 0, 2, 2, C(0x2A2A30));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_LAMPPOST], 6, 36);
}
static void torch(void)
{
    pix_clear(10, 16);
    R(4, 5, 2, 10, C(0x5A3A20)); R(4, 5, 1, 10, C(0x7A5430));
    R(3, 3, 4, 3, C(0x3A3A40)); R(2, 11, 6, 2, C(0x3A3A40));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_TORCH], 5, 15);
}
static void brazier(int lit)
{
    pix_clear(16, 16);
    R(7, 7, 2, 7, C(0x4A3A30)); R(4, 13, 8, 2, C(0x3A2E26));
    E(8, 6, 7, 3.4f, C(0x5A4A3E)); E(8, 5, 6, 2.2f, lit ? C(0xFF8020) : C(0x2A2420));
    if (lit) { P(6, 5, C(0xFFE080)); P(9, 4, C(0xFFD060)); P(10, 6, C(0xE05010)); }
    else { P(6, 5, C(0x4A4038)); P(9, 5, C(0x3A3028)); }
    for (int x = 2; x < 15; x += 3) P(x, 7, C(0x8A7A5A));
    pix_outline(OUT);
    pix_commit(&art_spr[lit ? SP_BRAZIER_LIT : SP_BRAZIER], 8, 14);
}
static void campfire(void)
{
    pix_clear(22, 12);
    for (int i = 0; i < 10; i++) { float a = (float)i * 0.628f; E(11 + cosf(a) * 8.5f, 6 + sinf(a) * 4.0f, 2.0f, 1.6f, (i & 1) ? C(0x6A6662) : C(0x86827C)); }
    Ld(6, 7, 15, 5, 2.2f, C(0x5A3A20)); Ld(7, 4, 16, 8, 2.2f, C(0x6A4428));
    E(11, 6, 3, 1.6f, C(0xFF6018)); P(11, 6, C(0xFFE080));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_CAMPFIRE], 11, 10);
}
static void bench(void)
{
    pix_clear(24, 12);
    R(1, 2, 22, 3, C(0x8A6438)); R(1, 2, 22, 1, C(0xA8804A)); R(1, 5, 22, 1, C(0x4A3018));
    R(3, 6, 2, 5, C(0x5A3A20)); R(19, 6, 2, 5, C(0x5A3A20));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BENCH], 12, 10);
    pix_clear(24, 18);                                     /* table */
    R(1, 1, 22, 9, C(0x8A6438));
    for (int y = 2; y < 10; y += 3) R(1, y, 22, 1, C(0x6A4A2A));
    R(1, 1, 22, 1, C(0xA8804A)); R(1, 10, 22, 2, C(0x4A3018));
    R(3, 12, 2, 5, C(0x4A3018)); R(19, 12, 2, 5, C(0x4A3018));
    R(8, 3, 3, 2, C(0xE8E8F0)); R(14, 5, 2, 3, CA(0x3A8A4A, 220));       /* a mug and a bottle */
    pix_outline(OUT);
    pix_commit(&art_spr[SP_TABLE], 12, 16);
}
static void fence(void)
{
    pix_clear(18, 16);
    for (int i = 0; i < 3; i++) { R(1 + i * 7, 2, 3, 13, C(0x7A5A3A)); R(1 + i * 7, 2, 1, 13, C(0x9A7A50)); P(2 + i * 7, 1, C(0x7A5A3A)); }
    R(0, 5, 18, 2, C(0x6A4A2A)); R(0, 10, 18, 2, C(0x6A4A2A)); R(0, 5, 18, 1, C(0x8A6A40));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_FENCE], 9, 15);
}
static void sign(void)
{
    pix_clear(16, 18);
    R(7, 8, 2, 9, C(0x5A3A20));
    R(1, 1, 14, 8, C(0x9A7444)); R(1, 1, 14, 1, C(0xB89058)); R(1, 8, 14, 1, C(0x5E3E22));
    for (int x = 3; x < 13; x += 2) P(x, 4, C(0x3A2410));
    for (int x = 3; x < 11; x += 2) P(x, 6, C(0x3A2410));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_SIGN], 8, 16);
}
static void anchor(void)
{
    pix_clear(20, 22);
    uint32_t c = C(0x5A5E66), l = C(0x8A8E98);
    E(10, 3, 2.5f, 2.5f, c); P(10, 3, 0); P(10, 2, 0);
    R(9, 5, 2, 13, c); R(9, 5, 1, 13, l); R(5, 7, 10, 2, c);
    for (int i = 0; i < 9; i++) { float a = 3.14159f * (float)i / 8.0f; Ld(10 + cosf(a) * 7.5f, 15 + sinf(a) * 4.5f, 10 + cosf(a) * 7.5f, 15 + sinf(a) * 4.5f, 2.2f, c); }
    E(2.5f, 13.5f, 1.8f, 2.4f, c); E(17.5f, 13.5f, 1.8f, 2.4f, c);
    P(4, 18, C(0x8A4A2A)); P(13, 19, C(0x8A4A2A));                       /* rust */
    pix_outline(OUT);
    pix_commit(&art_spr[SP_ANCHOR], 10, 20);
}
static void rowboat(void)
{
    pix_clear(40, 22);
    for (int x = 2; x < 38; x++) {
        float t = (float)(x - 2) / 35.0f;
        float h = sinf(t * 3.14159f);
        int top = 8 - (int)(h * 3.0f), bot = 12 + (int)(h * 6.0f);
        for (int y = top; y < bot; y++) {
            uint32_t c = (y - top) % 3 == 0 ? C(0x5E3E22) : C(0x7E5430);
            if (y == top) c = C(0x9A7444);
            P(x, y, c);
        }
        if (x > 4 && x < 36) for (int y = top + 1; y < top + 3 + (int)(h * 2); y++) P(x, y, C(0x3A2414));
    }
    R(12, 6, 2, 6, C(0x8A6438)); R(25, 6, 2, 6, C(0x8A6438));
    Ld(14, 9, 34, 4, 1.5f, C(0xA8804A));                                  /* oar */
    pix_outline(OUT);
    pix_commit(&art_spr[SP_ROWBOAT], 20, 19);
}
static void wreck(void)
{
    pix_clear(56, 44);
    /* broken ribs of a hull rising out of the sand */
    for (int i = 0; i < 6; i++) {
        float x0 = 6.0f + (float)i * 8.0f;
        float hgt = 30.0f - fabsf((float)i - 2.5f) * 4.0f;
        for (int k = 0; k < 14; k++) {
            float t = (float)k / 13.0f;
            Ld(x0 - sinf(t * 1.4f) * 4.0f, 40.0f - t * hgt, x0 - sinf(t * 1.4f) * 4.0f, 40.0f - t * hgt, 3.0f, (k & 1) ? C(0x5A3E26) : C(0x4A3220));
        }
        P((int)x0 - 3, (int)(40 - hgt), C(0x7A5A3A));
    }
    for (int y = 30; y < 42; y++) for (int x = 2; x < 54; x++) if (hn(x, y, 7) > 0.45f || y > 36) P(x, y, ((y - 30) % 4 == 0) ? C(0x3A2818) : C(0x5A3E26));
    Ld(4, 31, 52, 29, 2.0f, C(0x7A5A3A));
    for (int i = 0; i < 8; i++) P(6 + i * 6, 34 + (i & 1), C(0x3A4A3A));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_WRECK_BOW], 28, 42);
    pix_clear(18, 44);                                     /* broken mast */
    R(7, 6, 5, 36, C(0x6A4A2E)); R(7, 6, 1, 36, C(0x8A6A40)); R(11, 6, 1, 36, C(0x3A2818));
    for (int x = 7; x < 12; x++) P(x, 5 + (x & 1) * 2, C(0x6A4A2E));
    R(1, 16, 16, 3, C(0x5A3E26)); R(1, 16, 16, 1, C(0x7A5A3A));
    for (int y = 19; y < 34; y++) { P(2 + (y & 1), y, CA(0xC8BCA0, 200)); if (y < 28) P(14 - (y & 1), y, CA(0xB8AC90, 190)); }   /* shreds of sail */
    pix_outline(OUT);
    pix_commit(&art_spr[SP_WRECK_MAST], 9, 42);
}
static void bones(void)
{
    pix_clear(16, 10);
    E(4, 5, 3, 2.6f, C(0xE0D8C0)); P(3, 5, C(0x2A2020)); P(5, 5, C(0x2A2020));
    E(11, 6, 3, 2.6f, C(0xD0C8B0)); P(10, 6, C(0x2A2020)); P(12, 6, C(0x2A2020));
    E(7.5f, 3, 2.8f, 2.4f, C(0xE8E0C8)); P(7, 3, C(0x2A2020)); P(9, 3, C(0x2A2020));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_SKULLS], 8, 9);
    pix_clear(18, 10);
    Ld(2, 7, 14, 4, 1.6f, C(0xE0D8C0)); E(2, 7, 1.5f, 1.5f, C(0xE0D8C0)); E(14, 4, 1.5f, 1.5f, C(0xE0D8C0));
    Ld(5, 3, 12, 8, 1.4f, C(0xD0C8B0));
    E(15, 7, 2.6f, 2.2f, C(0xE8E0C8)); P(14, 7, C(0x2A2020)); P(16, 7, C(0x2A2020));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BONES], 9, 9);
}
static void goldpile(void)
{
    pix_clear(28, 16);
    for (int i = 0; i < 70; i++) {
        float t = hn(i, 0, 21), u = hn(i, 1, 21);
        float x = 3 + t * 22, y = 13 - u * (8 - fabsf(t - 0.5f) * 12);
        E(x, y, 1.8f, 1.2f, (i % 3 == 0) ? C(0xFFE070) : ((i % 3 == 1) ? C(0xE8A828) : C(0xC88418)));
    }
    P(9, 6, C(0xE02840)); P(10, 6, C(0xFF6070)); P(18, 8, C(0x3070F0)); P(14, 4, C(0xFFFFFF));
    pix_outline(C(0x4A2A08));
    pix_commit(&art_spr[SP_GOLDPILE], 14, 14);
}
static void chest(int big, int open)
{
    int w = big ? 30 : 18, h = big ? 30 : 20;
    pix_clear(w, h);
    int bx = 1, bw = w - 2, by = h - (big ? 14 : 9), bh = big ? 12 : 8;
    uint32_t wood = big ? C(0x7A2E1E) : C(0x7E5430), wd = big ? C(0x5A1A10) : C(0x5E3E22), band = C(0xD8A840), bandd = C(0x8A6420);
    R(bx, by, bw, bh, wood);
    for (int y = by + 2; y < by + bh; y += 3) R(bx, y, bw, 1, wd);
    R(bx, by + bh - 1, bw, 1, C(0x2A1810));
    R(bx + 2, by, 2, bh, band); R(bx + bw - 4, by, 2, bh, band); R(bx + 3, by, 1, bh, bandd); R(bx + bw - 3, by, 1, bh, bandd);
    if (!open) {
        int lh = big ? 8 : 6;
        E((float)w * 0.5f, (float)by, (float)bw * 0.5f, (float)lh, wood);
        R(bx, by - 1, bw, 2, wood);
        for (int x = bx; x < bx + bw; x++) for (int y = 0; y < by; y++) if ((G(x, y) & 255) && hn(x, y, 3) > 0.92f) P(x, y, pdark(wood, 1.2f));
        R(bx + 2, by - lh + 1, 2, lh, band); R(bx + bw - 4, by - lh + 1, 2, lh, band);
        R(bx, by, bw, 1, bandd);
        R(w / 2 - 2, by - 1, 4, 5, band); R(w / 2 - 1, by + 1, 2, 2, C(0x2A1810));
        if (big) { P(w / 2, by - 3, C(0xFFF0B0)); E((float)w * 0.5f, (float)by - 5, 3, 2, C(0xE8DCC0)); P(w / 2 - 1, by - 5, C(0x1A1010)); P(w / 2 + 1, by - 5, C(0x1A1010)); }
    } else {
        int lh = big ? 10 : 7;
        R(bx, by - lh - 2, bw, lh, wd);                                  /* lid tipped back */
        R(bx + 1, by - lh - 1, bw - 2, lh - 2, C(0x3A1A10));
        R(bx + 2, by - lh - 2, 2, lh, band); R(bx + bw - 4, by - lh - 2, 2, lh, band);
        for (int i = 0; i < (big ? 40 : 14); i++) {                     /* gold heaped inside */
            float x = (float)bx + 2 + hn(i, 2, 9) * (float)(bw - 4), y = (float)by + 1 - hn(i, 3, 9) * (big ? 5.0f : 3.0f);
            E(x, y, 1.6f, 1.1f, (i % 3 == 0) ? C(0xFFE878) : C(0xE8A828));
        }
        if (big) { P(bx + 6, by - 3, C(0xE02840)); P(bx + bw - 8, by - 2, C(0x30C060)); P(w / 2, by - 4, C(0x3070F0)); }
    }
    pix_outline(OUT);
    int id = big ? (open ? SP_BIGCHEST_OPEN : SP_BIGCHEST) : (open ? SP_CHEST_OPEN : SP_CHEST);
    pix_commit(&art_spr[id], w / 2, h - 2);
}
static void chain(void)
{
    pix_clear(8, 34);
    for (int y = 0; y < 34; y += 4) { if ((y / 4) & 1) R(3, y, 2, 5, C(0x5A5A62)); else { R(2, y, 4, 4, C(0x6E6E78)); R(3, y + 1, 2, 2, 0); } }
    pix_commit(&art_spr[SP_CHAIN], 4, 33);
}
static void pillar(int broken)
{
    int h = broken ? 22 : 44;
    pix_clear(18, h);
    for (int y = 4; y < h - 3; y++)
        for (int x = 3; x < 15; x++) {
            uint32_t c = x < 6 ? C(0x9A9478) : (x > 11 ? C(0x5A5644) : C(0x7A7660));
            if ((x - 3) % 3 == 0) c = pdark(c, 0.88f);
            if (hn(x, y, 41) > 0.93f) c = C(0x4E6A30);
            P(x, y, c);
        }
    R(1, h - 4, 16, 3, C(0x6A6652)); R(1, h - 4, 16, 1, C(0x8A8670));
    if (!broken) { R(1, 1, 16, 4, C(0x7A7660)); R(1, 1, 16, 1, C(0xA8A48C)); R(1, 4, 16, 1, C(0x4A4638)); }
    else for (int x = 3; x < 15; x++) for (int y = 3; y < 6 + (int)(hn(x, 0, 2) * 4); y++) P(x, y, 0);
    if (!broken) for (int y = 8; y < 24; y++) { P(5, y, C(0x3E5A28)); if (y < 16) P(6, y, C(0x4E6A30)); }
    pix_outline(OUT);
    pix_commit(&art_spr[broken ? SP_PILLAR_BROKEN : SP_PILLAR], 9, h - 2);
}
static void skullwall(void)
{
    pix_clear(72, 56);
    for (int y = 0; y < 56; y++)
        for (int x = 0; x < 72; x++) {
            int row = y / 7, off = (row & 1) * 6;
            uint32_t c = pmix(C(0x5E5A48), C(0x4A4638), hn((x + off) / 12, row, 7));
            if (y % 7 == 0 || (x + off) % 12 == 0) c = C(0x2E2A22);
            P(x, y, c);
        }
    E(36, 24, 22, 19, C(0x8A846C));
    E(36, 22, 20, 16, C(0xA49E84));
    E(28, 23, 6, 6.5f, C(0x14100C)); E(44, 23, 6, 6.5f, C(0x14100C));
    E(28, 23, 2, 2, C(0x3A1A0A)); E(44, 23, 2, 2, C(0x3A1A0A));
    R(34, 30, 4, 6, C(0x14100C)); P(35, 29, C(0x14100C));
    R(24, 38, 24, 9, C(0x8A846C));
    for (int x = 25; x < 48; x += 4) R(x, 39, 3, 7, C(0xB8B298));
    for (int x = 25; x < 48; x += 4) R(x + 3, 39, 1, 7, C(0x3A3628));
    L(18, 10, 26, 18, C(0x6A6450)); L(50, 12, 46, 20, C(0x6A6450));
    for (int y = 8; y < 30; y++) { P(14, y, C(0x3E5A28)); P(15, y + 2, C(0x4E6A30)); if (y < 22) P(58, y, C(0x3E5A28)); }
    pix_outline(OUT);
    pix_commit(&art_spr[SP_SKULLWALL], 36, 54);
}
static void lighthouse(void)
{
    pix_clear(52, 124);
    /* tapering striped tower */
    for (int y = 30; y < 118; y++) {
        float t = (float)(y - 30) / 88.0f;
        float hw = 10.0f + t * 9.0f;
        int band = ((y - 30) / 14) & 1;
        for (int x = (int)(26 - hw); x <= (int)(26 + hw); x++) {
            float s = ((float)x - 26.0f) / hw;
            uint32_t base = band ? C(0xB83028) : C(0xE8E0D4);
            uint32_t c = pdark(base, 1.08f - (s + 1.0f) * 0.28f);
            if (hn(x, y, 61) > 0.94f) c = pdark(c, 0.85f);
            P(x, y, c);
        }
    }
    R(14, 116, 24, 6, C(0x6A6662)); R(14, 116, 24, 1, C(0x8A8680));
    /* door + windows */
    R(22, 104, 8, 13, C(0x3A2416)); R(23, 105, 6, 12, C(0x5A3A22)); P(28, 111, C(0xE0B040));
    R(24, 60, 4, 6, C(0x2A2A3A)); R(25, 61, 2, 4, C(0xFFD27A));
    R(24, 84, 4, 6, C(0x2A2A3A));
    /* gallery + lamp room */
    R(10, 28, 32, 3, C(0x2A2A30)); for (int x = 11; x < 42; x += 3) R(x, 23, 1, 5, C(0x2A2A30)); R(10, 22, 32, 1, C(0x2A2A30));
    R(17, 10, 18, 13, C(0x1E2A30));
    R(19, 12, 14, 10, C(0xFFE8A8)); R(21, 13, 10, 8, C(0xFFFFFF));
    for (int x = 19; x < 33; x += 4) R(x, 12, 1, 10, C(0x2A2A30));
    E(26, 9, 11, 5, C(0x8A2420)); E(26, 8, 9, 3.5f, C(0xA83028)); R(25, 0, 2, 4, C(0x2A2A30));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_LIGHTHOUSE], 26, 121);
}
static void shack(void)
{
    pix_clear(72, 62);
    /* roof */
    for (int y = 2; y < 30; y++)
        for (int x = 2; x < 70; x++) {
            int row = y / 4, off = (row & 1) * 3;
            uint32_t c = pmix(C(0x4A3E46), C(0x38303A), hn((x + off) / 6, row, 13));
            if (y % 4 == 3) c = C(0x221A20);
            if ((x + off) % 6 == 0) c = C(0x2A2028);
            if (y < 4) c = C(0x5A4E56);
            P(x, y, c);
        }
    for (int x = 2; x < 70; x++) { P(x, 29, C(0x1A1418)); P(x, 30, C(0x1A1418)); }
    /* walls */
    for (int y = 31; y < 60; y++)
        for (int x = 4; x < 68; x++) {
            uint32_t c = pmix(C(0x6A4C36), C(0x52382A), hn(x / 4, 1, 17) * 0.6f + hn(x, y, 3) * 0.3f);
            if (x % 4 == 0) c = C(0x2E2018);
            P(x, y, c);
        }
    /* windows: warm light inside */
    for (int k = 0; k < 2; k++) {
        int wx = k ? 48 : 10;
        R(wx, 36, 13, 11, C(0x2A1A10)); R(wx + 1, 37, 11, 9, C(0xFFCF70)); R(wx + 1, 37, 11, 3, C(0xFFE6A8));
        R(wx + 6, 37, 1, 9, C(0x3A2416)); R(wx + 1, 41, 11, 1, C(0x3A2416)); R(wx - 1, 47, 15, 2, C(0x6A4A30));
    }
    R(29, 38, 14, 22, C(0x2A1A10)); R(30, 39, 12, 21, C(0x6A4428));
    for (int y = 41; y < 60; y += 4) R(30, y, 12, 1, C(0x52341E));
    P(39, 50, C(0xE0B040));
    /* chimney + nets */
    R(54, 0, 7, 12, C(0x5A4A44)); R(54, 0, 7, 2, C(0x3A2E2A));
    for (int i = 0; i < 6; i++) L(62 + (i & 1), 33 + i * 4, 66, 35 + i * 4, C(0xA89870));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_SHACK], 36, 59);
}
static void well(void)
{
    pix_clear(24, 28);
    R(3, 2, 2, 16, C(0x5A3A20)); R(19, 2, 2, 16, C(0x5A3A20)); R(1, 1, 22, 3, C(0x6A4A2E)); R(1, 1, 22, 1, C(0x8A6A40));
    R(11, 4, 2, 6, C(0x8A7A5A)); R(10, 9, 4, 4, C(0x5A3A20));
    E(12, 20, 10, 5, C(0x6E6A64)); E(12, 19, 8, 3.2f, C(0x14181E)); E(12, 19.5f, 6, 2, C(0x1E2A3A));
    for (int i = 0; i < 8; i++) P(3 + i * 2 + (i & 1), 22 + (i & 1), C(0x4E4A46));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_WELL], 12, 25);
}
static void cannon(void)
{
    pix_clear(26, 16);
    Ld(5, 7, 22, 5, 5.0f, C(0x2E2E34)); Ld(5, 6, 22, 4, 1.4f, C(0x5A5A66));
    E(23, 5, 2, 3, C(0x14141A));
    R(4, 9, 12, 4, C(0x5A3A20)); E(6, 13, 3, 2.4f, C(0x3A2414)); E(14, 13, 3, 2.4f, C(0x3A2414));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_CANNON], 11, 14);
}
static void pot(int broken)
{
    pix_clear(12, 14);
    if (broken) {
        E(4, 11, 3, 1.6f, C(0x9A5A3A)); E(9, 12, 2, 1.2f, C(0x7A4A2A)); P(6, 9, C(0xB87A4A));
    } else {
        E(6, 8, 5, 5, C(0x9A5A3A)); E(5, 7, 3, 3, C(0xB87A4A)); R(3, 1, 6, 3, C(0x8A4A2A)); R(4, 1, 4, 1, C(0x3A1A10));
        R(1, 8, 10, 1, C(0x6A3A20));
    }
    pix_outline(OUT);
    pix_commit(&art_spr[broken ? SP_POT_BROKEN : SP_POT], 6, 12);
}
static void nets(void)
{
    pix_clear(28, 16);
    for (int x = 2; x < 26; x++) for (int y = 3; y < 14; y++) if (((x + y) % 4 == 0 || (x - y + 40) % 4 == 0) && hn(x, y, 3) > 0.15f) P(x, y, C(0xA89870));
    E(6, 4, 2, 2, C(0xD84030)); E(15, 3, 2, 2, C(0xF0C030)); E(22, 5, 2, 2, C(0xD84030));
    pix_outline(CA(0x140E10, 200));
    pix_commit(&art_spr[SP_NETS], 14, 14);
}
static void crane(void)
{
    pix_clear(40, 50);
    R(6, 8, 4, 40, C(0x5A3A20)); R(6, 8, 1, 40, C(0x7A5430));
    Ld(8, 10, 36, 4, 3.0f, C(0x6A4428)); Ld(8, 9, 36, 3, 1.0f, C(0x8A6438));
    Ld(8, 30, 30, 6, 2.0f, C(0x5A3A20));
    R(35, 4, 1, 22, C(0xB8A880)); R(32, 26, 7, 5, C(0x5A5A62)); R(33, 25, 5, 1, C(0x8A8A94));
    /* the winch drum: missing gears shown as empty axles */
    E(8, 40, 6, 6, C(0x3A3A40)); E(8, 40, 4, 4, C(0x6A5A44)); E(8, 40, 1.4f, 1.4f, C(0x1A1A1E));
    R(2, 44, 16, 4, C(0x4A3020));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_CRANE], 8, 47);
}
static void lever(int right)
{
    pix_clear(14, 16);
    R(2, 10, 10, 5, C(0x4A4A52)); R(2, 10, 10, 1, C(0x7A7A84));
    if (right) Ld(7, 11, 11, 3, 1.6f, C(0x6A4428)); else Ld(7, 11, 3, 3, 1.6f, C(0x6A4428));
    E(right ? 11 : 3, 3, 1.8f, 1.8f, C(0xC83020));
    pix_outline(OUT);
    pix_commit(&art_spr[right ? SP_LEVER_R : SP_LEVER_L], 7, 14);
}
static void gate(int open)
{
    pix_clear(34, 36);
    R(0, 0, 4, 36, C(0x5E5A48)); R(30, 0, 4, 36, C(0x5E5A48)); R(0, 0, 34, 5, C(0x6E6A56)); R(0, 0, 34, 1, C(0x9A9478));
    R(0, 0, 1, 36, C(0x8A8670)); R(33, 0, 1, 36, C(0x2E2A22));
    int top = 5, bot = open ? 10 : 35;
    for (int x = 6; x < 30; x += 4) { R(x, top, 2, bot - top, C(0x3A3A42)); R(x, top, 1, bot - top, C(0x6A6A74)); if (!open) R(x, bot - 2, 2, 2, C(0x8A8A94)); }
    for (int y = top + 6; y < bot; y += 8) R(4, y, 26, 2, C(0x2E2E36));
    pix_outline(OUT);
    pix_commit(&art_spr[open ? SP_GATE_OPEN : SP_GATE], 17, 35);
}
static void block(void)
{
    pix_clear(18, 22);
    R(1, 1, 16, 8, C(0x9A9478)); R(1, 1, 16, 1, C(0xB8B298));
    R(1, 9, 16, 11, C(0x6A6652));
    for (int y = 11; y < 20; y += 4) R(1, y, 16, 1, C(0x4E4A3C));
    R(5, 3, 8, 4, C(0x8A846C)); E(9, 5, 2, 1.5f, C(0x5A5644));
    R(1, 19, 16, 1, C(0x2E2A22));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BLOCK], 9, 20);
}
static void bonepipe(void)
{
    pix_clear(12, 28);
    R(4, 4, 4, 20, C(0xE0D8C0)); R(4, 4, 1, 20, C(0xFFF8E8)); R(7, 4, 1, 20, C(0xA8A088));
    E(6, 4, 3.5f, 2.6f, C(0xE8E0C8)); E(6, 4, 1.6f, 1.2f, C(0x2A2020));
    E(6, 24, 3.5f, 2.2f, C(0xD0C8B0));
    for (int y = 8; y < 22; y += 5) R(4, y, 4, 1, C(0xB8B098));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BONEPIPE], 6, 26);
}
static void darter(void)
{
    pix_clear(16, 18);
    R(1, 1, 14, 16, C(0x6A6652)); R(1, 1, 14, 1, C(0x9A9478));
    E(8, 7, 5, 4.5f, C(0x8A846C));
    R(5, 5, 2, 2, C(0x1A1410)); R(9, 5, 2, 2, C(0x1A1410)); P(5, 5, C(0xFF5020)); P(10, 5, C(0xFF5020));
    E(8, 12, 2.5f, 2, C(0x14100C));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_DARTER], 8, 16);
}
static void flamejet(void)
{
    pix_clear(14, 10);
    E(7, 5, 6, 3.5f, C(0x4A4440)); E(7, 4.5f, 4, 2.2f, C(0x2A2420)); E(7, 4.5f, 2, 1.2f, C(0x6A2A10));
    for (int i = 0; i < 6; i++) P(2 + i * 2, 7, C(0x6A6662));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_FLAMEJET], 7, 8);
}
static void boulder(void)
{
    pix_clear(30, 30);
    E(15, 15, 13.5f, 13.5f, C(0x5A5450));
    E(13, 13, 10, 10, C(0x6E6862));
    E(11, 10, 5, 4, C(0x86807A));
    for (int i = 0; i < 9; i++) L((int)(hn(i, 1, 3) * 24) + 3, (int)(hn(i, 2, 3) * 24) + 3, (int)(hn(i, 3, 3) * 24) + 3, (int)(hn(i, 4, 3) * 24) + 3, C(0x48423E));
    for (int y = 0; y < 30; y++) for (int x = 0; x < 30; x++) if ((G(x, y) & 255) && ((x - 15) * (x - 15) + (y - 15) * (y - 15)) > 140 && x + y > 32) P(x, y, C(0x3E3834));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_BOULDER], 15, 27);
}
static void hook(void)
{
    pix_clear(14, 26);
    R(5, 6, 4, 19, C(0x5A3A20)); R(5, 6, 1, 19, C(0x7A5430));
    for (int k = 0; k < 20; k++) { float a = (float)k * 0.314f; P((int)(7 + cosf(a) * 4.5f), (int)(4 + sinf(a) * 3.5f), C(0xB8B8C4)); P((int)(7 + cosf(a) * 3.5f), (int)(4 + sinf(a) * 2.5f), C(0x6A6A74)); }
    R(4, 22, 6, 3, C(0x4A3018));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_HOOK], 7, 24);
}
static void statue(void)
{
    pix_clear(24, 42);
    R(3, 34, 18, 7, C(0x6A6652)); R(3, 34, 18, 1, C(0x9A9478));
    R(7, 16, 10, 18, C(0x8A846C)); R(7, 16, 2, 18, C(0xA8A28A)); R(15, 16, 2, 18, C(0x5E5A48));
    E(12, 10, 5.5f, 6, C(0x8A846C)); E(11, 9, 3.5f, 4, C(0xA8A28A));
    R(5, 3, 14, 3, C(0x6A6652)); R(8, 0, 8, 4, C(0x6A6652));          /* tricorn */
    R(9, 9, 2, 2, C(0x2A2620)); P(14, 9, C(0x2A2620)); R(13, 8, 3, 1, C(0x2A2620));   /* eyepatch */
    R(9, 13, 6, 2, C(0x7A808C));                                        /* iron jaw */
    Ld(17, 20, 22, 12, 2.0f, C(0x9A9478));                              /* pointing sword */
    pix_outline(OUT);
    pix_commit(&art_spr[SP_STATUE], 12, 40);
}
static void mast(void)
{
    pix_clear(40, 64);
    R(18, 4, 5, 58, C(0x6A4A2E)); R(18, 4, 1, 58, C(0x8A6A40)); R(22, 4, 1, 58, C(0x3A2818));
    R(2, 14, 36, 3, C(0x5A3E26)); R(2, 14, 36, 1, C(0x7A5A3A));
    for (int y = 17; y < 40; y++) for (int x = 4; x < 36; x++) if (hn(x, y, 5) > 0.08f && (x < 17 || x > 23) && ((y - 17) < 20 - abs(x - 20) / 4)) P(x, y, pmix(C(0xC8BCA0), C(0x9A8E74), hn(x / 3, y / 3, 6)));
    for (int y = 18; y < 38; y += 6) L(4, y, 36, y + 1, CA(0x6A5E48, 160));
    R(15, 2, 11, 3, C(0x3A2818));
    L(20, 4, 2, 60, CA(0x8A7A5A, 220)); L(20, 4, 38, 60, CA(0x8A7A5A, 220));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_MAST], 20, 62);
    pix_clear(20, 20);                                     /* ship's wheel */
    for (int k = 0; k < 8; k++) { float a = (float)k * 0.785f; Ld(10, 10, 10 + cosf(a) * 9, 10 + sinf(a) * 9, 1.2f, C(0x8A6438)); }
    for (int k = 0; k < 32; k++) { float a = (float)k * 0.196f; P((int)(10 + cosf(a) * 6.5f), (int)(10 + sinf(a) * 6.5f), C(0x6A4428)); P((int)(10 + cosf(a) * 5.5f), (int)(10 + sinf(a) * 5.5f), C(0x9A7444)); }
    E(10, 10, 2, 2, C(0xD8A840));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_WHEEL], 10, 19);
}
static void hanglantern(void)
{
    pix_clear(10, 18);
    R(4, 0, 2, 5, C(0x3A3A40));
    R(2, 5, 6, 2, C(0x6A5030)); R(2, 7, 6, 6, C(0xFFD068)); R(3, 8, 4, 4, C(0xFFF0B0)); R(2, 13, 6, 2, C(0x6A5030));
    R(2, 7, 1, 6, C(0x4A3A20)); R(7, 7, 1, 6, C(0x4A3A20));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_LANTERN_HANG], 5, 16);
}
static void grave(void)
{
    pix_clear(14, 18);
    R(5, 1, 3, 15, C(0x6A5E52)); R(1, 4, 11, 3, C(0x6A5E52)); R(5, 1, 1, 15, C(0x8A7E70)); R(1, 4, 11, 1, C(0x8A7E70));
    R(2, 15, 10, 2, C(0x4A4038));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_GRAVE], 7, 16);
}
static void totem(void)
{
    pix_clear(18, 34);
    for (int k = 0; k < 3; k++) {
        int y = 2 + k * 10;
        uint32_t c = k == 1 ? C(0x8A5A3A) : C(0x7A4A2E);
        R(3, y, 12, 10, c); R(3, y, 2, 10, pdark(c, 1.2f));
        R(5, y + 2, 3, 2, C(0x1A1010)); R(10, y + 2, 3, 2, C(0x1A1010)); P(6, y + 2, C(0xF0E040)); P(11, y + 2, C(0xF0E040));
        R(6, y + 6, 6, 2, C(0x1A1010)); for (int x = 7; x < 12; x += 2) P(x, y + 6, C(0xE0D8C0));
    }
    R(1, 2, 16, 2, C(0x3E6A30));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_TOTEM], 9, 32);
}
static void flora(void)
{
    pix_clear(18, 14);                                     /* fern */
    for (int f = 0; f < 5; f++) { float a = -2.6f + (float)f * 0.55f; for (int k = 0; k < 9; k++) { float t = (float)k / 8.0f; E(9 + cosf(a) * t * 8, 12 + sinf(a) * t * 9 + t * t * 3, 1.2f, 1.0f, (k & 1) ? C(0x3A6A30) : C(0x2A5426)); } }
    pix_outline(OUT);
    pix_commit(&art_spr[SP_FERN], 9, 12);
    pix_clear(10, 10);                                     /* glowing mushrooms */
    R(2, 5, 1, 4, C(0xD8D0C0)); E(2.5f, 4.5f, 2.5f, 1.6f, C(0x40D8C8)); P(1, 4, C(0xC0FFF8));
    R(7, 4, 1, 5, C(0xD8D0C0)); E(7.5f, 3.5f, 2.2f, 1.5f, C(0x50E0D0)); P(7, 3, C(0xC0FFF8));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_MUSHROOM], 5, 9);
    pix_clear(14, 16);                                     /* crystals */
    Ld(4, 14, 3, 5, 3, C(0x6A40C0)); Ld(8, 14, 9, 2, 3.4f, C(0x8A60E8)); Ld(11, 14, 12, 8, 2.4f, C(0x6A40C0));
    L(8, 3, 8, 12, C(0xE0C8FF)); L(3, 6, 3, 12, C(0xC0A0F0));
    pix_outline(C(0x1A0A2A));
    pix_commit(&art_spr[SP_CRYSTAL], 7, 14);
    pix_clear(12, 22);                                     /* stalagmite */
    for (int y = 0; y < 20; y++) { int hw = 1 + y / 4; R(6 - hw, y + 1, hw * 2, 1, y < 3 ? C(0x7A7280) : C(0x5A5262)); P(6 - hw, y + 1, C(0x8A8290)); }
    pix_outline(OUT);
    pix_commit(&art_spr[SP_STALAG], 6, 20);
    pix_clear(8, 6);                                       /* shell */
    E(4, 3, 3.4f, 2.4f, C(0xF0D8C8)); L(4, 1, 2, 5, C(0xC8A898)); L(4, 1, 6, 5, C(0xC8A898)); L(4, 1, 4, 5, C(0xC8A898));
    pix_outline(C(0x3A2A20));
    pix_commit(&art_spr[SP_SHELL], 4, 5);
    pix_clear(9, 9);                                       /* starfish */
    for (int k = 0; k < 5; k++) { float a = -1.5708f + (float)k * 1.2566f; Ld(4.5f, 4.5f, 4.5f + cosf(a) * 4, 4.5f + sinf(a) * 4, 1.4f, C(0xE87040)); }
    pix_outline(C(0x3A1A10));
    pix_commit(&art_spr[SP_STARFISH], 4, 8);
    pix_clear(20, 10);                                     /* puddle */
    E(10, 5, 9, 3.5f, CA(0x1E3048, 200)); E(8, 4, 5, 1.6f, CA(0x5A7A9A, 170));
    pix_commit(&art_spr[SP_PUDDLE], 10, 5);
    pix_clear(16, 10);                                     /* seaweed / lily */
    for (int i = 0; i < 5; i++) Ld(3 + i * 2.5f, 9, 2 + i * 2.5f + (i & 1) * 2, 2 + (i & 1), 1.2f, (i & 1) ? C(0x3A6A40) : C(0x2A5A36));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_LILY], 8, 9);
}
static void carving(void)
{
    pix_clear(40, 22);
    R(0, 0, 40, 22, C(0x6E6A56)); R(0, 0, 40, 1, C(0x9A9478)); R(0, 21, 40, 1, C(0x2E2A22));
    R(2, 2, 36, 18, C(0x5E5A48));
    /* five carved bones of rising and falling length: the tune */
    static const int len[5] = { 3, 1, 4, 2, 5 };
    for (int i = 0; i < 5; i++) {
        int x = 6 + i * 7, h = 3 + len[i] * 2;
        R(x, 17 - h, 3, h, C(0x2E2A20)); R(x, 17 - h, 3, 1, C(0x1A160E));
        E((float)x + 1.5f, (float)(17 - h), 2, 1.2f, C(0x2E2A20));
        P(x + 1, 19, C(0x2E2A20)); if (i > 0) P(x + 1 - 1, 19, C(0x2E2A20));
    }
    for (int i = 0; i < 5; i++) for (int k = 0; k <= i; k++) P(6 + i * 7 + k, 4, C(0x2E2A20));   /* tally marks: order */
    pix_outline(OUT);
    pix_commit(&art_spr[SP_CARVING], 20, 21);
}
static void cage(void)
{
    pix_clear(28, 30);
    R(1, 1, 26, 3, C(0x4A4A52)); R(1, 26, 26, 3, C(0x4A4A52)); R(1, 1, 26, 1, C(0x7A7A84));
    for (int x = 2; x < 27; x += 4) { R(x, 4, 2, 22, C(0x3A3A42)); R(x, 4, 1, 22, C(0x6A6A74)); }
    R(12, 13, 4, 4, C(0xC8A040)); P(13, 15, C(0x2A1A0A));
    pix_outline(OUT);
    pix_commit(&art_spr[SP_CAGE], 14, 28);
}

void art_props(void)
{
    pine(); palm(); bush(0); bush(1); rock(0); rock(1); barrel(); crate(0); crate(1);
    lamppost(); torch(); brazier(0); brazier(1); campfire(); bench(); fence(); sign();
    anchor(); rowboat(); wreck(); bones(); goldpile();
    chest(0, 0); chest(0, 1); chest(1, 0); chest(1, 1); chain(); pillar(0); pillar(1);
    skullwall(); lighthouse(); shack(); well(); cannon(); pot(0); pot(1); nets(); crane();
    lever(0); lever(1); gate(0); gate(1); block(); bonepipe(); darter(); flamejet();
    boulder(); hook(); statue(); mast(); hanglantern(); grave(); totem(); flora(); carving(); cage();
}
