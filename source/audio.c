/*
 * TREASURE HOUNDS - sound effects and ambience, synthesized at start-up. See audio.h.
 *
 * A small offline synth: band-limited and FM oscillators, swept resonant filters on noise,
 * formant filters for the kids' "oof"s and the Crowleys' "HEY!", and a short echo. Every
 * effect is normalised, then given a playback volume from the table below.
 */
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define SR    ((float)SFX_RATE)
#define TAU_F 6.28318531f
#define POOL  1700000                   /* samples for every effect together */
#define MAXLEN (SFX_RATE * 4)

static int16_t *pool;                    /* 32-byte aligned */
static int pool_used;
static int16_t *sfx_ptr[SFX_COUNT];
static int sfx_len[SFX_COUNT];
static float mix[MAXLEN + 64];
static int mixend;
static uint32_t rng = 0x2545F491u;

static int16_t *amb[AMB_COUNT];
static int amb_n[AMB_COUNT];

static const uint8_t VOL[SFX_COUNT] = {
    /* WHIP */ 200, /* WHIPHIT */ 190, /* STEP */ 60, /* COIN */ 150, /* GEM */ 170, /* ITEM */ 220, /* CHEST */ 210,
    /* GOLD */ 230, /* EAT */ 190, /* HURT */ 210, /* FAINT */ 190, /* ENEMYHIT */ 200, /* POOF */ 170, /* DIZZY */ 160,
    /* BAT */ 120, /* CRAB */ 120, /* SQUEAK */ 100, /* RATTLE */ 170, /* HEY */ 200, /* DART */ 170, /* SPIKES */ 150,
    /* FLAME */ 170, /* RUMBLE */ 230, /* CRUMBLE */ 190, /* FALL */ 200, /* SPLASH */ 190, /* DOOR */ 230, /* LEVER */ 180,
    /* CLICK */ 170, /* PUSH */ 170, /* SOLVED */ 210, /* WRONG */ 190, /* BONE */ 230, /* IGNITE */ 190, /* BLIP */ 70,
    /* MENU */ 110, /* SELECT */ 170, /* PAUSE */ 150, /* THUNDER */ 255, /* CRACK */ 220, /* WAVE */ 170, /* BREAK */ 200,
    /* RUSTLE */ 150, /* SWING */ 180, /* LAND */ 150, /* BOTTLE */ 190, /* ROAR */ 230, /* FANFARE */ 220, /* GAMEOVER */ 220,
    /* CHEER */ 210, /* CREAK */ 150, /* DRIP */ 120, /* CRANE */ 220, /* HEAL */ 110, /* THROW */ 150, /* BURP */ 220,
};

static float frnd(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return (float)(rng >> 8) / 8388608.0f - 1.0f;
}
static float mtof(float m) { return 440.0f * powf(2.0f, (m - 69.0f) / 12.0f); }

static void put(int i, float v)
{
    if (i < 0 || i >= MAXLEN) return;
    mix[i] += v;
    if (i + 1 > mixend) mixend = i + 1;
}

enum { W_SIN, W_TRI, W_SAW, W_SQR, W_FM };
/*
 * a pitched voice: start/dur in s, pitch glides f0 -> f1 (exponential), attack, decay time constant
 * (0 = held), wave shape. W_FM: a = ratio, b = index. W_SQR: a = pulse width.
 */
static void tone(float st, float dur, float f0, float f1, float vol, float atk, float tau, int wave, float a, float b)
{
    int s0 = (int)(st * SR), n = (int)(dur * SR);
    float ph = 0, pm = 0, f = f0;
    float gl = (f1 == f0 || n < 2) ? 1.0f : powf(f1 / f0, 1.0f / (float)n);
    float env = 1, k = tau > 0 ? expf(-1.0f / (tau * SR)) : 1.0f;
    float an = atk * SR > 1 ? atk * SR : 1, rn = 0.004f * SR;
    for (int i = 0; i < n; i++) {
        float amp = env;
        if ((float)i < an) amp *= (float)i / an;
        if ((float)(n - i) < rn) amp *= (float)(n - i) / rn;
        float v;
        switch (wave) {
        case W_TRI: v = 4.0f * fabsf(ph - 0.5f) - 1.0f; break;
        case W_SAW: {
            float dt = f / SR, t = ph;
            v = 2.0f * ph - 1.0f;
            if (t < dt) { t /= dt; v -= t + t - t * t - 1.0f; }
            else if (t > 1.0f - dt) { t = (t - 1.0f) / dt; v -= t * t + t + t + 1.0f; }
            break;
        }
        case W_SQR: v = ph < a ? 0.8f : -0.8f; break;
        case W_FM: v = sinf(TAU_F * ph + b * env * sinf(TAU_F * pm)); break;
        default: v = sinf(TAU_F * ph); break;
        }
        put(s0 + i, v * amp * vol);
        ph += f / SR; ph -= floorf(ph);
        pm += f * a / SR; pm -= floorf(pm);
        f *= gl; env *= k;
    }
}

enum { F_LP, F_BP, F_HP };
/* filtered noise: cutoff sweeps c0 -> c1, resonance q (state-variable filter) */
static void noise(float st, float dur, float c0, float c1, float q, int type, float vol, float atk, float tau)
{
    int s0 = (int)(st * SR), n = (int)(dur * SR);
    float lo = 0, bp = 0, env = 1, k = tau > 0 ? expf(-1.0f / (tau * SR)) : 1.0f;
    float gl = (c1 == c0 || n < 2) ? 1.0f : powf(c1 / c0, 1.0f / (float)n), c = c0;
    float an = atk * SR > 1 ? atk * SR : 1, rn = 0.004f * SR;
    float damp = 1.0f / (q > 0.5f ? q : 0.5f);
    for (int i = 0; i < n; i++) {
        float fc = 2.0f * sinf(3.14159f * fminf(c, SR * 0.22f) / SR);
        float in = frnd();
        lo += fc * bp;
        float hi = in - lo - damp * bp;
        bp += fc * hi;
        float v = type == F_LP ? lo : (type == F_BP ? bp : hi);
        float amp = env;
        if ((float)i < an) amp *= (float)i / an;
        if ((float)(n - i) < rn) amp *= (float)(n - i) / rn;
        put(s0 + i, v * amp * vol);
        c *= gl; env *= k;
    }
}

/* a voiced vowel: saw through two formant filters */
static void voice(float st, float dur, float f0, float f1, float F1, float F2, float vol, float atk, float rough)
{
    int s0 = (int)(st * SR), n = (int)(dur * SR);
    float ph = 0, f = f0, gl = powf(f1 / f0, 1.0f / (float)(n > 1 ? n : 1));
    float l1 = 0, b1 = 0, l2 = 0, b2 = 0;
    float c1 = 2.0f * sinf(3.14159f * F1 / SR), c2 = 2.0f * sinf(3.14159f * F2 / SR);
    for (int i = 0; i < n; i++) {
        float t = (float)i / (float)n;
        float amp = (float)i < atk * SR ? (float)i / (atk * SR) : 1.0f;
        amp *= t > 0.7f ? (1 - t) / 0.3f : 1.0f;
        float jit = 1.0f + rough * frnd() * 0.08f;
        float v = 2.0f * ph - 1.0f;
        l1 += c1 * b1; float h1 = v - l1 - 0.25f * b1; b1 += c1 * h1;
        l2 += c2 * b2; float h2 = v - l2 - 0.3f * b2; b2 += c2 * h2;
        put(s0 + i, (b1 + b2 * 0.6f) * amp * vol);
        ph += f * jit / SR; ph -= floorf(ph);
        f *= gl;
    }
}

static void echo(float delay, float fb, float wet)
{
    int d = (int)(delay * SR);
    int end = mixend + (int)(delay * SR * 6);
    if (end > MAXLEN) end = MAXLEN;
    for (int i = d; i < end; i++) mix[i] += mix[i - d] * fb * wet;
    if (end > mixend) mixend = end;
}

static void bell(float st, float midi, float vol, float decay) { tone(st, decay * 4, mtof(midi), mtof(midi), vol, 0.002f, decay, W_FM, 3.5f, 1.6f); tone(st, decay * 3, mtof(midi) * 2, mtof(midi) * 2, vol * 0.3f, 0.001f, decay * 0.5f, W_SIN, 0, 0); }
static void brass(float st, float dur, float midi, float vol)
{
    /* saw with a quick filter-ish brightness swell faked by layering */
    tone(st, dur, mtof(midi), mtof(midi), vol * 0.6f, 0.03f, 0, W_SAW, 0, 0);
    tone(st, dur, mtof(midi) * 1.003f, mtof(midi) * 1.003f, vol * 0.4f, 0.05f, 0, W_SQR, 0.4f, 0);
    tone(st, dur, mtof(midi), mtof(midi), vol * 0.5f, 0.01f, dur * 0.6f, W_FM, 1.0f, 2.0f);
}

/* ------------------------------------------------------------------ */
static void build(int id)
{
    switch (id) {
    case SFX_WHIP:
        noise(0, 0.13f, 500, 2400, 1.5f, F_BP, 0.5f, 0.09f, 0);
        noise(0.12f, 0.06f, 5000, 3000, 2.0f, F_HP, 1.6f, 0.0005f, 0.012f);
        tone(0.12f, 0.03f, 1800, 900, 0.5f, 0.0005f, 0.008f, W_SQR, 0.5f, 0);
        echo(0.045f, 0.25f, 1);
        break;
    case SFX_WHIPHIT:
        tone(0, 0.12f, 180, 55, 1.0f, 0.001f, 0.05f, W_SIN, 0, 0);
        noise(0, 0.08f, 3000, 600, 1, F_LP, 0.8f, 0.001f, 0.03f);
        break;
    case SFX_STEP:
        noise(0, 0.05f, 900, 300, 1.2f, F_LP, 1.0f, 0.002f, 0.015f);
        tone(0, 0.04f, 120, 70, 0.4f, 0.001f, 0.015f, W_SIN, 0, 0);
        break;
    case SFX_COIN:
        tone(0, 0.06f, mtof(88), mtof(88), 0.5f, 0.001f, 0, W_SQR, 0.5f, 0);
        tone(0.06f, 0.35f, mtof(93), mtof(93), 0.5f, 0.001f, 0.12f, W_SQR, 0.5f, 0);
        tone(0.06f, 0.35f, mtof(93), mtof(93), 0.3f, 0.001f, 0.12f, W_FM, 3.0f, 1.0f);
        break;
    case SFX_GEM:
        for (int i = 0; i < 4; i++) bell(0.05f * (float)i, 84.0f + (float)(i * 4), 0.5f, 0.18f);
        noise(0.15f, 0.4f, 7000, 9000, 4, F_BP, 0.25f, 0.02f, 0.15f);
        break;
    case SFX_ITEM: {
        static const float m[5] = { 72, 76, 79, 84, 88 };
        for (int i = 0; i < 4; i++) { brass(0.09f * (float)i, 0.11f, m[i], 0.5f); bell(0.09f * (float)i, m[i] + 12, 0.25f, 0.15f); }
        brass(0.36f, 0.55f, 84, 0.6f); brass(0.36f, 0.55f, 88, 0.4f); brass(0.36f, 0.55f, 79, 0.4f);
        bell(0.36f, 96, 0.4f, 0.4f);
        echo(0.11f, 0.25f, 1);
        break;
    }
    case SFX_CHEST:
        noise(0, 0.35f, 300, 700, 8, F_BP, 0.8f, 0.05f, 0);       /* creak */
        tone(0, 0.35f, 140, 190, 0.25f, 0.05f, 0, W_SAW, 0, 0);
        tone(0.35f, 0.08f, 900, 400, 0.6f, 0.001f, 0.02f, W_SQR, 0.3f, 0);  /* latch */
        for (int i = 0; i < 6; i++) bell(0.4f + 0.04f * (float)i, 86.0f + (float)((i * 5) % 12), 0.3f, 0.2f);
        break;
    case SFX_GOLD:
        for (int i = 0; i < 46; i++) {
            float t = (float)i * 0.03f + frnd() * 0.02f;
            float m = 86 + frnd() * 8;
            tone(t + 0.02f, 0.25f, mtof(m), mtof(m), 0.25f, 0.001f, 0.06f + 0.04f * (frnd() + 1), W_FM, 2.76f, 2.0f);
        }
        for (int i = 0; i < 6; i++) bell(0.1f * (float)i, 84.0f + (float)(i * 3), 0.3f, 0.3f);
        echo(0.09f, 0.3f, 1);
        break;
    case SFX_EAT:
        for (int i = 0; i < 3; i++) { noise(0.13f * (float)i, 0.08f, 2500, 800, 1.5f, F_BP, 1.0f, 0.003f, 0.03f); tone(0.13f * (float)i, 0.07f, 260 - (float)i * 30, 160, 0.4f, 0.01f, 0.03f, W_TRI, 0, 0); }
        break;
    case SFX_HURT:
        voice(0, 0.24f, 330, 180, 650, 1100, 1.0f, 0.01f, 0.5f);
        noise(0, 0.05f, 2000, 500, 1, F_LP, 0.5f, 0.001f, 0.02f);
        break;
    case SFX_FAINT:
        tone(0, 0.9f, 900, 180, 0.6f, 0.01f, 0, W_SIN, 0, 0);
        tone(0, 0.9f, 905, 182, 0.3f, 0.01f, 0, W_TRI, 0, 0);
        break;
    case SFX_ENEMYHIT:
        tone(0, 0.14f, 240, 60, 1.0f, 0.001f, 0.06f, W_SIN, 0, 0);
        noise(0, 0.07f, 4000, 900, 1, F_LP, 0.9f, 0.001f, 0.025f);
        tone(0, 0.05f, 900, 300, 0.3f, 0.001f, 0.02f, W_SQR, 0.5f, 0);
        break;
    case SFX_POOF:
        noise(0, 0.35f, 3000, 300, 1, F_LP, 1.0f, 0.005f, 0.1f);
        tone(0, 0.2f, 500, 1200, 0.25f, 0.005f, 0.06f, W_SIN, 0, 0);
        break;
    case SFX_DIZZY:
        for (int i = 0; i < 4; i++) tone(0.12f * (float)i, 0.09f, 2600 + (float)(i & 1) * 600, 3300, 0.35f, 0.005f, 0.04f, W_SIN, 0, 0);
        break;
    case SFX_BAT:
        for (int i = 0; i < 3; i++) tone(0.06f * (float)i, 0.05f, 4200, 3000, 0.4f, 0.002f, 0.02f, W_FM, 1.5f, 3.0f);
        noise(0, 0.25f, 600, 900, 2, F_BP, 0.3f, 0.02f, 0.08f);
        break;
    case SFX_CRAB:
        noise(0, 0.02f, 3000, 3000, 6, F_BP, 1.0f, 0.0005f, 0.006f);
        noise(0.08f, 0.02f, 3500, 3500, 6, F_BP, 1.0f, 0.0005f, 0.006f);
        break;
    case SFX_SQUEAK:
        tone(0, 0.08f, 2600, 3300, 0.4f, 0.005f, 0, W_SIN, 0, 0);
        tone(0.11f, 0.07f, 2800, 3500, 0.35f, 0.005f, 0, W_SIN, 0, 0);
        break;
    case SFX_RATTLE:
        for (int i = 0; i < 9; i++) noise(0.045f * (float)i + frnd() * 0.01f, 0.03f, 1800 + frnd() * 600, 1200, 5, F_BP, 0.9f, 0.0005f, 0.012f);
        break;
    case SFX_HEY:
        voice(0, 0.32f, 170, 230, 620, 1850, 1.0f, 0.02f, 0.8f);
        voice(0, 0.32f, 172, 232, 2600, 3000, 0.2f, 0.02f, 0.8f);
        break;
    case SFX_DART:
        noise(0, 0.12f, 6000, 2500, 2, F_BP, 0.6f, 0.002f, 0.05f);
        tone(0, 0.03f, 1200, 600, 0.4f, 0.001f, 0.01f, W_SQR, 0.5f, 0);
        break;
    case SFX_SPIKES:
        noise(0, 0.12f, 6000, 6000, 1, F_HP, 0.5f, 0.001f, 0.04f);
        tone(0, 0.35f, 2100, 2000, 0.25f, 0.001f, 0.1f, W_FM, 1.41f, 2.5f);
        tone(0, 0.05f, 300, 120, 0.5f, 0.001f, 0.02f, W_SIN, 0, 0);
        break;
    case SFX_FLAME:
        noise(0, 0.7f, 300, 2200, 0.8f, F_LP, 1.0f, 0.06f, 0.25f);
        noise(0.05f, 0.5f, 2500, 1000, 1, F_BP, 0.3f, 0.05f, 0.2f);
        break;
    case SFX_RUMBLE:
        noise(0, 1.6f, 140, 90, 1.5f, F_LP, 2.2f, 0.08f, 0.6f);
        tone(0, 1.4f, 48, 38, 0.5f, 0.1f, 0.5f, W_SIN, 0, 0);
        for (int i = 0; i < 8; i++) noise(0.15f * (float)i, 0.1f, 600, 300, 1, F_LP, 0.4f, 0.005f, 0.04f);
        break;
    case SFX_CRUMBLE:
        for (int i = 0; i < 12; i++) noise(0.04f * (float)i + frnd() * 0.02f, 0.05f, 1500 + frnd() * 800, 400, 1.5f, F_BP, 0.7f, 0.001f, 0.02f);
        noise(0, 0.5f, 200, 100, 1, F_LP, 0.9f, 0.02f, 0.2f);
        break;
    case SFX_FALL:
        tone(0, 0.7f, 1300, 160, 0.5f, 0.01f, 0, W_SIN, 0, 0);
        tone(0.68f, 0.2f, 120, 45, 0.9f, 0.001f, 0.06f, W_SIN, 0, 0);
        noise(0.68f, 0.15f, 1200, 300, 1, F_LP, 0.6f, 0.001f, 0.05f);
        break;
    case SFX_SPLASH:
        noise(0, 0.6f, 3500, 500, 1.3f, F_BP, 1.0f, 0.004f, 0.18f);
        noise(0.05f, 0.4f, 6000, 2000, 2, F_HP, 0.3f, 0.01f, 0.12f);
        for (int i = 0; i < 5; i++) tone(0.05f + 0.05f * (float)i, 0.05f, 900 + frnd() * 500, 1600, 0.12f, 0.001f, 0.02f, W_SIN, 0, 0);
        break;
    case SFX_DOOR:
        for (int i = 0; i < 14; i++) noise(0.09f * (float)i, 0.11f, 250 + frnd() * 80, 180, 2, F_BP, 1.0f, 0.01f, 0.05f);
        noise(0, 1.3f, 120, 80, 1.5f, F_LP, 1.2f, 0.05f, 0);
        tone(1.25f, 0.3f, 90, 40, 1.0f, 0.001f, 0.1f, W_SIN, 0, 0);
        break;
    case SFX_LEVER:
        tone(0, 0.08f, 300, 120, 0.8f, 0.001f, 0.03f, W_SQR, 0.3f, 0);
        noise(0, 0.05f, 2000, 800, 2, F_BP, 0.5f, 0.001f, 0.02f);
        break;
    case SFX_CLICK:
        noise(0, 0.02f, 4000, 4000, 4, F_BP, 0.8f, 0.0005f, 0.006f);
        tone(0.01f, 0.1f, 160, 90, 0.7f, 0.001f, 0.04f, W_SIN, 0, 0);
        break;
    case SFX_PUSH:
        for (int i = 0; i < 6; i++) noise(0.045f * (float)i, 0.06f, 500 + frnd() * 100, 350, 2, F_BP, 0.8f, 0.01f, 0.03f);
        break;
    case SFX_SOLVED: {
        static const float m[6] = { 72, 76, 79, 83, 86, 91 };
        for (int i = 0; i < 6; i++) bell(0.08f * (float)i, m[i], 0.45f, 0.35f);
        noise(0.4f, 0.6f, 8000, 9000, 3, F_BP, 0.2f, 0.05f, 0.25f);
        echo(0.13f, 0.3f, 1);
        break;
    }
    case SFX_WRONG:
        tone(0, 0.18f, 196, 190, 0.5f, 0.005f, 0, W_SQR, 0.5f, 0);
        tone(0.2f, 0.32f, 147, 140, 0.5f, 0.005f, 0, W_SQR, 0.5f, 0);
        break;
    case SFX_BONE:      /* a hollow bone marimba note at 330 Hz; pitched by playback */
        tone(0, 0.9f, 330, 330, 0.7f, 0.001f, 0.22f, W_FM, 4.0f, 1.8f);
        tone(0, 0.6f, 330, 330, 0.6f, 0.001f, 0.3f, W_SIN, 0, 0);
        tone(0, 0.2f, 1320, 1320, 0.2f, 0.001f, 0.03f, W_SIN, 0, 0);
        noise(0, 0.02f, 3000, 3000, 3, F_BP, 0.3f, 0.0005f, 0.005f);
        echo(0.12f, 0.3f, 1);
        break;
    case SFX_IGNITE:
        noise(0, 0.4f, 400, 2500, 0.8f, F_LP, 1.0f, 0.03f, 0.15f);
        for (int i = 0; i < 8; i++) noise(0.1f + 0.04f * (float)i, 0.015f, 3000, 3000, 3, F_BP, 0.4f, 0.0005f, 0.004f);
        break;
    case SFX_BLIP:
        tone(0, 0.035f, 520, 520, 0.6f, 0.002f, 0.015f, W_TRI, 0, 0);
        tone(0, 0.03f, 1040, 1040, 0.15f, 0.002f, 0.01f, W_SQR, 0.5f, 0);
        break;
    case SFX_MENU:
        tone(0, 0.05f, 900, 900, 0.5f, 0.001f, 0.02f, W_TRI, 0, 0);
        break;
    case SFX_SELECT:
        bell(0, 79, 0.5f, 0.1f); bell(0.07f, 86, 0.5f, 0.2f);
        break;
    case SFX_PAUSE:
        bell(0, 84, 0.4f, 0.2f); bell(0.1f, 79, 0.4f, 0.3f);
        break;
    case SFX_THUNDER:
        noise(0, 0.25f, 3000, 400, 0.8f, F_LP, 1.3f, 0.002f, 0.12f);
        for (int i = 0; i < 18; i++) {
            float t = 0.1f + (float)i * 0.17f + frnd() * 0.07f;
            noise(t, 0.6f, 260 + frnd() * 140, 70, 1.2f, F_LP, 1.4f * (1.0f - (float)i / 22.0f), 0.04f, 0.25f);
        }
        noise(0, 3.6f, 90, 50, 1, F_LP, 1.2f, 0.3f, 1.4f);
        break;
    case SFX_CRACK:
        noise(0, 0.5f, 9000, 1500, 0.7f, F_HP, 1.4f, 0.0005f, 0.09f);
        noise(0, 0.25f, 3000, 600, 0.8f, F_LP, 1.2f, 0.0005f, 0.07f);
        for (int i = 0; i < 10; i++) noise(0.03f * (float)i, 0.01f, 6000, 6000, 1, F_HP, 0.8f, 0.0005f, 0.003f);
        break;
    case SFX_WAVE:
        noise(0, 0.6f, 400, 2500, 0.8f, F_BP, 0.6f, 0.45f, 0);
        noise(0.55f, 2.4f, 2500, 500, 0.8f, F_BP, 0.8f, 0.01f, 0.7f);
        noise(0.55f, 2.0f, 6000, 3000, 1, F_HP, 0.25f, 0.02f, 0.5f);
        break;
    case SFX_BREAK:
        for (int i = 0; i < 7; i++) noise(0.025f * (float)i + frnd() * 0.01f, 0.05f, 1800 + frnd() * 900, 500, 2, F_BP, 0.8f, 0.0005f, 0.02f);
        tone(0, 0.12f, 220, 90, 0.7f, 0.001f, 0.05f, W_TRI, 0, 0);
        break;
    case SFX_RUSTLE:
        for (int i = 0; i < 10; i++) noise(0.03f * (float)i, 0.06f, 5000 + frnd() * 1500, 3000, 1, F_HP, 0.5f, 0.005f, 0.02f);
        break;
    case SFX_SWING:
        noise(0, 0.55f, 300, 1500, 2, F_BP, 0.8f, 0.2f, 0);
        tone(0, 0.55f, 600, 900, 0.15f, 0.2f, 0, W_SIN, 0, 0);
        break;
    case SFX_LAND:
        tone(0, 0.12f, 130, 60, 0.9f, 0.001f, 0.04f, W_SIN, 0, 0);
        noise(0, 0.08f, 1500, 400, 1, F_LP, 0.5f, 0.001f, 0.03f);
        break;
    case SFX_BOTTLE:
        noise(0, 0.12f, 8000, 4000, 1, F_HP, 1.0f, 0.0005f, 0.04f);
        for (int i = 0; i < 7; i++) tone(0.02f + 0.03f * (float)i, 0.25f, 2500 + frnd() * 2500, 2500, 0.2f, 0.001f, 0.05f, W_FM, 2.3f, 1.0f);
        break;
    case SFX_ROAR:
        voice(0, 0.75f, 95, 120, 600, 1750, 1.2f, 0.06f, 1.5f);
        voice(0, 0.75f, 96, 121, 300, 900, 0.7f, 0.06f, 1.5f);
        noise(0, 0.7f, 400, 300, 1, F_LP, 0.3f, 0.05f, 0);
        break;
    case SFX_FANFARE: {
        static const float mel[6] = { 67, 72, 76, 79, 76, 79 };
        static const float tm[6] = { 0, 0.12f, 0.24f, 0.36f, 0.6f, 0.72f };
        for (int i = 0; i < 6; i++) brass(tm[i], i == 5 ? 0.9f : 0.11f, mel[i], 0.5f);
        brass(0.72f, 0.9f, 72, 0.35f); brass(0.72f, 0.9f, 76, 0.3f); brass(0.72f, 0.9f, 60, 0.4f);
        for (int i = 0; i < 4; i++) tone(0.72f + 0.2f * (float)i, 0.15f, 90, 55, 0.6f, 0.001f, 0.06f, W_SIN, 0, 0);
        bell(0.72f, 91, 0.3f, 0.5f);
        echo(0.12f, 0.25f, 1);
        break;
    }
    case SFX_GAMEOVER: {            /* wah wah wah waaaah */
        static const float m[4] = { 55, 54, 53, 52 };
        for (int i = 0; i < 4; i++) {
            float d = i == 3 ? 1.2f : 0.38f;
            voice(0.42f * (float)i, d, mtof(m[i]), mtof(m[i]) * (i == 3 ? 0.97f : 0.995f), 500, 900, 0.9f, 0.04f, 0.2f);
            tone(0.42f * (float)i, d, mtof(m[i] - 12), mtof(m[i] - 12), 0.3f, 0.04f, 0, W_SAW, 0, 0);
        }
        break;
    }
    case SFX_CHEER: {
        static const float m[5] = { 72, 76, 79, 84, 88 };
        for (int i = 0; i < 5; i++) bell(0.06f * (float)i, m[i], 0.4f, 0.25f);
        for (int i = 0; i < 4; i++) voice(0.02f * (float)i, 0.5f, 380 + (float)i * 45, 520 + (float)i * 40, 700, 1600, 0.35f, 0.03f, 0.6f);
        for (int i = 0; i < 10; i++) noise(0.3f + 0.07f * (float)i + frnd() * 0.03f, 0.03f, 2000, 1500, 1.5f, F_BP, 0.5f, 0.001f, 0.01f);   /* claps */
        break;
    }
    case SFX_CREAK:
        noise(0, 0.6f, 280, 420, 10, F_BP, 0.7f, 0.08f, 0);
        tone(0, 0.6f, 110, 150, 0.2f, 0.08f, 0, W_SAW, 0, 0);
        break;
    case SFX_DRIP:
        tone(0, 0.12f, 1900, 900, 0.6f, 0.001f, 0.035f, W_SIN, 0, 0);
        echo(0.18f, 0.35f, 1);
        break;
    case SFX_CRANE:
        for (int i = 0; i < 16; i++) { tone(0.1f * (float)i, 0.04f, 700, 400, 0.4f, 0.001f, 0.015f, W_SQR, 0.3f, 0); noise(0.1f * (float)i, 0.03f, 3000, 2000, 3, F_BP, 0.4f, 0.0005f, 0.01f); }
        noise(0, 1.7f, 160, 120, 2, F_LP, 0.9f, 0.1f, 0);
        tone(1.6f, 0.35f, 110, 50, 1.0f, 0.001f, 0.12f, W_SIN, 0, 0);
        break;
    case SFX_HEAL: bell(0, 91, 0.3f, 0.15f); bell(0.05f, 96, 0.25f, 0.2f); break;
    case SFX_THROW: noise(0, 0.25f, 600, 2400, 2, F_BP, 0.7f, 0.08f, 0); break;
    case SFX_BURP:
        voice(0, 0.55f, 92, 70, 450, 800, 1.3f, 0.02f, 4.0f);
        voice(0, 0.55f, 46, 35, 300, 600, 0.5f, 0.02f, 4.0f);
        break;
    }
}

static void commit(int id)
{
    int n = mixend;
    if (n < 16) n = 16;
    n = (n + 15) & ~15;
    if (n > MAXLEN) n = MAXLEN;
    float peak = 1e-6f;
    for (int i = 0; i < n; i++) if (fabsf(mix[i]) > peak) peak = fabsf(mix[i]);
    float g = 0.95f / peak;
    if (pool_used + n > POOL) n = POOL - pool_used;
    if (n < 16) { sfx_ptr[id] = pool; sfx_len[id] = 16; return; }
    int16_t *d = pool + pool_used;
    for (int i = 0; i < n; i++) {
        float v = mix[i] * g;
        float fade = i > n - 64 ? (float)(n - i) / 64.0f : 1.0f;
        d[i] = (int16_t)(v * fade * 32000.0f);
    }
    sfx_ptr[id] = d; sfx_len[id] = n;
    pool_used += n;
}

/* ---- ambience loops ---- */
static void make_amb(int id)
{
    int n = id == AMB_RAIN ? AMB_RATE * 6 : AMB_RATE * 8;
    n &= ~15;
    float *buf = (float *)malloc(sizeof(float) * (size_t)n);
    if (!buf) return;
    float sr = (float)AMB_RATE;
    float lp1 = 0, lp2 = 0, hp = 0, b = 0, l = 0;
    for (int i = 0; i < n; i++) buf[i] = 0;
    if (id == AMB_RAIN) {
        for (int i = 0; i < n; i++) {
            float w = frnd();
            lp1 += (w - lp1) * 0.35f;                 /* hiss */
            hp = lp1 - lp2; lp2 += (lp1 - lp2) * 0.04f;
            float body = 0; l += (w - l) * 0.02f; body = l;
            float swell = 0.8f + 0.2f * sinf((float)i / sr * 0.9f) * sinf((float)i / sr * 0.37f);
            buf[i] = (hp * 0.55f + body * 1.4f) * swell;
        }
        /* individual drops on leaves and roofs */
        for (int k = 0; k < 900; k++) {
            int s = (int)((frnd() * 0.5f + 0.5f) * (float)n);
            float f = 1500 + (frnd() + 1) * 2500, ph = 0, a = 0.05f + (frnd() + 1) * 0.04f;
            for (int i = 0; i < 300; i++) { buf[(s + i) % n] += sinf(ph) * a * expf(-(float)i / 50.0f); ph += 6.2832f * f / sr; }
        }
    } else {
        for (int i = 0; i < n; i++) {
            float w = frnd();
            l += (w - l) * 0.012f;
            b += (l - b) * 0.05f;
            float t = (float)i / sr;
            float drone = sinf(6.2832f * 55.0f * t) * 0.05f * (0.6f + 0.4f * sinf(6.2832f * t / 8.0f)) + sinf(6.2832f * 82.5f * t + sinf(t * 0.785398f)) * 0.025f;
            buf[i] = b * 3.0f + drone;
        }
        for (int k = 0; k < 14; k++) {                /* far-off drips with long echoes */
            int s = (int)((frnd() * 0.5f + 0.5f) * (float)n);
            float f0 = 1400 + (frnd() + 1) * 600;
            for (int e = 0; e < 4; e++) {
                float ph = 0, f = f0, a = 0.18f * powf(0.45f, (float)e);
                int so = s + e * (int)(0.21f * sr);
                for (int i = 0; i < 1500; i++) { buf[(so + i) % n] += sinf(ph) * a * expf(-(float)i / 300.0f); ph += 6.2832f * f / sr; f *= 0.99985f; }
            }
        }
    }
    float peak = 1e-6f;
    for (int i = 0; i < n; i++) if (fabsf(buf[i]) > peak) peak = fabsf(buf[i]);
    int16_t *d = (int16_t *)malloc(sizeof(int16_t) * (size_t)n + 64);
    if (d) {
        int16_t *al = (int16_t *)(((uintptr_t)d + 31) & ~(uintptr_t)31);
        for (int i = 0; i < n; i++) al[i] = (int16_t)(buf[i] / peak * 30000.0f);
        amb[id] = al; amb_n[id] = n;
    }
    free(buf);
}

void audio_init(void)
{
    if (pool) return;
    void *raw = malloc(sizeof(int16_t) * POOL + 64);
    pool = (int16_t *)(((uintptr_t)raw + 31) & ~(uintptr_t)31);
    pool_used = 0;
    for (int id = 0; id < SFX_COUNT; id++) {
        memset(mix, 0, sizeof(mix));
        mixend = 0;
        build(id);
        commit(id);
    }
    for (int i = 0; i < AMB_COUNT; i++) make_amb(i);
}

const int16_t *audio_data(int id) { return sfx_ptr[id]; }
int audio_len(int id) { return sfx_len[id]; }
int audio_volume(int id) { return VOL[id]; }
const int16_t *amb_data(int id) { return amb[id]; }
int amb_len(int id) { return amb_n[id]; }
int amb_volume(int id) { return id == AMB_RAIN ? 110 : 120; }
