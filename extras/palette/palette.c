/* Palette Card: the main colours of each photo as swatches over its edge.

   Bottom Strip  a band along the bottom of the upright photo, one swatch
                 for each colour.
   Side Strip    a band along the right side, the swatches stacked.
   Dots          a row of round swatches at the bottom left, over the photo.

   The colours are those of the recorded picture, after the Film Look: a
   median cut of a 64 x 43 grid of samples, refined by k-means, most common
   first. Hex codes are the sRGB of each colour (JFIF YCbCr). The swatches
   follow the posture of the camera, and they stay off Roll's film edge. */
#include "grkit.h"
#include "grkit_module.h"

#define LAYOUT_BOTTOM 1U
#define LAYOUT_SIDE 2U
#define LAYOUT_DOTS 3U

#define BYTE(p,o) (GRKIT_BYTES((p)+(o))[0])

/* COLORS: 0 five, 1 three, 2 seven. */
u32 palette_count(u32 colors) { return colors == 1 ? 3U : colors == 2 ? 7U : 5U; }

/* ---- finding the colours --------------------------------------------------- */

#define GRID_W 64U
#define GRID_H 43U
#define SAMPLES (GRID_W * GRID_H)
#define MAX_COLORS 7U

/* The working memory of one shot, in PictureSet.scratch. */
typedef struct {
    u8 c[3][SAMPLES];      /* Y, Cb, Cr of each sample */
    u16 order[SAMPLES];    /* sample indices, in boxes for the median cut */
    u16 tmp[SAMPLES];
    u8 label[SAMPLES];     /* the colour of each sample */
} Work;

typedef struct { u32 y, cb, cr, n; } Color;

/* Chroma counts twice in a distance and in a range: the swatches should
   tell hues apart, not only shades. */
#define CHROMA_WEIGHT 2

/* The film edge of Roll on the sensor frame: its height on a picture of
   height h, as roll.c draws it, or 0 when Roll prints no edge. */
static u32 film_edge(u32 h)
{
    if (!setting_get(USED_ROLL_LENGTH) || !setting_get(USED_ROLL_EDGE)) return 0;
    return h * 82U / 1000U;
}

/* The grid of samples: the mean of 2 x 2 pixels at each grid point of the
   visible area above the film edge. */
static void sample(Work *w, const Plane *p, u32 mono)
{
    u32 width = p->right - p->left, height = p->bottom - p->top - film_edge(p->bottom - p->top);
    u32 gx, gy;
    for (gy = 0; gy < GRID_H; ++gy) {
        u32 y = p->top + (gy * 2 + 1) * height / (GRID_H * 2), y1 = y + 1 < p->bottom ? y + 1 : y;
        u32 l0 = p->y + y * p->pitch, l1 = p->y + y1 * p->pitch, c = chroma_row(p, y);
        for (gx = 0; gx < GRID_W; ++gx) {
            u32 x = (p->left + (gx * 2 + 1) * width / (GRID_W * 2)) & ~1U, i = gy * GRID_W + gx;
            w->c[0][i] = (u8)((BYTE(l0, x) + BYTE(l0, x + 1) + BYTE(l1, x) + BYTE(l1, x + 1) + 2) >> 2);
            w->c[1][i] = mono ? 128 : BYTE(c, x);
            w->c[2][i] = mono ? 128 : BYTE(c, x + 1);
        }
    }
}

/* The weighted range of channel k over order[a, b). */
static u32 range(const Work *w, u32 a, u32 b, u32 k)
{
    u32 lo = 255, hi = 0, i;
    for (i = a; i < b; ++i) {
        u32 v = w->c[k][w->order[i]];
        if (v < lo) lo = v;
        if (v > hi) hi = v;
    }
    return hi < lo ? 0 : (hi - lo) * (k ? CHROMA_WEIGHT : 1);
}

/* Sort order[a, b) by channel k, a counting sort. */
static void sort_box(Work *w, u32 a, u32 b, u32 k)
{
    static u16 start[256];
    u32 i, sum = 0;
    for (i = 0; i < 256; ++i) start[i] = 0;
    for (i = a; i < b; ++i) ++start[w->c[k][w->order[i]]];
    for (i = 0; i < 256; ++i) { u32 n = start[i]; start[i] = (u16)sum; sum += n; }
    for (i = a; i < b; ++i) w->tmp[a + start[w->c[k][w->order[i]]]++] = w->order[i];
    for (i = a; i < b; ++i) w->order[i] = w->tmp[i];
}

static u32 distance(const Work *w, u32 i, const Color *c)
{
    int dy = (int)w->c[0][i] - (int)c->y, db = (int)w->c[1][i] - (int)c->cb, dr = (int)w->c[2][i] - (int)c->cr;
    return (u32)(dy * dy + CHROMA_WEIGHT * CHROMA_WEIGHT * (db * db + dr * dr));
}

/* The k main colours of the samples, most common first. Returns how many
   colours have samples: fewer than k for a picture of few colours. */
u32 palette_find(u8 *scratch, const Plane *p, u32 mono, u32 k, Color *out)
{
    Work *w = (Work *)scratch;
    u32 box[MAX_COLORS + 1], boxes = 1, i, j, pass, found;
    sample(w, p, mono);
    for (i = 0; i < SAMPLES; ++i) w->order[i] = (u16)i;
    box[0] = 0;
    box[1] = SAMPLES;
    /* Median cut: split the box of the widest range at its median. */
    while (boxes < k) {
        u32 best = 0, best_k = 0, best_range = 0;
        for (i = 0; i < boxes; ++i) {
            if (box[i + 1] - box[i] < 2) continue;
            for (j = 0; j < 3; ++j) {
                u32 r = range(w, box[i], box[i + 1], j);
                if (r > best_range) { best_range = r; best = i; best_k = j; }
            }
        }
        if (!best_range) break;
        sort_box(w, box[best], box[best + 1], best_k);
        for (i = boxes + 1; i > best + 1; --i) box[i] = box[i - 1];
        box[best + 1] = (box[best] + box[best + 2]) / 2;
        ++boxes;
    }
    for (i = 0; i < boxes; ++i) {
        u32 s[3] = {0, 0, 0};
        for (j = box[i]; j < box[i + 1]; ++j) {
            s[0] += w->c[0][w->order[j]];
            s[1] += w->c[1][w->order[j]];
            s[2] += w->c[2][w->order[j]];
            w->label[w->order[j]] = (u8)i;
        }
        out[i].n = box[i + 1] - box[i];
        out[i].y = divide(s[0], out[i].n);
        out[i].cb = divide(s[1], out[i].n);
        out[i].cr = divide(s[2], out[i].n);
    }
    /* k-means: move each sample to its nearest colour, then each colour to
       the mean of its samples. */
    for (pass = 0; pass < 6; ++pass) {
        u32 sum[MAX_COLORS][3], n[MAX_COLORS];
        for (i = 0; i < boxes; ++i) sum[i][0] = sum[i][1] = sum[i][2] = n[i] = 0;
        for (j = 0; j < SAMPLES; ++j) {
            u32 near = 0, d = 0xffffffffU;
            for (i = 0; i < boxes; ++i) {
                u32 e = distance(w, j, &out[i]);
                if (e < d) { d = e; near = i; }
            }
            sum[near][0] += w->c[0][j];
            sum[near][1] += w->c[1][j];
            sum[near][2] += w->c[2][j];
            ++n[near];
        }
        for (i = 0; i < boxes; ++i) {
            out[i].n = n[i];
            if (!n[i]) continue;
            out[i].y = divide(sum[i][0] + n[i] / 2, n[i]);
            out[i].cb = divide(sum[i][1] + n[i] / 2, n[i]);
            out[i].cr = divide(sum[i][2] + n[i] / 2, n[i]);
        }
    }
    /* Most common first; colours without samples drop out. */
    for (i = 1; i < boxes; ++i)
        for (j = i; j > 0 && out[j].n > out[j - 1].n; --j) {
            Color t = out[j];
            out[j] = out[j - 1];
            out[j - 1] = t;
        }
    for (found = 0; found < boxes && out[found].n; ++found) {}
    return found;
}

/* The sRGB of a colour, JFIF YCbCr, as 0xRRGGBB. */
static u32 clamp(int v) { return v < 0 ? 0U : v > 255 ? 255U : (u32)v; }
u32 palette_rgb(u32 y, u32 cb, u32 cr)
{
    int b = (int)cb - 128, r = (int)cr - 128, l = (int)y << 16;
    u32 R = clamp((l + 91881 * r + 32768) >> 16);
    u32 G = clamp((l - 22554 * b - 46802 * r + 32768) >> 16);
    u32 B = clamp((l + 116130 * b + 32768) >> 16);
    return R << 16 | G << 8 | B;
}

/* ---- drawing ---------------------------------------------------------------- */

/* 5 x 7 characters, one byte for each row, bit 4 the left column: 0 to 9,
   A to F, and '#'. */
static const u8 font[17][7] = {
    {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},{0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
    {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f},{0x1f,0x02,0x04,0x02,0x01,0x11,0x0e},
    {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},{0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e},
    {0x06,0x08,0x10,0x1e,0x11,0x11,0x0e},{0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},{0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c},
    {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},{0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
    {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e},{0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},{0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
    {0x0a,0x0a,0x1f,0x0a,0x1f,0x0a,0x0a},
};
/* "#RRGGBB" is 7 characters, 6 dots apart, so 41 dots wide and 7 high. */
#define TEXT_DOTS 41U

/* One picture of the shot, seen upright: its plane and posture, and the
   upright area for the swatches, which leaves out the film edge. */
typedef struct {
    const Plane *p;
    u32 turns, mono;
    int l, t, r, b;
} View;

/* Fill the upright box [u0, u1) x [v0, v1) with one colour. */
static void box(const View *v, int u0, int v0, int u1, int v1, u32 y, u32 cb, u32 cr)
{
    const Plane *p = v->p;
    int L = (int)p->left, T = (int)p->top, w = (int)(p->right - p->left), h = (int)(p->bottom - p->top);
    int x0, y0, x1, y1, row, x;
    switch (v->turns & 3) {
    case 0: x0 = L + u0; y0 = T + v0; x1 = L + u1; y1 = T + v1; break;
    case 1: x0 = L + v0; y0 = T + h - u1; x1 = L + v1; y1 = T + h - u0; break;
    case 2: x0 = L + w - u1; y0 = T + h - v1; x1 = L + w - u0; y1 = T + h - v0; break;
    default: x0 = L + w - v1; y0 = T + u0; x1 = L + w - v0; y1 = T + u1; break;
    }
    if (x0 < L) x0 = L;
    if (y0 < T) y0 = T;
    if (x1 > (int)p->right) x1 = (int)p->right;
    if (y1 > (int)p->bottom) y1 = (int)p->bottom;
    if (v->mono) cb = cr = 128;
    for (row = y0; row < y1; ++row) {
        u32 luma = p->y + (u32)row * p->pitch;
        for (x = x0; x < x1; ++x) BYTE(luma, x) = (u8)y;
        if (p->half && (row & 1)) continue;
        {
            u32 c = chroma_row(p, (u32)row);
            for (x = x0 & ~1; x < x1; x += 2) {
                BYTE(c, x) = (u8)cb;
                BYTE(c, x + 1) = (u8)cr;
            }
        }
    }
}

/* The hex code of rgb at upright (u, v), with dots of s pixels. */
static void hex(const View *v, u32 rgb, int u, int top, int s, u32 ink)
{
    u32 i, row, column;
    for (i = 0; i < 7; ++i, u += 6 * s) {
        const u8 *g = font[i ? (rgb >> (4 * (6 - i))) & 15 : 16];
        for (row = 0; row < 7; ++row)
            for (column = 0; column < 5; ++column)
                if (g[row] & (0x10U >> column))
                    box(v, u + (int)column * s, top + (int)row * s, u + (int)column * s + s, top + (int)row * s + s, ink, 128, 128);
    }
}

/* Ink that reads on a swatch of luma y. */
#define LIGHT_INK 240U
#define DARK_INK 24U
static u32 ink_on(u32 y) { return y >= 140 ? DARK_INK : LIGHT_INK; }

/* The hex code centred in the upright box, at most `height` of it high. */
static void label(const View *v, const Color *c, int u0, int v0, int u1, int v1, u32 percent)
{
    int s = (u1 - u0) * 80 / 100 / (int)TEXT_DOTS, high = (v1 - v0) * (int)percent / 100 / 7;
    if (high < s) s = high;
    if (s < 1) return;
    hex(v, palette_rgb(c->y, c->cb, c->cr), (u0 + u1 - (int)TEXT_DOTS * s) / 2, (v0 + v1 - 7 * s) / 2, s, ink_on(c->y));
}

/* A disc of radius r centred at upright (cu, cv). */
static void disc(const View *v, int cu, int cv, int r, u32 y, u32 cb, u32 cr)
{
    int dv, half = 0;
    for (dv = -r; dv <= r; ++dv) {
        /* The half width grows to the middle row, then shrinks. */
        if (dv <= 0) while ((half + 1) * (half + 1) + dv * dv <= r * r) ++half;
        else while (half > 0 && half * half + dv * dv > r * r) --half;
        box(v, cu - half, cv + dv, cu + half + 1, cv + dv + 1, y, cb, cr);
    }
}

/* a / b for a >= 0 and b > 0: a module has no division by a variable. */
static int share(int a, int b) { return (int)divide((u32)a, (u32)b); }

static void strip(const View *v, const Color *c, u32 n, u32 side, u32 with_hex)
{
    int shorter = (v->r - v->l) < (v->b - v->t) ? v->r - v->l : v->b - v->t;
    int band = shorter * 12 / 100, i;
    for (i = 0; i < (int)n; ++i) {
        int u0, v0, u1, v1;
        if (side) {
            u0 = v->r - band; u1 = v->r;
            v0 = v->t + share((v->b - v->t) * i, (int)n); v1 = v->t + share((v->b - v->t) * (i + 1), (int)n);
        } else {
            u0 = v->l + share((v->r - v->l) * i, (int)n); u1 = v->l + share((v->r - v->l) * (i + 1), (int)n);
            v0 = v->b - band; v1 = v->b;
        }
        box(v, u0, v0, u1, v1, c[i].y, c[i].cb, c[i].cr);
        if (with_hex) label(v, &c[i], u0, v0, u1, v1, side ? 22U : 24U);
    }
}

#define RING_Y 240U
#define SHADOW_Y 16U
static void dots(const View *v, const Color *c, u32 n, u32 with_hex)
{
    int shorter = (v->r - v->l) < (v->b - v->t) ? v->r - v->l : v->b - v->t;
    int d = shorter * 9 / 100, gap, margin, ring, s, text, cv, i;
    /* Keep the row within 60% of the width. */
    if ((int)n * d * 4 / 3 > (v->r - v->l) * 60 / 100) d = share((v->r - v->l) * 60 / 100 * 3 / 4, (int)n);
    gap = d / 3;
    margin = d / 2;
    ring = d / 18 > 1 ? d / 18 : 1;
    s = with_hex ? (d + gap) * 85 / 100 / (int)TEXT_DOTS : 0;
    text = s ? 7 * s + gap / 2 : 0;
    cv = v->b - margin - text - d / 2;
    if (d < 6) return;
    for (i = 0; i < (int)n; ++i) {
        int cu = v->l + margin + d / 2 + i * (d + gap);
        disc(v, cu, cv, d / 2 + ring, RING_Y, 128, 128);
        disc(v, cu, cv, d / 2, c[i].y, c[i].cb, c[i].cr);
        if (s) {
            u32 rgb = palette_rgb(c[i].y, c[i].cb, c[i].cr);
            int u = cu - (int)TEXT_DOTS * s / 2, top = cv + d / 2 + ring + gap / 2;
            /* A shadow keeps the light text legible on a light photo. */
            hex(v, rgb, u + s / 2 + 1, top + s / 2 + 1, s, SHADOW_Y);
            hex(v, rgb, u, top, s, LIGHT_INK);
        }
    }
}

/* Draw the palette on one picture. */
void palette_draw(const Plane *p, u32 turns, u32 mono, u32 layout, const Color *c, u32 n, u32 with_hex)
{
    View v;
    u32 w = p->right - p->left, h = p->bottom - p->top, edge = film_edge(h);
    v.p = p;
    v.turns = turns & 3;
    v.mono = mono;
    v.l = 0;
    v.t = 0;
    v.r = (int)(v.turns & 1 ? h : w);
    v.b = (int)(v.turns & 1 ? w : h);
    /* The film edge is along the bottom of the sensor frame: upright, it is
       at the bottom, the left, the top, or the right. */
    switch (v.turns) {
    case 0: v.b -= (int)edge; break;
    case 1: v.l += (int)edge; break;
    case 2: v.t += (int)edge; break;
    default: v.r -= (int)edge; break;
    }
    if (v.r - v.l < 16 || v.b - v.t < 16 || !n) return;
    if (layout == LAYOUT_DOTS) dots(&v, c, n, with_hex);
    else strip(&v, c, n, layout == LAYOUT_SIDE, with_hex);
}

/* Each recorded picture: find the colours on the largest picture, then
   draw the same swatches on each one, so the screennail and the thumbnail
   match the main picture. */
void palette_apply(PictureSet *set, u32 layout)
{
    Color c[MAX_COLORS];
    u32 i, main = 0, n;
    if (layout < LAYOUT_BOTTOM || layout > LAYOUT_DOTS || !set->count) return;
    for (i = 1; i < set->count; ++i)
        if (set->planes[i].width * set->planes[i].height > set->planes[main].width * set->planes[main].height) main = i;
    n = palette_find(set->scratch, &set->planes[main], set->mono, palette_count(setting_get(SETTING_COLORS)), c);
    for (i = 0; i < set->count; ++i)
        palette_draw(&set->planes[i], set->turns, set->mono, layout, c, n, setting_get(SETTING_HEX));
}
