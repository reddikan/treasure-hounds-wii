/*
 * TREASURE HOUNDS - procedurally synthesized sound effects and ambience loops.
 * No audio files: every sound is built at start-up. 16-bit signed mono, host byte order.
 */
#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

#define SFX_RATE 32000
#define AMB_RATE 22050

enum {
    SFX_WHIP, SFX_WHIPHIT, SFX_STEP, SFX_COIN, SFX_GEM, SFX_ITEM, SFX_CHEST, SFX_GOLD, SFX_EAT, SFX_HURT,
    SFX_FAINT, SFX_ENEMYHIT, SFX_POOF, SFX_DIZZY, SFX_BAT, SFX_CRAB, SFX_SQUEAK, SFX_RATTLE, SFX_HEY,
    SFX_DART, SFX_SPIKES, SFX_FLAME, SFX_RUMBLE, SFX_CRUMBLE, SFX_FALL, SFX_SPLASH, SFX_DOOR, SFX_LEVER,
    SFX_CLICK, SFX_PUSH, SFX_SOLVED, SFX_WRONG, SFX_BONE, SFX_IGNITE, SFX_BLIP, SFX_MENU, SFX_SELECT,
    SFX_PAUSE, SFX_THUNDER, SFX_CRACK, SFX_WAVE, SFX_BREAK, SFX_RUSTLE, SFX_SWING, SFX_LAND, SFX_BOTTLE,
    SFX_ROAR, SFX_FANFARE, SFX_GAMEOVER, SFX_CHEER, SFX_CREAK, SFX_DRIP, SFX_CRANE, SFX_HEAL, SFX_THROW,
    SFX_BURP,
    SFX_COUNT
};

/* ambience loops (played on their own voice under the music) */
enum { AMB_RAIN, AMB_CAVE, AMB_COUNT };

void           audio_init(void);
const int16_t *audio_data(int id);       /* 32-byte aligned */
int            audio_len(int id);        /* samples, multiple of 16 */
int            audio_volume(int id);     /* 0..255 */
const int16_t *amb_data(int id);
int            amb_len(int id);
int            amb_volume(int id);

#endif
