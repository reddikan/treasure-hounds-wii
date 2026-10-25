/*
 * TREASURE HOUNDS - the script. Every line the kids say lives here.
 */
#include "dialog.h"
#include "art.h"

#define L_(w, m, t)      { w, m, ACT_NONE, t }
#define LA(w, m, a, t)   { w, m, a, t }
#define END              { 0, 0, -1, 0 }

#define SAM   CH_SAM
#define ROC   CH_ROCCO
#define GUS   CH_GUS
#define OLL   CH_OLLIE
#define VIC   CH_VIC
#define LEN   CH_LENNY
#define MA    CH_MA
#define TINY  CH_TINY
#define NARR  SPK_NARR
#define ALL   SPK_ALL
#define N 0
#define H 1
#define X 2

const char *const CHAR_NAME[] = { "Sam", "Rocco", "Gus", "Ollie", "Vic Crowley", "Lenny Crowley", "Ma Crowley", "Tiny" };

const char *const STORY_PAGES[] = {
    "Gull's Point.\nA little fishing town on a stormy coast.",
    "Down by the docks is the Shoals: a crooked old\nneighborhood where four best friends grew up.",
    "But on Monday, Thornbury Development is\nbulldozing the Shoals to build a golf course.\nEvery family has to move away.",
    "Unless somebody finds a whole lot of money.\nFast.",
    "Like... say... the lost gold of\n{IRONJAW JACK}, the pirate.",
};
const int NSTORY = 5;

const char *const ENDING_PAGES[] = {
    "With Ironjaw Jack's gold, every family in the Shoals\npaid off their houses. Thornbury Development\npacked up their bulldozers and left town.",
    "The Harbor Patrol fished the Crowleys out of\nSkeleton Cove at sunrise. Ma is still yelling.",
    "Tiny moved in with Ollie's family.\nHe eats a LOT. Nobody minds.",
    "And every summer, when the storms roll in,\nfour best friends still go hunting for treasure.",
};
const int NENDING = 4;

/* ---------------- LEVEL 1 ---------------- */
static const Line l1_intro[] = {
    L_(NARR, N, "Friday night. The last weekend before the Shoals get bulldozed. The storm is just getting started..."),
    L_(SAM, H, "Guys. GUYS. Look what I found in Grandpa's attic!"),
    L_(ROC, N, "If it's another jar of his toenail clippings, I'm going home."),
    L_(SAM, H, "It's a MAP. A real pirate map. Ironjaw Jack's treasure!"),
    L_(GUS, X, "Ironjaw Jack? The pirate with the iron jaw?"),
    L_(OLL, N, "So he couldn't chew gum?"),
    L_(ROC, N, "He could chew THROUGH gum. And anchors. And probably you, Ollie."),
    L_(SAM, N, "If we find his gold, we can pay off Thornbury and save the Shoals. Nobody has to move away."),
    L_(GUS, N, "The map says the way in is the old smugglers' grate up by the lighthouse. It's been sealed for fifty years."),
    L_(GUS, H, "But the winch next to it only needs three gears. Find me three gears and I can open it!"),
    L_(OLL, X, "In THIS storm? My mom just washed my hair."),
    L_(ROC, N, "Ollie, your hair hasn't been washed since the third grade."),
    LA(SAM, H, ACT_CHEER, "Treasure Hounds... stick together!"),
    LA(ALL, H, ACT_THUNDER, "STICK TOGETHER!"),
    END
};
static const Line l1_tutorial[] = {
    L_(NARR, N, "Left stick moves. Hold \x04 to run.\n\x02 cracks the whip.  \x01 talks, reads and uses things."),
    L_(NARR, N, "\x05 and \x06 choose which Hound leads. Hurt Hounds get their breath back at the back of the line.\n\x03 shows the map."),
    END
};
static const Line l1_square[] = {
    L_(GUS, N, "There's Thornbury's notice on the board. They're really doing it."),
    L_(SAM, N, "Not if we find that treasure first."),
    L_(OLL, N, "Can we find a pizza first? Treasure hunting burns a LOT of calories."),
    L_(ROC, N, "Ollie, we've been treasure hunting for four minutes."),
    END
};
static const Line l1_garden[] = {
    L_(ROC, N, "Widow Hatch's garden. They say she's a hundred and ten years old."),
    L_(OLL, X, "I heard she keeps kids in jars. Like pickles."),
    L_(GUS, N, "That's scientifically... actually, I can't prove that's NOT true."),
    L_(SAM, N, "Those bushes are blocking the gate. Give 'em a crack of the whip!"),
    END
};
static const Line l1_ingarden[] = {
    L_(OLL, X, "If I get pickled, tell my mom I love her. And tell my dad I ate his pudding."),
    L_(ROC, N, "You ate EVERYBODY'S pudding, Ollie."),
    END
};
static const Line l1_crowleys[] = {
    L_(GUS, X, "Get down! That's the Crowley brothers! They broke out of the county jail last week!"),
    L_(ROC, N, "Vic and Lenny Crowley. Their combined IQ is a ham sandwich."),
    L_(SAM, N, "Stay out of their flashlights. If they spot us, they'll grab the map!"),
    L_(OLL, X, "And what if they catch us?"),
    L_(ROC, H, "Three cracks of the whip and they see stars. Then we RUN."),
    END
};
static const Line l1_tiny[] = {
    L_(OLL, X, "Uh... guys? There's a GIANT in that cage."),
    L_(TINY, N, "...Tiny hungry."),
    L_(ROC, X, "THAT'S Tiny? He's the size of a refrigerator! A refrigerator that ATE a refrigerator!"),
    L_(GUS, N, "Ma Crowley keeps her own son locked up behind the cannery? That's awful."),
    L_(OLL, N, "Hey, big guy. You want my Choco-Crunch bar? It's my last one. I was saving it for an emergency."),
    LA(TINY, H, ACT_TINY_LOVE, "Choco... Crunch... TINY LIKE YOU!"),
    L_(OLL, H, "I think I made a friend!"),
    L_(GUS, N, "That lock is huge. Ma Crowley must have the key. We'll come back for you, Tiny. Promise."),
    L_(TINY, H, "Tiny wait. Tiny good wait-er."),
    END
};
static const Line l1_plateau[] = {
    L_(SAM, H, "There's the lighthouse! The smugglers' grate is right next to it."),
    L_(ROC, H, "Lightning, a graveyard AND a creepy lighthouse. Best night of my life."),
    L_(GUS, N, "Rats are coming out of the rocks to get out of the rain. Watch your ankles."),
    END
};
static const Line l1_docks[] = {
    L_(OLL, N, "Look at all those crates. You think there's food in any of them?"),
    L_(ROC, N, "Ollie, you'd eat the crate."),
    L_(OLL, N, "...Only if it was a nice crate."),
    END
};
static const Line crane_need[] = {
    L_(GUS, N, "The winch needs three gears to turn. We've only got %G."),
    L_(SAM, N, "Then we keep looking. Check the docks, the old garden and the woods up here."),
    END
};
static const Line crane_done[] = {
    L_(GUS, H, "Gear one... gear two... gear three! Stand back, gentlemen. SCIENCE!"),
    LA(NARR, N, ACT_OPEN_GRATE, "CLANK... CLANK... CLANK... The old winch groans, and the rusty grate creaks open."),
    L_(ROC, X, "Whoa. It actually worked! Gus, I take back half the things I said about you."),
    L_(GUS, N, "Which half?"),
    L_(ROC, N, "The half about your haircut."),
    L_(SAM, H, "Into the tunnel, Hounds! I've got the lantern."),
    END
};

/* ---------------- LEVEL 2 ---------------- */
static const Line l2_intro[] = {
    L_(NARR, N, "Beneath Gull's Point, the old smugglers' tunnels wind out under the sea..."),
    L_(OLL, X, "It's SO dark. Who's got the lantern?"),
    L_(SAM, N, "Whoever's in front carries it. Everybody stay close to the light."),
    L_(ROC, N, "Ollie, if you're holding my hand, that's fine. But if you're NOT holding my hand... who is?"),
    LA(OLL, X, ACT_SHAKE, "AAAAAAAAH!"),
    L_(GUS, N, "Relax. Bats only bite when they're scared. ...Of course, we're probably scaring them."),
    END
};
static const Line l2_well[] = {
    L_(OLL, X, "Is it... raining money?"),
    L_(GUS, H, "We're right under the town wishing well! Every coin anybody ever wished on ends up down here!"),
    L_(ROC, H, "So all those wishes came true for US. Nice."),
    L_(SAM, N, "Somebody probably wished for something really important with these."),
    L_(OLL, N, "I wished for a pony once. Do you think the pony's down here too?"),
    END
};
static const Line l2_river[] = {
    L_(SAM, N, "An underground river! Careful on that bridge."),
    L_(GUS, N, "Watch out for the crabs. Those claws can pinch right through a sneaker."),
    L_(ROC, N, "Good. Ollie's sneakers could use some air holes."),
    END
};
static const Line l2_spikes[] = {
    L_(GUS, X, "Spike traps! They pop up on a rhythm. Watch them, and go when they drop."),
    L_(OLL, X, "I don't have rhythm. Rhythm is scared of me."),
    END
};
static const Line l2_braziers[] = {
    L_(SAM, N, "'Four fires to wake the door.' Four old braziers... and one lantern!"),
    L_(GUS, N, "Walk up to each brazier and press \x01 to light it with the lantern."),
    END
};
static const Line l2_braziers_done[] = {
    L_(GUS, H, "That's all four! The door's waking up!"),
    L_(ROC, N, "Doors don't wake up, Gus."),
    LA(NARR, N, ACT_RUMBLE, "RRRRRRUMBLE..."),
    L_(ROC, X, "...Okay. That door woke up."),
    END
};
static const Line l2_chasm[] = {
    L_(ROC, X, "That bridge looks like it's made out of crackers."),
    L_(SAM, N, "Don't stop on it. Keep moving and we'll be fine."),
    L_(OLL, N, "Mmm. Crackers."),
    END
};
static const Line l2_plates[] = {
    L_(SAM, N, "'No Hound crosses alone.' Four plates... and four of us."),
    L_(GUS, H, "Stand on a plate and press \x01. We'll each grab one!"),
    END
};
static const Line l2_plates_done[] = {
    L_(SAM, H, "See? Teamwork!"),
    L_(ROC, N, "Ollie stood on his plate so hard it's never coming back up."),
    L_(OLL, H, "Thank you."),
    END
};
static const Line l2_lenny[] = {
    L_(GUS, X, "It's Lenny Crowley! They're following us!"),
    L_(ROC, N, "How'd he get down here before us?"),
    L_(OLL, N, "Maybe he knows a shortcut. A shortcut with snacks."),
    L_(SAM, N, "Sneak past him or whip him dizzy. Don't let him get the map!"),
    END
};
static const Line l2_hooks[] = {
    L_(GUS, N, "A chasm! But look: iron hooks on both sides."),
    L_(SAM, H, "Face the hook and crack the whip. We'll swing across like Ironjaw's crew!"),
    L_(ROC, X, "If I don't make it, bury me with my baseball cards."),
    L_(OLL, N, "Can I have your baseball cards?"),
    END
};
static const Line l2_darts[] = {
    L_(GUS, X, "Those carved faces in the walls... they shoot darts!"),
    L_(ROC, N, "Of course they do. Why would anything down here NOT shoot darts?"),
    L_(SAM, N, "They fire straight across. Time your run between them."),
    END
};
static const Line l2_boulder_pre[] = {
    L_(OLL, H, "Ooh, a nice long hallway. Nothing bad ever happens in a nice long hallway."),
    END
};

/* ---------------- LEVEL 3 ---------------- */
static const Line l3_intro[] = {
    L_(NARR, N, "Deep below the cove lies the tomb of Ironjaw Jack..."),
    L_(SAM, H, "This is it. Ironjaw Jack's temple!"),
    L_(GUS, N, "Look at these carvings. These booby traps are three hundred years old."),
    L_(ROC, N, "Three hundred years old and they still work? My dad's car is from LAST year and it doesn't work."),
    END
};
static const Line l3_court[] = {
    L_(SAM, N, "Three doors... and the north one's sealed tight."),
    L_(GUS, N, "The west room has stone blocks and floor plates. Some kind of puzzle!"),
    L_(OLL, X, "And the east room is... on fire. Let's do the west room."),
    L_(ROC, N, "There's a little locked door in the west room too. Bet there's a switch somewhere."),
    END
};
static const Line l3_blocks[] = {
    L_(GUS, N, "Push the stone blocks onto the plates. Just walk into a block to shove it."),
    L_(ROC, N, "And don't push one into a corner, genius. Then it's stuck forever."),
    L_(GUS, N, "I KNOW how blocks work, Rocco."),
    END
};
static const Line l3_blocks_done[] = {
    L_(SAM, H, "Did you hear that? The big north door in the courtyard just opened!"),
    L_(OLL, N, "Pushing rocks around. Finally, a puzzle that uses my muscles."),
    END
};
static const Line l3_flames[] = {
    L_(OLL, X, "Fire, skeletons, and more fire. Did I mention the fire?"),
    L_(GUS, N, "The jets glow right before they burst. Watch for the glow!"),
    L_(SAM, N, "That gate on the far side looks like a way out. And is that a lever on the wall?"),
    L_(ROC, N, "Skeletons with swords. This is the WORST field trip ever."),
    END
};
static const Line l3_organ[] = {
    L_(GUS, X, "Whoa. It's an organ... made out of BONES!"),
    L_(SAM, N, "'Play the song of the bones.' The carving on the north wall must be the music!"),
    L_(GUS, N, "The carved bones are all different lengths. Crack the real bone pipes in the same order, matching by size!"),
    L_(ROC, N, "And if we get it wrong?"),
    L_(GUS, N, "Probably darts."),
    L_(ROC, X, "It's ALWAYS darts."),
    END
};
static const Line l3_organ_done[] = {
    L_(OLL, H, "We're musicians! We should start a band!"),
    L_(ROC, H, "The Treasure Hounds and the Bone Zone. I'm on drums."),
    END
};
static const Line l3_vault[] = {
    L_(SAM, X, "The Skull Vault... and the Skull Key!"),
    L_(OLL, H, "Wow. It's beautiful."),
    L_(ROC, N, "Nobody touch anything. Every time somebody touches the shiny thing in a movie, the whole place falls down."),
    END
};
static const Line l3_key[] = {
    L_(SAM, H, "Got it! The key to Ironjaw Jack's treasure chest!"),
    LA(NARR, N, ACT_COLLAPSE, "RRRRRUMMMBLE!"),
    L_(GUS, X, "Uh oh. The temple's coming down!"),
    L_(ROC, X, "WHAT DID I SAY? WHAT DID I JUST SAY?!"),
    L_(SAM, N, "Something opened in the east room! Run for it!"),
    END
};
static const Line l3_escape[] = {
    L_(GUS, N, "I hear waves! This way goes out to the sea!"),
    END
};

/* ---------------- LEVEL 4 ---------------- */
static const Line l4_intro[] = {
    L_(NARR, N, "Skeleton Cove. Where Ironjaw Jack's ship, the Black Gull, vanished three hundred years ago."),
    L_(SAM, N, "We made it outside... but the storm's even worse out here!"),
    L_(OLL, N, "My shoes are full of ocean."),
    L_(ROC, N, "Your shoes were full of something way worse before we got wet."),
    END
};
static const Line l4_wreck[] = {
    L_(GUS, N, "That's not the Black Gull. Just some poor old fishing boat."),
    L_(SAM, N, "The map says Jack's ship is hidden in the lagoon, east past the cliffs."),
    END
};
static const Line l4_camp[] = {
    L_(OLL, X, "A campfire! And sausages! ...And the Crowleys."),
    L_(ROC, X, "They beat us here AGAIN?"),
    L_(GUS, N, "They must have a copy of the map. We have to hurry!"),
    END
};
static const Line l4_ship[] = {
    L_(SAM, H, "THERE! The Black Gull! A real pirate ship!"),
    L_(ROC, H, "It's... it's beautiful."),
    L_(OLL, N, "Rocco, are you crying?"),
    L_(ROC, N, "It's RAIN, Ollie."),
    L_(GUS, N, "There's an iron hook on the rocks. Face it and crack the whip to swing over!"),
    END
};
static const Line l4_board[] = {
    L_(MA, N, "Well, well, well. If it ain't the little brats from the Shoals."),
    L_(VIC, H, "Hi, Ma! We found 'em!"),
    L_(LEN, H, "Can we keep the treasure, Ma? Can we? Can we?"),
    L_(MA, N, "Hand over that skull key, sweeties, and nobody walks the plank."),
    L_(SAM, N, "Never! This treasure is going to save the Shoals!"),
    LA(MA, X, ACT_BOSS_START, "Then I'll take it myself! BOYS! GET 'EM!"),
    END
};
static const Line l4_boss_down[] = {
    L_(MA, X, "Ow! OW! You rotten little..."),
    L_(VIC, X, "Ma, they're beatin' us!"),
    LA(TINY, X, ACT_TINY_ENTER, "HEEEY! YOU LEAVE TINY'S FRIENDS ALONE!"),
    L_(MA, X, "TINY?! How did you get out of your cage?"),
    L_(TINY, N, "Tiny bend bars. Bars weak. Tiny strong."),
    L_(OLL, H, "TINY! Buddy!"),
    LA(NARR, N, ACT_DUNK, "SPLOOOSH!"),
    L_(LEN, X, "Ma! I'm all wet!"),
    L_(MA, X, "SHUT UP, LENNY!"),
    L_(TINY, H, "Tiny help. Tiny good friend."),
    L_(OLL, H, "The BEST friend. Want half a sandwich? It's been in my pocket all night."),
    L_(TINY, H, "...Tiny eat anything."),
    L_(SAM, H, "The captain's cabin! Let's open that chest!"),
    END
};
static const Line l4_chest[] = {
    L_(SAM, N, "Ready, Hounds?"),
    L_(ALL, H, "READY!"),
    LA(NARR, N, ACT_CHEST_OPEN, "The Skull Key turns with a heavy CLUNK..."),
    L_(OLL, H, "GOOOOOOOLD!"),
    L_(ROC, H, "We're RICH! I'm buying a boat! A boat with a smaller boat inside it!"),
    L_(GUS, H, "There's enough here to save every single house in the Shoals!"),
    L_(SAM, H, "Grandpa was right. He was right all along."),
    L_(TINY, H, "Shiny!"),
    LA(SAM, H, ACT_CHEER, "Treasure Hounds..."),
    LA(ALL, H, ACT_ENDING, "...STICK TOGETHER!!"),
    END
};

/* ---------------- misc ---------------- */
static const Line exit_l1[] = { LA(NARR, N, ACT_LEVEL_DONE, "The Hounds climb down into the dark..."), END };
static const Line exit_l2[] = {
    L_(GUS, N, "Stairs going down... and they're carved like bones."),
    L_(OLL, X, "Bones. Great. I love bones. ...Said nobody ever."),
    LA(NARR, N, ACT_LEVEL_DONE, ""), END
};
static const Line exit_l3[] = {
    L_(SAM, H, "Daylight! ...Well, lightning-light!"),
    LA(NARR, N, ACT_LEVEL_DONE, ""), END
};
static const Line gate_locked[] = { L_(GUS, N, "%H"), END };
static const Line sign_line[] = { L_(NARR, N, "%S"), END };
static const Line carving_line[] = { L_(SPK_SIGN, N, "%S"), END };
static const Line organ_wrong[] = { L_(ROC, X, "Wrong note! DARTS! I TOLD you it'd be darts!"), END };

const Line *const SCENES[SC_COUNT] = {
    [SC_NONE] = 0,
    [SC_L1_INTRO] = l1_intro, [SC_L1_TUTORIAL] = l1_tutorial, [SC_L1_SQUARE] = l1_square, [SC_L1_GARDEN] = l1_garden,
    [SC_L1_INGARDEN] = l1_ingarden, [SC_L1_CROWLEYS] = l1_crowleys, [SC_L1_TINY] = l1_tiny, [SC_L1_PLATEAU] = l1_plateau,
    [SC_L1_DOCKS] = l1_docks, [SC_CRANE_NEED] = crane_need, [SC_CRANE_DONE] = crane_done,
    [SC_L2_INTRO] = l2_intro, [SC_L2_WELL] = l2_well, [SC_L2_RIVER] = l2_river, [SC_L2_SPIKES] = l2_spikes,
    [SC_L2_BRAZIERS] = l2_braziers, [SC_L2_BRAZIERS_DONE] = l2_braziers_done, [SC_L2_CHASM] = l2_chasm,
    [SC_L2_PLATES] = l2_plates, [SC_L2_PLATES_DONE] = l2_plates_done, [SC_L2_LENNY] = l2_lenny, [SC_L2_HOOKS] = l2_hooks,
    [SC_L2_DARTS] = l2_darts, [SC_L2_BOULDER_PRE] = l2_boulder_pre,
    [SC_L3_INTRO] = l3_intro, [SC_L3_COURT] = l3_court, [SC_L3_BLOCKS] = l3_blocks, [SC_L3_BLOCKS_DONE] = l3_blocks_done,
    [SC_L3_FLAMES] = l3_flames, [SC_L3_ORGAN] = l3_organ, [SC_L3_ORGAN_DONE] = l3_organ_done, [SC_L3_VAULT] = l3_vault,
    [SC_L3_KEY] = l3_key, [SC_L3_ESCAPE] = l3_escape,
    [SC_L4_INTRO] = l4_intro, [SC_L4_WRECK] = l4_wreck, [SC_L4_CAMP] = l4_camp, [SC_L4_SHIP] = l4_ship,
    [SC_L4_BOARD] = l4_board, [SC_L4_BOSS_DOWN] = l4_boss_down, [SC_L4_CHEST] = l4_chest,
    [SC_EXIT] = 0, [SC_EXIT_L1] = exit_l1, [SC_EXIT_L2] = exit_l2, [SC_EXIT_L3] = exit_l3,
    [SC_SIGN] = sign_line, [SC_CARVING] = carving_line, [SC_GATE_LOCKED] = gate_locked, [SC_ORGAN_WRONG] = organ_wrong,
};

/* ---------------- banter (speech bubbles while exploring) ---------------- */
const Banter BANTER_TOWN[] = {
    { { ROC, OLL, -1 }, { "Hey Ollie, why'd the pirate go to school?", "To improve his ARRRR-ticulation?" } },
    { { OLL, GUS, ROC }, { "I'm so wet I've got fish in my pockets.", "That's not rain. That's sideways ocean.", "That's the sea sneezing on us." } },
    { { GUS, SAM, -1 }, { "Statistically, being outside in a lightning storm is... not great.", "Then let's be quick about it." } },
    { { ROC, SAM, -1 }, { "If we find the gold, I'm buying a hot tub. For my DOG.", "You don't have a dog." } },
    { { OLL, ROC, OLL }, { "Pull my finger.", "Absolutely not.", "...Too late." } },
    { { SAM, GUS, -1 }, { "Grandpa used to say Ironjaw Jack was too clever to be caught.", "Good thing we're cleverer than the Coast Guard." } },
    { { ROC, GUS, -1 }, { "Gus, what's the plan if we see Widow Hatch?", "Run. It's a very simple plan." } },
    { { OLL, SAM, -1 }, { "My stomach's growling louder than the thunder.", "That WAS your stomach?" } },
    { { GUS, ROC, -1 }, { "I'm inventing a raincoat for my glasses.", "It's called a hat, Gus." } },
};
const int NBANTER_TOWN = sizeof(BANTER_TOWN) / sizeof(BANTER_TOWN[0]);

const Banter BANTER_CAVE[] = {
    { { OLL, ROC, -1 }, { "Something just touched my ear.", "That was me. I'm bored." } },
    { { GUS, SAM, -1 }, { "These tunnels were dug by smugglers in the 1700s.", "And nobody's been down here since. Creepy." } },
    { { ROC, OLL, ROC }, { "Knock knock.", "Who's there?", "Bat. Bat who? BAT-ter watch your HEAD!" } },
    { { OLL, GUS, -1 }, { "It smells like low tide and old socks down here.", "That might be Rocco." } },
    { { SAM, ROC, -1 }, { "Hear that dripping? We must be under the bay.", "Great. A whole ocean right over my head. Cool. Cool cool cool." } },
    { { ROC, GUS, -1 }, { "Gus, invent something that makes caves less scary.", "It's called a lantern. Sam's holding it." } },
    { { OLL, SAM, -1 }, { "If I get stuck in a tunnel, just leave me with a sandwich.", "Nobody's getting left behind, Ollie. Ever." } },
    { { GUS, OLL, -1 }, { "Those mushrooms glow from bioluminescence.", "Can you eat bioluminescence?" } },
};
const int NBANTER_CAVE = sizeof(BANTER_CAVE) / sizeof(BANTER_CAVE[0]);

const Banter BANTER_TEMPLE[] = {
    { { ROC, GUS, -1 }, { "Ironjaw Jack must've been a total weirdo.", "He built a pipe organ out of bones, Rocco. Yes." } },
    { { OLL, SAM, -1 }, { "Do skeletons get cold? They don't have any skin.", "Ask one. Then run." } },
    { { GUS, ROC, OLL }, { "This place is a masterpiece of booby-trap engineering.", "You're blushing, Gus.", "He loves it here. Leave him alone." } },
    { { SAM, OLL, -1 }, { "Every step, we're closer to saving the Shoals.", "Every step, I'm closer to wetting my pants." } },
    { { ROC, OLL, -1 }, { "Why didn't the skeleton go to the party?", "He had no BODY to go with! ...I saw that one coming." } },
    { { OLL, GUS, -1 }, { "Hey, these torches are still burning after three hundred years?", "Somebody's been down here keeping them lit. Somebody... or something." } },
};
const int NBANTER_TEMPLE = sizeof(BANTER_TEMPLE) / sizeof(BANTER_TEMPLE[0]);

const Banter BANTER_COVE[] = {
    { { OLL, ROC, -1 }, { "I've never been this wet in my whole life.", "Ollie, you've never taken a bath in your whole life." } },
    { { GUS, SAM, -1 }, { "The tide's coming in. We don't have long.", "Then let's move, Hounds!" } },
    { { ROC, OLL, -1 }, { "What's a pirate's favorite letter?", "R! ...No wait. It's the C. The SEA!" } },
    { { SAM, GUS, -1 }, { "My grandpa looked for this ship his whole life.", "And we're gonna find it for him." } },
    { { OLL, SAM, -1 }, { "Sam, do pirates have dental plans? Is that why his jaw was iron?", "...You know, that actually makes sense." } },
};
const int NBANTER_COVE = sizeof(BANTER_COVE) / sizeof(BANTER_COVE[0]);

const char *const SAY_HURT[4][4] = {
    { "Ow!", "Ouch!", "Not cool!", "That's gonna leave a mark!" },
    { "OW! My face!", "Hey! Watch the hat!", "Ow ow ow ow!", "Lawyer! I want a lawyer!" },
    { "My glasses!", "Ouch! Unscientific!", "That hurt my brain!", "Ow! Recalculating!" },
    { "OOF!", "My tummy!", "Owie!", "That's it, I need a snack." },
};
const char *const SAY_LEAD[4][3] = {
    { "Follow me, Hounds!", "I've got the map. Let's go!", "Stick together!" },
    { "Rocco's in charge! Everybody zip it!", "Finally. Some real leadership.", "Follow the cap!" },
    { "Engaging leadership protocol.", "I calculated the route. Probably.", "Science leads the way!" },
    { "Me? Really? Okay! ...Which way?", "Ollie's turn! Snack break first?", "I won't let you guys down!" },
};
const char *const SAY_FAINT[4] = {
    "I'm pooped... somebody else lead!",
    "I need a sec... my legs are spaghetti.",
    "Battery low... switching leaders...",
    "Can't... go on... need... pizza...",
};
const char *const SAY_THUNDER[4][3] = {
    { "Whoa! That was close!", "Keep moving!", "Grandpa called that 'Jack's cannon'." },
    { "I wasn't scared. My SHOES were scared.", "Did somebody fart? Oh. Thunder.", "Mother Nature needs to chill." },
    { "One Mississippi, two Miss... it's right on top of us!", "Lightning is 30,000 degrees!", "Fascinating! Terrifying! Fascinating!" },
    { "MOMMY! ...I mean, cool.", "I'm not scared, YOU'RE scared!", "My hair is standing up!" },
};
const char *const SAY_FOOD[4] = { "Snack break!", "Ollie, don't eat that... oh, too late.", "Mmm, nutritious.", "YES. Food!" };
const char *const SAY_DOUBLOON[4] = {
    "One of Ironjaw Jack's golden doubloons!", "Ka-CHING! Pirate gold!", "A genuine 1700s doubloon!", "Is it chocolate? ...It's not chocolate."
};
const char *const SAY_DIZZY_VIC[3] = { "Ohhh... I see birdies...", "Mommy? Is that you?", "Who turned out the stars?" };
const char *const SAY_DIZZY_LENNY[3] = { "Ow... my everything...", "Five more minutes, Ma...", "Pretty stars..." };
const char *const SAY_SPOT_VIC[3] = { "HEY! You kids!", "Get back here, ya brats!", "Gimme that map!" };
const char *const SAY_SPOT_LENNY[3] = { "Ma said to get ya!", "Hey! Hey! HEY!", "I see you! I think!" };
