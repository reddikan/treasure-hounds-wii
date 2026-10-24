# Treasure Hounds

*~ The Black Lantern's Gold ~*

A stormy, top-down pirate-treasure adventure for the Nintendo Wii (homebrew), played with the Classic Controller.

The Shoals are getting bulldozed on Monday. Four best friends find a pirate map in Grandpa's attic and set out to find the lost gold of **Ironjaw Jack** before the weekend is over: through a storm-lashed fishing town, the old smugglers' caves, a booby-trapped temple of bones, and out to Skeleton Cove, where his ship still waits. They have a whip, a lantern, and each other. Hounds stick together!

## The Hounds

- **Sam** (red) - the leader and dreamer. It's his grandpa's map.
- **Rocco** (yellow) - backwards cap, braces, and a comeback for everything.
- **Gus** (green) - the inventor. Big glasses, bigger vocabulary.
- **Ollie** (blue) - always hungry, never scared (okay, sometimes scared), makes friends with anybody. Even a giant.

The whole gang stays together: the leader carries the lantern and the others follow in a line. Whoever's in front takes the hits; Hounds at the back of the line slowly catch their breath, so swap leaders with L / R to keep everyone going. If the leader runs out of breath, the next Hound steps up automatically. They talk the whole way: story scenes, puzzle hints, and a lot of bad jokes in speech bubbles.

## Chapters

1. **Gull's Point** - a fishing town on a stormy night: rain, lightning, a lighthouse sweeping its beam over the docks. Find three gears for the old winch to open the smugglers' grate, and stay out of the Crowley brothers' flashlights. Meet Tiny.
2. **Smugglers' Caves** - pitch dark except for the lantern. Bats, crabs, spike traps, a coin-raining wishing well, four braziers to light, a crumbling bridge, a four-plate teamwork door, swinging across a chasm with the whip, dart walls and a rolling boulder.
3. **Temple of Bones** - torchlit halls over water channels. Push stone blocks onto plates, dodge flame jets and skeleton pirates, find the lever to a hidden nook, play the bone organ, and take the Skull Key... then run.
4. **Skeleton Cove** - the storm again: a beach with an old wreck, the Crowleys' camp, and Ironjaw Jack's ship, the Black Gull. Ma Crowley is waiting on deck. Then open the chest.

Every chapter hides three of Ironjaw's golden doubloons, plus coins and gems in crates, pots, bushes and chests. The map (X) marks what you've found.

Start a new adventure, or jump to any chapter, from the title screen. Music and sound can be turned on and off on the title screen and in the pause menu.

## Controls (Classic Controller)

| Button | Action |
| --- | --- |
| Left stick / D-Pad | walk |
| Y (hold) | run |
| B | crack the whip (fight, break crates and bushes, pull levers, play bone pipes, swing on hooks) |
| A | talk / read / use / next line (light braziers, open chests, call everyone onto plates) |
| L / R | choose who leads |
| X | map |
| + | pause (resume, music on/off, sound on/off, back to checkpoint, quit to title) |
| - | 16:9 / letterboxed 4:3 |
| HOME | exit |

## Building

Push to GitHub: the Actions workflow builds `boot.dol` in the devkitPro container and uploads it (plus a ready `apps/treasurehounds/` folder for an SD card) as the `treasure-hounds-wii` artifact. Open `boot.dol` in Dolphin.

Locally with devkitPro installed: `make`.

## Tests

`sh tools/run_tests.sh` builds a native test harness (any C compiler; Pillow for PNG export). It checks every level can be finished (start, items, puzzle pieces, exits and hook landings reachable), plays scripted runs through every puzzle and the final fight, renders screenshots with a software copy of the GPU path, and measures frame cost. `sh tools/run_tests.sh audio` writes the soundtrack and every sound effect to WAV files in `tools/out/`.

The levels are drawn by `tools/maps/make_maps.py` (`make maps` rewrites `source/maps.c`).

## How it is made

- No image or sound files at all. Every tile, character (4 directions, walk / whip / hurt / cheer frames), portrait, prop and the font is painted in code at start-up into one 1024x1024 pixel-art atlas.
- The world is drawn at 2x pixel scale with smooth lighting on top: a screen-space light map from the lantern, torches, lamps, braziers, glowing mushrooms and crystals, and lightning, blended with "modulate 2x" so warm light brightens as well as darkens. Flames, glows, the lighthouse beam and the Crowleys' flashlights are added on top.
- Outdoors it storms: wind-bent rain, splashes and rings in puddles, foam on the shore, swaying trees, and lightning with thunder that rolls in a moment later.
- The soundtrack (six pieces: a swashbuckling main theme, a rainy-night shanty, mysterious caves, a tribal temple, the big finale and a victory reprise) is composed as note data and synthesized at start-up by a small orchestra of bowed strings, fiddle, musette accordion, brass, whistle, harp, bone marimba, glockenspiel, choir, timpani and drums. The theme is rendered behind the loading bar; the rest is rendered in the background while you're on the title screen.
