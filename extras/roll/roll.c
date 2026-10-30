/* Load a Roll: shoot a roll of 12, 24, or 36 frames of the chosen recipe.
   The viewfinder counts the frames, as a film camera does, and shows REWIND
   when the roll is full; the next shot loads a new roll. An optional film
   edge prints the film name and the frame number along the bottom of each
   picture, over the photo, as on a scanned negative.

   The film is the chosen recipe of the Recipe module (RECIPE.PICK, optional):
   films.h gives the name of each recipe, in the order of PICK. */
#include "grkit.h"
#include "grkit_module.h"
#include "films.h"

/* LENGTH: 0 Off, then 12, 24, or 36 frames. */
static u32 frames(u32 length) { return length == 1 ? 12U : length == 2 ? 24U : length == 3 ? 36U : 0U; }

/* The name of the film of the roll, or 0 without a recipe. */
static const char *film_name(void)
{
    u32 pick = setting_get(USED_RECIPE_PICK);
    return pick && pick <= FILM_COUNT ? films[pick - 1] : 0;
}

/* ---- the counter ----------------------------------------------------------- */

/* A new length or New Roll loads an empty roll. A setting_set here sends no
   second event. */
void roll_event(u32 event, u32 handle, u32 value)
{
    if (event != GRKIT_EVENT_SETTING_CHANGED) return;
    if (handle == SETTING_LENGTH) {
        setting_set(SETTING_COUNT, 0);
    } else if (handle == SETTING_NEW && value) {
        setting_set(SETTING_COUNT, 0);
        setting_set(SETTING_NEW, 0);
    }
}

/* The frame of the next picture: 1 after a full roll, which the shot loads. */
u32 roll_next_frame(u32 count, u32 total) { return count >= total ? 1U : count + 1U; }

/* ---- the film edge --------------------------------------------------------- */

#define BYTE(p,o) (GRKIT_BYTES((p)+(o))[0])

/* 5 x 7 characters, one byte for each row, bit 4 the left column: A to Z,
   0 to 9, then '-', '+', '.', '/'. Other characters are blank. */
static const u8 font[40][7] = {
    {0x0e,0x11,0x11,0x1f,0x11,0x11,0x11},{0x1e,0x11,0x11,0x1e,0x11,0x11,0x1e},
    {0x0e,0x11,0x10,0x10,0x10,0x11,0x0e},{0x1e,0x11,0x11,0x11,0x11,0x11,0x1e},
    {0x1f,0x10,0x10,0x1e,0x10,0x10,0x1f},{0x1f,0x10,0x10,0x1e,0x10,0x10,0x10},
    {0x0e,0x11,0x10,0x17,0x11,0x11,0x0f},{0x11,0x11,0x11,0x1f,0x11,0x11,0x11},
    {0x0e,0x04,0x04,0x04,0x04,0x04,0x0e},{0x07,0x02,0x02,0x02,0x02,0x12,0x0c},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1f},
    {0x11,0x1b,0x15,0x15,0x11,0x11,0x11},{0x11,0x11,0x19,0x15,0x13,0x11,0x11},
    {0x0e,0x11,0x11,0x11,0x11,0x11,0x0e},{0x1e,0x11,0x11,0x1e,0x10,0x10,0x10},
    {0x0e,0x11,0x11,0x11,0x15,0x12,0x0d},{0x1e,0x11,0x11,0x1e,0x14,0x12,0x11},
    {0x0f,0x10,0x10,0x0e,0x01,0x01,0x1e},{0x1f,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0e},{0x11,0x11,0x11,0x11,0x11,0x0a,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x15,0x0a},{0x11,0x11,0x0a,0x04,0x0a,0x11,0x11},
    {0x11,0x11,0x11,0x0a,0x04,0x04,0x04},{0x1f,0x01,0x02,0x04,0x08,0x10,0x1f},
    {0x0e,0x11,0x13,0x15,0x19,0x11,0x0e},{0x04,0x0c,0x04,0x04,0x04,0x04,0x0e},
    {0x0e,0x11,0x01,0x02,0x04,0x08,0x1f},{0x1f,0x02,0x04,0x02,0x01,0x11,0x0e},
    {0x02,0x06,0x0a,0x12,0x1f,0x02,0x02},{0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e},
    {0x06,0x08,0x10,0x1e,0x11,0x11,0x0e},{0x1f,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e},{0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c},
    {0x00,0x00,0x00,0x1f,0x00,0x00,0x00},{0x00,0x04,0x04,0x1f,0x04,0x04,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0c,0x0c},{0x01,0x01,0x02,0x04,0x08,0x10,0x10},
};
static const u8 *glyph(char c)
{
    if (c >= 'A' && c <= 'Z') return font[c - 'A'];
    if (c >= '0' && c <= '9') return font[26 + (c - '0')];
    switch (c) {
    case '-': return font[36];
    case '+': return font[37];
    case '.': return font[38];
    case '/': return font[39];
    default: return 0;
    }
}

/* The decimal digits of n < 100, after `out`. Returns the end. */
static char *number(char *out, u32 n)
{
    if (n >= 10) *out++ = (char)('0' + n / 10);
    *out++ = (char)('0' + n % 10);
    return out;
}

/* The edge, in luma and chroma of a picture: the base of the film edge, the
   sprocket holes that light shows through, and the orange edge print. */
#define EDGE_Y 20U
#define HOLE_Y 225U
#define INK_Y 150U
#define INK_CB 64U
#define INK_CR 190U
#define MONO_INK_Y 200U

/* Set the pixels [x0, x1) x [y0, y1) of `p`, clipped to its visible area,
   to luma y and, when `chroma`, to Cb cb and Cr cr. CbCr are pairs, so the
   chroma of a pixel pair follows its even pixel. */
static void fill(const Plane *p, u32 x0, u32 y0, u32 x1, u32 y1, u32 y, u32 cb, u32 cr, u32 chroma)
{
    u32 row, x;
    if (x0 < p->left) x0 = p->left;
    if (y0 < p->top) y0 = p->top;
    if (x1 > p->right) x1 = p->right;
    if (y1 > p->bottom) y1 = p->bottom;
    for (row = y0; row < y1; ++row) {
        u32 luma = p->y + row * p->pitch;
        for (x = x0; x < x1; ++x) BYTE(luma, x) = (u8)y;
        if (!chroma || (p->half && (row & 1))) continue;
        for (x = x0 & ~1U; x < x1; x += 2) {
            u32 c = chroma_row(p, row);
            BYTE(c, x) = (u8)cb;
            BYTE(c, x + 1) = (u8)cr;
        }
    }
}

/* Draw `text` at (x, y) of `p` with dots of s x s pixels. Returns the x
   after the text. */
static u32 print(const Plane *p, const char *text, u32 x, u32 y, u32 s, u32 mono)
{
    for (; *text; ++text, x += 6 * s) {
        const u8 *g = glyph(*text);
        u32 row, column;
        if (!g) continue;
        for (row = 0; row < 7; ++row)
            for (column = 0; column < 5; ++column)
                if (g[row] & (0x10U >> column))
                    fill(p, x + column * s, y + row * s, x + column * s + s, y + row * s + s,
                         mono ? MONO_INK_Y : INK_Y, INK_CB, INK_CR, !mono);
    }
    return x;
}

/* The film edge of frame `frame` on one picture of the shot: a band of 8.2%
   of the picture height along the bottom of the sensor frame, as the film
   edge of a scan, whatever the posture of the camera. Every size follows the
   band, so the screennail and the thumbnail get the same edge. */
void roll_edge(const Plane *p, u32 frame, const char *name, u32 mono)
{
    u32 h = p->bottom - p->top, w = p->right - p->left;
    u32 band = h * 82U / 1000U, y0, hole_w, hole_h, pitch, margin, x, s;
    char text[8], *end;
    if (band < 4 || w < 8 * band) return;
    y0 = p->bottom - band;
    fill(p, p->left, y0, p->right, p->bottom, EDGE_Y, 128, 128, 1);
    /* Sprocket holes along the outer half of the band. */
    hole_h = band * 40U / 100U;
    hole_w = band * 58U / 100U;
    pitch = band * 105U / 100U;
    margin = band * 12U / 100U;
    for (x = p->left + pitch / 2; x + hole_w < p->right; x += pitch)
        fill(p, x, p->bottom - margin - hole_h, x + hole_w, p->bottom - margin, HOLE_Y, 128, 128, 1);
    /* The edge print along the inner half: the film name, the frame number,
       and the frame number with A, the half frame after it. */
    s = band * 30U / 100U / 7U;
    if (!s) s = 1;
    if (name) print(p, name, p->left + 2 * band, y0 + band * 8U / 100U, s, mono);
    end = number(text, frame);
    *end = 0;
    print(p, text, p->left + w * 45U / 100U, y0 + band * 8U / 100U, s, mono);
    *end++ = 'A';
    *end = 0;
    print(p, text, p->left + w * 70U / 100U, y0 + band * 8U / 100U, s, mono);
}

/* Each recorded picture while a roll is loaded: count the frame, then print
   the film edge when Film Edge Print is On. The core calls this once for
   each shot, with its main picture, screennail, and thumbnail. */
void roll_apply(PictureSet *set, u32 length)
{
    u32 total = frames(length), frame, i;
    if (!total) return;
    frame = roll_next_frame(setting_get(SETTING_COUNT), total);
    setting_set(SETTING_COUNT, frame);
    if (!setting_get(SETTING_EDGE)) return;
    for (i = 0; i < set->count; ++i) roll_edge(&set->planes[i], frame, film_name(), set->mono);
}

/* ---- the viewfinder -------------------------------------------------------- */

#define SCALE 2U
#define ADVANCE (6U * SCALE)
/* The fourth row of the native text, below the name of the recipe. */
#define COUNTER_ROW (LABEL_TOP + 48U)
#define WHITE 0xffffffffU
#define SHADE 0xc0000000U
static void put(const Surface *s, u32 x, u32 y, u32 pixel)
{
    if (x < 720 && y < 480) SURFACE_PIXEL(s, x, y) = pixel;
}
static void draw_char(const Surface *s, char c, u32 x, u32 y)
{
    const u8 *g = glyph(c);
    u32 row, column, dx, dy, pass;
    if (!g) return;
    for (pass = 0; pass < 2; ++pass)
        for (row = 0; row < 7; ++row)
            for (column = 0; column < 5; ++column) {
                if (!(g[row] & (0x10U >> column))) continue;
                for (dy = 0; dy < SCALE + 2 * (1 - pass); ++dy)
                    for (dx = 0; dx < SCALE + 2 * (1 - pass); ++dx)
                        put(s, x + column * SCALE + dx + pass, y + row * SCALE + dy + pass, pass ? WHITE : SHADE);
            }
}
/* The counter: frames shot of the roll, such as 14/36, or REWIND when the
   roll is full. */
void roll_counter(const Surface *s, u32 length)
{
    u32 total = frames(length), count, x = LABEL_LEFT;
    char text[8], *end = text;
    const char *p;
    if (!total) return;
    count = setting_get(SETTING_COUNT);
    if (count >= total) {
        for (p = "REWIND"; *p; ++p, x += ADVANCE) draw_char(s, *p, x, COUNTER_ROW);
        return;
    }
    end = number(end, count);
    *end++ = '/';
    end = number(end, total);
    *end = 0;
    for (p = text; *p; ++p, x += ADVANCE) draw_char(s, *p, x, COUNTER_ROW);
}
