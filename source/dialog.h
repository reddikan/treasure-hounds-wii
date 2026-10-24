/*
 * TREASURE HOUNDS - the script: story scenes, puzzle lines, and the kids' banter.
 */
#ifndef DIALOG_H
#define DIALOG_H

#include <stdint.h>

#define SPK_NARR  (-1)
#define SPK_SIGN  (-2)    /* an old inscription: the pirate skull portrait */
#define SPK_ALL   (-3)    /* all four kids at once */

typedef struct { int8_t who, mood; int16_t act; const char *text; } Line;

enum {
    ACT_NONE, ACT_SHAKE, ACT_THUNDER, ACT_CHEER, ACT_OPEN_GRATE, ACT_TINY_LOVE, ACT_TINY_ENTER, ACT_BOSS_START,
    ACT_DUNK, ACT_CHEST_OPEN, ACT_COLLAPSE, ACT_ENDING, ACT_LEVEL_DONE, ACT_SPREAD, ACT_RUMBLE, ACT_FANFARE
};

enum {
    SC_NONE,
    SC_L1_INTRO, SC_L1_TUTORIAL, SC_L1_SQUARE, SC_L1_GARDEN, SC_L1_INGARDEN, SC_L1_CROWLEYS, SC_L1_TINY, SC_L1_PLATEAU,
    SC_L1_DOCKS, SC_CRANE_NEED, SC_CRANE_DONE,
    SC_L2_INTRO, SC_L2_WELL, SC_L2_RIVER, SC_L2_SPIKES, SC_L2_BRAZIERS, SC_L2_BRAZIERS_DONE, SC_L2_CHASM,
    SC_L2_PLATES, SC_L2_PLATES_DONE, SC_L2_LENNY, SC_L2_HOOKS, SC_L2_DARTS, SC_L2_BOULDER_PRE,
    SC_L3_INTRO, SC_L3_COURT, SC_L3_BLOCKS, SC_L3_BLOCKS_DONE, SC_L3_FLAMES, SC_L3_ORGAN, SC_L3_ORGAN_DONE,
    SC_L3_VAULT, SC_L3_KEY, SC_L3_ESCAPE,
    SC_L4_INTRO, SC_L4_WRECK, SC_L4_CAMP, SC_L4_SHIP, SC_L4_BOARD, SC_L4_BOSS_DOWN, SC_L4_CHEST,
    SC_EXIT, SC_EXIT_L1, SC_EXIT_L2, SC_EXIT_L3, SC_SIGN, SC_CARVING, SC_GATE_LOCKED, SC_ORGAN_WRONG,
    SC_COUNT
};

extern const Line *const SCENES[SC_COUNT];

/* banter: little two- or three-line exchanges shown as speech bubbles while you explore */
typedef struct { int8_t who[3]; const char *text[3]; } Banter;
extern const Banter BANTER_TOWN[], BANTER_CAVE[], BANTER_TEMPLE[], BANTER_COVE[];
extern const int NBANTER_TOWN, NBANTER_CAVE, NBANTER_TEMPLE, NBANTER_COVE;

/* one-liners */
extern const char *const SAY_HURT[4][4];
extern const char *const SAY_LEAD[4][3];
extern const char *const SAY_FAINT[4];
extern const char *const SAY_THUNDER[4][3];
extern const char *const SAY_FOOD[4];
extern const char *const SAY_DOUBLOON[4];
extern const char *const SAY_DIZZY_VIC[3];
extern const char *const SAY_DIZZY_LENNY[3];
extern const char *const SAY_SPOT_VIC[3];
extern const char *const SAY_SPOT_LENNY[3];

extern const char *const STORY_PAGES[];
extern const int NSTORY;
extern const char *const ENDING_PAGES[];
extern const int NENDING;
extern const char *const CHAR_NAME[];

#endif
