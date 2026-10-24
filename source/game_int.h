/*
 * TREASURE HOUNDS - game internals shared by the game's C files.
 */
#ifndef GAME_INT_H
#define GAME_INT_H

#include <stdint.h>
#include "game.h"
#include "gfx.h"
#include "art.h"
#include "maps.h"
#include "dialog.h"

#define TS       16
#define MAXW     96
#define MAXH     64
#define MAX_ENT  1100
#define MAX_PART 900
#define MAX_LIGHT 72
#define MAX_SND  32
#define NKIDS    4
#define KID_HP   6
#define TRAIL_N  256
#define TRAIL_GAP 7          /* trail samples (2 px each) between kids */
#define ZOOM     2.0f        /* world texels -> virtual screen pixels */
#define VIEW_W   (VW / ZOOM)
#define VIEW_H   (VH / ZOOM)

#define PI_F  3.14159265f
#define TAU_F 6.28318531f

/* ---- terrain ---- */
enum {
    K_VOID, K_GRASS, K_DIRT, K_COBBLE, K_PLANK, K_SAND, K_WETSAND, K_CAVE, K_MUD, K_SLAB, K_MOSS, K_DECK, K_RUG,
    K_SEA, K_POOL, K_SHALLOW, K_ROCK, K_CAVEWALL, K_BRICK, K_WOOD, K_HULL, K_HEDGE,
    K_SPIKES, K_CRUMBLE, K_PLATE, K_GRATE, K_HOLE,
    K_COUNT
};
typedef struct {
    uint8_t solid, wall, water, floor_art, top_art, face_art, step;   /* step: footstep sound flavour */
    uint8_t face2_art;   /* lower half of a two-tile-tall face */
} KindInfo;
extern const KindInfo KINFO[K_COUNT];

/* ---- entities ---- */
enum {
    E_NONE,
    /* scenery */
    E_PINE, E_PALM, E_BUSH, E_ROCK, E_ROCKBIG, E_BARREL, E_CRATE, E_POT, E_LAMP, E_TORCH, E_FIRE, E_BENCH, E_TABLE,
    E_FENCE, E_SIGN, E_ANCHOR, E_ROWBOAT, E_WRECK, E_WRECKMAST, E_SKULLS, E_BONES, E_GOLDPILE, E_CHAIN, E_PILLAR,
    E_PILLARB, E_SKULLWALL, E_LIGHTHOUSE, E_SHACK, E_WELL, E_CANNON, E_NETS, E_STATUE, E_MAST, E_WHEEL, E_HLANTERN,
    E_GRAVE, E_TOTEM, E_FERN, E_MUSHROOM, E_CRYSTAL, E_STALAG, E_SHELL, E_PUDDLE, E_SEAWEED, E_CARVING,
    /* things you use */
    E_CHEST, E_BIGCHEST, E_CRANE, E_LEVER, E_GATE, E_BLOCK, E_PIPE, E_BRAZIER, E_HOOK, E_CAGE,
    /* pickups */
    E_COIN, E_GEM, E_GEAR, E_DOUBLOON, E_FOOD, E_SKULLKEY,
    /* creatures and people */
    E_BAT, E_CRAB, E_RAT, E_SKEL, E_VIC, E_LENNY, E_MA, E_TINY,
    /* traps and missiles */
    E_DARTER, E_FLAMEJET, E_BOULDER, E_DART, E_BOTTLE,
    E_COUNT
};

/* entity flags */
#define EF_SOLID   1     /* blocks the leader (footprint fw x fh above the anchor) */
#define EF_BREAK   2     /* the whip breaks it */
#define EF_ENEMY   4
#define EF_PICKUP  8
#define EF_FLAT    16    /* drawn on the ground layer, before the y-sorted pass */
#define EF_SWAY    32    /* sways in the storm wind */
#define EF_USE     64    /* A does something next to it */
#define EF_WALLMNT 128   /* mounted on a wall tile */

typedef struct {
    int16_t spr;           /* art_spr index, or -1 */
    int8_t  fw, fh;        /* solid footprint half-width, height (px) */
    uint8_t flags;
} EntInfo;
extern const EntInfo EINFO[E_COUNT];

typedef struct {
    uint8_t type, on, state, dir;
    uint8_t flip, frame, hit_n, seen;
    int16_t hp, a, b, c;
    float x, y, z, vx, vy, vz;
    float t, t2, t3, flash;
    float hx, hy;          /* home */
} Ent;

/* ---- particles ---- */
enum {
    PK_SPARK, PK_DUST, PK_SPLASH, PK_RING, PK_EMBER, PK_COIN, PK_DEBRIS, PK_LEAF, PK_BONE, PK_SMOKE,
    PK_MOTE, PK_DRIP, PK_TEXT, PK_HEART, PK_STAR, PK_GOLD, PK_FOAM, PK_BUBBLE, PK_FLAME, PK_SHARD
};
typedef struct {
    float x, y, z, vx, vy, vz, life, max, size, rot;
    uint32_t c;
    uint8_t kind, add;
    int16_t val;
} Part;

/* ---- lights ---- */
typedef struct { float x, y, r, i; float cr, cg, cb; } Light;

/* ---- the kids ---- */
enum { KS_FOLLOW, KS_WALKTO, KS_HOLD, KS_SWING, KS_FALL, KS_CHEER };
typedef struct {
    float x, y, z, vz;
    int   dir, flip, frame;
    float anim, step_t;
    int   hp;
    float hurt, inv, regen;
    int   state;
    float sx, sy, tx, ty, mt, mdur;    /* scripted motion */
    float bubble_t, bubble_max;
    char  bubble[100];
    float blink;
} Kid;

/* ---- game states ---- */
enum { ST_TITLE, ST_STORY, ST_CHAPTER, ST_PLAY, ST_DIALOG, ST_PAUSE, ST_GAMEOVER, ST_LEVELDONE, ST_ENDING, ST_MAP };

typedef struct {
    int w, h, theme;
    uint8_t k[MAXH][MAXW];
    uint8_t v[MAXH][MAXW];
    uint8_t st[MAXH][MAXW];
    float   tt[MAXH][MAXW];          /* timers for crumbling floor */
    uint8_t seen[MAXH][MAXW];        /* explored, for the map */
} Level;

typedef struct { int16_t id; uint8_t pitch; int8_t pan; uint8_t vol; } Snd;

typedef struct { float x, y, len, spd; } Drop;

struct Game {
    int state, prev_state;
    float st_t, fade, time;
    int fade_to;                 /* state to switch to when the fade-out finishes, -1 = none */
    int level, menu, chapter_pick;
    int music_on, sound_on, quit;
    uint32_t rng;

    Level lv;
    const LevelData *ld;
    Ent ent[MAX_ENT];
    int nent;
    Part part[MAX_PART];
    int npart;
    Light light[MAX_LIGHT];
    int nlight;
    float ambient[3];

    Kid kid[NKIDS];
    int order[NKIDS];            /* order[0] is the leader */
    float trail_x[TRAIL_N], trail_y[TRAIL_N];
    int trail_head;
    float trail_acc;
    int lead_dir, lead_flip;
    float whip_t, whip_cool, push_t;
    int whip_hit_done;
    float lead_vx, lead_vy;
    float safe_x, safe_y, safe_t;
    float idle_t;
    int running;

    float camx, camy, shake, shake_t;
    float cam_tx, cam_ty;

    /* progress */
    int coins, gems, gears, doubloons, food;
    int lvl_coins, lvl_doubloons, total_doubloons;
    int has_key, boss_down, tiny_free;
    int gate_open[8];
    int organ_pos;
    float organ_fail_t;
    int braziers_lit;
    int crane_done;
    uint8_t trig_done[64];
    float chk_x, chk_y;
    int chk_hp[NKIDS];
    int deaths;

    /* dialog */
    int scene, line;
    float line_t, chars;
    int scene_after;             /* sign index for SC_SIGN */
    int after_act;               /* what happens when the current scene ends */
    float collapse_t;            /* the temple coming down */
    char wrap[400];
    int wrapped_line;
    float banter_t;
    int banter_i;
    int bubble_seq, bubble_pos;  /* running bubble conversation */
    float bubble_gap;

    /* weather */
    Drop drop[300];
    int ndrop;
    float flash, flash_t, bolt_t, bolt_x, bolt_y, thunder_t;
    uint32_t bolt_seed;
    float wind;
    float wave_t;

    /* boss / ending */
    float boss_t;
    int boss_phase;
    float end_t;
    int end_stage;

    /* ui */
    float hud_coin_t, toast_t;
    char toast[80];
    float map_open;
    float title_t;
    int story_page;

    Snd snd[MAX_SND];
    int nsnd;
};

/* game.c */
void  sfx(Game *g, int id, int pitch, float pan, int vol);
void  sfx_at(Game *g, int id, float x, float y, int pitch);
float frand(Game *g);                 /* 0..1 */
float frand2(Game *g);                /* -1..1 */
void  toast(Game *g, const char *s);
void  start_scene(Game *g, int scene);
void  checkpoint(Game *g);

/* world.c */
void  world_load(Game *g, int level);
int   tile_at(const Game *g, int tx, int ty);
int   solid_at(const Game *g, float x, float y);     /* terrain only */
int   blocked(const Game *g, float x, float y, float r, int ignore_ent);   /* terrain + solid entities */
int   walkable_tile(const Game *g, int tx, int ty);
Ent  *ent_new(Game *g, int type, float x, float y);
void  move_circle(const Game *g, float *x, float *y, float dx, float dy, float r, int ignore);

/* party.c */
void  party_reset(Game *g, float x, float y, int dir);
void  party_update(Game *g, const Input *in, float dt);
Kid  *leader(Game *g);
int   leader_id(const Game *g);
void  hurt_leader(Game *g, int dmg, float fromx, float fromy);
void  party_heal(Game *g, int amt);
void  kid_say(Game *g, int who, const char *s, float dur);
void  party_walk_to(Game *g, int k, float x, float y, float dur, int hold);
void  party_regroup(Game *g);
int   party_busy(const Game *g);
void  party_swing(Game *g, float lx, float ly, float hx, float hy);
void  party_fall(Game *g);
void  trail_rebuild(Game *g);

/* ents.c */
void  ents_update(Game *g, float dt);
void  ents_whip(Game *g, float x0, float y0, float x1, float y1);
int   ents_use(Game *g, float x, float y, int dir);   /* A pressed next to something */
void  open_gate(Game *g, int idx);
void  puzzles_check(Game *g);
void  enemy_hurt(Game *g, Ent *e, int dmg, float kx, float ky);

/* parts.c */
Part *part_new(Game *g, int kind, float x, float y);
void  parts_update(Game *g, float dt);
void  burst(Game *g, int kind, float x, float y, int n, float spd, uint32_t c);
void  float_text(Game *g, float x, float y, const char *s, uint32_t c);

/* weather.c / light */
void  weather_update(Game *g, float dt);
void  lights_collect(Game *g);

/* render.c */
void  render_world(const Game *g);

/* ui.c */
void  render_ui(const Game *g);
void  render_title(const Game *g);
void  render_story(const Game *g);
void  render_chapter(const Game *g);
void  render_pause(const Game *g);
void  render_gameover(const Game *g);
void  render_leveldone(const Game *g);
void  render_ending(const Game *g);
void  render_map(const Game *g);
void  render_dialog(const Game *g);
void  render_loading(float progress);

/* dialog.c */
void  dialog_update(Game *g, const Input *in, float dt);
void  banter_update(Game *g, float dt);
void  scene_action(Game *g, int act);

static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

#endif
