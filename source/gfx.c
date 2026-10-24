/*
 * TREASURE HOUNDS - triangle list builder, primitives and text. See gfx.h.
 */
#include "gfx.h"
#include "art.h"

#include <math.h>
#include <string.h>

#define TAU 6.28318530718f

GVtx     gfx_v[GFX_MAX_VTX] __attribute__((aligned(32)));
GSeg     gfx_seg[GFX_MAX_SEG];
int      gfx_nv, gfx_nseg;
uint32_t gfx_atlas[ATLAS_W * ATLAS_H] __attribute__((aligned(32)));

static int   cur_mode = -1;
static float xs = 1.0f, xtx = 0.0f, xty = 0.0f;

uint32_t col_mix(uint32_t a, uint32_t b, float t)
{
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    uint32_t r = 0;
    for (int s = 0; s < 32; s += 8) {
        float x = (float)((a >> s) & 255), y = (float)((b >> s) & 255);
        r |= (uint32_t)(int)(x + (y - x) * t + 0.5f) << s;
    }
    return r;
}

uint32_t col_scale(uint32_t c, float k)
{
    return RGBA((int)((float)(c >> 24) * k), (int)((float)((c >> 16) & 255) * k),
                (int)((float)((c >> 8) & 255) * k), (int)(c & 255));
}

void gfx_begin(void)
{
    gfx_nv = 0;
    gfx_nseg = 0;
    cur_mode = -1;
    gfx_xf_reset();
    gfx_blend(BL_ALPHA);
}

void gfx_blend(int mode)
{
    if (mode == cur_mode) return;
    if (gfx_nseg > 0 && gfx_seg[gfx_nseg - 1].count == 0) {
        gfx_seg[gfx_nseg - 1].mode = mode;
        cur_mode = mode;
        return;
    }
    if (gfx_nseg >= GFX_MAX_SEG) return;
    gfx_seg[gfx_nseg].mode = mode;
    gfx_seg[gfx_nseg].start = gfx_nv;
    gfx_seg[gfx_nseg].count = 0;
    gfx_nseg++;
    cur_mode = mode;
}

void gfx_xf(float s, float tx, float ty) { xs = s; xtx = tx; xty = ty; }
void gfx_xf_reset(void) { xs = 1.0f; xtx = 0.0f; xty = 0.0f; }
void gfx_xf_get(float *s, float *tx, float *ty) { *s = xs; *tx = xtx; *ty = xty; }

static inline int room(int n)
{
    if (gfx_nv + n > GFX_MAX_VTX || gfx_nseg == 0) return 0;
    return 1;
}
static inline void vtx(float x, float y, float u, float v, uint32_t c)
{
    GVtx *p = &gfx_v[gfx_nv++];
    p->x = x * xs + xtx; p->y = y * xs + xty; p->u = u; p->v = v; p->c = c;
}
static inline void done(int n) { gfx_seg[gfx_nseg - 1].count += n; }

#define IU (1.0f / (float)ATLAS_W)
#define IV (1.0f / (float)ATLAS_H)

void gfx_tri_uv(float x0, float y0, float u0, float v0, uint32_t c0,
                float x1, float y1, float u1, float v1, uint32_t c1,
                float x2, float y2, float u2, float v2, uint32_t c2)
{
    if (!room(3)) return;
    vtx(x0, y0, u0 * IU, v0 * IV, c0);
    vtx(x1, y1, u1 * IU, v1 * IV, c1);
    vtx(x2, y2, u2 * IU, v2 * IV, c2);
    done(3);
}

void gfx_quad_uv(const float *p, float sx, float sy, float sw, float sh, const uint32_t *c, int flip)
{
    if (!room(6)) return;
    float u0 = sx * IU, v0 = sy * IV, u1 = (sx + sw) * IU, v1 = (sy + sh) * IV;
    /* a hair inside the texel edges so nearest sampling never picks up a neighbour */
    u0 += 0.02f * IU; u1 -= 0.02f * IU; v0 += 0.02f * IV; v1 -= 0.02f * IV;
    if (flip) { float t = u0; u0 = u1; u1 = t; }
    vtx(p[0], p[1], u0, v0, c[0]);
    vtx(p[2], p[3], u1, v0, c[1]);
    vtx(p[4], p[5], u1, v1, c[2]);
    vtx(p[0], p[1], u0, v0, c[0]);
    vtx(p[4], p[5], u1, v1, c[2]);
    vtx(p[6], p[7], u0, v1, c[3]);
    done(6);
}

void gfx_img(float x, float y, float w, float h, float sx, float sy, float sw, float sh, uint32_t c, int flip)
{
    float p[8] = { x, y, x + w, y, x + w, y + h, x, y + h };
    uint32_t c4[4] = { c, c, c, c };
    gfx_quad_uv(p, sx, sy, sw, sh, c4, flip);
}

/* ------------------------------------------------------------------ */
/* solid shapes                                                        */
/* ------------------------------------------------------------------ */
void gfx_tri(float x0, float y0, uint32_t c0, float x1, float y1, uint32_t c1, float x2, float y2, uint32_t c2)
{
    if (!room(3)) return;
    vtx(x0, y0, WHITE_U, WHITE_V, c0);
    vtx(x1, y1, WHITE_U, WHITE_V, c1);
    vtx(x2, y2, WHITE_U, WHITE_V, c2);
    done(3);
}
void gfx_tri1(float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c)
{
    gfx_tri(x0, y0, c, x1, y1, c, x2, y2, c);
}
void gfx_quad4(float x0, float y0, uint32_t c0, float x1, float y1, uint32_t c1,
               float x2, float y2, uint32_t c2, float x3, float y3, uint32_t c3)
{
    gfx_tri(x0, y0, c0, x1, y1, c1, x2, y2, c2);
    gfx_tri(x0, y0, c0, x2, y2, c2, x3, y3, c3);
}
void gfx_rect(float x, float y, float w, float h, uint32_t c)
{
    gfx_quad4(x, y, c, x + w, y, c, x + w, y + h, c, x, y + h, c);
}
void gfx_rect_v(float x, float y, float w, float h, uint32_t top, uint32_t bot)
{
    gfx_quad4(x, y, top, x + w, y, top, x + w, y + h, bot, x, y + h, bot);
}
void gfx_rect_h(float x, float y, float w, float h, uint32_t l, uint32_t r)
{
    gfx_quad4(x, y, l, x + w, y, r, x + w, y + h, r, x, y + h, l);
}
void gfx_frame(float x, float y, float w, float h, float t, uint32_t c)
{
    gfx_rect(x, y, w, t, c);
    gfx_rect(x, y + h - t, w, t, c);
    gfx_rect(x, y + t, t, h - 2 * t, c);
    gfx_rect(x + w - t, y + t, t, h - 2 * t, c);
}
void gfx_poly(float cx, float cy, float r, int n, float rot, uint32_t c)
{
    for (int i = 0; i < n; i++) {
        float a0 = rot + TAU * (float)i / (float)n, a1 = rot + TAU * (float)(i + 1) / (float)n;
        gfx_tri1(cx, cy, cx + cosf(a0) * r, cy + sinf(a0) * r, cx + cosf(a1) * r, cy + sinf(a1) * r, c);
    }
}
void gfx_ellipse(float cx, float cy, float rx, float ry, int n, uint32_t cin, uint32_t cout)
{
    float px = cx + rx, py = cy;
    for (int i = 1; i <= n; i++) {
        float a = TAU * (float)i / (float)n;
        float x = cx + cosf(a) * rx, y = cy + sinf(a) * ry;
        gfx_tri(cx, cy, cin, px, py, cout, x, y, cout);
        px = x; py = y;
    }
}
void gfx_ring(float cx, float cy, float r, float th, int n, float rot, uint32_t c)
{
    for (int i = 0; i < n; i++) {
        float a0 = rot + TAU * (float)i / (float)n, a1 = rot + TAU * (float)(i + 1) / (float)n;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        float ri = r - th;
        gfx_quad4(cx + c0 * ri, cy + s0 * ri, c, cx + c0 * r, cy + s0 * r, c,
                  cx + c1 * r, cy + s1 * r, c, cx + c1 * ri, cy + s1 * ri, c);
    }
}
void gfx_glow(float cx, float cy, float r, uint32_t c)
{
    int n = r * xs > 60 ? 20 : (r * xs > 20 ? 14 : 9);
    gfx_ellipse(cx, cy, r, r, n, c, c & 0xFFFFFF00u);
}
void gfx_glow_e(float cx, float cy, float rx, float ry, uint32_t c)
{
    gfx_ellipse(cx, cy, rx, ry, 16, c, c & 0xFFFFFF00u);
}
void gfx_line(float x0, float y0, float x1, float y1, float w, uint32_t c0, uint32_t c1)
{
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    if (l < 1e-4f) return;
    float nx = -dy / l * w * 0.5f, ny = dx / l * w * 0.5f;
    gfx_quad4(x0 + nx, y0 + ny, c0, x1 + nx, y1 + ny, c1, x1 - nx, y1 - ny, c1, x0 - nx, y0 - ny, c0);
}
void gfx_glowline(float x0, float y0, float x1, float y1, float w, uint32_t c)
{
    float dx = x1 - x0, dy = y1 - y0, l = sqrtf(dx * dx + dy * dy);
    if (l < 1e-4f) return;
    float nx = -dy / l * w * 0.5f, ny = dx / l * w * 0.5f;
    uint32_t z = c & 0xFFFFFF00u;
    gfx_quad4(x0 + nx, y0 + ny, z, x1 + nx, y1 + ny, z, x1, y1, c, x0, y0, c);
    gfx_quad4(x0, y0, c, x1, y1, c, x1 - nx, y1 - ny, z, x0 - nx, y0 - ny, z);
}

/* ------------------------------------------------------------------ */
/* text                                                                */
/* ------------------------------------------------------------------ */
#define HILITE 0xFFD25AFFu

static float glyph_adv(unsigned char ch)
{
    if (ch == ' ') return 3.0f;
    if (ch >= 1 && ch <= 8) return 10.0f;
    if (ch >= 128 || !art_font[ch].w) return 0.0f;
    return (float)art_font[ch].w + 1.0f;
}

float gfx_text_w(const char *s, float px)
{
    float w = 0, best = 0;
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == '\n') { if (w > best) best = w; w = 0; continue; }
        if (ch == '{' || ch == '}') continue;
        w += glyph_adv(ch);
    }
    if (w > best) best = w;
    return best > 0 ? (best - 1.0f) * px : 0.0f;
}

static void draw_text(const char *s, float x, float y, float px, uint32_t c, int limit, float wave_t, float amp)
{
    float cx = x, cy = y;
    int hi = 0, count = 0;
    uint32_t a = c & 255;
    for (; *s; s++) {
        unsigned char ch = (unsigned char)*s;
        if (ch == '\n') { cx = x; cy += 12.0f * px; continue; }
        if (ch == '{') { hi = 1; continue; }
        if (ch == '}') { hi = 0; continue; }
        if (limit >= 0 && count >= limit) break;
        count++;
        float yo = 0;
        if (amp != 0.0f) yo = sinf(wave_t * 5.0f + cx * 0.05f / px) * amp;
        if (ch >= 1 && ch <= 8) {
            const ArtRect *r = &art_icon[ch - 1];
            gfx_img(cx, cy - 1.0f * px + yo, (float)r->w * px, (float)r->h * px, r->x, r->y, r->w, r->h,
                    0xFFFFFF00u | a, 0);
            cx += glyph_adv(ch) * px;
            continue;
        }
        if (ch < 128 && art_font[ch].w) {
            const ArtGlyph *g = &art_font[ch];
            /* highlight: gold on dark backgrounds, deep red on parchment */
            int lum = (int)(c >> 24) + (int)((c >> 16) & 255) + (int)((c >> 8) & 255);
            uint32_t cc = hi ? (((lum < 300 ? 0xB02818FFu : HILITE) & 0xFFFFFF00u) | a) : c;
            gfx_img(cx, cy + yo, (float)g->w * px, 9.0f * px, g->x, g->y, g->w, 9, cc, 0);
        }
        cx += glyph_adv(ch) * px;
    }
}

void gfx_text(const char *s, float x, float y, float px, uint32_t c) { draw_text(s, x, y, px, c, -1, 0, 0); }
void gfx_text_c(const char *s, float cx, float y, float px, uint32_t c)
{
    draw_text(s, floorf(cx - gfx_text_w(s, px) * 0.5f), y, px, c, -1, 0, 0);
}
void gfx_text_r(const char *s, float rx, float y, float px, uint32_t c) { draw_text(s, rx - gfx_text_w(s, px), y, px, c, -1, 0, 0); }
void gfx_text_s(const char *s, float x, float y, float px, uint32_t c)
{
    uint32_t sh = RGBA(8, 6, 14, (int)((float)(c & 255) * 0.8f));
    draw_text(s, x + px, y + px, px, sh, -1, 0, 0);
    draw_text(s, x, y, px, c, -1, 0, 0);
}
void gfx_text_sc(const char *s, float cx, float y, float px, uint32_t c)
{
    gfx_text_s(s, floorf(cx - gfx_text_w(s, px) * 0.5f), y, px, c);
}
void gfx_text_n(const char *s, float x, float y, float px, uint32_t c, int n, float wrapw)
{
    (void)wrapw;
    draw_text(s, x, y, px, c, n, 0, 0);
}
void gfx_text_wave(const char *s, float cx, float y, float px, uint32_t c, float t, float amp)
{
    draw_text(s, floorf(cx - gfx_text_w(s, px) * 0.5f), y, px, c, -1, t, amp);
}

int gfx_wrap(const char *src, char *dst, int dstn, float px, float w)
{
    int n = 0, lines = 1, last_space = -1;
    float lw = 0;
    for (const char *s = src; *s && n < dstn - 1; s++) {
        unsigned char ch = (unsigned char)*s;
        dst[n] = (char)ch;
        if (ch == '\n') { lw = 0; last_space = -1; lines++; n++; continue; }
        if (ch == ' ') last_space = n;
        if (ch != '{' && ch != '}') lw += glyph_adv(ch) * px;
        if (lw > w && last_space >= 0) {
            dst[last_space] = '\n';
            lines++;
            lw = 0;
            for (int k = last_space + 1; k <= n; k++) {
                unsigned char c2 = (unsigned char)dst[k];
                if (c2 != '{' && c2 != '}') lw += glyph_adv(c2) * px;
            }
            last_space = -1;
        }
        n++;
    }
    dst[n] = 0;
    return lines;
}
