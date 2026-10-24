/*
 * TREASURE HOUNDS - a stormy top-down pirate treasure adventure for the Wii.
 *
 * Everything except main.c is plain C99 and knows nothing about the console: the game takes
 * a BTN_* bitmask plus the left stick, fills the gfx triangle list, and emits sound events
 * and music requests that the platform layer plays.
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include "audio.h"

enum {
    BTN_UP    = 1 << 0,
    BTN_DOWN  = 1 << 1,
    BTN_LEFT  = 1 << 2,
    BTN_RIGHT = 1 << 3,
    BTN_A     = 1 << 4,     /* talk / use / next line   */
    BTN_B     = 1 << 5,     /* crack the whip           */
    BTN_X     = 1 << 6,     /* map                      */
    BTN_Y     = 1 << 7,     /* hold to run              */
    BTN_L     = 1 << 8,     /* previous leader          */
    BTN_R     = 1 << 9,     /* next leader              */
    BTN_PLUS  = 1 << 10,    /* pause                    */
    BTN_MINUS = 1 << 11
};

typedef struct {
    uint32_t held, pressed;
    float    sx, sy;        /* left stick -1..1, y down is + */
} Input;

typedef struct Game Game;
extern const int GAME_SIZE;

void game_init(Game *g);
void game_update(Game *g, const Input *in, float dt);
void game_render(const Game *g);
int  game_pop_sound(Game *g, int *pitch, float *pan, int *vol);   /* -1 when empty; pitch 128 = normal, vol 0..255 */
int  game_music(const Game *g);                                   /* track (music.h) or -1 */
int  game_ambience(const Game *g);                                /* AMB_* loop or -1 */
int  game_quit_requested(const Game *g);

#endif
