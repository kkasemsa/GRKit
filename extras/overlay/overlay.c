/* Overlay: film artefacts drawn into each recorded picture.

   Vignette   the corners darken, as through a cheap or a wide lens.
   Dust       specks and hairs, as on a scanned negative.
   Leak       light that got into the cassette: a warm wash from one side, a
              band across the frame, or a flare from a corner.
   Scratches  thin pale lines along the film, from the gate.

   Each is Off, Light, Medium, or Strong. The LOOK row chooses Custom, which
   uses the four rows, or a preset. The dust, the leak, and the scratches
   fall differently on every shot: a seed comes from the picture itself. The
   artefacts follow the upright picture, so a vertical shot gets its leak
   along its own edge. Every picture of the shot gets the same artefacts in
   its own scale, so the thumbnail matches the main picture. */
#include "grkit.h"
#include "grkit_module.h"

#define LOOK_CUSTOM 1U
#define LOOK_FADED 2U
#define LOOK_DUSTY 3U
#define LOOK_LEAKY 4U
#define LOOK_EXPIRED 5U

#define BYTE(p,o) (GRKIT_BYTES((p)+(o))[0])

/* The strengths of the four artefacts, 0 to 3. */
typedef struct { u32 vignette, dust, leak, scratch; } Strengths;

void overlay_strengths(u32 look, Strengths *s)
{
    switch (look) {
    case LOOK_CUSTOM:
        s->vignette = setting_get(SETTING_VIGNETTE);
        s->dust = setting_get(SETTING_DUST);
        s->leak = setting_get(SETTING_LEAK);
        s->scratch = setting_get(SETTING_SCRATCH);
        break;
    case LOOK_FADED: s->vignette = 2; s->dust = 0; s->leak = 0; s->scratch = 0; break;
    case LOOK_DUSTY: s->vignette = 1; s->dust = 2; s->leak = 0; s->scratch = 1; break;
    case LOOK_LEAKY: s->vignette = 1; s->dust = 0; s->leak = 2; s->scratch = 0; break;
    case LOOK_EXPIRED: s->vignette = 2; s->dust = 2; s->leak = 1; s->scratch = 2; break;
    default: s->vignette = s->dust = s->leak = s->scratch = 0; break;
    }
}

/* ---- a view of one picture, upright ---------------------------------------- */

typedef struct {
    const Plane *p;
    u32 turns, mono;
    u32 w, h;          /* the upright size */
    u32 seed;
} View;

/* The sensor pixel of upright (u, v). */
static void sensor(const View *v, int u, int w, int *x, int *y)
{
    int L = (int)v->p->left, T = (int)v->p->top, W = (int)(v->p->right - v->p->left), H = (int)(v->p->bottom - v->p->top);
    switch (v->turns) {
    case 0: *x = L + u; *y = T + w; break;
    case 1: *x = L + w; *y = T + H - 1 - u; break;
    case 2: *x = L + W - 1 - u; *y = T + H - 1 - w; break;
    default: *x = L + W - 1 - w; *y = T + u; break;
    }
}

static u32 clamp8(int v) { return v < 0 ? 0U : v > 255 ? 255U : (u32)v; }

/* Add dy to the luma and (dcb, dcr) to the chroma of the sensor pixel. */
static void add_px(const View *v, int x, int y, int dy, int dcb, int dcr)
{
    const Plane *p = v->p;
    u32 l = p->y + (u32)y * p->pitch;
    if (x < (int)p->left || x >= (int)p->right || y < (int)p->top || y >= (int)p->bottom) return;
    BYTE(l, x) = (u8)clamp8((int)BYTE(l, x) + dy);
    if ((dcb || dcr) && !v->mono && !(x & 1) && (!p->half || !(y & 1))) {
        u32 c = chroma_row(p, (u32)y);
        BYTE(c, x) = (u8)clamp8((int)BYTE(c, x) + dcb);
        BYTE(c, x + 1) = (u8)clamp8((int)BYTE(c, x + 1) + dcr);
    }
}

/* A small random number generator, xorshift. */
static u32 next(u32 *s) { u32 x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return *s = x; }
/* A random number below n. A module has no division by a variable except
   through divide(). */
static u32 rnd(u32 *s, u32 n) { u32 r = next(s) >> 8; return n ? r - divide(r, n) * n : 0; }

/* A seed from the picture: sums of a few luma samples of the largest plane.
   The same shot gives the same seed in each of its planes only if sampled
   from one plane, so overlay_apply passes it down. */
u32 overlay_seed(const Plane *p)
{
    u32 s = 0x9e3779b9U, i;
    for (i = 0; i < 64; ++i) {
        u32 x = p->left + ((p->right - p->left) * (i * 37 % 64)) / 64;
        u32 y = p->top + ((p->bottom - p->top) * (i * 11 % 64)) / 64;
        s = (s ^ BYTE(p->y + y * p->pitch, x)) * 0x01000193U;
    }
    return s ? s : 1;
}

/* ---- vignette --------------------------------------------------------------- */

/* The vignette darkens by strength * d^2 / 4096 of the luma, where d is the
   distance from the centre in 1/64 of the half diagonal: full strength at the
   corners, none at the centre. Per-row and per-column squares come from
   scratch. Monochrome and colour alike. */
static void vignette(const View *v, u32 strength, u8 *scratch)
{
    const Plane *p = v->p;
    u32 W = p->right - p->left, H = p->bottom - p->top, x, y;
    u16 *dx2 = (u16 *)scratch, *dy2 = dx2 + W;
    /* d^2 in 1/4096 of the squared half diagonal, split into x and y parts. */
    u32 half2 = (W * W + H * H) / 4;
    u32 k = strength == 1 ? 28 : strength == 2 ? 44 : 64;   /* darkening at the corner, in 1/100 */
    for (x = 0; x < W; ++x) { int d = (int)x - (int)W / 2; dx2[x] = (u16)divide((u32)(d * d) * 4096U, half2); }
    for (y = 0; y < H; ++y) { int d = (int)y - (int)H / 2; dy2[y] = (u16)divide((u32)(d * d) * 4096U, half2); }
    for (y = 0; y < H; ++y) {
        u32 l = p->y + (p->top + y) * p->pitch + p->left;
        u32 ry = dy2[y];
        for (x = 0; x < W; ++x) {
            u32 d2 = ry + dx2[x];                  /* 0 at centre, 4096 at the corner */
            if (d2 < 900) continue;                /* a clear centre */
            d2 -= 900;
            /* smooth: (d2/3196)^1.5-ish via d2*d2 >> 12 then scale */
            u32 f = (d2 * d2) >> 12;               /* 0..~2500 */
            u32 cut = (f * k) / 2500U;             /* 0..k percent */
            if (cut > k) cut = k;
            u32 yv = BYTE(l, x);
            BYTE(l, x) = (u8)(yv - (yv * cut) / 100U);
        }
    }
}

/* ---- dust and hairs ---------------------------------------------------------- */

/* Blend the sensor pixel's luma towards `target` by alpha a (0..256): the
   scanner's dust is translucent, not ink. */
static void blend_px(const View *v, int x, int y, u32 target, u32 a)
{
    const Plane *p = v->p;
    u32 l;
    if (x < (int)p->left || x >= (int)p->right || y < (int)p->top || y >= (int)p->bottom || !a) return;
    l = p->y + (u32)y * p->pitch;
    {
        u32 yv = BYTE(l, x);
        BYTE(l, x) = (u8)(yv + (((int)target - (int)yv) * (int)a >> 8));
    }
}

/* A speck: a soft disc of radius r (in 1/4 px) at upright (u, w), full
   alpha a at the centre, fading to the edge. Dark specks are dust on the
   negative; pale ones are dust on the print. */
static void speck(const View *v, int u, int w, int r4, u32 target, u32 a)
{
    int r = (r4 + 3) / 4, du, dw, x, y;
    if (r < 1) r = 1;
    for (dw = -r; dw <= r; ++dw)
        for (du = -r; du <= r; ++du) {
            int d2 = du * du * 16 + dw * dw * 16;      /* in 1/16 px^2 */
            int r2 = r4 * r4;
            u32 f;
            if (d2 >= r2) continue;
            f = 256U - (u32)divide((u32)d2 * 256U, (u32)r2);   /* 256 at the centre, 0 at the edge */
            f = (f * f) >> 8;                              /* a soft shoulder */
            sensor(v, u + du, w + dw, &x, &y);
            blend_px(v, x, y, target, (a * f) >> 8);
        }
}

/* A hair: a thin curved fibre of length n from (u, w), pale, faint, with a
   slightly darker core, as a fibre on a scanner's glass. */
static void hair(const View *v, u32 *rs, int u, int w, int n, u32 a, int thick)
{
    int i, x, y, ddu = (int)rnd(rs, 3) - 1, ddw = (int)rnd(rs, 3) - 1;
    int fu = u * 16, fw = w * 16, vu = (int)rnd(rs, 33) - 16, vw = (int)rnd(rs, 33) - 16;
    if (vu == 0 && vw == 0) vu = 11;
    for (i = 0; i < n; ++i) {
        int t;
        fu += vu; fw += vw;
        if ((i & 15) == 0) { vu += ddu; vw += ddw; if (rnd(rs, 4) == 0) { ddu = (int)rnd(rs, 3) - 1; ddw = (int)rnd(rs, 3) - 1; } }
        if (vu > 16) vu = 16;
        if (vu < -16) vu = -16;
        if (vw > 16) vw = 16;
        if (vw < -16) vw = -16;
        /* The alpha wanders along the fibre: a hair is never one even line. */
        {
            u32 ai = (a * (160U + ((u32)(i * 7) & 95U))) >> 8;
            for (t = 0; t < thick; ++t) {
                /* perpendicular offset: thicker fibres are a few pixels wide, fainter at the sides */
                int side = t - thick / 2, px = fu / 16 + (vw > 0 ? side : -side) * (vw != 0), py = fw / 16 + (vu != 0 ? side : 0);
                u32 as = side == 0 ? ai : ai / 2;
                sensor(v, px, py, &x, &y);
                blend_px(v, x, y, 235, as);
            }
        }
    }
}

static void dust(const View *v, u32 strength)
{
    u32 rs = v->seed ^ 0xd0d0U, i;
    u32 scale = v->w / 300;                        /* 1 at 300 px wide, 20 at 6000 */
    if (!scale) scale = 1;
    u32 n = (strength == 1 ? 40 : strength == 2 ? 110 : 260);
    u32 hairs = strength == 1 ? 1 : strength == 2 ? 3 : 7;
    for (i = 0; i < n; ++i) {
        int u = (int)rnd(&rs, v->w), w = (int)rnd(&rs, v->h);
        /* Mostly tiny specks; a few larger blots. Radius in 1/4 px. */
        u32 big = rnd(&rs, 10) == 0;
        int r4 = (int)(scale * (big ? 6 + rnd(&rs, 10) : 2 + rnd(&rs, 5)));
        u32 dark = rnd(&rs, 3) != 0;               /* two in three are dark */
        u32 a = 70 + rnd(&rs, 150);                /* translucent, varied */
        if (big) a = a / 2 + 20;
        speck(v, u, w, r4, dark ? 24U : 236U, a);
    }
    for (i = 0; i < hairs; ++i) {
        int u = (int)rnd(&rs, v->w), w = (int)rnd(&rs, v->h);
        hair(v, &rs, u, w, (int)(scale * (25 + rnd(&rs, 70))), 90 + rnd(&rs, 90), scale >= 6 ? 3 : scale >= 3 ? 2 : 1);
    }
}

/* ---- light leak --------------------------------------------------------------- */

/* The leak: a warm wash. style 0 a wash from the left or right edge, 1 a
   vertical band, 2 a flare from a top corner. The strength sets the width
   and the brightness. Luma rises and the chroma turns orange-red. */
static void leak(const View *v, u32 strength)
{
    u32 rs = v->seed ^ 0x1ea4U, style = rnd(&rs, 3), side = rnd(&rs, 2);
    u32 W = v->w, H = v->h, u, w;
    int x, y;
    u32 width = (W * (strength == 1 ? 18 : strength == 2 ? 28 : 40)) / 100;
    int peak = strength == 1 ? 70 : strength == 2 ? 110 : 150;
    u32 band_at = width / 2 + rnd(&rs, W - width);
    u32 step = v->w >= 1200 ? 2 : 1;               /* every pixel on small planes */
    u32 period = H / 3 > 8 ? H / 3 : 8;
    for (w = 0; w < H; w += 1) {
        /* A slow ripple down the frame, so the leak is not a flat wash: a
           triangle wave of period H/3, 208 to 255 in 1/256. */
        u32 t = divide(w * 256, period) & 255, rip = t < 128 ? t : 255 - t;
        u32 gain = 208 + (rip * 48) / 128;
        for (u = 0; u < W; u += step) {
            u32 a;                                 /* 0..256 */
            if (style == 0) {
                u32 d = side ? W - 1 - u : u;
                if (d >= width) continue;
                a = 256 - divide(d * 256, width);
                a = (a * a) >> 8;
            } else if (style == 1) {
                u32 d = u > band_at ? u - band_at : band_at - u;
                if (d >= width / 2) continue;
                a = 256 - divide(d * 512, width);
            } else {
                u32 du = side ? W - 1 - u : u, dw = w;
                u32 d = du + dw;                   /* a diagonal fall-off from the corner */
                if (d >= width * 2) continue;
                a = 256 - divide(d * 128, width);
            }
            a = (a * gain) >> 8;
            {
                int dy = (peak * (int)a) >> 8;
                int dcr = (dy * 3) / 8, dcb = -(dy / 5);
                sensor(v, (int)u, (int)w, &x, &y);
                add_px(v, x, y, dy, dcb, dcr);
                if (step == 2) add_px(v, x + (v->turns & 1 ? 0 : 1), y + (v->turns & 1 ? 1 : 0), dy, dcb, dcr);
            }
        }
    }
}

/* ---- scratches ----------------------------------------------------------------- */

/* Thin pale lines along the long side of the film, which is the sensor's x
   axis in either posture. A scratch is faint and uneven: its alpha wanders
   along its length and it fades in and out. */
static void scratches(const View *v, u32 strength)
{
    u32 rs = v->seed ^ 0x5c7aU, i, n = strength == 1 ? 2 : strength == 2 ? 4 : 8;
    const Plane *p = v->p;
    u32 W = p->right - p->left, H = p->bottom - p->top;
    u32 scale = W / 1500; if (!scale) scale = 1;
    for (i = 0; i < n; ++i) {
        u32 y0 = p->top + rnd(&rs, H), len = W / 5 + rnd(&rs, W - W / 5), x0 = p->left + rnd(&rs, W - len);
        u32 thick = 1 + rnd(&rs, scale), x;
        u32 a0 = 60 + rnd(&rs, 110), wobble = 0, fade_in = len / 8 + 1, segment = 0;
        u32 bright = rnd(&rs, 5) != 0;             /* most scratches are pale, some dark */
        for (x = x0; x < x0 + len; ++x) {
            u32 t, a = a0, d = x - x0;
            if ((x & 63) == 0) { wobble = rnd(&rs, 3); segment = rnd(&rs, 4); }
            if (d < fade_in) a = divide(a * d, fade_in);
            if (len - d < fade_in) a = divide(a * (len - d), fade_in);
            if (segment == 0) a = a / 3;           /* a faint stretch */
            for (t = 0; t < thick; ++t) {
                int yy = (int)(y0 + t + wobble) - 1;
                u32 at = thick > 1 && (t == 0 || t == thick - 1) ? a / 2 : a;
                if (yy >= (int)p->top && yy < (int)p->bottom) {
                    u32 l = p->y + (u32)yy * p->pitch, yv = BYTE(l, x), target = bright ? 240U : 30U;
                    BYTE(l, x) = (u8)(yv + (((int)target - (int)yv) * (int)at >> 8));
                }
            }
        }
    }
}

/* ---- the filter --------------------------------------------------------------- */

void overlay_plane(const Plane *p, u32 turns, u32 mono, u32 seed, const Strengths *s, u8 *scratch)
{
    View v;
    u32 W = p->right - p->left, H = p->bottom - p->top;
    v.p = p; v.turns = turns & 3; v.mono = mono; v.seed = seed;
    v.w = v.turns & 1 ? H : W;
    v.h = v.turns & 1 ? W : H;
    if (W < 16 || H < 16) return;
    if (s->leak) leak(&v, s->leak);
    if (s->vignette) vignette(&v, s->vignette, scratch);
    if (s->dust) dust(&v, s->dust);
    if (s->scratch) scratches(&v, s->scratch);
}

void overlay_apply(PictureSet *set, u32 look)
{
    Strengths s;
    u32 i, main = 0, seed;
    if (!look || look > LOOK_EXPIRED || !set->count) return;
    overlay_strengths(look, &s);
    if (!(s.vignette | s.dust | s.leak | s.scratch)) return;
    for (i = 1; i < set->count; ++i)
        if (set->planes[i].width * set->planes[i].height > set->planes[main].width * set->planes[main].height) main = i;
    seed = overlay_seed(&set->planes[main]);
    for (i = 0; i < set->count; ++i)
        overlay_plane(&set->planes[i], set->turns, set->mono, seed, &s, set->scratch);
}
