/*
 * TREASURE HOUNDS - level loading, terrain, collision.
 */
#include "game_int.h"
#include <string.h>
#include <math.h>

/* solid wall water floor top face step */
const KindInfo KINFO[K_COUNT] = {
    [K_VOID]     = { 1, 0, 0, TA_PIT, 0, 0, 0 },
    [K_GRASS]    = { 0, 0, 0, TA_GRASS, 0, 0, 1 },
    [K_DIRT]     = { 0, 0, 0, TA_DIRT, 0, 0, 1 },
    [K_COBBLE]   = { 0, 0, 0, TA_COBBLE, 0, 0, 2 },
    [K_PLANK]    = { 0, 0, 0, TA_PLANK, 0, 0, 3 },
    [K_SAND]     = { 0, 0, 0, TA_SAND, 0, 0, 1 },
    [K_WETSAND]  = { 0, 0, 0, TA_WETSAND, 0, 0, 4 },
    [K_CAVE]     = { 0, 0, 0, TA_CAVE, 0, 0, 2 },
    [K_MUD]      = { 0, 0, 0, TA_MUD, 0, 0, 4 },
    [K_SLAB]     = { 0, 0, 0, TA_SLAB, 0, 0, 2 },
    [K_MOSS]     = { 0, 0, 0, TA_MOSS, 0, 0, 2 },
    [K_DECK]     = { 0, 0, 0, TA_DECK, 0, 0, 3 },
    [K_RUG]      = { 0, 0, 0, TA_RUG, 0, 0, 1 },
    [K_SEA]      = { 1, 0, 1, TA_WATER, 0, 0, 0 },
    [K_POOL]     = { 1, 0, 1, TA_CAVEWATER, 0, 0, 0 },
    [K_SHALLOW]  = { 0, 0, 2, TA_SHALLOW, 0, 0, 4 },
    [K_ROCK]     = { 1, 1, 0, TA_ROCK_TOP, TA_ROCK_TOP, TA_ROCK_FACE, 0, TA_ROCK_FACEL },
    [K_CAVEWALL] = { 1, 1, 0, TA_CAVE_TOP, TA_CAVE_TOP, TA_CAVE_FACE, 0, TA_CAVE_FACEL },
    [K_BRICK]    = { 1, 1, 0, TA_BRICK_TOP, TA_BRICK_TOP, TA_BRICK_FACE, 0, TA_BRICK_FACEL },
    [K_WOOD]     = { 1, 1, 0, TA_ROOF, TA_ROOF, TA_WOOD_FACE, 0, TA_WOOD_FACE },
    [K_HULL]     = { 1, 1, 0, TA_HULL, TA_HULL, TA_HULL, 0, TA_HULL },
    [K_HEDGE]    = { 1, 1, 0, TA_HEDGE_TOP, TA_HEDGE_TOP, TA_HEDGE_FACE, 0, TA_HEDGE_FACE },
    [K_SPIKES]   = { 0, 0, 0, TA_SPIKES, 0, 0, 2 },
    [K_CRUMBLE]  = { 0, 0, 0, TA_CRUMBLE, 0, 0, 2 },
    [K_PLATE]    = { 0, 0, 0, TA_PLATE, 0, 0, 2 },
    [K_GRATE]    = { 0, 0, 0, TA_GRATE, 0, 0, 3 },
    [K_HOLE]     = { 1, 0, 0, TA_CRUMBLE, 0, 0, 0 },
};

#define F_S EF_SOLID
const EntInfo EINFO[E_COUNT] = {
    [E_NONE]      = { -1, 0, 0, 0 },
    [E_PINE]      = { SP_PINE, 5, 5, F_S | EF_SWAY },
    [E_PALM]      = { SP_PALM, 4, 4, F_S | EF_SWAY },
    [E_BUSH]      = { SP_BUSH, 8, 8, F_S | EF_BREAK | EF_SWAY },
    [E_ROCK]      = { SP_ROCK, 6, 6, F_S },
    [E_ROCKBIG]   = { SP_ROCK_BIG, 12, 12, F_S },
    [E_BARREL]    = { SP_BARREL, 6, 8, F_S | EF_BREAK },
    [E_CRATE]     = { SP_CRATE, 7, 10, F_S | EF_BREAK },
    [E_POT]       = { SP_POT, 5, 6, F_S | EF_BREAK },
    [E_LAMP]      = { SP_LAMPPOST, 3, 3, F_S },
    [E_TORCH]     = { SP_TORCH, 0, 0, EF_WALLMNT },
    [E_FIRE]      = { SP_CAMPFIRE, 9, 6, F_S },
    [E_BENCH]     = { SP_BENCH, 11, 5, F_S },
    [E_TABLE]     = { SP_TABLE, 11, 9, F_S },
    [E_FENCE]     = { SP_FENCE, 8, 4, F_S },
    [E_SIGN]      = { SP_SIGN, 5, 4, F_S | EF_USE },
    [E_ANCHOR]    = { SP_ANCHOR, 6, 4, F_S },
    [E_ROWBOAT]   = { SP_ROWBOAT, 16, 8, F_S },
    [E_WRECK]     = { SP_WRECK_BOW, 26, 12, F_S },
    [E_WRECKMAST] = { SP_WRECK_MAST, 4, 4, F_S },
    [E_SKULLS]    = { SP_SKULLS, 0, 0, EF_FLAT },
    [E_BONES]     = { SP_BONES, 0, 0, EF_FLAT },
    [E_GOLDPILE]  = { SP_GOLDPILE, 10, 6, F_S },
    [E_CHAIN]     = { SP_CHAIN, 0, 0, 0 },
    [E_PILLAR]    = { SP_PILLAR, 7, 6, F_S },
    [E_PILLARB]   = { SP_PILLAR_BROKEN, 7, 6, F_S },
    [E_SKULLWALL] = { SP_SKULLWALL, 34, 16, F_S },
    [E_LIGHTHOUSE]= { SP_LIGHTHOUSE, 19, 20, F_S },
    [E_SHACK]     = { SP_SHACK, 33, 34, F_S },
    [E_WELL]      = { SP_WELL, 10, 8, F_S },
    [E_CANNON]    = { SP_CANNON, 10, 6, F_S },
    [E_NETS]      = { SP_NETS, 0, 0, EF_FLAT },
    [E_STATUE]    = { SP_STATUE, 9, 7, F_S },
    [E_MAST]      = { SP_MAST, 4, 4, F_S },
    [E_WHEEL]     = { SP_WHEEL, 6, 4, F_S },
    [E_HLANTERN]  = { SP_LANTERN_HANG, 0, 0, 0 },
    [E_GRAVE]     = { SP_GRAVE, 5, 4, F_S },
    [E_TOTEM]     = { SP_TOTEM, 7, 6, F_S },
    [E_FERN]      = { SP_FERN, 0, 0, EF_SWAY },
    [E_MUSHROOM]  = { SP_MUSHROOM, 0, 0, 0 },
    [E_CRYSTAL]   = { SP_CRYSTAL, 5, 5, F_S },
    [E_STALAG]    = { SP_STALAG, 5, 5, F_S },
    [E_SHELL]     = { SP_SHELL, 0, 0, EF_FLAT },
    [E_PUDDLE]    = { SP_PUDDLE, 0, 0, EF_FLAT },
    [E_SEAWEED]   = { SP_LILY, 0, 0, EF_FLAT },
    [E_CARVING]   = { SP_CARVING, 18, 8, F_S | EF_USE },
    [E_CHEST]     = { SP_CHEST, 8, 8, F_S | EF_USE },
    [E_BIGCHEST]  = { SP_BIGCHEST, 14, 12, F_S | EF_USE },
    [E_CRANE]     = { SP_CRANE, 8, 8, F_S | EF_USE },
    [E_LEVER]     = { SP_LEVER_L, 5, 4, F_S | EF_USE },
    [E_GATE]      = { SP_GATE, 16, 10, F_S | EF_USE },
    [E_BLOCK]     = { SP_BLOCK, 8, 16, F_S },
    [E_PIPE]      = { SP_BONEPIPE, 7, 5, F_S },
    [E_BRAZIER]   = { SP_BRAZIER, 7, 7, F_S | EF_USE },
    [E_HOOK]      = { SP_HOOK, 4, 4, F_S },
    [E_CAGE]      = { SP_CAGE, 14, 16, F_S | EF_USE },
    [E_COIN]      = { -1, 0, 0, EF_PICKUP },
    [E_GEM]       = { -1, 0, 0, EF_PICKUP },
    [E_GEAR]      = { -1, 0, 0, EF_PICKUP },
    [E_DOUBLOON]  = { -1, 0, 0, EF_PICKUP },
    [E_FOOD]      = { -1, 0, 0, EF_PICKUP },
    [E_SKULLKEY]  = { -1, 0, 0, EF_PICKUP },
    [E_BAT]       = { -1, 0, 0, EF_ENEMY },
    [E_CRAB]      = { -1, 0, 0, EF_ENEMY },
    [E_RAT]       = { -1, 0, 0, EF_ENEMY },
    [E_SKEL]      = { -1, 0, 0, EF_ENEMY },
    [E_VIC]       = { -1, 0, 0, EF_ENEMY },
    [E_LENNY]     = { -1, 0, 0, EF_ENEMY },
    [E_MA]        = { -1, 0, 0, EF_ENEMY },
    [E_TINY]      = { -1, 0, 0, 0 },
    [E_DARTER]    = { SP_DARTER, 0, 0, EF_WALLMNT },
    [E_FLAMEJET]  = { SP_FLAMEJET, 0, 0, EF_FLAT },
    [E_BOULDER]   = { SP_BOULDER, 0, 0, 0 },
    [E_DART]      = { -1, 0, 0, 0 },
    [E_BOTTLE]    = { -1, 0, 0, 0 },
};

static int theme_kind(int theme, char c)
{
    static const int fl1[4] = { K_GRASS, K_CAVE, K_SLAB, K_SAND };
    static const int fl2[4] = { K_DIRT, K_MUD, K_MOSS, K_WETSAND };
    static const int wall[4] = { K_ROCK, K_CAVEWALL, K_BRICK, K_ROCK };
    static const int deep[4] = { K_SEA, K_POOL, K_POOL, K_SEA };
    switch (c) {
    case '.': return fl1[theme];
    case ',': return fl2[theme];
    case ':': return K_COBBLE;
    case '=': return K_PLANK;
    case '-': return K_DECK;
    case '"': return K_RUG;
    case '`': return K_SAND;
    case '\'': return K_WETSAND;
    case '~': return deep[theme];
    case 'w': return K_SHALLOW;
    case ' ': return K_VOID;
    case '#': return wall[theme];
    case 'W': return K_WOOD;
    case 'U': return K_HULL;
    case 'h': return K_HEDGE;
    case '^': return K_SPIKES;
    case '_': return K_CRUMBLE;
    case 'p': return K_PLATE;
    case 'G': return K_GRATE;
    }
    return -1;     /* an object character */
}

typedef struct { char c; uint8_t type; } ObjChar;
static const ObjChar OBJ[] = {
    {'T', E_PINE}, {'P', E_PALM}, {'b', E_BUSH}, {'o', E_ROCK}, {'O', E_ROCKBIG}, {'B', E_BARREL}, {'C', E_CRATE},
    {'c', E_POT}, {'L', E_LAMP}, {'t', E_TORCH}, {'F', E_FIRE}, {'n', E_BENCH}, {'N', E_TABLE}, {'f', E_FENCE},
    {'s', E_SIGN}, {'A', E_ANCHOR}, {'R', E_ROWBOAT}, {'K', E_WRECK}, {'k', E_WRECKMAST}, {'x', E_SKULLS},
    {'X', E_BONES}, {'g', E_GOLDPILE}, {'H', E_CHEST}, {'Y', E_BIGCHEST}, {'j', E_CHAIN}, {'I', E_PILLAR},
    {'i', E_PILLARB}, {'Z', E_SKULLWALL}, {'Q', E_LIGHTHOUSE}, {'S', E_SHACK}, {'V', E_WELL}, {'v', E_CANNON},
    {'e', E_NETS}, {'M', E_MAST}, {'m', E_WHEEL}, {'l', E_HLANTERN}, {'+', E_GRAVE}, {'y', E_TOTEM}, {'r', E_FERN},
    {'u', E_MUSHROOM}, {'z', E_CRYSTAL}, {'a', E_STALAG}, {'*', E_SHELL}, {'q', E_PUDDLE}, {'%', E_SEAWEED},
    {'E', E_CARVING}, {'D', E_GATE}, {'J', E_CRANE}, {'!', E_LEVER}, {'[', E_BLOCK}, {'|', E_PIPE}, {'&', E_BRAZIER},
    {'@', E_HOOK}, {'{', E_CAGE}, {'$', E_COIN}, {'1', E_GEM}, {'2', E_GEAR}, {'3', E_DOUBLOON}, {'4', E_FOOD},
    {'5', E_SKULLKEY}, {'6', E_BAT}, {'7', E_CRAB}, {'8', E_RAT}, {'9', E_SKEL}, {'<', E_VIC}, {'>', E_LENNY},
    {'}', E_MA}, {'d', E_DARTER}, {';', E_FLAMEJET}, {'(', E_BOULDER},
};

static char map_ch(const LevelData *ld, int x, int y)
{
    if (x < 0 || y < 0 || x >= ld->w || y >= ld->h) return '#';
    return ld->rows[y][x];
}

Ent *ent_new(Game *g, int type, float x, float y)
{
    for (int i = 0; i < g->nent; i++)
        if (!g->ent[i].on && g->ent[i].type == E_NONE) {
            Ent *e = &g->ent[i];
            memset(e, 0, sizeof(*e));
            e->type = (uint8_t)type; e->on = 1; e->x = e->hx = x; e->y = e->hy = y;
            return e;
        }
    if (g->nent >= MAX_ENT) return 0;
    Ent *e = &g->ent[g->nent++];
    memset(e, 0, sizeof(*e));
    e->type = (uint8_t)type; e->on = 1; e->x = e->hx = x; e->y = e->hy = y;
    return e;
}

int tile_at(const Game *g, int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= g->lv.w || ty >= g->lv.h) return K_VOID;
    return g->lv.k[ty][tx];
}

int solid_at(const Game *g, float x, float y)
{
    int tx = (int)floorf(x / TS), ty = (int)floorf(y / TS);
    int k = tile_at(g, tx, ty);
    if (KINFO[k].solid) return 1;
    return 0;
}

int walkable_tile(const Game *g, int tx, int ty)
{
    return !KINFO[tile_at(g, tx, ty)].solid;
}

static int ent_blocks(const Ent *e)
{
    if (!e->on) return 0;
    const EntInfo *in = &EINFO[e->type];
    if (!(in->flags & EF_SOLID)) return 0;
    if ((in->flags & EF_BREAK) && e->state) return 0;        /* broken */
    if (e->type == E_GATE && e->state) return 0;            /* open */
    if (e->type == E_CAGE && e->state) return 0;            /* bent open */
    return 1;
}

static int last_hit_type;     /* what stopped the last blocked() test (0 = terrain) */

int blocked(const Game *g, float x, float y, float r, int ignore)
{
    last_hit_type = 0;
    if (solid_at(g, x - r, y - r) || solid_at(g, x + r, y - r) || solid_at(g, x - r, y + r) || solid_at(g, x + r, y + r)) return 1;
    for (int i = 0; i < g->nent; i++) {
        const Ent *e = &g->ent[i];
        if (i == ignore || !ent_blocks(e)) continue;
        const EntInfo *in = &EINFO[e->type];
        float fw = in->fw, fh = in->fh;
        if (fabsf(x - e->x) > fw + r + 1 || y < e->y - fh - r - 1 || y > e->y + r + 1) continue;
        float cx = clampf(x, e->x - fw, e->x + fw), cy = clampf(y, e->y - fh, e->y);
        float dx = x - cx, dy = y - cy;
        if (dx * dx + dy * dy < r * r) { last_hit_type = e->type; return 1; }
    }
    return 0;
}

/* slide a circle along the walls */
void move_circle(const Game *g, float *x, float *y, float dx, float dy, float r, int ignore)
{
    int n = (int)(fmaxf(fabsf(dx), fabsf(dy)) / 3.0f) + 1;
    float sx = dx / (float)n, sy = dy / (float)n;
    for (int i = 0; i < n; i++) {
        /* slide round corners - but not off a stone block you're trying to push */
        if (!blocked(g, *x + sx, *y, r, ignore)) *x += sx;
        else if (last_hit_type != E_BLOCK && sy == 0.0f && !blocked(g, *x + sx, *y - 2.5f, r, ignore)) { *x += sx * 0.6f; *y -= 1.0f; }
        else if (last_hit_type != E_BLOCK && sy == 0.0f && !blocked(g, *x + sx, *y + 2.5f, r, ignore)) { *x += sx * 0.6f; *y += 1.0f; }
        if (!blocked(g, *x, *y + sy, r, ignore)) *y += sy;
        else if (last_hit_type != E_BLOCK && sx == 0.0f && !blocked(g, *x - 2.5f, *y + sy, r, ignore)) { *y += sy * 0.6f; *x -= 1.0f; }
        else if (last_hit_type != E_BLOCK && sx == 0.0f && !blocked(g, *x + 2.5f, *y + sy, r, ignore)) { *y += sy * 0.6f; *x += 1.0f; }
    }
}

/* the floor an object stands on: the nearest terrain character next to it */
static int ground_for(const LevelData *ld, int x, int y, int theme)
{
    static const int dx[4] = { -1, 1, 0, 0 }, dy[4] = { 0, 0, -1, 1 };
    for (int k = 0; k < 4; k++) {
        int kk = theme_kind(theme, map_ch(ld, x + dx[k], y + dy[k]));
        if (kk >= 0 && !KINFO[kk].solid && kk != K_SPIKES && kk != K_CRUMBLE && kk != K_PLATE && kk != K_GRATE) return kk;
    }
    for (int k = 0; k < 4; k++) {
        int kk = theme_kind(theme, map_ch(ld, x + dx[k] * 2, y + dy[k] * 2));
        if (kk >= 0 && !KINFO[kk].solid && kk != K_SPIKES && kk != K_CRUMBLE && kk != K_PLATE) return kk;
    }
    return theme_kind(theme, '.');
}

void world_load(Game *g, int level)
{
    const LevelData *ld = &LEVELS[level];
    Level *lv = &g->lv;
    memset(lv, 0, sizeof(*lv));
    g->ld = ld;
    g->level = level;
    lv->w = ld->w; lv->h = ld->h; lv->theme = ld->theme;
    g->nent = 0;
    memset(g->ent, 0, sizeof(g->ent));
    g->npart = 0;
    int sign_i = 0, pipe_i = 0, hook_i = 0, boul_i = 0, gate_i = 0;
    float sx = 64, sy = 64;
    for (int y = 0; y < ld->h; y++)
        for (int x = 0; x < ld->w; x++) {
            char c = map_ch(ld, x, y);
            int k = theme_kind(ld->theme, c);
            uint32_t h = (uint32_t)(x * 73856093) ^ (uint32_t)(y * 19349663) ^ (uint32_t)(level * 83492791);
            h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
            lv->v[y][x] = (uint8_t)(h & 3);
            if ((h >> 4) % 5 != 0 && lv->v[y][x] == 3) lv->v[y][x] = (uint8_t)((h >> 8) % 3);   /* the 4th variant is rarer */
            float wx = (float)(x * TS + TS / 2), wy = (float)(y * TS + TS);
            if (k >= 0) { lv->k[y][x] = (uint8_t)k; continue; }
            if (c == '/') { lv->k[y][x] = (uint8_t)ground_for(ld, x, y, ld->theme); sx = wx; sy = wy - 4; continue; }
            if (c == ')') { lv->k[y][x] = (uint8_t)ground_for(ld, x, y, ld->theme); continue; }
            int type = E_NONE;
            for (unsigned i = 0; i < sizeof(OBJ) / sizeof(OBJ[0]); i++) if (OBJ[i].c == c) { type = OBJ[i].type; break; }
            if (EINFO[type].flags & EF_WALLMNT) lv->k[y][x] = (uint8_t)theme_kind(ld->theme, '#');
            else lv->k[y][x] = (uint8_t)ground_for(ld, x, y, ld->theme);
            if (type == E_NONE) continue;
            Ent *e;
            switch (type) {
            case E_GATE:
                e = ent_new(g, type, (float)(x * TS + TS), wy);
                if (e) e->a = (int16_t)gate_i++;
                break;
            case E_CAGE:
                e = ent_new(g, E_CAGE, wx, wy);
                ent_new(g, E_TINY, wx, wy - 6);
                break;
            case E_SIGN: case E_CARVING:
                e = ent_new(g, type, wx, wy - 2);
                if (e) e->a = (int16_t)sign_i++;
                break;
            case E_PIPE:
                e = ent_new(g, type, wx, wy - 2);
                if (e) { e->a = (int16_t)(pipe_i < ld->npipe ? ld->pipe[pipe_i] : 0); e->b = (int16_t)pipe_i++; }
                break;
            case E_HOOK:
                e = ent_new(g, type, wx, wy - 3);
                if (e) e->a = (int16_t)hook_i++;
                break;
            case E_BOULDER:
                e = ent_new(g, type, wx, wy - 2);
                if (e) e->a = (int16_t)boul_i++;
                break;
            case E_TORCH: case E_DARTER:
                e = ent_new(g, type, wx, wy);
                if (e && type == E_DARTER) {
                    /* fire away from the wall, toward the open side */
                    e->dir = 0;
                    if (!KINFO[theme_kind(ld->theme, map_ch(ld, x, y + 1))].solid) e->dir = DIR_DOWN;
                    else if (!KINFO[theme_kind(ld->theme, map_ch(ld, x, y - 1))].solid) e->dir = DIR_UP;
                    e->t = (float)((x * 7 + y * 3) % 10) * 0.21f;
                    if (level == 2 && y < 12) e->c = 1;      /* the bone organ's darters only fire on a wrong note */
                }
                break;
            case E_FLAMEJET:
                e = ent_new(g, type, wx, wy - 4);
                if (e) e->t = (float)((x * 5 + y * 11) % 7) * 0.45f;
                break;
            case E_BLOCK:
                e = ent_new(g, type, wx, wy - 1);
                break;
            default:
                e = ent_new(g, type, wx + ((type == E_COIN || type == E_GEM) ? 0 : 0), wy - 2);
                break;
            }
            if (!e) continue;
            e->flip = (uint8_t)((h >> 9) & 1);
            if (type == E_PINE || type == E_PALM || type == E_BUSH || type == E_FERN) {
                e->x += (float)((int)((h >> 11) % 7) - 3);
                e->t = (float)(h % 100) * 0.0628f;
            }
            if (type == E_SHACK || type == E_LIGHTHOUSE || type == E_SKULLWALL || type == E_GATE || type == E_SIGN ||
                type == E_CARVING || type == E_CHEST || type == E_BIGCHEST || type == E_CRANE || type == E_CAGE || type == E_STATUE ||
                type == E_WRECK || type == E_MAST || type == E_ROWBOAT)
                e->flip = 0;
            if (type == E_ROWBOAT && (h & 1)) e->flip = 1;
        }
    g->trail_head = 0;
    party_reset(g, sx, sy, DIR_DOWN);
    g->chk_x = sx; g->chk_y = sy;
    g->camx = sx - VIEW_W * 0.5f; g->camy = sy - VIEW_H * 0.5f;
}
