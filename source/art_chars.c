/*
 * TREASURE HOUNDS - characters, portraits and creatures, painted from a parametric body.
 *
 *  Sam    - the leader and dreamer: messy brown hair, red hoodie, jeans
 *  Rocco  - the motor-mouth: backwards blue cap, yellow tee, shorts, red kicks, braces
 *  Gus    - the inventor: ginger curls, big glasses, green gadget vest, freckles
 *  Ollie  - the big-hearted snack fiend: blond mop, blue striped shirt, rosy cheeks
 *  Vic, Lenny and Ma Crowley - the bumbling crooks;  Tiny - Ma's gentle giant of a son
 */
#include "pix.h"
#include <string.h>

enum { HAIR_MESSY, HAIR_CAP, HAIR_CURLY, HAIR_MOP, HAIR_SLICK, HAIR_BEANIE, HAIR_BUN, HAIR_BALD, HAIR_BANDANA };

typedef struct {
    int w, h;                    /* canvas */
    int torso_w, torso_h, leg_h; /* body proportions */
    float head_rx, head_ry;
    uint32_t skin, hair, shirt, shirt2, pants, shoe, outline;
    int hair_style, stripes, glasses, freckles, braces, mustache, backpack, coat, shorts, cheeks, skeleton, belly;
    uint32_t pack;
} Body;

static Body BODIES[CH_COUNT];
static Body SKEL;

static void init_bodies(void)
{
    Body b;
    memset(&b, 0, sizeof(b));
    b.w = 20; b.h = 28; b.torso_w = 10; b.torso_h = 7; b.leg_h = 4; b.head_rx = 6.0f; b.head_ry = 5.6f;
    b.outline = C(0x1A1018); b.backpack = 1;

    Body s = b;                                     /* SAM */
    s.skin = C(0xF0BE96); s.hair = C(0x6A4024); s.shirt = C(0xC8302C); s.shirt2 = C(0xE8E0D0);
    s.pants = C(0x3A5A8C); s.shoe = C(0xE8E8E8); s.hair_style = HAIR_MESSY; s.pack = C(0x6A5A38);
    BODIES[CH_SAM] = s;

    Body r = b;                                     /* ROCCO */
    r.skin = C(0xC88E62); r.hair = C(0x2A1E1A); r.shirt = C(0xF0C030); r.shirt2 = C(0x2E6AD0);
    r.pants = C(0x9A8A5E); r.shoe = C(0xD83030); r.hair_style = HAIR_CAP; r.braces = 1; r.shorts = 1; r.pack = C(0x3A4A6A);
    BODIES[CH_ROCCO] = r;

    Body g = b;                                     /* GUS */
    g.skin = C(0xF6CCAA); g.hair = C(0xD86A28); g.shirt = C(0x4E8A3E); g.shirt2 = C(0xE8E4D4);
    g.pants = C(0x6A5440); g.shoe = C(0x5A4A3A); g.hair_style = HAIR_CURLY; g.glasses = 1; g.freckles = 1; g.pack = C(0x8A7A5A);
    BODIES[CH_GUS] = g;

    Body o = b;                                     /* OLLIE */
    o.torso_w = 12; o.head_rx = 6.3f;
    o.skin = C(0xF4C4A0); o.hair = C(0xE8C860); o.shirt = C(0x3A6AB8); o.shirt2 = C(0xD8E4F4);
    o.pants = C(0x3A3A4A); o.shoe = C(0x7A4A2A); o.hair_style = HAIR_MOP; o.stripes = 1; o.cheeks = 1; o.shorts = 1; o.belly = 1;
    o.pack = C(0x9A3A3A);
    BODIES[CH_OLLIE] = o;

    Body adult = b;
    adult.w = 22; adult.h = 32; adult.torso_h = 9; adult.leg_h = 6; adult.head_rx = 5.4f; adult.head_ry = 5.2f; adult.backpack = 0;

    Body v = adult;                                 /* VIC */
    v.torso_w = 10; v.skin = C(0xE0B090); v.hair = C(0x1A1418); v.shirt = C(0x6A6A70); v.shirt2 = C(0x4A2A2A);
    v.pants = C(0x2A2A30); v.shoe = C(0x1A1414); v.hair_style = HAIR_SLICK; v.mustache = 1; v.coat = 1;
    BODIES[CH_VIC] = v;

    Body l = adult;                                 /* LENNY */
    l.torso_w = 12; l.torso_h = 8; l.leg_h = 4; l.h = 30; l.skin = C(0xE8B494); l.hair = C(0xB02A2A); l.shirt = C(0x7A5A3A);
    l.shirt2 = C(0x3A3A3A); l.pants = C(0x3A4A5A); l.shoe = C(0x2A1E14); l.hair_style = HAIR_BEANIE; l.belly = 1;
    BODIES[CH_LENNY] = l;

    Body m = adult;                                 /* MA */
    m.torso_w = 12; m.skin = C(0xEAB898); m.hair = C(0xA8A4A8); m.shirt = C(0x6A2A4A); m.shirt2 = C(0x4A1A30);
    m.pants = C(0x6A2A4A); m.shoe = C(0x2A1A1A); m.hair_style = HAIR_BUN; m.glasses = 1; m.coat = 1; m.belly = 1;
    BODIES[CH_MA] = m;

    Body t = adult;                                 /* TINY */
    t.w = 30; t.h = 40; t.torso_w = 18; t.torso_h = 12; t.leg_h = 7; t.head_rx = 7.0f; t.head_ry = 6.6f;
    t.skin = C(0xE6B898); t.hair = C(0x6A4A30); t.shirt = C(0xD8C890); t.shirt2 = C(0x3A5A8A); t.pants = C(0x3A5A8A);
    t.shoe = C(0x3A2A1A); t.hair_style = HAIR_BALD; t.cheeks = 1; t.belly = 1;
    BODIES[CH_TINY] = t;

    SKEL = adult;                                   /* skeleton pirate */
    SKEL.torso_w = 8; SKEL.skin = C(0xE0D8C0); SKEL.hair = C(0xB02828); SKEL.shirt = C(0xD8D0B8); SKEL.shirt2 = C(0x5A3A2A);
    SKEL.pants = C(0x4A3A2E); SKEL.shoe = C(0xD8D0B8); SKEL.hair_style = HAIR_BANDANA; SKEL.skeleton = 1;
}

/* ---- painting one frame ---- */
typedef struct { int dir, frame; } Pose;

static uint32_t shade(uint32_t c) { return pdark(c, 0.72f); }
static uint32_t lite(uint32_t c) { return pdark(c, 1.18f); }

static void draw_leg(int x, int ytop, int len, int w, const Body *b, int lift)
{
    int yb = ytop + len - lift;
    if (b->shorts) {
        R(x, ytop, w, 2, b->pants);                 /* shorts hem, then bare shins */
        R(x, ytop + 2, w, yb - ytop - 2, b->skin);
        R(x + w - 1, ytop + 2, 1, yb - ytop - 2, shade(b->skin));
    } else {
        R(x, ytop, w, yb - ytop, b->pants);
        R(x + w - 1, ytop, 1, yb - ytop, shade(b->pants));
    }
    if (b->coat) R(x, ytop, w, 2 > yb - ytop ? yb - ytop : 2, b->shirt);
    R(x - (w > 2 ? 0 : 0), yb, w + 1, 2, b->shoe);
    R(x, yb + 1, w + 1, 1, shade(b->shoe));
}

static void draw_torso(int x, int y, int w, int h, const Body *b, int dir)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++) {
            uint32_t c = b->shirt;
            if (b->stripes && (j % 3) == 1) c = b->shirt2;
            if (i == 0 || (dir != DIR_SIDE && i == w - 1)) c = shade(c);
            if (i == 1 && j < h - 1) c = lite(c);
            if (j == h - 1) c = shade(c);
            if (b->belly && j >= h - 4 && j < h - 1 && i > 1 && i < w - 2 && dir != DIR_UP) c = lite(c);
            P(x + i, y + j, c);
        }
    if (b->skeleton) {                                   /* ribs */
        for (int j = 1; j < h - 2; j += 2) R(x + 1, y + j, w - 2, 1, C(0x3A3028));
        R(x + w / 2, y + 1, 1, h - 3, b->shirt);
        R(x, y + h - 2, w, 2, b->pants);                   /* belt + rags */
        P(x + w / 2, y + h - 2, C(0xD8A840));
        return;
    }
    if (dir == DIR_DOWN) {
        if (b == &BODIES[CH_SAM]) { R(x + w / 2 - 1, y, 2, h - 1, b->shirt2); R(x + w / 2 - 2, y + 2, 1, 2, C(0xE8E0D0)); P(x + w / 2 + 1, y + 3, C(0xE8E0D0)); }  /* hoodie strings */
        if (b == &BODIES[CH_ROCCO]) { R(x + 3, y + 2, 4, 3, b->shirt2); P(x + 4, y + 3, C(0xF0F0F0)); }        /* logo */
        if (b == &BODIES[CH_GUS]) {                                                                  /* vest + pockets */
            R(x + w / 2 - 1, y, 2, h, b->shirt2);
            R(x + 1, y + 3, 2, 2, shade(b->shirt)); R(x + w - 3, y + 3, 2, 2, shade(b->shirt));
            P(x + w - 3, y + 2, C(0xD8D8E0)); P(x + 2, y + 1, C(0xE0A030));
        }
        if (b->coat) {
            R(x + w / 2 - 1, y, 2, h, b->shirt2);
            R(x + w / 2, y + 1, 1, h - 1, shade(b->shirt));
            for (int j = 2; j < h - 1; j += 3) P(x + w / 2 - 2, y + j, C(0xC8B060));
        }
        if (b == &BODIES[CH_TINY]) {                                                                 /* overalls */
            R(x + 3, y + 4, w - 6, h - 4, b->shirt2); R(x + 3, y, 2, 4, b->shirt2); R(x + w - 5, y, 2, 4, b->shirt2);
            P(x + 4, y + 4, C(0xE0C060)); P(x + w - 4, y + 4, C(0xE0C060));
            R(x + w / 2 - 2, y + 6, 4, 3, shade(b->shirt2));
        }
        if (b == &BODIES[CH_LENNY]) { R(x + w / 2, y, 1, h, C(0xC8B060)); }
    }
    if (dir == DIR_UP && b == &BODIES[CH_TINY]) { R(x + 3, y, 2, h, b->shirt2); R(x + w - 5, y, 2, h, b->shirt2); R(x + 3, y + h - 4, w - 6, 4, b->shirt2); }
}

static void draw_arm(int x, int y, int len, const Body *b, int back)
{
    uint32_t sl = back ? shade(b->shirt) : b->shirt;
    if (b->skeleton) sl = back ? shade(b->skin) : b->skin;
    R(x, y, 2, len - 2, sl);
    if (b->stripes) for (int j = 1; j < len - 2; j += 3) R(x, y + j, 2, 1, back ? shade(b->shirt2) : b->shirt2);
    R(x, y + len - 2, 2, 2, back ? shade(b->skin) : b->skin);
}

static void draw_hair_front(const Body *b, float cx, float cy, int dir)
{
    float rx = b->head_rx, ry = b->head_ry;
    uint32_t h = b->hair, hs = shade(b->hair), hl = lite(b->hair);
    int up = dir == DIR_UP, side = dir == DIR_SIDE;
    switch (b->hair_style) {
    case HAIR_MESSY:
        E(cx, cy - 1.5f, rx + 0.6f, ry - 0.6f, h);
        for (int i = -3; i <= 3; i++) E(cx + (float)i * 1.8f, cy - ry + 0.6f - (float)(i & 1), 1.3f, 1.8f, h);
        if (!up) {
            /* jagged fringe */
            for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) {
                int y0 = (int)(cy - 1.0f) + ((x * 7) % 3 == 0 ? 1 : 0);
                if (side && x > (int)cx + 2) y0 -= 1;
                for (int y = (int)(cy - ry); y <= y0; y++) P(x, y, (x + y) % 4 == 0 ? hl : h);
            }
            if (!side) { R((int)(cx - rx), (int)cy - 1, 2, 4, h); R((int)(cx + rx) - 1, (int)cy - 1, 2, 4, h); }
            else R((int)(cx - rx), (int)cy - 2, 4, 5, h);
        } else E(cx, cy, rx + 0.3f, ry, h);
        for (int i = 0; i < 5; i++) P((int)(cx - 3 + i * 1.5f), (int)(cy - ry + 1), hl);
        break;
    case HAIR_CAP: {
        uint32_t cap = b->shirt2, capd = shade(cap);
        /* hair tufts under the cap */
        if (!up) { R((int)(cx - rx), (int)cy - 1, 2, 3, h); R((int)(cx + rx) - 1, (int)cy - 1, 2, 3, h); }
        else E(cx, cy + 0.5f, rx, ry - 0.5f, h);
        E(cx, cy - 1.8f, rx + 0.4f, ry - 1.2f, cap);
        for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) P(x, (int)(cy - 1.2f), capd);
        P((int)cx, (int)(cy - ry + 0.5f), C(0xF0F0F0));
        if (up) { R((int)cx - 3, (int)(cy + 0.5f), 7, 2, capd); R((int)cx - 2, (int)(cy + 2.0f), 5, 1, cap); }     /* brim, facing us */
        if (side) { R((int)(cx - rx) - 2, (int)(cy - 1.5f), 4, 2, capd); }                                         /* brim out the back */
        if (!up && !side) { P((int)cx - 1, (int)(cy - 2), C(0xF0F0F0)); P((int)cx, (int)(cy - 2), C(0xF0F0F0)); }
        break;
    }
    case HAIR_CURLY:
        for (int i = 0; i < 9; i++) {
            float a = 3.14159f * (0.95f + (float)i / 8.0f * 1.1f);
            E(cx + cosf(a) * rx * 0.85f, cy - 1.0f + sinf(a) * ry * 0.85f, 2.2f, 2.2f, (i & 1) ? h : hs);
        }
        E(cx, cy - 2.2f, rx, ry - 1.4f, h);
        if (up) E(cx, cy, rx + 0.5f, ry, h);
        if (side) E(cx - 2.5f, cy, 3.0f, 3.5f, h);
        for (int i = 0; i < 6; i++) P((int)(cx - 4 + i * 1.6f), (int)(cy - ry + 0.5f + (float)(i & 1)), hl);
        if (!up && !side) for (int x = (int)(cx - rx) + 1; x < (int)(cx + rx); x += 2) P(x, (int)cy - 1, hs);
        break;
    case HAIR_MOP:
        E(cx, cy - 1.2f, rx + 0.8f, ry - 0.2f, h);
        if (!up) {
            for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) {
                int y0 = (int)cy - 1 + ((x & 1) ? 0 : 1);
                if (side && x > (int)cx + 3) y0 -= 2;
                for (int y = (int)(cy - ry); y <= y0; y++) P(x, y, h);
            }
            if (!side) { R((int)(cx - rx) - 1, (int)cy - 1, 2, 5, h); R((int)(cx + rx), (int)cy - 1, 2, 5, h); }
            else R((int)(cx - rx) - 1, (int)cy - 2, 5, 6, h);
        } else E(cx, cy + 0.5f, rx + 0.8f, ry + 0.3f, h);
        for (int i = 0; i < 4; i++) P((int)(cx - 3 + i * 2), (int)(cy - ry + 1), hl);
        for (int i = 0; i < 4; i++) P((int)(cx - 2 + i * 2), (int)(cy - ry + 2), lite(hl));
        break;
    case HAIR_SLICK:
        E(cx, cy - 1.8f, rx + 0.3f, ry - 1.0f, h);
        if (up) E(cx, cy, rx + 0.2f, ry - 0.2f, h);
        if (side) R((int)(cx - rx), (int)cy - 2, 3, 4, h);
        for (int i = 0; i < 4; i++) P((int)(cx - 2 + i), (int)(cy - ry + 1), C(0x5A5A6A));
        break;
    case HAIR_BEANIE: {
        E(cx, cy - 1.8f, rx + 0.5f, ry - 0.8f, h);
        R((int)(cx - rx), (int)(cy - 1.5f), (int)(rx * 2) + 1, 2, shade(h));
        P((int)cx, (int)(cy - ry - 0.5f), lite(h)); P((int)cx + 1, (int)(cy - ry - 0.5f), lite(h));
        for (int x = (int)(cx - rx) + 1; x < (int)(cx + rx); x += 2) P(x, (int)(cy - 3), shade(h));
        if (up) E(cx, cy, rx + 0.4f, ry - 0.5f, h);
        break;
    }
    case HAIR_BUN:
        E(cx, cy - 1.6f, rx + 0.5f, ry - 0.8f, h);
        E(cx, cy - ry - 1.5f, 2.6f, 2.2f, h);
        P((int)cx - 1, (int)(cy - ry - 2), lite(h));
        if (up) E(cx, cy, rx + 0.5f, ry, h);
        if (side) R((int)(cx - rx), (int)cy - 2, 3, 5, h);
        break;
    case HAIR_BALD:
        P((int)cx, (int)(cy - ry), h); P((int)cx + 1, (int)(cy - ry - 1), h); P((int)cx - 1, (int)(cy - ry - 1), h);
        if (up || side) for (int x = (int)(cx - rx) + 1; x < (int)(cx + rx); x++) if (side ? x < (int)cx : 1) P(x, (int)cy + 2, hs);
        break;
    case HAIR_BANDANA:
        E(cx, cy - 2.0f, rx + 0.3f, ry - 1.6f, h);
        for (int x = (int)(cx - rx); x <= (int)(cx + rx); x++) P(x, (int)(cy - 1), shade(h));
        P((int)(cx - 2), (int)(cy - 3), C(0xF0E0D0)); P((int)(cx + 1), (int)(cy - 4), C(0xF0E0D0));
        if (!side || up) { P((int)(cx + rx), (int)cy, h); P((int)(cx + rx) + 1, (int)cy + 1, h); P((int)(cx + rx), (int)cy + 2, shade(h)); }
        else { P((int)(cx - rx) - 1, (int)cy, h); P((int)(cx - rx) - 2, (int)cy + 1, h); P((int)(cx - rx) - 1, (int)cy + 2, shade(h)); }
        break;
    }
}

static void draw_face(const Body *b, float cx, float cy, int dir, int blink, int mood)
{
    uint32_t eye = C(0x1A1420), white = C(0xFFFFFF);
    int ey = (int)(cy + 1.0f);
    if (b->skeleton) {
        if (dir == DIR_DOWN) { R((int)cx - 3, ey - 1, 2, 2, C(0x1A1010)); R((int)cx + 1, ey - 1, 2, 2, C(0x1A1010)); P((int)cx - 3, ey - 1, C(0xFF5030)); P((int)cx + 2, ey - 1, C(0xFF5030)); P((int)cx, ey + 2, C(0x3A3028)); R((int)cx - 2, ey + 3, 4, 1, C(0x3A3028)); }
        if (dir == DIR_SIDE) { R((int)cx + 2, ey - 1, 2, 2, C(0x1A1010)); P((int)cx + 3, ey - 1, C(0xFF5030)); R((int)cx + 3, ey + 3, 3, 1, C(0x3A3028)); }
        return;
    }
    if (dir == DIR_DOWN) {
        int ex0 = (int)cx - 3, ex1 = (int)cx + 2;
        if (b == &BODIES[CH_TINY]) { ex0 = (int)cx - 3; ex1 = (int)cx + 2; }
        if (mood == 2) {                       /* X X eyes (dizzy) */
            P(ex0 - 1, ey - 1, eye); P(ex0 + 1, ey + 1, eye); P(ex0, ey, eye); P(ex0 + 1, ey - 1, eye); P(ex0 - 1, ey + 1, eye);
            P(ex1 - 1, ey - 1, eye); P(ex1 + 1, ey + 1, eye); P(ex1, ey, eye); P(ex1 + 1, ey - 1, eye); P(ex1 - 1, ey + 1, eye);
        } else if (blink) { P(ex0, ey, eye); P(ex0 + 1, ey, eye); P(ex1, ey, eye); P(ex1 - 1, ey, eye); }
        else {
            R(ex0, ey - 1, 1, 2, eye); R(ex1, ey - 1, 1, 2, eye);
            if (mood == 1) { P(ex0, ey - 1, white); P(ex1, ey - 1, white); P(ex0 - 1, ey - 1, eye); P(ex1 + 1, ey - 1, eye); }
        }
        if (b->glasses) {
            uint32_t fr = b == &BODIES[CH_MA] ? C(0x8A6A3A) : C(0x3A3A48);
            for (int k = 0; k < 2; k++) {
                int x = k ? ex1 : ex0;
                P(x - 1, ey - 2, fr); P(x, ey - 2, fr); P(x + 1, ey - 2, fr);
                P(x - 1, ey + 1, fr); P(x, ey + 1, fr); P(x + 1, ey + 1, fr);
                P(x - 1, ey - 1, fr); P(x + 1, ey - 1, fr); P(x - 1, ey, fr); P(x + 1, ey, fr);
                P(x + 1, ey - 1, C(0xD8F0FF));
            }
            P((int)cx - 1, ey - 1, fr); P((int)cx, ey - 1, fr);
        }
        if (b->freckles) { P(ex0 - 1, ey + 2, C(0xC8784A)); P(ex0 + 1, ey + 2, C(0xC8784A)); P(ex1 - 1, ey + 2, C(0xC8784A)); P(ex1 + 1, ey + 2, C(0xC8784A)); }
        if (b->cheeks) { P(ex0 - 1, ey + 2, C(0xE88A80)); P(ex1 + 1, ey + 2, C(0xE88A80)); }
        int my = ey + 3;
        if (mood == 3) { R((int)cx - 1, my, 2, 2, C(0x5A1A1A)); }                       /* shock O */
        else if (b->mustache) { R((int)cx - 2, my - 1, 4, 1, C(0x1A1418)); P((int)cx - 3, my, C(0x1A1418)); P((int)cx + 2, my, C(0x1A1418)); }
        else if (b->braces) { R((int)cx - 1, my, 3, 1, C(0xFFFFFF)); P((int)cx, my, C(0xA8B0C0)); }
        else R((int)cx - 1, my, 2, 1, C(0xA84A4A));
        if (b == &BODIES[CH_LENNY]) for (int x = (int)cx - 3; x <= (int)cx + 3; x += 2) P(x, my + 1, pdark(b->skin, 0.8f));
    } else if (dir == DIR_SIDE) {
        int ex = (int)cx + 3;
        if (mood == 2) { P(ex - 1, ey - 1, eye); P(ex + 1, ey + 1, eye); P(ex, ey, eye); P(ex + 1, ey - 1, eye); P(ex - 1, ey + 1, eye); }
        else if (blink) { P(ex, ey, eye); P(ex - 1, ey, eye); }
        else R(ex, ey - 1, 1, 2, eye);
        P((int)(cx + b->head_rx), ey + 1, b->skin);                     /* nose */
        P((int)(cx + b->head_rx), ey + 2, shade(b->skin));
        if (b->glasses) { R(ex - 1, ey - 2, 3, 1, C(0x3A3A48)); R(ex - 1, ey + 1, 3, 1, C(0x3A3A48)); P(ex + 1, ey - 1, C(0xD8F0FF)); R((int)cx - 1, ey - 1, ex - (int)cx, 1, C(0x3A3A48)); }
        if (b->mustache) R(ex, ey + 2, 3, 1, C(0x1A1418));
        else if (mood == 3) R(ex, ey + 2, 2, 2, C(0x5A1A1A));
        else R(ex + 1, ey + 3, 2, 1, b->braces ? C(0xFFFFFF) : C(0xA84A4A));
        if (b->cheeks) P(ex - 1, ey + 2, C(0xE88A80));
        if (b->freckles) { P(ex - 1, ey + 2, C(0xC8784A)); P(ex + 1, ey + 2, C(0xC8784A)); }
        P((int)cx - 1, ey, shade(b->skin));                               /* ear */
        P((int)cx - 1, ey + 1, shade(b->skin));
    }
}

static void paint(const Body *b, int dir, int fr)
{
    pix_clear(b->w, b->h);
    int FY = b->h - 3;                                         /* shoe bottom row */
    int bob = 0, lift_l = 0, lift_r = 0, swing = 0, blink = 0, mood = 0;
    int whip = -1, cheer = -1, hurt = 0, dizzy = 0;
    switch (fr) {
    case FR_IDLE2: bob = 1; blink = 1; break;
    case FR_WALK0: lift_l = 1; swing = 1; break;
    case FR_WALK1: bob = -1; break;
    case FR_WALK2: lift_r = 1; swing = -1; break;
    case FR_WALK3: bob = -1; break;
    case FR_WHIP0: whip = 0; break;
    case FR_WHIP1: whip = 1; mood = 0; break;
    case FR_WHIP2: whip = 2; break;
    case FR_HURT: hurt = 1; mood = 3; break;
    case FR_CHEER0: cheer = 0; mood = 1; break;
    case FR_CHEER1: cheer = 1; mood = 1; bob = -1; break;
    case FR_DIZZY: dizzy = 1; mood = 2; bob = 1; break;
    }
    int tw = b->torso_w, th = b->torso_h, lh = b->leg_h;
    int cxi = b->w / 2;
    int legtop = FY - 1 - lh;
    int ttop = legtop - th + 1 + bob;
    float hcx = (float)cxi, hcy = (float)ttop - b->head_ry + 1.5f;
    if (hurt) hcy -= 0.0f;
    if (dizzy) hcx += 0.0f;
    int tx = cxi - tw / 2;
    int legw = tw >= 12 ? 3 : 2;

    if (dir == DIR_SIDE) {
        int sw = tw - 3;                                       /* narrower in profile */
        int stx = cxi - sw / 2;
        /* legs: stride */
        int st = (fr == FR_WALK0) ? 2 : (fr == FR_WALK2 ? -2 : 0);
        if (whip >= 0) st = 1;
        draw_leg(cxi - 1 - st, legtop, lh + 1, legw, b, st != 0 ? 1 : 0);     /* back leg */
        { uint32_t keep = b->pants; (void)keep; }
        draw_leg(cxi - 1 + st, legtop, lh + 1, legw, b, 0);                    /* front leg */
        /* back arm */
        int asw = (fr == FR_WALK0) ? -2 : (fr == FR_WALK2 ? 2 : 0);
        if (whip < 0 && cheer < 0) draw_arm(cxi - 1 - asw, ttop + 1, th - 1, b, 1);
        if (b->backpack) { R(stx - 3, ttop + 1, 4, th - 2, b->pack); R(stx - 3, ttop + 1, 1, th - 2, shade(b->pack)); R(stx - 2, ttop, 3, 1, lite(b->pack)); }
        draw_torso(stx, ttop, sw, th, b, DIR_SIDE);
        if (b->coat) R(stx, ttop + th - 1, sw + 1, 3, b->shirt);
        /* head */
        E(hcx, hcy, b->head_rx, b->head_ry, b->skeleton ? b->skin : b->skin);
        E(hcx + 1.0f, hcy + 1.5f, b->head_rx - 1.5f, b->head_ry - 2.0f, lite(b->skin));
        draw_hair_front(b, hcx, hcy, DIR_SIDE);
        draw_face(b, hcx, hcy, DIR_SIDE, blink, dizzy ? 2 : (mood == 3 ? 3 : mood));
        /* front arm */
        if (whip == 0) { R(cxi - 3, ttop - 5, 2, 7, b->shirt); R(cxi - 4, ttop - 7, 2, 2, b->skin); }
        else if (whip == 1) { R(cxi, ttop + 2, 7, 2, b->shirt); R(cxi + 7, ttop + 2, 2, 2, b->skin); }
        else if (whip == 2) { R(cxi + 1, ttop + 3, 5, 2, b->shirt); R(cxi + 5, ttop + 4, 2, 3, b->skin); }
        else if (cheer >= 0) { R(cxi, ttop - 5 + cheer, 2, 7, b->shirt); R(cxi, ttop - 7 + cheer, 2, 2, b->skin); }
        else if (hurt) { R(cxi - 4, ttop + 1, 5, 2, b->shirt); R(cxi - 6, ttop, 2, 2, b->skin); }
        else draw_arm(cxi + asw, ttop + 1, th - 1, b, 0);
    } else {
        /* legs */
        int lx = tx + (tw - legw * 2) / 2 - 1, rx = tx + tw - (tw - legw * 2) / 2 - legw + 1;
        draw_leg(lx, legtop, lh + 1, legw, b, lift_l);
        draw_leg(rx, legtop, lh + 1, legw, b, lift_r);
        if (dir == DIR_UP && b->backpack) { }
        draw_torso(tx, ttop, tw, th, b, dir);
        if (b->coat) { R(tx, ttop + th - 1, tw, 3, b->shirt); R(tx + tw / 2 - (dir == DIR_DOWN), ttop + th - 1, 1 + (dir == DIR_DOWN), 3, shade(b->shirt)); }
        if (dir == DIR_UP && b->backpack) {
            R(tx + 1, ttop + 1, tw - 2, th - 1, b->pack);
            R(tx + 1, ttop + 1, tw - 2, 1, lite(b->pack));
            R(tx + 2, ttop + 3, tw - 4, 1, shade(b->pack));
            R(tx + tw / 2 - 1, ttop + 4, 2, 2, C(0xC8A848));
            if (b == &BODIES[CH_SAM]) { R(tx + tw - 3, ttop - 1, 2, 3, C(0xE8D8A0)); }   /* the rolled-up map sticking out */
        }
        /* arms */
        int ay = ttop + 1, al = th;
        int la_y = ay + (swing > 0 ? -1 : (swing < 0 ? 1 : 0)), ra_y = ay + (swing < 0 ? -1 : (swing > 0 ? 1 : 0));
        int lax = tx - 2, rax = tx + tw;
        if (cheer >= 0) {
            R(lax, ttop - 6 + cheer, 2, 7, b->shirt); R(lax, ttop - 8 + cheer, 2, 2, b->skin);
            R(rax, ttop - 6 + cheer, 2, 7, b->shirt); R(rax, ttop - 8 + cheer, 2, 2, b->skin);
        } else if (hurt) {
            R(lax - 1, ay - 2, 2, 5, b->shirt); R(lax - 1, ay - 4, 2, 2, b->skin);
            R(rax + 1, ay - 2, 2, 5, b->shirt); R(rax + 1, ay - 4, 2, 2, b->skin);
        } else if (whip >= 0) {
            draw_arm(rax, ay, al, b, 0);
            /* whip arm: the character's right hand = screen left when facing us */
            int wx = dir == DIR_DOWN ? lax : rax;
            if (dir == DIR_UP) draw_arm(lax, ay, al, b, 0);
            if (whip == 0) { R(wx, ttop - 6, 2, 7, b->shirt); R(wx, ttop - 8, 2, 2, b->skin); }
            else if (whip == 1) { R(wx + (dir == DIR_DOWN ? 1 : -1), ay + 1, 2, 4, b->shirt); R(wx + (dir == DIR_DOWN ? 2 : -2), ay + 5, 2, 3, b->skin); }
            else { R(wx, ay + 1, 2, al - 1, b->shirt); R(wx + (dir == DIR_DOWN ? 1 : -1), ay + al - 1, 2, 2, b->skin); }
        } else if (dizzy) {
            draw_arm(lax, ay + 1, al, b, 0); draw_arm(rax, ay + 1, al, b, 0);
        } else {
            draw_arm(lax, la_y, al, b, 0);
            draw_arm(rax, ra_y, al, b, 0);
        }
        if (dir == DIR_UP && b->backpack) {                     /* straps over the shoulders */
            P(tx + 1, ttop, shade(b->pack)); P(tx + tw - 2, ttop, shade(b->pack));
        }
        /* head */
        E(hcx, hcy, b->head_rx, b->head_ry, b->skin);
        if (dir == DIR_DOWN) E(hcx - 1.0f, hcy + 1.3f, b->head_rx - 2.0f, b->head_ry - 2.0f, lite(b->skin));
        if (dir == DIR_UP) E(hcx, hcy + 1.0f, b->head_rx - 0.5f, b->head_ry - 1.0f, b->skin);
        draw_hair_front(b, hcx, hcy, dir);
        if (dir == DIR_DOWN) draw_face(b, hcx, hcy, DIR_DOWN, blink, dizzy ? 2 : mood);
        if (dir == DIR_UP && !b->skeleton) {                     /* ears */
            P((int)(hcx - b->head_rx), (int)hcy + 1, shade(b->skin)); P((int)(hcx + b->head_rx), (int)hcy + 1, shade(b->skin));
        }
    }
    pix_outline(b->outline);
}

/* ---- portraits (32x32) ---- */
static void portrait(int who, int mood)
{
    pix_clear(32, 32);
    if (who == FACE_SKULL) {
        E(16, 14, 11, 11, C(0xE8DCC0));
        E(16, 22, 7, 5, C(0xE8DCC0));
        E(11, 14, 3.5f, 4, C(0x1A1010)); E(21, 14, 3.5f, 4, C(0x1A1010));
        P(11, 14, C(0xFF7030)); P(21, 14, C(0xFF7030));
        R(15, 19, 2, 3, C(0x2A2020));
        for (int x = 11; x < 22; x += 2) R(x, 24, 1, 3, C(0x3A3028));
        R(10, 3, 12, 4, C(0x1A1A1A)); R(4, 6, 24, 2, C(0x1A1A1A));          /* tricorn */
        R(14, 4, 4, 2, C(0xE8DCC0));
        /* the iron jaw */
        R(10, 26, 12, 3, C(0x7A808C)); R(10, 26, 12, 1, C(0xB0B8C4)); P(11, 27, C(0x3A3A44)); P(20, 27, C(0x3A3A44));
        pix_outline(C(0x140C0C));
        pix_commit(&art_face[who][mood], 0, 0);
        return;
    }
    const Body *b = &BODIES[who];
    float cx = 16, cy = 17;
    float rx = 10.5f, ry = 10.0f;
    if (who == CH_TINY) { rx = 12; ry = 11; }
    if (who >= CH_VIC && who != CH_TINY) { rx = 9.5f; ry = 10.5f; }
    /* shoulders */
    R(4, 28, 24, 4, b->shirt); R(4, 28, 24, 1, lite(b->shirt));
    if (who == CH_GUS || b->coat) R(15, 28, 2, 4, b->shirt2);
    if (who == CH_TINY) { R(8, 28, 3, 4, b->shirt2); R(21, 28, 3, 4, b->shirt2); }
    if (b->stripes) R(4, 30, 24, 1, b->shirt2);
    E(cx, cy, rx, ry, b->skin);
    E(cx - 2, cy + 2, rx - 4, ry - 4, lite(b->skin));
    E(cx - rx + 0.5f, cy + 1, 2, 3, shade(b->skin)); E(cx + rx - 0.5f, cy + 1, 2, 3, shade(b->skin));   /* ears */
    /* hair */
    uint32_t h = b->hair, hs = shade(h), hl = lite(h);
    switch (b->hair_style) {
    case HAIR_MESSY:
        E(cx, cy - 5, rx + 1, ry - 4, h);
        for (int i = -4; i <= 4; i++) E(cx + (float)i * 2.4f, cy - 13 + (float)((i + 8) % 3), 2.2f, 3.2f, h);
        for (int x = 6; x < 27; x++) { int y0 = 13 + ((x * 5) % 4 == 0 ? 2 : 0) + ((x * 3) % 5 == 1); for (int y = 6; y < y0; y++) P(x, y, ((x + y) % 5 == 0) ? hl : h); }
        R(5, 12, 3, 7, h); R(24, 12, 3, 7, h);
        break;
    case HAIR_CAP:
        R(5, 13, 3, 5, h); R(24, 13, 3, 5, h);
        E(cx, cy - 7, rx + 1, ry - 3, b->shirt2);
        R(5, 11, 22, 2, shade(b->shirt2));
        E(cx, 6, 3, 2, lite(b->shirt2)); P(16, 4, C(0xF0F0F0));
        R(13, 7, 6, 3, C(0xF0F0F0)); R(14, 8, 4, 1, b->shirt2);
        break;
    case HAIR_CURLY:
        for (int i = 0; i < 12; i++) { float a = 3.14159f * (0.9f + (float)i / 11.0f * 1.2f); E(cx + cosf(a) * rx, cy - 3 + sinf(a) * ry, 3.4f, 3.4f, (i & 1) ? h : hs); }
        E(cx, cy - 7, rx, ry - 4, h);
        for (int i = 0; i < 8; i++) P(9 + i * 2, 6 + (i & 1), hl);
        break;
    case HAIR_MOP:
        E(cx, cy - 5, rx + 1.5f, ry - 2, h);
        for (int x = 5; x < 28; x++) { int y0 = 14 + (x & 1); for (int y = 5; y < y0; y++) P(x, y, h); }
        R(4, 13, 3, 9, h); R(26, 13, 3, 9, h);
        for (int i = 0; i < 6; i++) P(8 + i * 3, 7, hl);
        break;
    case HAIR_SLICK:
        E(cx, cy - 6, rx + 0.5f, ry - 4, h);
        for (int i = 0; i < 5; i++) L(9 + i * 3, 9, 11 + i * 3, 6, C(0x5A5A6A));
        break;
    case HAIR_BEANIE:
        E(cx, cy - 6, rx + 1, ry - 3, h);
        R(5, 11, 22, 3, hs);
        for (int x = 7; x < 26; x += 3) R(x, 5, 1, 6, hs);
        E(cx, 3, 2.5f, 2, hl);
        break;
    case HAIR_BUN:
        E(cx, cy - 6, rx + 1, ry - 4, h);
        E(cx, 4, 5, 3.5f, h); P(14, 3, hl); P(17, 4, hl);
        R(5, 11, 3, 6, h); R(24, 11, 3, 6, h);
        break;
    case HAIR_BALD:
        P(15, 6, h); P(16, 5, h); P(17, 6, h); P(16, 6, h);
        E(cx - 3, 10, 3, 1.5f, lite(lite(b->skin)));
        break;
    }
    /* eyes */
    int ey = 18, ex0 = 11, ex1 = 21;
    for (int k = 0; k < 2; k++) {
        int x = k ? ex1 : ex0;
        if (mood == MOOD_HAPPY) { P(x - 1, ey, C(0x1A1420)); P(x, ey - 1, C(0x1A1420)); P(x + 1, ey - 1, C(0x1A1420)); P(x + 2, ey, C(0x1A1420)); }
        else {
            int big = mood == MOOD_SHOCK;
            R(x - 1 - big, ey - 2 - big, 3 + big * 2, 3 + big * 2, C(0xFFFFFF));
            R(x, ey - 1, 2, 2 + big, C(0x1A1420));
            if (!big) P(x, ey - 1, C(0x3A3050));
            P(x + 1, ey - 1, C(0xFFFFFF));
        }
        /* brows */
        uint32_t bc = b->hair_style == HAIR_CAP ? C(0x2A1E1A) : (b->hair_style == HAIR_BALD ? C(0x6A4A30) : hs);
        if (mood == MOOD_SHOCK) R(x - 1, ey - 6, 4, 1, bc);
        else if (who == CH_VIC || who == CH_MA) { P(x - 1 + k * 3, ey - 4, bc); R(x, ey - 4 + 0, 2, 1, bc); P(x + 2 - k * 3, ey - 3, bc); }
        else R(x - 1, ey - 4, 4, 1, bc);
    }
    if (b->glasses) {
        uint32_t fr = who == CH_MA ? C(0x8A6A3A) : C(0x2A2A38);
        for (int k = 0; k < 2; k++) {
            int x = k ? ex1 : ex0;
            R(x - 3, ey - 4, 7, 1, fr); R(x - 3, ey + 2, 7, 1, fr); R(x - 3, ey - 4, 1, 7, fr); R(x + 3, ey - 4, 1, 7, fr);
            P(x + 2, ey - 3, CA(0xD8F0FF, 200)); P(x + 1, ey - 3, CA(0xD8F0FF, 140));
        }
        R(ex0 + 4, ey - 2, ex1 - ex0 - 7, 1, fr);
    }
    if (b->freckles) for (int i = 0; i < 6; i++) P(8 + (i % 3) * 2 + (i / 3) * 14, 22 + (i & 1), C(0xC8784A));
    if (b->cheeks || mood == MOOD_HAPPY) { R(7, 22, 3, 1, CA(0xE87A70, 180)); R(23, 22, 3, 1, CA(0xE87A70, 180)); }
    P(16, 21, shade(b->skin)); P(17, 22, shade(b->skin));                 /* nose */
    /* mouth */
    if (mood == MOOD_SHOCK) { E(16.5f, 25.5f, 2, 2.5f, C(0x5A1A1A)); }
    else if (mood == MOOD_HAPPY) {
        R(12, 24, 9, 1, C(0x5A1A1A)); R(13, 25, 7, 2, C(0x8A2A2A)); R(14, 27, 5, 1, C(0x5A1A1A));
        R(13, 24, 7, 1, C(0xFFFFFF));
        if (b->braces) for (int x = 13; x < 20; x += 2) P(x, 24, C(0x9AA0B0));
    } else {
        L(13, 25, 15, 26, C(0x6A2A2A)); L(15, 26, 19, 26, C(0x6A2A2A)); P(20, 25, C(0x6A2A2A));
        if (b->braces) { P(16, 25, C(0xFFFFFF)); P(17, 25, C(0x9AA0B0)); }
    }
    if (b->mustache) { R(11, 23, 11, 2, C(0x1A1418)); P(10, 24, C(0x1A1418)); P(22, 24, C(0x1A1418)); }
    if (who == CH_LENNY) for (int x = 9; x < 25; x += 2) { P(x, 27, pdark(b->skin, 0.8f)); P(x + 1, 28, pdark(b->skin, 0.8f)); }
    pix_outline(C(0x140C10));
    pix_commit(&art_face[who][mood], 0, 0);
}

/* ---- creatures ---- */
static void bat(int f)
{
    pix_clear(18, 14);
    uint32_t body = C(0x3A2A44), wing = C(0x2A1E34), wl = C(0x4A3A58);
    float wy[4] = { -4, -1, 3, -1 };
    float y = 7;
    for (int s = -1; s <= 1; s += 2) {
        float tipx = 9 + (float)s * 8.0f, tipy = y + wy[f];
        for (int i = 0; i <= 6; i++) {
            float t = (float)i / 6.0f;
            Ld(9.0f + (float)s * 1.5f, y, tipx * t + (9.0f + (float)s * 1.5f) * (1 - t), y + (tipy - y) * t + (float)(i & 1), 1.6f, wing);
        }
        Ld(9.0f + (float)s * 1.5f, y - 1, tipx, tipy - 1, 1.0f, wl);
    }
    E(9, y, 2.6f, 3.0f, body);
    P(8, (int)y - 3, body); P(10, (int)y - 3, body);
    P(8, (int)y - 1, C(0xFF4040)); P(10, (int)y - 1, C(0xFF4040));
    P(9, (int)y + 1, C(0xF0F0F0));
    pix_outline(C(0x0E0814));
    pix_commit(&art_crit[CR_BAT][0][f], 9, 13);
}
static void crab(int f)
{
    pix_clear(20, 14);
    uint32_t sh = C(0xC8482A), sd = C(0x8A2A1A), sl = C(0xF07850);
    for (int k = 0; k < 3; k++) {                    /* legs */
        int lo = ((f + k) & 1);
        L(5, 8 + k, 2, 10 + k + lo, sd); L(14, 8 + k, 17, 10 + k + (1 - lo), sd);
    }
    E(10, 8, 6, 4, sh);
    E(9, 7, 4, 2, sl);
    int open = f == 1;
    E(3, 4, 2.4f, 2.0f, sh); E(17, 4, 2.4f, 2.0f, sh);
    if (open) { P(2, 2, 0); P(18, 2, 0); P(3, 2, 0); P(17, 2, 0); }
    L(5, 7, 3, 5, sd); L(15, 7, 17, 5, sd);
    R(8, 3, 1, 3, sd); R(12, 3, 1, 3, sd);
    P(8, 3, C(0x101010)); P(12, 3, C(0x101010)); P(8, 2, C(0xFFFFFF)); P(12, 2, C(0xFFFFFF));
    pix_outline(C(0x2A0E08));
    pix_commit(&art_crit[CR_CRAB][0][f], 10, 13);
}
static void rat(int f)
{
    pix_clear(18, 10);
    uint32_t fur = C(0x6A5E58), fd = C(0x4A403C);
    E(8, 6, 5, 3, fur);
    E(13, 5.5f, 3, 2.4f, fur);
    P(15, 4, C(0x101010)); P(16, 6, C(0xE8A0A0)); P(12, 3, C(0xC89090)); P(11, 3, fur);
    L(3, 6, 0, 5 - (f & 1), C(0xC89090)); L(0, 5 - (f & 1), 0, 3, C(0xC89090));
    R(5 + f, 8, 1, 2, fd); R(10 - f, 8, 1, 2, fd);
    E(7, 5, 3, 1.2f, C(0x7E7068));
    pix_outline(C(0x1A1414));
    pix_commit(&art_crit[CR_RAT][0][f], 9, 9);
}

void art_chars(void)
{
    init_bodies();
    for (int c = 0; c < CH_COUNT; c++) {
        const Body *b = &BODIES[c];
        for (int d = 0; d < 3; d++)
            for (int f = 0; f < FR_COUNT; f++) {
                paint(b, d, f);
                pix_commit(&art_char[c][d][f], b->w / 2, b->h - 2);
            }
        for (int m = 0; m < MOOD_COUNT; m++) portrait(c, m);
    }
    for (int m = 0; m < MOOD_COUNT; m++) portrait(FACE_SKULL, m);
    for (int f = 0; f < 4; f++) bat(f);
    for (int f = 0; f < 3; f++) crab(f);
    for (int f = 0; f < 2; f++) rat(f);
    /* skeleton: idle, walk x4, attack */
    static const int sf[6] = { FR_IDLE, FR_WALK0, FR_WALK1, FR_WALK2, FR_WALK3, FR_WHIP1 };
    for (int d = 0; d < 3; d++)
        for (int i = 0; i < 6; i++) {
            paint(&SKEL, d, sf[i]);
            pix_commit(&art_crit[CR_SKEL][d][i], SKEL.w / 2, SKEL.h - 2);
        }
}
