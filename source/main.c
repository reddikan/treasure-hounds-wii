/*
 * TREASURE HOUNDS - Nintendo Wii platform layer (devkitPPC + libogc).
 *
 *   video : the game's triangle list goes to the GPU (GX): every vertex is textured from the
 *           one 1024x1024 pixel-art atlas (RGBA8, nearest filtering) and tinted by its colour.
 *           Three blend modes: alpha, additive glow, and "modulate 2x" for the light map.
 *           60 fps, 16:9 anamorphic (MINUS letterboxes it inside a 4:3 picture instead).
 *   input : Wii Classic Controller (in Wii Remote 1): buttons + left stick.
 *   audio : synthesized effects through ASND, pitched and panned per event. The music loops
 *           on voice 0 and the rain / cave ambience on voice 1.
 *
 * The theme music is rendered behind a loading bar; the other five pieces are rendered on a
 * low-priority background thread while you're on the title screen.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>

#include <gccore.h>
#include <ogc/lwp_watchdog.h>
#include <wiiuse/wpad.h>
#include <asndlib.h>

#include "game.h"
#include "music.h"
#include "gfx.h"
#include "art.h"

#define FIFO_SIZE (1024 * 1024)

#define X_HOME  (1u << 16)

static GXRModeObj *rmode;
static void       *xfb[2];
static int         fbi;
static int         wide = 1;
static volatile int quit;
static Game       *game;
static GXTexObj    atlas_obj;

void render_loading(float progress);

/* ------------------------------------------------------------------ */
/* video                                                               */
/* ------------------------------------------------------------------ */
static void set_view(void)
{
    float w = (float)rmode->fbWidth, h = (float)rmode->efbHeight;
    if (wide) {
        GX_SetViewport(0, 0, w, h, 0, 1);
        GX_SetScissor(0, 0, rmode->fbWidth, rmode->efbHeight);
    } else {
        float bh = h * 0.75f, top = (h - bh) * 0.5f;
        GX_SetViewport(0, top, w, bh, 0, 1);
        GX_SetScissor(0, (u32)top, rmode->fbWidth, (u32)bh);
    }
}

/* the atlas in the GPU's tiled RGBA8 layout: 4x4 blocks, alpha+red pairs then green+blue pairs */
static void atlas_upload(void)
{
    u8 *tex = (u8 *)memalign(32, ATLAS_W * ATLAS_H * 4);
    for (int by = 0; by < ATLAS_H; by += 4)
        for (int bx = 0; bx < ATLAS_W; bx += 4) {
            u8 *blk = tex + ((by / 4) * (ATLAS_W / 4) + bx / 4) * 64;
            for (int i = 0; i < 16; i++) {
                u32 c = gfx_atlas[(by + i / 4) * ATLAS_W + bx + (i & 3)];
                blk[i * 2] = (u8)(c & 255);              /* A */
                blk[i * 2 + 1] = (u8)(c >> 24);          /* R */
                blk[32 + i * 2] = (u8)(c >> 16);         /* G */
                blk[32 + i * 2 + 1] = (u8)(c >> 8);      /* B */
            }
        }
    DCFlushRange(tex, ATLAS_W * ATLAS_H * 4);
    GX_InitTexObj(&atlas_obj, tex, ATLAS_W, ATLAS_H, GX_TF_RGBA8, GX_CLAMP, GX_CLAMP, GX_FALSE);
    GX_InitTexObjFilterMode(&atlas_obj, GX_NEAR, GX_NEAR);
    GX_InvalidateTexAll();
    GX_LoadTexObj(&atlas_obj, GX_TEXMAP0);
}

static void video_init(void)
{
    VIDEO_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    if ((rmode->viTVMode >> 2) == VI_PAL) rmode = &TVEurgb60Hz480IntDf;    /* always 60 Hz */
    rmode->viWidth = 678;
    rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - rmode->viWidth) / 2;

    xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    xfb[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    VIDEO_Configure(rmode);
    VIDEO_ClearFrameBuffer(rmode, xfb[0], COLOR_BLACK);
    VIDEO_ClearFrameBuffer(rmode, xfb[1], COLOR_BLACK);
    VIDEO_SetNextFramebuffer(xfb[0]);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    void *fifo = memalign(32, FIFO_SIZE);
    memset(fifo, 0, FIFO_SIZE);
    GX_Init(fifo, FIFO_SIZE);

    GXColor black = { 0, 0, 0, 255 };
    GX_SetCopyClear(black, 0x00FFFFFF);
    f32 yscale = GX_GetYScaleFactor(rmode->efbHeight, rmode->xfbHeight);
    u32 xfbh = GX_SetDispCopyYScale(yscale);
    GX_SetDispCopySrc(0, 0, rmode->fbWidth, rmode->efbHeight);
    GX_SetDispCopyDst(rmode->fbWidth, xfbh);
    GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight == 2 * rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));
    GX_SetPixelFmt(rmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetDispCopyGamma(GX_GM_1_0);
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetDither(GX_ENABLE);
    set_view();

    /* vertex: 2D position, colour, texture coordinate - all sent directly */
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    /* texel x vertex colour */
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GX_SetAlphaUpdate(GX_FALSE);
    GX_SetColorUpdate(GX_TRUE);
    GX_InvVtxCache();
    GX_InvalidateTexAll();

    Mtx44 proj;
    guOrtho(proj, 0, VH, 0, VW, 0, 300);
    GX_LoadProjectionMtx(proj, GX_ORTHOGRAPHIC);
    Mtx view;
    guMtxIdentity(view);
    guMtxTransApply(view, view, 0.0f, 0.0f, -50.0f);
    GX_LoadPosMtxImm(view, GX_PNMTX0);
}

static void draw_list(void)
{
    for (int s = 0; s < gfx_nseg; s++) {
        const GSeg *seg = &gfx_seg[s];
        if (seg->count < 3) continue;
        switch (seg->mode) {
        case BL_ADD:  GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR); break;
        case BL_MUL2: GX_SetBlendMode(GX_BM_BLEND, GX_BL_DSTCLR, GX_BL_SRCCLR, GX_LO_CLEAR); break;
        default:      GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR); break;
        }
        int i = seg->start, end = seg->start + seg->count;
        while (i + 2 < end) {
            int n = end - i;
            if (n > 65532) n = 65532;
            n -= n % 3;
            GX_Begin(GX_TRIANGLES, GX_VTXFMT0, (u16)n);
            for (int k = 0; k < n; k++) {
                const GVtx *v = &gfx_v[i + k];
                GX_Position2f32(v->x, v->y);
                GX_Color1u32(v->c);
                GX_TexCoord2f32(v->u, v->v);
            }
            GX_End();
            i += n;
        }
    }
}

static void present(void)
{
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(xfb[fbi], GX_TRUE);
    GX_DrawDone();
    VIDEO_SetNextFramebuffer(xfb[fbi]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    fbi ^= 1;
}

/* ------------------------------------------------------------------ */
/* input                                                               */
/* ------------------------------------------------------------------ */
static float stick_axis(int pos, int center, int lo, int hi)
{
    float d = (float)(pos - center);
    float range = d >= 0 ? (float)(hi - center) : (float)(center - lo);
    if (range < 4.0f) return 0.0f;
    float v = d / range;
    return v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
}

static u32 read_input(Input *in)
{
    WPAD_ScanPads();
    u32 h = WPAD_ButtonsHeld(0), b = 0;
    if (h & WPAD_CLASSIC_BUTTON_UP)    b |= BTN_UP;
    if (h & WPAD_CLASSIC_BUTTON_DOWN)  b |= BTN_DOWN;
    if (h & WPAD_CLASSIC_BUTTON_LEFT)  b |= BTN_LEFT;
    if (h & WPAD_CLASSIC_BUTTON_RIGHT) b |= BTN_RIGHT;
    if (h & WPAD_CLASSIC_BUTTON_A)     b |= BTN_A;
    if (h & WPAD_CLASSIC_BUTTON_B)     b |= BTN_B;
    if (h & WPAD_CLASSIC_BUTTON_X)     b |= BTN_X;
    if (h & WPAD_CLASSIC_BUTTON_Y)     b |= BTN_Y;
    if (h & (WPAD_CLASSIC_BUTTON_FULL_L | WPAD_CLASSIC_BUTTON_ZL)) b |= BTN_L;
    if (h & (WPAD_CLASSIC_BUTTON_FULL_R | WPAD_CLASSIC_BUTTON_ZR)) b |= BTN_R;
    if (h & WPAD_CLASSIC_BUTTON_PLUS)  b |= BTN_PLUS;
    if (h & WPAD_CLASSIC_BUTTON_MINUS) b |= BTN_MINUS;
    if ((h & WPAD_CLASSIC_BUTTON_HOME) || (h & WPAD_BUTTON_HOME)) b |= X_HOME;

    in->sx = in->sy = 0.0f;
    WPADData *wd = WPAD_Data(0);
    if (wd && wd->exp.type == WPAD_EXP_CLASSIC) {
        const joystick_t *js = &wd->exp.classic.ljs;
        float x = stick_axis(js->pos.x, js->center.x, js->min.x, js->max.x);
        float y = -stick_axis(js->pos.y, js->center.y, js->min.y, js->max.y);
        if (x == 0.0f && y == 0.0f && js->mag > 0.0f && js->mag <= 1.5f) {
            float a = js->ang * 3.14159265f / 180.0f;
            x = sinf(a) * js->mag; y = -cosf(a) * js->mag;
        }
        float m = sqrtf(x * x + y * y);
        if (m < 0.2f || !isfinite(m)) { x = y = 0.0f; }
        else { float k = (m > 1.0f ? 1.0f : m); k = (k - 0.2f) / 0.8f / m; x *= k; y *= k; }
        in->sx = x; in->sy = y;
    }
    return b;
}

/* ------------------------------------------------------------------ */
/* audio                                                               */
/* ------------------------------------------------------------------ */
#define MUSIC_VOICE 0
#define AMB_VOICE   1

static void play_sfx(int id, int pitch, float pan, int vol)
{
    /* voices 0 and 1 belong to the music and the ambience */
    s32 v = -1;
    for (s32 k = 2; k < MAX_SND_VOICES; k++) if (ASND_StatusVoice(k) == SND_UNUSED) { v = k; break; }
    if (v < 0) return;
    int base = audio_volume(id) * vol / 255;
    int vl = (int)((float)base * (pan > 0 ? 1.0f - pan * 0.7f : 1.0f));
    int vr = (int)((float)base * (pan < 0 ? 1.0f + pan * 0.7f : 1.0f));
    s32 rate = SFX_RATE * pitch / 128;
    if (rate > MAX_PITCH) rate = MAX_PITCH;
    ASND_SetVoice(v, VOICE_MONO_16BIT, rate, 0, (void *)audio_data(id), audio_len(id) * 2, vl, vr, NULL);
}

/* the rest of the soundtrack, rendered in the background */
static volatile int music_done;
static u8 bg_stack[256 * 1024] __attribute__((aligned(32)));
static void *music_thread(void *arg)
{
    (void)arg;
    for (int t = 1; t < MUS_TRACKS; t++) music_init_step(t);
    music_done = 1;
    return NULL;
}
static int flushed[MUS_TRACKS];
static int music_ready(int t)
{
    if (t < 0 || t >= MUS_TRACKS || !music_data(t)) return 0;    /* tracks finish one by one */
    if (!flushed[t]) { DCFlushRange((void *)music_data(t), (u32)music_len(t) * 2); flushed[t] = 1; }
    return 1;
}

static void on_reset(u32 irq, void *ctx) { (void)irq; (void)ctx; quit = 1; }
static void on_power(void) { quit = 1; }

/* ------------------------------------------------------------------ */
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    video_init();
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC);
    ASND_Init();
    ASND_Pause(0);
    SYS_SetResetCallback(on_reset);
    SYS_SetPowerCallback(on_power);

    game = (Game *)memalign(32, GAME_SIZE);
    game_init(game);                    /* paints the art, synthesizes the effects */
    atlas_upload();
    for (int id = 0; id < SFX_COUNT; id++) DCFlushRange((void *)audio_data(id), (u32)audio_len(id) * 2);
    for (int a = 0; a < AMB_COUNT; a++) if (amb_data(a)) DCFlushRange((void *)amb_data(a), (u32)amb_len(a) * 2);

    /* the main theme, behind a loading bar (rendered in a few slices so the bar moves) */
    render_loading(0.1f); draw_list(); present();
    render_loading(0.3f); draw_list(); present();
    music_init_step(0);
    render_loading(1.0f); draw_list(); present();
    lwp_t th;
    LWP_CreateThread(&th, music_thread, NULL, bg_stack, sizeof(bg_stack), 30);

    int playing = -1, amb_playing = -1;
    const float dt = 1.0f / 60.0f;
    u32 prev = 0;
    u64 last = gettime();

    while (!quit) {
        Input in;
        u32 held = read_input(&in);
        u32 pressed = held & ~prev;
        prev = held;
        if (held & X_HOME) break;
        if (pressed & BTN_MINUS) { wide = !wide; set_view(); }

        u64 now = gettime();
        float elapsed = (float)diff_usec(last, now) * 1e-6f;
        last = now;
        int steps = (int)(elapsed / dt + 0.5f);
        if (steps < 1) steps = 1;
        if (steps > 3) steps = 3;
        for (int s = 0; s < steps; s++) {
            in.held = held & 0xFFFFu;
            in.pressed = s == 0 ? (pressed & 0xFFFFu & ~BTN_MINUS) : 0;
            game_update(game, &in, dt);
        }
        if (game_quit_requested(game)) break;

        int id, pitch, vol;
        float pan;
        while ((id = game_pop_sound(game, &pitch, &pan, &vol)) >= 0) play_sfx(id, pitch, pan, vol);

        int want = game_music(game);
        if (want >= 0 && !music_ready(want)) want = -1;     /* still being composed */
        if (want != playing) {
            ASND_StopVoice(MUSIC_VOICE);
            if (want >= 0)
                ASND_SetInfiniteVoice(MUSIC_VOICE, VOICE_MONO_16BIT, MUS_RATE, 0, (void *)music_data(want),
                                      music_len(want) * 2, music_volume(want), music_volume(want));
            playing = want;
        }
        int amb = game_ambience(game);
        if (amb != amb_playing) {
            ASND_StopVoice(AMB_VOICE);
            if (amb >= 0 && amb_data(amb))
                ASND_SetInfiniteVoice(AMB_VOICE, VOICE_MONO_16BIT, AMB_RATE, 0, (void *)amb_data(amb),
                                      amb_len(amb) * 2, amb_volume(amb), amb_volume(amb));
            amb_playing = amb;
        }

        game_render(game);
        draw_list();
        present();
    }

    ASND_End();
    return 0;
}
