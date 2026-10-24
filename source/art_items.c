/*
 * TREASURE HOUNDS - pickups and HUD icons.
 */
#include "pix.h"

static void coin(int f)
{
    static const float wv[4] = { 4.0f, 2.8f, 0.9f, 2.8f };
    pix_clear(10, 10);
    float w = wv[f];
    E(5, 5, w, 4.0f, C(0x9A6410));
    if (w > 1.5f) {
        E(5, 4.6f, w - 0.8f, 3.3f, C(0xF2C230));
        E(4.4f, 4.0f, (w - 0.8f) * 0.5f, 1.6f, C(0xFFEC98));
        if (f == 0) { R(5, 3, 1, 4, C(0xC88E18)); }
    } else R(5, 1, 1, 8, C(0xF8D860));
    pix_commit(&art_item[IT_COIN + f], 5, 9);
}
static void gem(int it, uint32_t c)
{
    pix_clear(10, 10);
    uint32_t d = pdark(c, 0.6f), l = pdark(c, 1.4f);
    for (int y = 0; y < 9; y++) {
        int half = y < 3 ? 2 + y : 8 - y;
        for (int x = 5 - half; x <= 4 + half; x++) P(x, y, y < 3 ? l : (x < 5 ? c : d));
    }
    R(3, 2, 4, 1, C(0xFFFFFF)); P(2, 3, l); P(4, 1, C(0xFFFFFF));
    pix_outline(pdark(c, 0.3f));
    pix_commit(&art_item[it], 5, 9);
}
static void gear(void)
{
    pix_clear(14, 14);
    for (int i = 0; i < 8; i++) {
        float a = (float)i * 0.785398f;
        E(7 + cosf(a) * 5.0f, 7 + sinf(a) * 5.0f, 1.6f, 1.6f, C(0xA87A3A));
    }
    E(7, 7, 4.8f, 4.8f, C(0xB8883E));
    E(6.5f, 6.5f, 3.2f, 3.2f, C(0xD8A858));
    E(7, 7, 1.6f, 1.6f, C(0x3A2410));
    P(5, 9, C(0x7A4A28)); P(9, 4, C(0x7A4A28));               /* rust */
    pix_outline(C(0x3A2410));
    pix_commit(&art_item[IT_GEAR], 7, 13);
}
static void skullkey(void)
{
    pix_clear(20, 12);
    uint32_t g = C(0xF0C040), gd = C(0xA8781C), gl = C(0xFFF0A8);
    E(5, 5, 4.5f, 4.2f, g);
    E(4, 4, 2.5f, 2.2f, gl);
    R(3, 4, 2, 2, C(0x3A2010)); R(6, 4, 2, 2, C(0x3A2010));
    P(5, 7, C(0x3A2010)); R(3, 8, 5, 1, gd);
    R(9, 4, 9, 2, g); R(9, 5, 9, 1, gd);
    R(15, 6, 1, 3, g); R(17, 6, 1, 4, g); R(12, 6, 1, 2, g);
    pix_outline(C(0x3A2208));
    pix_commit(&art_item[IT_SKULLKEY], 10, 11);
}
static void doubloon(void)
{
    pix_clear(12, 12);
    E(6, 6, 5.5f, 5.5f, C(0x9A6410)); E(6, 5.6f, 4.8f, 4.8f, C(0xF2C230)); E(6, 6, 3.4f, 3.4f, C(0xD8A020));
    R(4, 4, 1, 2, C(0x6A4010)); R(7, 4, 1, 2, C(0x6A4010)); R(5, 7, 2, 1, C(0x6A4010));
    P(3, 2, C(0xFFF0B0)); P(4, 2, C(0xFFF0B0));
    pix_commit(&art_item[IT_DOUBLOON], 6, 11);
}
static void heart(int it, int fill)
{
    pix_clear(10, 9);
    uint32_t c = C(0xE83040), d = C(0xA01828), e = C(0x3A2A30);
    for (int y = 0; y < 9; y++)
        for (int x = 0; x < 10; x++) {
            float fx = (float)x - 4.5f, fy = (float)y - 3.0f;
            float l = fabsf(fx);
            int in = (fy < 0) ? ((l - 2.3f) * (l - 2.3f) + (fy + 0.5f) * (fy + 0.5f) < 5.5f) : (l + fy < 5.0f);
            if (!in) continue;
            int full = fill == 2 || (fill == 1 && x < 5);
            P(x, y, full ? (y > 4 || x > 6 ? d : c) : e);
        }
    if (fill) { P(2, 2, C(0xFFB0B8)); P(3, 1, C(0xFFB0B8)); }
    pix_outline(C(0x1A0A10));
    pix_commit(&art_item[it], 5, 8);
}
static void candy(void)
{
    pix_clear(14, 8);
    R(3, 2, 8, 4, C(0x6A3A1A)); R(3, 2, 8, 1, C(0x8A5A2A));
    R(5, 2, 4, 4, C(0x2A6AD0)); R(5, 3, 4, 1, C(0xF0F0F0));
    R(1, 2, 2, 4, C(0xC8C8D0)); R(11, 2, 2, 4, C(0xC8C8D0)); P(0, 1, C(0xC8C8D0)); P(13, 6, C(0xC8C8D0));
    pix_outline(C(0x1A1010));
    pix_commit(&art_item[IT_CANDY], 7, 7);
}
static void pizza(void)
{
    pix_clear(14, 12);
    for (int y = 0; y < 11; y++) { int w = 11 - y; R(1 + y / 2, y, w, 1, y < 2 ? C(0xC8862A) : C(0xF0C850)); }
    P(4, 4, C(0xC83020)); P(7, 3, C(0xC83020)); P(6, 6, C(0xC83020)); P(5, 4, C(0xE84030));
    pix_outline(C(0x3A2010));
    pix_commit(&art_item[IT_PIZZA], 7, 11);
}
static void lantern(void)
{
    pix_clear(10, 14);
    R(4, 0, 2, 1, C(0x6A5030)); P(3, 1, C(0x6A5030)); P(6, 1, C(0x6A5030));
    R(2, 2, 6, 2, C(0x8A6A30)); R(2, 2, 6, 1, C(0xC8A048));
    R(2, 4, 6, 6, C(0xFFD068)); R(3, 5, 4, 4, C(0xFFF0B0));
    R(2, 4, 1, 6, C(0x6A5030)); R(7, 4, 1, 6, C(0x6A5030)); R(4, 4, 1, 6, CA(0x6A5030, 160));
    R(1, 10, 8, 2, C(0x8A6A30)); R(1, 10, 8, 1, C(0xC8A048));
    pix_outline(C(0x2A1A0A));
    pix_commit(&art_item[IT_LANTERN], 5, 12);
}
static void whip(void)
{
    pix_clear(14, 14);
    for (int i = 0; i < 3; i++) { float r = 5.0f - (float)i * 1.4f; for (int k = 0; k < 24; k++) { float a = (float)k * 0.2618f; P((int)(7 + cosf(a) * r), (int)(7 + sinf(a) * r * 0.8f), i & 1 ? C(0x7A4A24) : C(0x9A6A34)); } }
    R(9, 9, 4, 2, C(0x4A2A14)); R(9, 9, 4, 1, C(0x6A4020));
    pix_outline(C(0x1E100A));
    pix_commit(&art_item[IT_WHIP], 7, 13);
}
static void map(void)
{
    pix_clear(14, 12);
    R(1, 1, 12, 10, C(0xE0C890)); R(1, 1, 12, 1, C(0xF0DCA8));
    for (int y = 2; y < 11; y++) if (hn(0, y, 4) > 0.5f) P(1, y, C(0xB89860));
    L(3, 8, 6, 5, C(0x8A6A3A)); L(6, 5, 9, 6, C(0x8A6A3A));
    P(9, 3, C(0xC82020)); P(10, 4, C(0xC82020)); P(10, 3, C(0xC82020)); P(9, 4, C(0xC82020)); P(8, 2, C(0xC82020)); P(11, 5, C(0xC82020)); P(11, 2, C(0xC82020)); P(8, 5, C(0xC82020));
    R(0, 0, 2, 12, C(0xC8B078)); R(12, 0, 2, 12, C(0xC8B078));
    pix_outline(C(0x3A2A14));
    pix_commit(&art_item[IT_MAP], 7, 11);
}
static void rope(void)
{
    pix_clear(14, 12);
    for (int i = 0; i < 4; i++) for (int k = 0; k < 28; k++) { float a = (float)k * 0.2244f; float r = 5.4f - (float)i; P((int)(7 + cosf(a) * r), (int)(6 + sinf(a) * r * 0.75f), (k + i) & 1 ? C(0xC8A060) : C(0xA8804A)); }
    pix_outline(C(0x3A2A14));
    pix_commit(&art_item[IT_ROPE], 7, 11);
}
static void misc(void)
{
    pix_clear(10, 3);                                      /* dart */
    R(0, 1, 7, 1, C(0x6A5030)); R(7, 0, 3, 3, C(0xC8C8D0)); P(9, 1, C(0xFFFFFF)); P(0, 0, C(0xC83020)); P(0, 2, C(0xC83020));
    pix_commit(&art_item[IT_DART], 5, 2);
    pix_clear(8, 12);                                      /* bottle */
    R(3, 0, 2, 3, C(0x6A4A2A)); R(2, 3, 4, 2, CA(0x3A8A4A, 230)); R(1, 5, 6, 6, CA(0x2A7A3A, 230)); R(2, 6, 1, 4, C(0x9AE0A8));
    pix_outline(C(0x0A1A0E));
    pix_commit(&art_item[IT_BOTTLE], 4, 11);
    pix_clear(9, 9);                                       /* dizzy star */
    R(4, 0, 1, 9, C(0xFFE860)); R(0, 4, 9, 1, C(0xFFE860)); R(3, 3, 3, 3, C(0xFFE860)); P(4, 4, C(0xFFFFFF));
    pix_commit(&art_item[IT_STAR], 4, 4);
    pix_clear(10, 10);                                     /* map scrap */
    for (int y = 1; y < 9; y++) for (int x = 1 + (y & 1); x < 9 - (y % 3 == 0); x++) P(x, y, C(0xE0C890));
    P(5, 4, C(0xC82020)); P(4, 5, C(0xC82020)); P(6, 5, C(0xC82020)); P(5, 6, C(0xC82020));
    pix_outline(C(0x3A2A14));
    pix_commit(&art_item[IT_MAPSCRAP], 5, 9);
    pix_clear(14, 8);                                      /* iron key */
    E(3, 4, 2.6f, 2.6f, C(0x8A8A94)); P(3, 4, 0); R(5, 3, 8, 2, C(0x8A8A94)); R(10, 5, 1, 2, C(0x8A8A94)); R(12, 5, 1, 2, C(0x8A8A94));
    pix_outline(C(0x1A1A20));
    pix_commit(&art_item[IT_KEY], 7, 7);
}

void art_items(void)
{
    for (int f = 0; f < 4; f++) coin(f);
    gem(IT_GEM_R, C(0xE02840)); gem(IT_GEM_B, C(0x3070F0)); gem(IT_GEM_G, C(0x30C060));
    gear(); skullkey(); doubloon();
    heart(IT_HEART, 2); heart(IT_HEART_HALF, 1); heart(IT_HEART_EMPTY, 0);
    candy(); pizza(); lantern(); whip(); map(); rope(); misc();
}
