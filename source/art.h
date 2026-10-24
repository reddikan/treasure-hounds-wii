/*
 * TREASURE HOUNDS - procedurally painted pixel art.
 *
 * art_init() paints every tile, character, prop, portrait and the font into the
 * 1024x1024 atlas (gfx_atlas) and fills the lookup tables below. There are no
 * image files: everything is drawn in code at start-up.
 */
#ifndef ART_H
#define ART_H

#include <stdint.h>

typedef struct { uint16_t x, y; uint8_t w, h; int8_t ax, ay; } ArtRect;   /* ax, ay: anchor (feet) inside the rect */
typedef struct { uint16_t x, y; uint8_t w; } ArtGlyph;

/* ---- 16x16 ground / wall tiles, 4 variants (or animation frames) each ---- */
enum {
    TA_GRASS, TA_DIRT, TA_COBBLE, TA_PLANK, TA_SAND, TA_WETSAND, TA_CAVE, TA_MUD, TA_SLAB, TA_MOSS, TA_DECK,
    TA_WATER, TA_CAVEWATER, TA_SHALLOW, TA_PIT,
    TA_ROCK_TOP, TA_ROCK_FACE, TA_CAVE_TOP, TA_CAVE_FACE, TA_BRICK_TOP, TA_BRICK_FACE,
    TA_WOOD_FACE, TA_WINDOW, TA_ROOF, TA_HULL, TA_HEDGE_TOP, TA_HEDGE_FACE,
    TA_ROCK_FACEL, TA_CAVE_FACEL, TA_BRICK_FACEL,   /* lower half of two-tile-tall wall faces */
    TA_SPIKES,       /* 0 down, 1 rising, 2 up */
    TA_CRUMBLE,      /* 0 whole, 1 cracked, 2 crumbling, 3 hole */
    TA_PLATE,        /* 0 up, 1 pressed */
    TA_GRATE,        /* 0 closed, 1 open */
    TA_RUG,          /* carpet in the cabin */
    TA_FRINGE_GRASS, /* grass spilling over a neighbouring tile: 0 from above, 1 below, 2 left, 3 right */
    TA_FRINGE_SAND,
    TA_COUNT
};
extern ArtRect art_tile[TA_COUNT][4];

/* ---- characters ---- */
enum { CH_SAM, CH_ROCCO, CH_GUS, CH_OLLIE, CH_VIC, CH_LENNY, CH_MA, CH_TINY, CH_COUNT };
enum { DIR_DOWN, DIR_UP, DIR_SIDE };               /* side faces right; drawn flipped for left */
enum {
    FR_IDLE, FR_IDLE2, FR_WALK0, FR_WALK1, FR_WALK2, FR_WALK3,
    FR_WHIP0, FR_WHIP1, FR_WHIP2, FR_HURT, FR_CHEER0, FR_CHEER1, FR_DIZZY,
    FR_COUNT
};
extern ArtRect art_char[CH_COUNT][3][FR_COUNT];

/* portraits for the dialog box: 3 moods */
enum { MOOD_NORMAL, MOOD_HAPPY, MOOD_SHOCK, MOOD_COUNT };
extern ArtRect art_face[CH_COUNT + 1][MOOD_COUNT];  /* +1: the pirate captain's skull */
#define FACE_SKULL CH_COUNT

/* ---- creatures ---- */
enum { CR_BAT, CR_CRAB, CR_RAT, CR_SKEL, CR_COUNT };
extern ArtRect art_crit[CR_COUNT][3][6];             /* [type][dir][frame] (bats/crabs ignore dir) */

/* ---- props ---- */
enum {
    SP_PINE, SP_PALM, SP_BUSH, SP_BUSH_CUT, SP_ROCK, SP_ROCK_BIG, SP_BARREL, SP_CRATE, SP_CRATE_BROKEN,
    SP_LAMPPOST, SP_TORCH, SP_BRAZIER, SP_BRAZIER_LIT, SP_CAMPFIRE, SP_BENCH, SP_TABLE, SP_FENCE, SP_SIGN,
    SP_ANCHOR, SP_ROWBOAT, SP_WRECK_BOW, SP_WRECK_MAST, SP_SKULLS, SP_BONES, SP_GOLDPILE,
    SP_CHEST, SP_CHEST_OPEN, SP_BIGCHEST, SP_BIGCHEST_OPEN, SP_CHAIN, SP_PILLAR, SP_PILLAR_BROKEN,
    SP_SKULLWALL, SP_LIGHTHOUSE, SP_SHACK, SP_WELL, SP_CANNON, SP_POT, SP_POT_BROKEN, SP_NETS, SP_CRANE,
    SP_LEVER_L, SP_LEVER_R, SP_GATE, SP_GATE_OPEN, SP_BLOCK, SP_BONEPIPE, SP_DARTER, SP_FLAMEJET,
    SP_BOULDER, SP_HOOK, SP_STATUE, SP_MAST, SP_WHEEL, SP_LANTERN_HANG, SP_GRAVE, SP_TOTEM, SP_FERN,
    SP_MUSHROOM, SP_CRYSTAL, SP_STALAG, SP_SHELL, SP_STARFISH, SP_PUDDLE, SP_LILY, SP_CARVING, SP_CAGE,
    SP_COUNT
};
extern ArtRect art_spr[SP_COUNT];

/* ---- items and HUD icons ---- */
enum {
    IT_COIN, IT_COIN2, IT_COIN3, IT_COIN4, IT_GEM_R, IT_GEM_B, IT_GEM_G, IT_GEAR, IT_SKULLKEY, IT_DOUBLOON,
    IT_HEART, IT_HEART_HALF, IT_HEART_EMPTY, IT_CANDY, IT_PIZZA, IT_LANTERN, IT_WHIP, IT_MAP, IT_ROPE,
    IT_DART, IT_BOTTLE, IT_STAR, IT_MAPSCRAP, IT_KEY, IT_COUNT
};
extern ArtRect art_item[IT_COUNT];

extern ArtRect  art_icon[8];          /* text icons: A B X Y L R + coin */
extern ArtGlyph art_font[128];         /* 9 px tall (7 + 2 descender) */
extern ArtRect  art_logo;             /* "TREASURE HOUNDS" title lettering */
extern ArtRect  art_parch;            /* 16x16 parchment 9-slice for dialog boxes */
extern ArtRect  art_frame;            /* 16x16 dark wood 9-slice */

void art_init(void);

#endif
