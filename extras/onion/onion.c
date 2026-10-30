/* Onion Skin: a faint picture over the live view to line up a shot.

   Last Shot     the last picture, for stop-motion, panoramas, and
                 then-and-now photos.
   Hold Reference the first picture after choosing it, held until the mode
                 changes: photograph a reference pose, such as a picture on a
                 phone, and pose your subject against it.

   A module cannot write files, so a held picture lasts until the camera
   turns off. */
#include "grkit.h"
#include "grkit_module.h"

#define MODE_LAST 1U
#define MODE_HOLD 2U

/* The held picture: the luma of the smallest picture of a shot (the
   thumbnail), reduced to GHOST_W x GHOST_H, and its edges. */
#define GHOST_W 180U
#define GHOST_H 120U
static u8 ghost[GHOST_W * GHOST_H];
static u8 edges[GHOST_W * GHOST_H];
static u32 have_ghost;
/* Hold Reference keeps the first picture after it is chosen. */
static u32 held;

/* ---- taking the picture ---------------------------------------------------- */

#define BYTE(p,o) (GRKIT_BYTES((p)+(o))[0])
/* An edge: the summed horizontal and vertical Sobel gradients over this. */
#define EDGE_LEVEL 90

static void take(const Plane *p)
{
    u32 w = p->right - p->left, h = p->bottom - p->top, x, y;
    for (y = 0; y < GHOST_H; ++y) {
        u32 row = p->y + (p->top + y * h / GHOST_H) * p->pitch;
        for (x = 0; x < GHOST_W; ++x) ghost[y * GHOST_W + x] = BYTE(row, p->left + x * w / GHOST_W);
    }
    for (y = 0; y < GHOST_H; ++y)
        for (x = 0; x < GHOST_W; ++x) {
            int gx, gy;
            const u8 *g = ghost + y * GHOST_W + x;
            if (!x || !y || x + 1 == GHOST_W || y + 1 == GHOST_H) { edges[y * GHOST_W + x] = 0; continue; }
            /* A signed row offset: -GHOST_W would wrap as an unsigned value. */
            const int row = (int)GHOST_W;
            gx = (g[-row + 1] + 2 * g[1] + g[row + 1]) - (g[-row - 1] + 2 * g[-1] + g[row - 1]);
            gy = (g[row - 1] + 2 * g[row] + g[row + 1]) - (g[-row - 1] + 2 * g[-row] + g[-row + 1]);
            edges[y * GHOST_W + x] = (gx < 0 ? -gx : gx) + (gy < 0 ? -gy : gy) > EDGE_LEVEL;
        }
    have_ghost = 1;
}

/* Each recorded picture: Last Shot takes every one, Hold Reference the first
   after it is chosen. */
void onion_apply(PictureSet *set, u32 mode)
{
    u32 i, small = 0;
    if (mode != MODE_LAST && !(mode == MODE_HOLD && !held)) return;
    for (i = 1; i < set->count; ++i)
        if (set->planes[i].width * set->planes[i].height < set->planes[small].width * set->planes[small].height) small = i;
    take(&set->planes[small]);
    if (mode == MODE_HOLD) held = 1;
}

/* Choosing Hold Reference again takes a new reference. */
void onion_event(u32 event, u32 handle, u32 value)
{
    (void)value;
    if (event == GRKIT_EVENT_SETTING_CHANGED && handle == SETTING_MODE) held = 0;
}

/* ---- drawing --------------------------------------------------------------- */

#define STYLE_FAINT 0U
#define STYLE_STRONG 1U
#define STYLE_EDGES 2U
#define WHITE 0x00ffffffU
static u32 alpha(u32 style) { return style == STYLE_STRONG ? 0x90U : 0x50U; }
#define EDGE_ALPHA 0xe0U

/* The held picture, scaled to the photo area. */
static void paint_ghost(const Surface *s, u32 style)
{
    u32 l = s->box[0], t = s->box[1], r = s->box[2], b = s->box[3], x, y, sy, sx, step_x, step_y;
    if (r <= l || b <= t) return;
    step_x = divide(GHOST_W << 16, r - l);
    step_y = divide(GHOST_H << 16, b - t);
    for (y = t, sy = 0; y < b && y < 480; ++y, sy += step_y) {
        const u8 *g = ghost + (sy >> 16) * GHOST_W, *e = edges + (sy >> 16) * GHOST_W;
        for (x = l, sx = 0; x < r && x < 720; ++x, sx += step_x) {
            u32 i = sx >> 16;
            if (style == STYLE_EDGES) {
                if (e[i]) SURFACE_PIXEL(s, x, y) = EDGE_ALPHA << 24 | WHITE;
            } else {
                u32 v = g[i];
                SURFACE_PIXEL(s, x, y) = alpha(style) << 24 | v << 16 | v << 8 | v;
            }
        }
    }
}

/* The layer: the held picture for Last Shot and Hold Reference, once there is
   one. */
void onion_paint(const Surface *s, u32 mode)
{
    if ((mode == MODE_LAST || mode == MODE_HOLD) && have_ghost) paint_ghost(s, setting_get(SETTING_STYLE));
}
