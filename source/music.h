/*
 * TREASURE HOUNDS - composed and synthesized soundtrack, one seamless loop per piece:
 *   0  Treasure Hounds (title)   - swashbuckling 6/8 theme: fiddle, accordion, strings, brass, timpani
 *   1  Gull's Point              - a rainy-night sea shanty: lonely concertina, low strings, soft drum
 *   2  Smugglers' Caves          - mysterious: plucked bass ostinato, dripping glockenspiel, airy whistle
 *   3  Temple of Bones           - tribal toms, bone marimba, an eerie flute over a choir pad
 *   4  Skeleton Cove             - the big finale: fast strings, brass fanfares, pounding timpani
 *   5  The Gold!                 - triumphant reprise of the main theme with bells
 */
#ifndef MUSIC_H
#define MUSIC_H

#include <stdint.h>

#define MUS_RATE   22050
#define MUS_TRACKS 6
enum { MUS_TITLE, MUS_TOWN, MUS_CAVES, MUS_TEMPLE, MUS_COVE, MUS_VICTORY };

void           music_init(void);            /* compose + render every track (call once) */
int            music_init_step(int i);      /* render track i; returns 1 when all are done (for a progress bar) */
const int16_t *music_data(int track);
int            music_len(int track);
int            music_volume(int track);

#endif
