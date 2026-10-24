/*
 * TREASURE HOUNDS - the four kids: the leader you steer, the three who follow in a line,
 * the whip, swapping leaders, getting hurt, catching your breath, swinging and falling.
 */
#include "game_int.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

#define WALK_SPD 64.0f
#define RUN_SPD  104.0f
#define KID_R    5.0f

Kid *leader(Game *g) { return &g->kid[g->order[0]]; }
int leader_id(const Game *g) { return g->order[0]; }

static void trail_push(Game *g, float x, float y)
{
    g->trail_head = (g->trail_head + 1) % TRAIL_N;
    g->trail_x[g->trail_head] = x;
    g->trail_y[g->trail_head] = y;
}
static void trail_get(const Game *g, int back, float *x, float *y)
{
    int i = ((g->trail_head - back) % TRAIL_N + TRAIL_N) % TRAIL_N;
    *x = g->trail_x[i]; *y = g->trail_y[i];
}

/* lay a fresh trail through the kids' current positions, leader first */
void trail_rebuild(Game *g)
{
    float pts[NKIDS + 1][2];
    for (int i = 0; i < NKIDS; i++) { pts[i][0] = g->kid[g->order[i]].x; pts[i][1] = g->kid[g->order[i]].y; }
    /* extend past the last kid a little */
    pts[NKIDS][0] = pts[NKIDS - 1][0]; pts[NKIDS][1] = pts[NKIDS - 1][1];
    float buf[TRAIL_N][2];
    int n = 0;
    for (int i = 0; i < NKIDS && n < TRAIL_N; i++) {
        float ax = pts[i][0], ay = pts[i][1], bx = pts[i + 1][0], by = pts[i + 1][1];
        for (int k = 0; k < TRAIL_GAP && n < TRAIL_N; k++) {
            float t = (float)k / (float)TRAIL_GAP;
            buf[n][0] = ax + (bx - ax) * t; buf[n][1] = ay + (by - ay) * t; n++;
        }
    }
    while (n < TRAIL_N) { buf[n][0] = pts[NKIDS][0]; buf[n][1] = pts[NKIDS][1]; n++; }
    /* newest sample is the leader */
    for (int i = 0; i < TRAIL_N; i++) {
        int idx = ((g->trail_head - i) % TRAIL_N + TRAIL_N) % TRAIL_N;
        g->trail_x[idx] = buf[i][0]; g->trail_y[idx] = buf[i][1];
    }
    g->trail_acc = 0;
}

void party_reset(Game *g, float x, float y, int dir)
{
    for (int i = 0; i < NKIDS; i++) {
        Kid *k = &g->kid[i];
        int hp = k->hp;
        memset(k, 0, sizeof(*k));
        k->hp = hp > 0 ? hp : KID_HP;
        k->x = x; k->y = y + (float)i * 2.0f; k->dir = dir;
        k->blink = 2.0f + (float)i * 0.7f;
    }
    /* line them up behind the leader, in whichever direction has room */
    static const float ddx[4] = { 0, 0, -1, 1 }, ddy[4] = { -1, 1, 0, 0 };
    int best = 0;
    for (int d = 0; d < 4; d++) {
        int ok = 1;
        for (int i = 1; i < NKIDS && ok; i++) if (blocked(g, x + ddx[d] * 14.0f * (float)i, y + ddy[d] * 14.0f * (float)i, 4, -1)) ok = 0;
        if (ok) { best = d; break; }
    }
    for (int i = 0; i < NKIDS; i++) {
        Kid *k = &g->kid[g->order[i]];
        k->x = x + ddx[best] * 14.0f * (float)i; k->y = y + ddy[best] * 14.0f * (float)i;
        if (blocked(g, k->x, k->y, 4, -1)) { k->x = x; k->y = y; }
    }
    for (int i = 0; i < TRAIL_N; i++) { g->trail_x[i] = x; g->trail_y[i] = y; }
    trail_rebuild(g);
    g->trail_acc = 0;
    g->safe_x = x; g->safe_y = y;
    g->whip_t = 0;
    g->lead_dir = dir;
    g->lead_flip = 0;
}

void kid_say(Game *g, int who, const char *s, float dur)
{
    if (who < 0 || who >= NKIDS) return;
    Kid *k = &g->kid[who];
    snprintf(k->bubble, sizeof(k->bubble), "%s", s);
    if (dur <= 0) dur = 1.6f + (float)strlen(s) * 0.055f;
    k->bubble_t = k->bubble_max = dur;
    /* only one bubble at a time reads well: hush the others */
    for (int i = 0; i < NKIDS; i++) if (i != who && g->kid[i].bubble_t > 0.3f) g->kid[i].bubble_t = 0.3f;
}

void party_walk_to(Game *g, int k, float x, float y, float dur, int hold)
{
    Kid *kid = &g->kid[k];
    kid->sx = kid->x; kid->sy = kid->y; kid->tx = x; kid->ty = y;
    kid->mt = 0; kid->mdur = dur > 0.05f ? dur : 0.05f;
    kid->state = hold ? KS_HOLD : KS_WALKTO;
}

void party_regroup(Game *g)
{
    for (int i = 0; i < NKIDS; i++) if (g->kid[i].state == KS_HOLD || g->kid[i].state == KS_CHEER) g->kid[i].state = KS_FOLLOW;
}

int party_busy(const Game *g)
{
    for (int i = 0; i < NKIDS; i++) {
        int s = g->kid[i].state;
        if (s == KS_SWING || s == KS_FALL) return 1;
        if (s == KS_HOLD && g->kid[i].mt < g->kid[i].mdur) return 1;
    }
    const Kid *l = &g->kid[g->order[0]];
    if (l->state == KS_WALKTO || l->state == KS_HOLD) return 1;
    return 0;
}

/* everyone swings across on the hook, one after another, leader first */
void party_swing(Game *g, float lx, float ly, float hx, float hy)
{
    (void)hx; (void)hy;
    for (int i = 0; i < NKIDS; i++) {
        Kid *k = &g->kid[g->order[i]];
        k->state = KS_SWING;
        k->sx = k->x; k->sy = k->y;
        k->tx = lx + (float)(i == 0 ? 0 : ((i & 1) ? -10 : 10)) * 0.0f;
        k->ty = ly + (float)i * 0.0f;
        k->mt = -(float)i * 0.45f;           /* staggered start */
        k->mdur = 0.75f;
    }
    sfx(g, SFX_SWING, 128, 0, 200);
}

void party_fall(Game *g)
{
    Kid *l = leader(g);
    if (l->state == KS_FALL) return;
    l->state = KS_FALL;
    l->mt = 0; l->mdur = 0.7f;
    sfx(g, SFX_FALL, 128, 0, 220);
}

static void faint_check(Game *g)
{
    Kid *l = leader(g);
    if (l->hp > 0) return;
    int alive = 0;
    for (int i = 0; i < NKIDS; i++) if (g->kid[i].hp > 0) alive++;
    if (!alive) {
        g->state = ST_GAMEOVER;
        g->st_t = 0;
        sfx(g, SFX_GAMEOVER, 128, 0, 230);
        return;
    }
    sfx(g, SFX_FAINT, 128, 0, 200);
    int who = g->order[0];
    kid_say(g, who, SAY_FAINT[who], 2.2f);
    /* the next Hound with breath left steps up; the tired one goes to the back */
    int nord[NKIDS], n = 0;
    for (int i = 1; i < NKIDS; i++) if (g->kid[g->order[i]].hp > 0) nord[n++] = g->order[i];
    for (int i = 1; i < NKIDS; i++) if (g->kid[g->order[i]].hp <= 0) nord[n++] = g->order[i];
    nord[n++] = who;
    Kid *nl = &g->kid[nord[0]];
    float lx = nl->x, ly = nl->y;
    nl->x = l->x; nl->y = l->y;              /* the new leader steps into the front spot */
    l->x = lx; l->y = ly;
    memcpy(g->order, nord, sizeof(nord));
    trail_rebuild(g);
}

void hurt_leader(Game *g, int dmg, float fx, float fy)
{
    Kid *l = leader(g);
    if (l->inv > 0 || l->state == KS_FALL || l->state == KS_SWING) return;
    if (g->state != ST_PLAY) return;
    l->hp -= dmg;
    if (l->hp < 0) l->hp = 0;
    l->inv = 1.3f;
    l->hurt = 0.35f;
    float dx = l->x - fx, dy = l->y - fy, d = sqrtf(dx * dx + dy * dy);
    if (d > 0.01f) { g->lead_vx = dx / d * 150.0f; g->lead_vy = dy / d * 150.0f; }
    g->shake = fmaxf(g->shake, 4.0f);
    sfx(g, SFX_HURT, 110 + (int)(frand(g) * 30) + leader_id(g) * 8, 0, 220);
    burst(g, PK_STAR, l->x, l->y - 18, 5, 40, 0xFFE860FFu);
    if (l->hp > 0 && frand(g) < 0.6f) kid_say(g, leader_id(g), SAY_HURT[leader_id(g)][(int)(frand(g) * 4) & 3], 1.4f);
    faint_check(g);
}

void party_heal(Game *g, int amt)
{
    for (int i = 0; i < NKIDS; i++) {
        g->kid[i].hp += amt;
        if (g->kid[i].hp > KID_HP) g->kid[i].hp = KID_HP;
    }
}

static void set_leader(Game *g, int step)
{
    /* rotate the line so the next Hound with enough breath leads */
    for (int tries = 1; tries < NKIDS; tries++) {
        int nord[NKIDS];
        int s = (step > 0) ? tries : NKIDS - tries;
        for (int i = 0; i < NKIDS; i++) nord[i] = g->order[(i + s) % NKIDS];
        if (g->kid[nord[0]].hp < 1) continue;
        /* the kids swap places: new leader walks to the front spot */
        float fx[NKIDS], fy[NKIDS];
        for (int i = 0; i < NKIDS; i++) { fx[i] = g->kid[g->order[i]].x; fy[i] = g->kid[g->order[i]].y; }
        memcpy(g->order, nord, sizeof(nord));
        for (int i = 0; i < NKIDS; i++) { g->kid[g->order[i]].x = fx[i]; g->kid[g->order[i]].y = fy[i]; }
        trail_rebuild(g);
        Kid *l = leader(g);
        l->dir = g->lead_dir; l->flip = g->lead_flip;
        burst(g, PK_SPARK, l->x, l->y - 14, 8, 50, 0xFFE8A0FFu);
        sfx(g, SFX_SELECT, 120 + leader_id(g) * 10, 0, 140);
        kid_say(g, leader_id(g), SAY_LEAD[leader_id(g)][(int)(frand(g) * 3) % 3], 1.6f);
        return;
    }
    sfx(g, SFX_WRONG, 128, 0, 120);
}

static void anim_kid(Kid *k, float spd, float dt)
{
    if (spd > 2.0f) {
        k->anim += dt * (spd > 80 ? 11.0f : 8.0f);
        k->frame = FR_WALK0 + ((int)k->anim & 3);
    } else {
        k->anim += dt * 1.6f;
        k->frame = ((int)k->anim & 1) ? FR_IDLE2 : FR_IDLE;
    }
}

static void face_from(Kid *k, float dx, float dy)
{
    if (fabsf(dx) < 0.05f && fabsf(dy) < 0.05f) return;
    if (fabsf(dx) > fabsf(dy) * 1.1f) { k->dir = DIR_SIDE; k->flip = dx < 0; }
    else k->dir = dy < 0 ? DIR_UP : DIR_DOWN;
}

/* footsteps: a soft tap per step, flavoured by the ground */
static void footstep(Game *g, Kid *k, int i, float dt, float spd)
{
    if (spd < 2) return;
    k->step_t -= dt * (spd > 80 ? 1.5f : 1.0f);
    if (k->step_t > 0) return;
    k->step_t = 0.27f;
    int tk = tile_at(g, (int)(k->x / TS), (int)(k->y / TS));
    int step = KINFO[tk].step;
    int pitch = step == 3 ? 150 : (step == 2 ? 120 : (step == 4 ? 90 : 105));
    if (i == 0) sfx_at(g, SFX_STEP, k->x, k->y, pitch + (int)(frand(g) * 16));
    if (KINFO[tk].water == 2 || (step == 4 && g->lv.theme != THEME_CAVE && frand(g) < 0.5f)) {
        Part *p = part_new(g, PK_SPLASH, k->x, k->y);
        if (p) { p->life = p->max = 0.35f; p->size = 3; }
        if (i == 0 && KINFO[tk].water == 2) sfx_at(g, SFX_SPLASH, k->x, k->y, 150);
    }
}

void party_update(Game *g, const Input *in, float dt)
{
    Kid *l = leader(g);
    int playing = g->state == ST_PLAY;
    int busy = party_busy(g);

    /* ---- the leader ---- */
    float mx = 0, my = 0;
    if (playing && !busy && l->state == KS_FOLLOW) {
        mx = in->sx; my = in->sy;
        if (in->held & BTN_LEFT) mx = -1;
        if (in->held & BTN_RIGHT) mx = 1;
        if (in->held & BTN_UP) my = -1;
        if (in->held & BTN_DOWN) my = 1;
        float m = sqrtf(mx * mx + my * my);
        if (m > 1.0f) { mx /= m; my /= m; }
        if (in->pressed & BTN_L) set_leader(g, -1);
        if (in->pressed & BTN_R) set_leader(g, 1);
        l = leader(g);
    }
    g->running = (in->held & BTN_Y) != 0;
    float spd = g->running ? RUN_SPD : WALK_SPD;
    if (g->whip_t > 0) spd *= 0.25f;
    float vx = mx * spd, vy = my * spd;
    /* knockback */
    vx += g->lead_vx; vy += g->lead_vy;
    g->lead_vx *= expf(-dt * 9.0f); g->lead_vy *= expf(-dt * 9.0f);
    if (fabsf(g->lead_vx) < 1) g->lead_vx = 0;
    if (fabsf(g->lead_vy) < 1) g->lead_vy = 0;

    float ox = l->x, oy = l->y;
    if (l->state == KS_FOLLOW && (vx != 0 || vy != 0)) {
        move_circle(g, &l->x, &l->y, vx * dt, vy * dt, KID_R, -1);
        if (mx != 0 || my != 0) {
            face_from(l, mx, my);
            g->lead_dir = l->dir; g->lead_flip = l->flip;
        }
    }
    float moved = sqrtf((l->x - ox) * (l->x - ox) + (l->y - oy) * (l->y - oy));
    float lspd = moved / (dt > 0 ? dt : 1);
    if (l->state == KS_FOLLOW) {
        g->trail_acc += moved;
        while (g->trail_acc >= 2.0f) {
            g->trail_acc -= 2.0f;
            trail_push(g, l->x, l->y);
        }
        if (g->trail_acc > 0) { g->trail_x[g->trail_head] = l->x; g->trail_y[g->trail_head] = l->y; }
    }

    /* pushing a stone block: keep walking into it */
    g->push_t = (playing && (mx != 0 || my != 0) && moved < 0.2f * dt * spd) ? g->push_t + dt : 0;

    /* the whip */
    if (g->whip_cool > 0) g->whip_cool -= dt;
    if (playing && !busy && l->state == KS_FOLLOW && (in->pressed & BTN_B) && g->whip_cool <= 0) {
        g->whip_t = 0.34f; g->whip_cool = 0.42f; g->whip_hit_done = 0;
        sfx(g, SFX_WHIP, 120 + (int)(frand(g) * 20), 0, 200);
    }
    if (g->whip_t > 0) {
        g->whip_t -= dt;
        float age = 0.34f - g->whip_t;
        l->frame = age < 0.09f ? FR_WHIP0 : (age < 0.22f ? FR_WHIP1 : FR_WHIP2);
        if (age >= 0.1f && !g->whip_hit_done) {
            g->whip_hit_done = 1;
            float fx = 0, fy = 0;
            if (l->dir == DIR_SIDE) fx = l->flip ? -1.0f : 1.0f;
            else fy = l->dir == DIR_UP ? -1.0f : 1.0f;
            float hx = l->x + fx * 4, hy = l->y - 10 + fy * 2;
            ents_whip(g, hx, hy, hx + fx * 44, hy + fy * 40);
        }
    } else if (l->hurt > 0) l->frame = FR_HURT;
    else if (l->state == KS_FOLLOW) anim_kid(l, lspd, dt);

    if (playing && !busy && l->state == KS_FOLLOW && (in->pressed & BTN_A)) ents_use(g, l->x, l->y, l->dir);

    /* safe ground, for when someone falls in a hole */
    g->safe_t -= dt;
    if (g->safe_t <= 0 && l->state == KS_FOLLOW && l->z == 0) {
        int tk = tile_at(g, (int)(l->x / TS), (int)(l->y / TS));
        int ok = tk != K_CRUMBLE && tk != K_SPIKES && !KINFO[tk].solid && !blocked(g, l->x, l->y, KID_R + 2, -1);
        if (ok) { g->safe_x = l->x; g->safe_y = l->y; g->safe_t = 0.4f; }
    }

    /* ---- everyone ---- */
    for (int i = 0; i < NKIDS; i++) {
        int id = g->order[i];
        Kid *k = &g->kid[id];
        float px = k->x, py = k->y;
        if (k->inv > 0) k->inv -= dt;
        if (k->hurt > 0) k->hurt -= dt;
        if (k->bubble_t > 0) k->bubble_t -= dt;
        k->blink -= dt;
        if (k->blink < 0) k->blink = 2.5f + (float)((id * 37 + (int)(g->time * 10)) % 30) * 0.1f;
        /* catching their breath at the back of the line */
        if (i > 0 && k->hp < KID_HP && g->state == ST_PLAY) {
            k->regen += dt;
            if (k->regen > (k->hp == 0 ? 5.0f : 8.0f)) {
                k->regen = 0; k->hp++;
                Part *p = part_new(g, PK_HEART, k->x, k->y - 26);
                if (p) { p->vy = -16; p->life = p->max = 1.0f; }
                sfx(g, SFX_HEAL, 150, 0, 70);
            }
        } else k->regen = 0;

        switch (k->state) {
        case KS_FOLLOW:
            if (i > 0) {
                float tx, ty;
                trail_get(g, i * TRAIL_GAP, &tx, &ty);
                float dx = tx - k->x, dy = ty - k->y, d = sqrtf(dx * dx + dy * dy);
                if (d > 0.05f) {
                    float step = fminf(d, fmaxf(RUN_SPD * 1.3f * dt, d * 12.0f * dt));
                    k->x += dx / d * step; k->y += dy / d * step;
                    if (d > 0.6f) face_from(k, dx, dy);
                }
                if (d > 80) { k->x = tx; k->y = ty; }
                float ks = sqrtf((k->x - px) * (k->x - px) + (k->y - py) * (k->y - py)) / (dt > 0 ? dt : 1);
                if (k->hurt > 0) k->frame = FR_HURT; else anim_kid(k, ks, dt);
                /* little hop over holes in the floor */
                int tk = tile_at(g, (int)(k->x / TS), (int)(k->y / TS));
                if ((tk == K_HOLE || tk == K_VOID) && k->z == 0) k->vz = 90;
                footstep(g, k, i, dt, ks);
            } else footstep(g, k, 0, dt, lspd);
            break;
        case KS_WALKTO:
        case KS_HOLD: {
            k->mt += dt;
            float t = clampf(k->mt / k->mdur, 0, 1);
            float nx = lerpf(k->sx, k->tx, t), ny = lerpf(k->sy, k->ty, t);
            float dx = nx - k->x, dy = ny - k->y;
            k->x = nx; k->y = ny;
            if (t < 1) { face_from(k, dx, dy); anim_kid(k, 60, dt); }
            else {
                anim_kid(k, 0, dt);
                if (k->state == KS_WALKTO) { k->state = KS_FOLLOW; if (i == 0) trail_rebuild(g); }
            }
            break;
        }
        case KS_SWING: {
            k->mt += dt;
            if (k->mt < 0) { anim_kid(k, 0, dt); break; }
            float t = clampf(k->mt / k->mdur, 0, 1);
            float e = t * t * (3 - 2 * t);
            k->x = lerpf(k->sx, k->tx, e);
            k->y = lerpf(k->sy, k->ty, e);
            k->z = sinf(t * PI_F) * 22.0f;
            k->frame = FR_CHEER0;
            face_from(k, k->tx - k->sx, k->ty - k->sy);
            if (t >= 1) {
                k->z = 0; k->state = KS_FOLLOW;
                sfx_at(g, SFX_LAND, k->x, k->y, 120);
                burst(g, PK_DUST, k->x, k->y, 6, 30, 0xA89878A0u);
                int all = 1;
                for (int j = 0; j < NKIDS; j++) if (g->kid[j].state == KS_SWING) all = 0;
                if (all) trail_rebuild(g);
            }
            break;
        }
        case KS_FALL: {
            k->mt += dt;
            k->frame = FR_HURT;
            if (k->mt >= k->mdur) {
                /* climb out at the last safe spot, a bit bruised */
                k->state = KS_FOLLOW;
                k->x = g->safe_x; k->y = g->safe_y;
                for (int j = 0; j < NKIDS; j++) { g->kid[j].x = g->safe_x; g->kid[j].y = g->safe_y; }
                trail_rebuild(g);
                k->inv = 0;
                hurt_leader(g, 1, k->x, k->y - 1);
                k->inv = 1.5f;
                g->lead_vx = g->lead_vy = 0;
            }
            break;
        }
        case KS_CHEER:
            k->anim += dt * 4;
            k->frame = ((int)k->anim & 1) ? FR_CHEER1 : FR_CHEER0;
            k->dir = DIR_DOWN;
            break;
        }
        /* little hops */
        if (k->vz != 0 || k->z > 0) {
            if (k->state != KS_SWING) {
                k->z += k->vz * dt; k->vz -= 500 * dt;
                if (k->z <= 0) { k->z = 0; k->vz = 0; }
            }
        }
        if (k->blink < 0.12f && (k->frame == FR_IDLE)) k->frame = FR_IDLE2;
    }
}
