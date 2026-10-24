/*
 * TREASURE HOUNDS - the soundtrack. See music.h.
 *
 * Every piece is written below as note strings (note+octave:beats, '+' for chords, 'r' rests,
 * '|' bar lines are ignored) and rendered at start-up by a small offline orchestra: bowed
 * strings, a fiddle, a musette accordion, brass, a breathy whistle, pizzicato and harp
 * (Karplus-Strong), bone marimba and glockenspiel (FM), choir, timpani and drums. Notes that
 * run past the end of the loop wrap round to the start, and the reverb is run round the loop
 * twice, so every track loops seamlessly.
 */
#include "music.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define SR    ((float)MUS_RATE)
#define TAU_F 6.28318531f

static int16_t *trk[MUS_TRACKS];
static int trk_len[MUS_TRACKS];
static float *mix;
static int N;                    /* loop length in samples */
static float BEAT;               /* seconds per beat */
static uint32_t rng = 0xC0FFEEu;
static float sintab[4097];

static const uint8_t VOL[MUS_TRACKS] = { 200, 140, 155, 150, 195, 155 };

static float frnd(void) { rng = rng * 1664525u + 1013904223u; return (float)(rng >> 8) / 8388608.0f - 1.0f; }
static float mtof(float m) { return 440.0f * powf(2.0f, (m - 69.0f) / 12.0f); }
static inline float sn(float ph) { float x = ph * 4096.0f; int i = (int)x; float f = x - (float)i; i &= 4095; return sintab[i] + (sintab[i + 1] - sintab[i]) * f; }
static inline void put(int i, float v) { i %= N; if (i < 0) i += N; mix[i] += v; }
static inline float blep_saw(float ph, float dt)
{
    float v = 2.0f * ph - 1.0f;
    if (ph < dt) { float t = ph / dt; v -= t + t - t * t - 1.0f; }
    else if (ph > 1.0f - dt) { float t = (ph - 1.0f) / dt; v -= t * t + t + t + 1.0f; }
    return v;
}
static int s_of(float b) { return (int)(b * BEAT * SR); }

/* ------------------------------------------------------------------ */
/* instruments: (start beat, length in beats, midi note, velocity)     */
/* ------------------------------------------------------------------ */
static void strings(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.45f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), p[3] = { 0, 0.33f, 0.71f }, det[3] = { 1.0f, 1.0045f, 0.9958f };
    float lp1 = 0, lp2 = 0, k = 1.0f - expf(-TAU_F * fminf(3200.0f, f * 5.0f) / SR);
    float a = 0, atk = 1.0f / (0.22f * SR), rel = expf(-1.0f / (0.16f * SR)), vib = 0;
    for (int i = 0; i < n; i++) {
        if (i < held) { a += atk; if (a > 1) a = 1; } else a *= rel;
        vib += 5.2f / SR; if (vib > 1) vib -= 1;
        float fm = 1.0f + 0.0025f * sn(vib) * fminf(1.0f, (float)i / (0.4f * SR));
        float s = 0;
        for (int o = 0; o < 3; o++) { float dt = f * det[o] * fm / SR; s += blep_saw(p[o], dt); p[o] += dt; if (p[o] >= 1) p[o] -= 1; }
        lp1 += (s - lp1) * k; lp2 += (lp1 - lp2) * k;
        put(s0 + i, lp2 * a * v * 0.075f);
    }
}
static void fiddle(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.12f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), ph = 0, lo = 0, bp = 0, lp = 0, vib = 0;
    float c = 2.0f * sinf(3.14159f * 1500.0f / SR), k = 1.0f - expf(-TAU_F * 4200.0f / SR);
    float a = 0, atk = 1.0f / (0.035f * SR), rel = expf(-1.0f / (0.05f * SR));
    for (int i = 0; i < n; i++) {
        if (i < held) { a += atk; if (a > 1) a = 1; } else a *= rel;
        vib += 5.8f / SR; if (vib > 1) vib -= 1;
        float depth = fminf(1.0f, (float)i / (0.25f * SR)) * 0.006f;
        float dt = f * (1.0f + depth * sn(vib)) / SR;
        float s = blep_saw(ph, dt);
        ph += dt; if (ph >= 1) ph -= 1;
        lo += c * bp; float hi = s - lo - 0.7f * bp; bp += c * hi;
        lp += (s - lp) * k;
        float bow = 1.0f + 0.08f * frnd() * (1.0f - a * 0.7f);
        put(s0 + i, (lp * 0.55f + bp * 0.45f) * a * v * 0.16f * bow);
    }
}
static void accordion(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.08f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), p1 = 0, p2 = 0.5f, lp = 0, lp2 = 0, k = 1.0f - expf(-TAU_F * 2600.0f / SR);
    float a = 0, atk = 1.0f / (0.03f * SR), rel = expf(-1.0f / (0.04f * SR));
    for (int i = 0; i < n; i++) {
        if (i < held) { a += atk; if (a > 1) a = 1; } else a *= rel;
        float s = (p1 < 0.32f ? 1.0f : -0.5f) + (p2 < 0.32f ? 1.0f : -0.5f) + 0.5f * (p1 < 0.5f ? 0.5f : -0.5f);
        p1 += f * 1.0026f / SR; if (p1 >= 1) p1 -= 1;
        p2 += f * 0.9974f / SR; if (p2 >= 1) p2 -= 1;
        lp += (s - lp) * k; lp2 += (lp - lp2) * k;
        put(s0 + i, lp2 * a * v * 0.075f);
    }
}
static void brass(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.1f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), ph = 0, ph2 = 0.4f, lp1 = 0, lp2 = 0;
    float a = 0, atk = 1.0f / (0.035f * SR), rel = expf(-1.0f / (0.06f * SR));
    for (int i = 0; i < n; i++) {
        if (i < held) { a += atk; if (a > 1) a = 1; } else a *= rel;
        float t = (float)i / SR;
        float cut = 500.0f + 2600.0f * fminf(1.0f, t / 0.06f) * (0.65f + 0.35f * expf(-t * 4.0f)) * (0.5f + 0.5f * v);
        float k = 1.0f - expf(-TAU_F * fminf(cut, f * 9.0f) / SR);
        float dt = f / SR;
        float s = blep_saw(ph, dt) + 0.6f * blep_saw(ph2, dt * 1.003f);
        ph += dt; if (ph >= 1) ph -= 1;
        ph2 += dt * 1.003f; if (ph2 >= 1) ph2 -= 1;
        lp1 += (s - lp1) * k; lp2 += (lp1 - lp2) * k;
        put(s0 + i, lp2 * a * v * 0.11f);
    }
}
static void flute(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.15f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), ph = 0, bn = 0, vib = 0;
    float a = 0, atk = 1.0f / (0.06f * SR), rel = expf(-1.0f / (0.08f * SR));
    for (int i = 0; i < n; i++) {
        if (i < held) { a += atk; if (a > 1) a = 1; } else a *= rel;
        vib += 5.0f / SR; if (vib > 1) vib -= 1;
        float dt = f * (1.0f + 0.005f * sn(vib) * fminf(1.0f, (float)i / (0.35f * SR))) / SR;
        float s = sn(ph) + 0.12f * sn(ph * 2.0f - floorf(ph * 2.0f));
        ph += dt; if (ph >= 1) ph -= 1;
        bn += (frnd() - bn) * 0.25f;
        put(s0 + i, (s + bn * 0.18f) * a * v * 0.2f);
    }
}
static float ks[4096];
static void pluck(float b, float d, float m, float v, float bright)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.7f) * SR);
    float f = mtof(m);
    int L = (int)(SR / f);
    if (L < 2) L = 2; if (L > 4095) L = 4095;
    float lp = 0;
    for (int i = 0; i < L; i++) { lp += (frnd() - lp) * bright; ks[i] = lp; }
    int p = 0;
    float damp = 0.4985f + 0.0012f * bright;
    for (int i = 0; i < n; i++) {
        int q = p + 1 == L ? 0 : p + 1;
        float y = ks[p];
        ks[p] = (ks[p] + ks[q]) * damp;
        p = q;
        float e = (n - i) < (int)(0.08f * SR) ? (float)(n - i) / (0.08f * SR) : 1.0f;
        put(s0 + i, y * e * v * 0.55f);
    }
}
static void pizz(float b, float d, float m, float v) { pluck(b, fminf(d, 1.0f), m, v * 1.2f, 0.35f); }
static void harp(float b, float d, float m, float v) { pluck(b, d, m, v * 0.8f, 0.75f); }
static void fm_note(float b, float m, float v, float ratio, float index, float idx_decay, float decay, float len)
{
    int s0 = s_of(b), n = (int)(len * SR);
    float f = mtof(m), pc = 0, pm = 0;
    float env = 1, k = expf(-1.0f / (decay * SR)), ie = 1, ik = expf(-1.0f / (idx_decay * SR));
    for (int i = 0; i < n; i++) {
        float atk = i < 40 ? (float)i / 40.0f : 1.0f;
        float x = pc + index * ie * sn(pm) * 0.159f;
        x -= floorf(x);
        put(s0 + i, sn(x) * env * atk * v * 0.22f);
        pc += f / SR; if (pc >= 1) pc -= 1;
        pm += f * ratio / SR; if (pm >= 1) pm -= 1;
        env *= k; ie *= ik;
    }
}
static void marimba(float b, float d, float m, float v) { (void)d; fm_note(b, m, v * 1.3f, 4.0f, 2.2f, 0.03f, 0.22f, 0.9f); fm_note(b, m, v * 0.8f, 1.0f, 0.0f, 0.1f, 0.3f, 0.9f); }
static void glock(float b, float d, float m, float v) { (void)d; fm_note(b, m, v, 3.5f, 1.6f, 0.08f, 0.45f, 1.6f); }
static void bassn(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.08f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), ph = 0, lp = 0, k = 1.0f - expf(-TAU_F * 700.0f / SR);
    float a = 0, rel = expf(-1.0f / (0.05f * SR)), dec = expf(-1.0f / (0.6f * SR)), e = 1;
    for (int i = 0; i < n; i++) {
        if (i < held) { a += 1.0f / (0.006f * SR); if (a > 1) a = 1; } else a *= rel;
        float dt = f / SR;
        float s = sn(ph) * 0.8f + blep_saw(ph, dt) * 0.35f;
        ph += dt; if (ph >= 1) ph -= 1;
        lp += (s - lp) * k;
        put(s0 + i, lp * a * e * v * 0.32f);
        e = e * dec + (1 - dec) * 0.55f;
    }
}
static void choir(float b, float d, float m, float v)
{
    int s0 = s_of(b), n = (int)((d * BEAT + 0.5f) * SR), held = (int)(d * BEAT * SR);
    float f = mtof(m), p[2] = { 0, 0.5f };
    float l1 = 0, b1 = 0, l2 = 0, b2 = 0;
    float c1 = 2.0f * sinf(3.14159f * 750.0f / SR), c2 = 2.0f * sinf(3.14159f * 1150.0f / SR);
    float a = 0, rel = expf(-1.0f / (0.25f * SR)), vib = 0;
    for (int i = 0; i < n; i++) {
        if (i < held) { a += 1.0f / (0.35f * SR); if (a > 1) a = 1; } else a *= rel;
        vib += 4.6f / SR; if (vib > 1) vib -= 1;
        float s = 0;
        for (int o = 0; o < 2; o++) { float dt = f * (o ? 1.006f : 0.997f) * (1.0f + 0.004f * sn(vib)) / SR; s += blep_saw(p[o], dt); p[o] += dt; if (p[o] >= 1) p[o] -= 1; }
        l1 += c1 * b1; float h1 = s - l1 - 0.35f * b1; b1 += c1 * h1;
        l2 += c2 * b2; float h2 = s - l2 - 0.4f * b2; b2 += c2 * h2;
        put(s0 + i, (b1 + b2 * 0.7f) * a * v * 0.05f);
    }
}
/* drums take (beat, velocity) */
static void timpani(float b, float m, float v)
{
    int s0 = s_of(b), n = (int)(1.3f * SR);
    float f = mtof(m), ph = 0, e = 1, k = expf(-1.0f / (0.45f * SR)), lo = 0;
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR;
        float ff = f * (1.0f + 0.12f * expf(-t * 30.0f));
        float s = sn(ph) + 0.4f * sn(ph * 1.5f - floorf(ph * 1.5f)) * expf(-t * 6.0f);
        ph += ff / SR; if (ph >= 1) ph -= 1;
        lo += (frnd() - lo) * 0.08f;
        put(s0 + i, (s * 0.8f + lo * 1.5f * expf(-t * 25.0f)) * e * v * 0.5f);
        e *= k;
    }
}
static void tom(float b, float m, float v)
{
    int s0 = s_of(b), n = (int)(0.5f * SR);
    float f = mtof(m), ph = 0, e = 1, k = expf(-1.0f / (0.16f * SR));
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR;
        ph += f * (1.0f + 0.6f * expf(-t * 40.0f)) / SR; if (ph >= 1) ph -= 1;
        put(s0 + i, (sn(ph) + 0.15f * frnd() * expf(-t * 60)) * e * v * 0.55f);
        e *= k;
    }
}
static void kick(float b, float v)
{
    int s0 = s_of(b), n = (int)(0.35f * SR);
    float ph = 0, e = 1, k = expf(-1.0f / (0.12f * SR));
    for (int i = 0; i < n; i++) {
        float t = (float)i / SR;
        ph += (45.0f + 80.0f * expf(-t * 35.0f)) / SR; if (ph >= 1) ph -= 1;
        put(s0 + i, sn(ph) * e * v * 0.7f);
        e *= k;
    }
}
static void snare(float b, float v)
{
    int s0 = s_of(b), n = (int)(0.25f * SR);
    float ph = 0, e = 1, k = expf(-1.0f / (0.07f * SR)), hp = 0, prev = 0;
    for (int i = 0; i < n; i++) {
        float w = frnd();
        hp = 0.85f * (hp + w - prev); prev = w;
        ph += 190.0f / SR; if (ph >= 1) ph -= 1;
        put(s0 + i, (hp * 0.7f + sn(ph) * 0.4f * expf(-(float)i / (0.03f * SR))) * e * v * 0.35f);
        e *= k;
    }
}
static void shaker(float b, float v)
{
    int s0 = s_of(b), n = (int)(0.08f * SR);
    float hp = 0, prev = 0;
    for (int i = 0; i < n; i++) {
        float w = frnd();
        hp = 0.7f * (hp + w - prev); prev = w;
        float e = (float)i < 0.01f * SR ? (float)i / (0.01f * SR) : expf(-((float)i - 0.01f * SR) / (0.025f * SR));
        put(s0 + i, hp * e * v * 0.08f);
    }
}
static void cymbal(float b, float v, float len)
{
    int s0 = s_of(b), n = (int)(len * SR);
    float hp = 0, prev = 0;
    for (int i = 0; i < n; i++) {
        float w = frnd();
        hp = 0.92f * (hp + w - prev); prev = w;
        float t = (float)i / (float)n;
        put(s0 + i, hp * (1 - t) * (1 - t) * v * 0.07f);
    }
}
static void swell(float b, float v, float len)          /* reverse cymbal into a downbeat */
{
    int s0 = s_of(b) - (int)(len * SR), n = (int)(len * SR);
    float hp = 0, prev = 0;
    for (int i = 0; i < n; i++) {
        float w = frnd();
        hp = 0.92f * (hp + w - prev); prev = w;
        float t = (float)i / (float)n;
        put(s0 + i, hp * t * t * v * 0.06f);
    }
}

/* ------------------------------------------------------------------ */
/* notation                                                            */
/* ------------------------------------------------------------------ */
typedef void (*Inst)(float b, float d, float m, float v);

static int parse_note(const char **ps)
{
    const char *s = *ps;
    static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 };    /* A B C D E F G */
    int n = base[(*s & ~32) - 'A'];
    s++;
    if (*s == '#') { n++; s++; }
    else if (*s == 'b') { n--; s++; }
    int oct = *s - '0';
    s++;
    *ps = s;
    return n + (oct + 1) * 12;
}

/* plays a note string; returns the beat after the last note */
static float seq(const char *s, float b, Inst inst, float v, int tr)
{
    while (*s) {
        while (*s == ' ' || *s == '|' || *s == '\n') s++;
        if (!*s) break;
        int notes[6], nn = 0, rest = 0;
        if (*s == 'r') { rest = 1; s++; }
        else {
            for (;;) {
                if (nn < 6) notes[nn++] = parse_note(&s);
                if (*s == '+') { s++; continue; }
                break;
            }
        }
        float d = 1;
        if (*s == ':') { s++; d = (float)strtod(s, (char **)&s); }
        if (!rest) for (int i = 0; i < nn; i++) inst(b, d * 0.96f, (float)(notes[i] + tr), v);
        b += d;
    }
    return b;
}

/* chord names -> triad (root in octave oct) */
static void chord(const char *name, int oct, int out[3])
{
    const char *s = name;
    static const int base[7] = { 9, 11, 0, 2, 4, 5, 7 };
    int r = base[(*s & ~32) - 'A'];
    s++;
    if (*s == '#') { r++; s++; } else if (*s == 'b') { r--; s++; }
    int minor = *s == 'm';
    out[0] = r + (oct + 1) * 12;
    out[1] = out[0] + (minor ? 3 : 4);
    out[2] = out[0] + 7;
}
static const char *chord_at(const char *list, int idx, char *buf)
{
    const char *s = list;
    for (int i = 0; ; i++) {
        while (*s == ' ' || *s == '|') s++;
        if (!*s) { s = list; i--; continue; }
        const char *e = s;
        while (*e && *e != ' ' && *e != '|') e++;
        if (i == idx) { int n = (int)(e - s); memcpy(buf, s, (size_t)n); buf[n] = 0; return buf; }
        s = e;
    }
}

/* ------------------------------------------------------------------ */
/* reverb + output                                                     */
/* ------------------------------------------------------------------ */
static void reverb(float wet, float room)
{
    static const int cl[4] = { 1116, 1188, 1277, 1356 }, al[2] = { 556, 441 };
    float *dry = (float *)malloc(sizeof(float) * (size_t)N);
    float *wetb = (float *)calloc((size_t)N, sizeof(float));
    if (!dry || !wetb) { free(dry); free(wetb); return; }
    memcpy(dry, mix, sizeof(float) * (size_t)N);
    float *cb[4], *ab[2];
    int ci[4] = { 0 }, ai[2] = { 0 }, clen[4], alen[2];
    float fs[4] = { 0 };
    for (int k = 0; k < 4; k++) { clen[k] = cl[k] * 22050 / 44100 + 13 * k; cb[k] = (float *)calloc((size_t)clen[k], sizeof(float)); }
    for (int k = 0; k < 2; k++) { alen[k] = al[k] * 22050 / 44100; ab[k] = (float *)calloc((size_t)alen[k], sizeof(float)); }
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < N; i++) {
            float in = dry[i] * 0.3f, out = 0;
            for (int k = 0; k < 4; k++) {
                float y = cb[k][ci[k]];
                fs[k] = y * 0.6f + fs[k] * 0.4f;
                cb[k][ci[k]] = in + fs[k] * room;
                if (++ci[k] >= clen[k]) ci[k] = 0;
                out += y;
            }
            for (int k = 0; k < 2; k++) {
                float bo = ab[k][ai[k]];
                float y = -out + bo;
                ab[k][ai[k]] = out + bo * 0.5f;
                if (++ai[k] >= alen[k]) ai[k] = 0;
                out = y;
            }
            if (pass == 1) wetb[i] = out;
        }
    for (int i = 0; i < N; i++) mix[i] = dry[i] + wetb[i] * wet;
    for (int k = 0; k < 4; k++) free(cb[k]);
    for (int k = 0; k < 2; k++) free(ab[k]);
    free(dry); free(wetb);
}

static void finish(int t)
{
    float peak = 1e-6f;
    for (int i = 0; i < N; i++) if (fabsf(mix[i]) > peak) peak = fabsf(mix[i]);
    float g = 1.25f / peak;
    void *raw = malloc(sizeof(int16_t) * (size_t)N + 64);
    if (!raw) return;
    int16_t *d = (int16_t *)(((uintptr_t)raw + 31) & ~(uintptr_t)31);
    for (int i = 0; i < N; i++) {
        float x = mix[i] * g;
        x = x / (1.0f + fabsf(x) * 0.25f);          /* gentle limiter */
        d[i] = (int16_t)(x * 29000.0f);
    }
    trk[t] = d; trk_len[t] = N;
}

static void begin(float beat_s, int beats)
{
    BEAT = beat_s;
    N = ((int)(beat_s * (float)beats * SR)) & ~15;
    memset(mix, 0, sizeof(float) * (size_t)N);
}

/* ------------------------------------------------------------------ */
/* 0. TREASURE HOUNDS - the main theme. 6/8, beat = an eighth note       */
/* ------------------------------------------------------------------ */
static const char *THEME_A =
    "D4:2 A4:1 A4:2 F4:1 | G4:1 A4:1 Bb4:1 A4:2 G4:1 | F4:2 E4:1 D4:2 E4:1 | F4:3 A4:3 |"
    "D5:2 C5:1 Bb4:2 A4:1 | G4:1 Bb4:1 D5:1 C5:2 A4:1 | Bb4:2 A4:1 G4:2 E4:1 | D4:3 r:3";
static const char *THEME_B =
    "F4:2 A4:1 C5:2 F5:1 | E5:2 D5:1 C5:3 | Bb4:2 D5:1 F5:2 D5:1 | C5:6 |"
    "A4:2 C5:1 F5:2 A5:1 | G5:2 F5:1 E5:2 C5:1 | D5:1 E5:1 F5:1 E5:2 C#5:1 | D5:6";
static const char *THEME_C =
    "A4:1 A4:1 A4:1 D5:3 | C5:1 C5:1 C5:1 F5:3 | Bb4:1 Bb4:1 Bb4:1 E5:2 D5:1 | C#5:3 E5:3 |"
    "F5:2 E5:1 D5:2 C5:1 | Bb4:2 A4:1 G4:2 F4:1 | E4:2 F4:1 G4:2 A4:1 | A4:3 C#5:3";
static const char *CH_A = "Dm Gm Dm F Bb Gm A Dm";
static const char *CH_B = "F C Bb C F C A Dm";
static const char *CH_C = "Dm F Gm A Dm Gm C A";

static void jig_bar(float b, const char *ch, float v, int full)
{
    int c[3];
    chord(ch, 2, c);
    pizz(b, 2.5f, (float)c[0], 0.9f * v);
    pizz(b + 3, 2.5f, (float)(c[2] - 12 + 12), 0.7f * v);
    if (full) {
        int cc[3];
        chord(ch, 4, cc);
        for (int k = 0; k < 3; k++) strings(b, 5.8f, (float)cc[k] - (k == 2 ? 12.0f : 0.0f), 0.55f * v);
        /* accordion off-beats: oom-pah-pah */
        for (int beat = 1; beat < 6; beat++) {
            if (beat == 3) continue;
            for (int k = 0; k < 3; k++) accordion(b + (float)beat, 0.7f, (float)cc[k], 0.32f * v);
        }
    }
}

static void track_title(void)
{
    begin(0.2f, 192);
    for (int bar = 0; bar < 32; bar++) {
        float b = (float)(bar * 6);
        char buf[8];
        const char *list = bar < 8 ? CH_A : (bar < 16 ? CH_B : (bar < 24 ? CH_A : CH_C));
        const char *ch = chord_at(list, bar % 8, buf);
        jig_bar(b, ch, 1.0f, 1);
        int c[3];
        chord(ch, 2, c);
        if (bar % 4 == 0) timpani(b, (float)c[0], 0.9f);
        if (bar >= 24) { timpani(b, (float)c[0], 0.6f); timpani(b + 3, (float)c[2] - 12, 0.5f); snare(b + 3, 0.5f); snare(b + 4.5f, 0.3f); snare(b + 5, 0.4f); }
        else { snare(b + 3, 0.22f); shaker(b + 1, 0.4f); shaker(b + 2, 0.3f); shaker(b + 4, 0.4f); shaker(b + 5, 0.3f); }
        if (bar >= 24) {
            int cc[3];
            chord(ch, 3, cc);
            for (int k = 0; k < 3; k++) brass(b, 1.2f, (float)cc[k] + 12, 0.45f);
        }
    }
    seq(THEME_A, 0, fiddle, 0.85f, 0);
    seq(THEME_B, 48, fiddle, 0.9f, 0);
    seq(THEME_B, 48, accordion, 0.4f, -12);
    seq(THEME_B, 48, glock, 0.25f, 12);
    seq(THEME_A, 96, fiddle, 0.8f, 12);
    seq(THEME_A, 96, brass, 0.6f, 0);
    seq(THEME_C, 144, brass, 0.85f, 0);
    seq(THEME_C, 144, fiddle, 0.55f, 12);
    swell(48, 0.8f, 1.2f); swell(96, 0.8f, 1.2f); swell(144, 1.0f, 1.2f); swell(192, 1.0f, 1.4f);
    cymbal(0, 0.7f, 1.6f); cymbal(48, 0.6f, 1.6f); cymbal(144, 0.8f, 1.6f);
    reverb(0.32f, 0.78f);
}

/* ------------------------------------------------------------------ */
/* 1. GULL'S POINT - a rainy-night shanty. 4/4, beat = a quarter         */
/* ------------------------------------------------------------------ */
static void track_town(void)
{
    begin(0.72f, 48);
    static const char *mel =
        "A3:1 C4:1 E4:1.5 D4:0.5 | C4:1 B3:1 A3:2 | G3:1 B3:1 D4:1.5 C4:0.5 | B3:1 G3:1 E3:2 |"
        "A3:1 C4:1 E4:1.5 G4:0.5 | A4:2 G4:1 E4:1 | F4:1 E4:1 D4:1 B3:1 | C4:1 B3:1 A3:2 |"
        "E4:1.5 F4:0.5 E4:1 C4:1 | D4:1.5 E4:0.5 D4:1 B3:1 | C4:1 E4:1 A4:1.5 G#4:0.5 | A4:4";
    static const char *ch = "Am E G Em Am C Dm Am C G E Am";
    for (int bar = 0; bar < 12; bar++) {
        float b = (float)(bar * 4);
        char buf[8];
        int c[3];
        chord(chord_at(ch, bar, buf), 2, c);
        strings(b, 3.9f, (float)c[0], 0.5f);
        strings(b, 3.9f, (float)c[2], 0.35f);
        bassn(b, 1.5f, (float)c[0], 0.7f);
        bassn(b + 2, 1.5f, (float)c[2] - 12, 0.5f);
        /* harp arpeggio in eighths */
        int cc[3];
        chord(buf, 3, cc);
        static const int pat[8] = { 0, 1, 2, 1, 0, 2, 1, 2 };
        for (int k = 0; k < 8; k++) harp(b + (float)k * 0.5f, 1.0f, (float)cc[pat[k]] + (k >= 4 && k != 6 ? 12.0f : 0.0f), 0.32f);
        tom(b, 45, 0.45f); tom(b + 1.5f, 50, 0.25f); tom(b + 2, 45, 0.35f);
    }
    static const char *last = "E4:1.5 F4:0.5 E4:1 C4:1 | D4:1.5 E4:0.5 D4:1 B3:1 | C4:1 E4:1 A4:1.5 G#4:0.5 | A4:4";
    seq(mel, 0, accordion, 0.9f, 12);
    seq(last, 32, flute, 0.4f, 24);             /* a whistle joins for the last phrase */
    reverb(0.4f, 0.82f);
}

/* ------------------------------------------------------------------ */
/* 2. SMUGGLERS' CAVES - mysterious. 4/4                                */
/* ------------------------------------------------------------------ */
static void track_caves(void)
{
    begin(0.79f, 48);
    static const char *ch = "Em Em C B Em Em Am B C Am B Em";
    for (int bar = 0; bar < 12; bar++) {
        float b = (float)(bar * 4);
        char buf[8];
        int c[3];
        chord(chord_at(ch, bar, buf), 2, c);
        /* plucked bass ostinato */
        int pat[8] = { c[0], c[2], c[0] + 12, c[2], c[1], c[2], c[0] + 2, c[2] };
        for (int k = 0; k < 8; k++) pluck(b + (float)k * 0.5f, 0.5f, (float)pat[k], k == 0 ? 0.8f : 0.5f, 0.3f);
        int cc[3];
        chord(buf, 3, cc);
        for (int k = 0; k < 3; k++) choir(b, 3.9f, (float)cc[k] + 12, 0.32f);
        strings(b, 3.9f, (float)c[0] - 12, 0.4f);
        if (bar % 2 == 0) tom(b, 38, 0.5f);
        tom(b + 0.5f, 38, 0.25f);
        /* drips: glockenspiel notes from the scale */
        static const int pent[6] = { 76, 79, 81, 83, 86, 88 };
        for (int k = 0; k < 8; k++) {
            float r = frnd();
            if (r > 0.45f) glock(b + (float)k * 0.5f + 0.25f * (float)(r > 0.8f), 1, (float)pent[(int)((frnd() + 1) * 2.99f)], 0.18f + 0.1f * r);
        }
    }
    static const char *mel =
        "r:2 B4:1 E5:1 | D5:2 B4:2 | C5:1.5 B4:0.5 A4:2 | B4:4 |"
        "G4:1 A4:1 B4:1.5 C5:0.5 | A4:2 E4:2 | F#4:1 A4:1 B4:1 D#5:1 | E5:4";
    seq(mel, 16, flute, 0.75f, 0);
    seq("E5:4 | D5:4 | C5:4 | B4:4", 0, flute, 0.3f, -12);
    reverb(0.55f, 0.86f);
}

/* ------------------------------------------------------------------ */
/* 3. TEMPLE OF BONES - tribal drums, bone marimba, eerie flute          */
/* ------------------------------------------------------------------ */
static void track_temple(void)
{
    begin(0.6f, 64);
    static const char *ch = "Dm Dm Eb Dm Dm Dm C Eb Bb C Dm Dm Eb C Dm Dm";
    static const char *osti[2] = { "D4:0.5 F4:0.5 Eb4:0.5 D4:0.5 C4:0.5 D4:0.5 Eb4:0.5 F4:0.5",
                                   "Bb3:0.5 D4:0.5 C4:0.5 Bb3:0.5 A3:0.5 Bb3:0.5 C4:0.5 D4:0.5" };
    for (int bar = 0; bar < 16; bar++) {
        float b = (float)(bar * 4);
        char buf[8];
        int c[3];
        chord(chord_at(ch, bar, buf), 3, c);
        int tr = c[0] - 62;                       /* move the ostinato with the chord */
        seq(osti[bar >= 8 && bar < 12 ? 1 : 0], b, marimba, 0.55f, (bar >= 8 && bar < 12) ? tr + 2 : tr);
        for (int k = 0; k < 3; k++) choir(b, 3.9f, (float)c[k], 0.38f);
        bassn(b, 3.5f, (float)c[0] - 24, 0.6f);
        tom(b, 40, 0.9f); tom(b + 1.5f, 40, 0.6f); tom(b + 2, 47, 0.7f); tom(b + 3, 43, 0.6f); tom(b + 3.5f, 47, 0.45f);
        for (int k = 0; k < 8; k++) shaker(b + (float)k * 0.5f, k & 1 ? 0.5f : 0.8f);
        if (bar % 4 == 3) { tom(b + 3.25f, 50, 0.4f); tom(b + 3.75f, 52, 0.5f); }
    }
    static const char *mel =
        "A4:2 Bb4:1 A4:1 | G4:1 F4:1 Eb4:2 | D4:1 Eb4:1 F4:1 G4:1 | A4:4 |"
        "D5:2 C5:1 Bb4:1 | C5:2 Bb4:1 A4:1 | Bb4:1 A4:1 G4:1 F4:1 | Eb4:2 D4:2 |"
        "F4:1 G4:1 A4:2 | Bb4:1 C5:1 D5:2 | Eb5:2 D5:1 C5:1 | D5:4";
    seq(mel, 16, flute, 0.85f, 0);
    reverb(0.45f, 0.84f);
}

/* ------------------------------------------------------------------ */
/* 4. SKELETON COVE - the finale. Fast 6/8                               */
/* ------------------------------------------------------------------ */
static void track_cove(void)
{
    begin(0.172f, 192);
    static const char *ch = "Gm Gm Eb F Gm Eb D D Bb F Gm D Eb F Gm D Gm Gm Eb F Gm Eb D D Cm D Gm Eb Cm D Gm D";
    for (int bar = 0; bar < 32; bar++) {
        float b = (float)(bar * 6);
        char buf[8];
        int c[3];
        chord(chord_at(ch, bar, buf), 3, c);
        /* driving string eighths */
        int pat[6] = { c[0], c[2], c[0] + 12, c[2], c[1] + 12, c[2] };
        for (int k = 0; k < 6; k++) strings(b + (float)k, 0.8f, (float)pat[k], k == 0 || k == 3 ? 0.55f : 0.35f);
        pizz(b, 2, (float)c[0] - 12, 1.0f); pizz(b + 3, 2, (float)c[2] - 12, 0.8f);
        timpani(b, (float)c[0] - 12, 0.8f); timpani(b + 3, (float)c[2] - 24, 0.6f);
        snare(b + 3, 0.6f); snare(b + 5, 0.3f);
        if (bar % 2 == 1) { snare(b + 4, 0.25f); snare(b + 4.5f, 0.3f); }
        kick(b, 0.7f);
        if (bar % 8 == 7) { for (int k = 0; k < 6; k++) snare(b + (float)k * 0.5f + 3, 0.2f + 0.08f * (float)k); }
        int cc[3];
        chord(buf, 4, cc);
        if (bar >= 24) for (int k = 0; k < 3; k++) brass(b, 1.0f, (float)cc[k], 0.35f);
    }
    static const char *A =
        "G4:3 D5:3 | Bb4:2 A4:1 G4:3 | Eb5:2 D5:1 C5:2 Bb4:1 | A4:3 F4:3 |"
        "G4:2 A4:1 Bb4:2 C5:1 | D5:2 Eb5:1 D5:2 C5:1 | Bb4:2 A4:1 F#4:3 | A4:3 D4:3";
    static const char *B =
        "D5:2 F5:1 D5:2 Bb4:1 | C5:2 A4:1 F4:3 | G4:2 Bb4:1 D5:2 G5:1 | F#5:3 D5:3 |"
        "Eb5:2 D5:1 C5:2 Bb4:1 | C5:2 D5:1 Eb5:2 F5:1 | G5:2 F5:1 Eb5:2 D5:1 | D5:6";
    static const char *D =
        "C5:1 C5:1 C5:1 Eb5:3 | D5:1 D5:1 D5:1 F#5:3 | G5:2 F5:1 Eb5:2 D5:1 | Eb5:3 Bb4:3 |"
        "C5:2 Bb4:1 A4:2 G4:1 | F#4:2 G4:1 A4:3 | Bb4:2 C5:1 D5:2 Bb4:1 | A4:3 F#4:3";
    seq(A, 0, brass, 0.85f, 0);
    seq(B, 48, brass, 0.8f, 0);
    seq(B, 48, fiddle, 0.5f, 12);
    seq(A, 96, fiddle, 0.8f, 12);
    seq(A, 96, brass, 0.6f, 0);
    seq(D, 144, brass, 0.9f, 0);
    seq(D, 144, fiddle, 0.55f, 12);
    swell(48, 0.9f, 1.0f); swell(96, 0.9f, 1.0f); swell(144, 1.0f, 1.0f); swell(192, 1.0f, 1.0f);
    cymbal(0, 0.8f, 1.4f); cymbal(48, 0.8f, 1.4f); cymbal(96, 0.8f, 1.4f); cymbal(144, 0.9f, 1.4f);
    reverb(0.3f, 0.76f);
}

/* ------------------------------------------------------------------ */
/* 5. THE GOLD! - the theme in D major, with bells                     */
/* ------------------------------------------------------------------ */
static void track_victory(void)
{
    begin(0.19f, 96);
    static const char *A =
        "D4:2 A4:1 A4:2 F#4:1 | G4:1 A4:1 B4:1 A4:2 G4:1 | F#4:2 E4:1 D4:2 E4:1 | F#4:3 A4:3 |"
        "D5:2 C#5:1 B4:2 A4:1 | G4:1 B4:1 D5:1 C#5:2 A4:1 | B4:2 A4:1 G4:2 E4:1 | D4:3 r:3";
    static const char *B =
        "F#4:2 A4:1 D5:2 F#5:1 | E5:2 D5:1 C#5:3 | B4:2 D5:1 G5:2 B5:1 | A5:6 |"
        "F#5:2 E5:1 D5:2 A4:1 | G4:2 B4:1 E5:2 G5:1 | F#5:2 E5:1 C#5:2 E5:1 | D5:6";
    static const char *chA = "D G D D G Em A D", *chB = "D A G A D Em A D";
    for (int bar = 0; bar < 16; bar++) {
        float b = (float)(bar * 6);
        char buf[8];
        const char *ch = chord_at(bar < 8 ? chA : chB, bar % 8, buf);
        jig_bar(b, ch, 1.0f, 1);
        int c[3];
        chord(ch, 2, c);
        timpani(b, (float)c[0] + 12, 0.6f);
        snare(b + 3, 0.3f);
        shaker(b + 1, 0.4f); shaker(b + 2, 0.3f); shaker(b + 4, 0.4f); shaker(b + 5, 0.3f);
        if (bar >= 8) {
            int cc[3];
            chord(ch, 4, cc);
            brass(b, 1.0f, (float)cc[0], 0.4f); brass(b, 1.0f, (float)cc[1], 0.35f); brass(b, 1.0f, (float)cc[2], 0.35f);
        }
    }
    seq(A, 0, fiddle, 0.8f, 0);
    seq(A, 0, glock, 0.3f, 12);
    seq(B, 48, brass, 0.75f, 0);
    seq(B, 48, fiddle, 0.6f, 0);
    seq(B, 48, glock, 0.35f, 12);
    cymbal(0, 0.8f, 1.6f); cymbal(48, 0.9f, 1.8f); swell(48, 0.8f, 1.0f); swell(96, 0.8f, 1.0f);
    reverb(0.35f, 0.8f);
}

/* ------------------------------------------------------------------ */
int music_init_step(int i)
{
    if (i == 0) {
        for (int k = 0; k <= 4096; k++) sintab[k] = sinf(TAU_F * (float)k / 4096.0f);
        if (!mix) mix = (float *)malloc(sizeof(float) * (size_t)(MUS_RATE * 42));
    }
    if (!mix) return 1;
    if (i < MUS_TRACKS && !trk[i]) {
        rng = 0xC0FFEEu + (uint32_t)i * 7919u;
        switch (i) {
        case MUS_TITLE: track_title(); break;
        case MUS_TOWN: track_town(); break;
        case MUS_CAVES: track_caves(); break;
        case MUS_TEMPLE: track_temple(); break;
        case MUS_COVE: track_cove(); break;
        case MUS_VICTORY: track_victory(); break;
        }
        finish(i);
    }
    if (i >= MUS_TRACKS - 1) { free(mix); mix = 0; return 1; }
    return 0;
}

void music_init(void)
{
    for (int i = 0; i < MUS_TRACKS; i++) if (music_init_step(i)) break;
}

const int16_t *music_data(int t) { return trk[t]; }
int music_len(int t) { return trk_len[t]; }
int music_volume(int t) { return VOL[t]; }
