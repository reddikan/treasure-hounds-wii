/*
 * TREASURE HOUNDS - tiny 2D renderer (platform independent).
 *
 * Everything is drawn as triangles into one big vertex list. Every vertex carries a
 * position on the virtual 854x480 (16:9) screen, a texture coordinate into the one
 * 1024x1024 pixel-art atlas, and an RGBA colour that tints the texel. Solid shapes
 * simply point at a block of white texels, so one GPU setup draws everything.
 *
 * The list is split into segments by blend mode:
 *   BL_ALPHA  normal alpha blending
 *   BL_ADD    additive glow
 *   BL_MUL2   "modulate 2x": dst = dst * src * 2. The light map is drawn this way, so a
 *             vertex colour of 128 leaves the picture alone, darker darkens and brighter
 *             over-brightens (warm lantern light can push colours past their painted value)
 *
 * On the Wii, main.c sends the list to GX each frame; the native test harness
 * rasterizes the very same list in software for screenshots.
 */
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

#define VW 854.0f
#define VH 480.0f

#define ATLAS_W 1024
#define ATLAS_H 1024
/* the 4x4 block of white texels that solid shapes sample */
#define WHITE_U ((1021.0f + 0.5f) / 1024.0f)
#define WHITE_V ((1021.0f + 0.5f) / 1024.0f)

#define GFX_MAX_VTX 120000
#define GFX_MAX_SEG 2048

enum { BL_ALPHA, BL_ADD, BL_MUL2 };

typedef struct { float x, y, u, v; uint32_t c; } GVtx;     /* c = 0xRRGGBBAA */
typedef struct { int mode, start, count; } GSeg;

extern GVtx gfx_v[GFX_MAX_VTX];
extern GSeg gfx_seg[GFX_MAX_SEG];
extern int  gfx_nv, gfx_nseg;

/* the atlas: 0xRRGGBBAA texels, filled by art.c at start-up */
extern uint32_t gfx_atlas[ATLAS_W * ATLAS_H];

static inline uint32_t RGBA(int r, int g, int b, int a)
{
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    if (a < 0) a = 0; if (a > 255) a = 255;
    return ((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a;
}
static inline uint32_t col_a(uint32_t c, float a)            /* scale alpha */
{
    int al = (int)((float)(c & 255) * a);
    if (al < 0) al = 0; if (al > 255) al = 255;
    return (c & 0xFFFFFF00u) | (uint32_t)al;
}
uint32_t col_mix(uint32_t a, uint32_t b, float t);            /* lerp all four channels */
uint32_t col_scale(uint32_t c, float k);                      /* scale rgb, keep alpha */

void gfx_begin(void);
void gfx_blend(int mode);
/* coordinate transform applied to every vertex: screen = p * s + (tx, ty) */
void gfx_xf(float s, float tx, float ty);
void gfx_xf_reset(void);
void gfx_xf_get(float *s, float *tx, float *ty);

/* raw triangle (positions in the current transform, uv in texels) */
void gfx_tri_uv(float x0, float y0, float u0, float v0, uint32_t c0,
                float x1, float y1, float u1, float v1, uint32_t c1,
                float x2, float y2, float u2, float v2, uint32_t c2);
/* textured quad: source rect (atlas texels) onto four corners (tl, tr, br, bl) with a colour per corner */
void gfx_quad_uv(const float *xy /* 8 floats */, float sx, float sy, float sw, float sh, const uint32_t *c4, int flip);
/* textured axis-aligned sprite */
void gfx_img(float x, float y, float w, float h, float sx, float sy, float sw, float sh, uint32_t c, int flip);

/* solid shapes */
void gfx_tri(float x0, float y0, uint32_t c0, float x1, float y1, uint32_t c1, float x2, float y2, uint32_t c2);
void gfx_tri1(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c);
void gfx_quad4(float x0, float y0, uint32_t c0, float x1, float y1, uint32_t c1,
               float x2, float y2, uint32_t c2, float x3, float y3, uint32_t c3);
void gfx_rect(float x, float y, float w, float h, uint32_t c);
void gfx_rect_v(float x, float y, float w, float h, uint32_t top, uint32_t bot);
void gfx_rect_h(float x, float y, float w, float h, uint32_t left, uint32_t right);
void gfx_frame(float x, float y, float w, float h, float t, uint32_t c);
void gfx_poly(float cx, float cy, float r, int n, float rot, uint32_t c);
void gfx_ellipse(float cx, float cy, float rx, float ry, int n, uint32_t cin, uint32_t cout);
void gfx_ring(float cx, float cy, float r, float thick, int n, float rot, uint32_t c);
void gfx_glow(float cx, float cy, float r, uint32_t c);          /* soft blob: c at the centre fading out */
void gfx_glow_e(float cx, float cy, float rx, float ry, uint32_t c);
void gfx_line(float x0, float y0, float x1, float y1, float w, uint32_t c0, uint32_t c1);
void gfx_glowline(float x0, float y0, float x1, float y1, float w, uint32_t c);

/* pixel font from the atlas. px = size of one font pixel. Codes:
 *   '{' .. '}' highlight in gold;  '\n' new line;  \x01..\x08 button / coin icons */
float gfx_text_w(const char *s, float px);
void  gfx_text(const char *s, float x, float y, float px, uint32_t c);
void  gfx_text_c(const char *s, float cx, float y, float px, uint32_t c);
void  gfx_text_r(const char *s, float rx, float y, float px, uint32_t c);
void  gfx_text_s(const char *s, float x, float y, float px, uint32_t c);   /* with a dark shadow */
void  gfx_text_sc(const char *s, float cx, float y, float px, uint32_t c);
/* draws only the first n visible characters (typewriter effect) */
void  gfx_text_n(const char *s, float x, float y, float px, uint32_t c, int n, float wrapw);
/* word wrap: inserts '\n' into dst so no line is wider than w; returns the number of lines */
int   gfx_wrap(const char *src, char *dst, int dstn, float px, float w);
void  gfx_text_wave(const char *s, float cx, float y, float px, uint32_t c, float t, float amp);

#define ICON_A    "\x01"
#define ICON_B    "\x02"
#define ICON_X    "\x03"
#define ICON_Y    "\x04"
#define ICON_L    "\x05"
#define ICON_R    "\x06"
#define ICON_PLUS "\x07"
#define ICON_COIN "\x08"

#endif
