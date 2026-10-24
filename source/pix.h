/*
 * TREASURE HOUNDS - a tiny pixel painter used by the art_*.c files.
 *
 * Everything is painted onto a scratch canvas, then copied into the atlas by pix_commit().
 */
#ifndef PIX_H
#define PIX_H

#include <stdint.h>
#include <math.h>
#include "art.h"
#include "gfx.h"

#define CVW 256
#define CVH 256
extern uint32_t cv[CVW * CVH];
extern int cvw, cvh;                        /* size of the current drawing */

/* 0xRRGGBB -> opaque RGBA */
static inline uint32_t C(uint32_t rgb) { return (rgb << 8) | 0xFFu; }
static inline uint32_t CA(uint32_t rgb, int a) { return (rgb << 8) | (uint32_t)(a & 255); }

static inline uint32_t pmix(uint32_t a, uint32_t b, float t) { return col_mix(a, b, t); }
static inline uint32_t pdark(uint32_t c, float k)   /* k < 1 darker, > 1 lighter; hue-shifts darks to blue */
{
    int r = (int)(c >> 24), g = (int)((c >> 16) & 255), b = (int)((c >> 8) & 255);
    if (k < 1.0f) {
        float s = 1.0f - k;
        r = (int)((float)r * k - 6.0f * s); g = (int)((float)g * k - 2.0f * s); b = (int)((float)b * k + 14.0f * s);
    } else {
        float s = k - 1.0f;
        r = (int)((float)r * k + 10.0f * s); g = (int)((float)g * k + 6.0f * s); b = (int)((float)b * k - 4.0f * s);
    }
    return RGBA(r, g, b, (int)(c & 255));
}

static inline uint32_t hash2(int x, int y, uint32_t s)
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}
static inline float hn(int x, int y, uint32_t s) { return (float)(hash2(x, y, s) & 0xFFFF) / 65535.0f; }

/* smooth value noise that wraps every `per` pixels (per = 0: no wrap) */
float pnoise(float x, float y, int cell, int per, uint32_t seed);

void pix_clear(int w, int h);
void P(int x, int y, uint32_t c);                     /* blends if c has alpha < 255 */
uint32_t G(int x, int y);
void R(int x, int y, int w, int h, uint32_t c);
void E(float cx, float cy, float rx, float ry, uint32_t c);    /* filled ellipse */
void L(int x0, int y0, int x1, int y1, uint32_t c);
void Ld(float x0, float y0, float x1, float y1, float w, uint32_t c);   /* thick line */
void pix_outline(uint32_t c);                          /* dark outline round every opaque pixel */
void pix_outline_soft(uint32_t c);                     /* outline only on bottom/side edges */
void pix_shade_rim(uint32_t hi, int dx, int dy);       /* lighten pixels whose (dx,dy) neighbour is empty */
void pix_flip(void);                                   /* mirror horizontally */
void pix_commit(ArtRect *r, int ax, int ay);            /* copy canvas (cvw x cvh) into the atlas */
void pix_commit_at(ArtRect *r, int sx, int sy, int w, int h, int ax, int ay);   /* a sub-rect */
void pix_alloc(int w, int h, int *x, int *y);           /* reserve atlas space */

#endif
