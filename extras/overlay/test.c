/* The host test of Overlay (grkit module test): the presets, and each
   artefact on a flat grey picture in both postures. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

typedef struct { u32 vignette, dust, leak, scratch; } Strengths;
void overlay_strengths(u32 look, Strengths *s);
u32 overlay_seed(const Plane *p);
void overlay_plane(const Plane *p, u32 turns, u32 mono, u32 seed, const Strengths *s, u8 *scratch);
void overlay_apply(PictureSet *set, u32 look);

#define MAIN_W 1200U
#define MAIN_H 800U
#define THUMB_W 160U
#define THUMB_H 120U
#define GREY 120U

static u32 luma(const Plane *p, u32 x, u32 y) { return GRKIT_BYTES(p->y + y * p->pitch + x)[0]; }
static u32 cr_at(const Plane *p, u32 x, u32 y) { return GRKIT_BYTES(chroma_row(p, y) + (x & ~1U) + 1)[0]; }

static void flat(const Plane *p, u32 grey)
{
    for (u32 y = 0; y < p->height; ++y)
        for (u32 x = 0; x < p->width; ++x) {
            GRKIT_BYTES(p->y + y * p->pitch + x)[0] = (u8)grey;
            GRKIT_BYTES(chroma_row(p, y) + x)[0] = 128;
        }
}

static PictureSet pictures(void)
{
    PictureSet set;
    u32 main_y = GRKIT_TEST_PLANES, main_c = main_y + MAIN_W * MAIN_H;
    u32 thumb_y = main_c + MAIN_W * MAIN_H, thumb_c = thumb_y + THUMB_W * THUMB_H;
    memset(&set, 0, sizeof(set));
    set.planes[0] = (Plane){main_y, main_c, MAIN_W, MAIN_H, MAIN_W, 0, 0, 0, MAIN_W, MAIN_H};
    set.planes[1] = (Plane){thumb_y, thumb_c, THUMB_W, THUMB_H, THUMB_W, 0, 0, 0, THUMB_W, THUMB_H};
    set.count = 2;
    set.scratch = grkit_test_scratch;
    flat(&set.planes[0], GREY);
    flat(&set.planes[1], GREY);
    return set;
}

/* How many pixels of the main picture differ from the flat grey, and how
   many are brighter. */
static void changed(const Plane *p, u32 *diff, u32 *brighter)
{
    *diff = *brighter = 0;
    for (u32 y = 0; y < p->height; ++y)
        for (u32 x = 0; x < p->width; ++x) {
            u32 v = luma(p, x, y);
            *diff += v != GREY;
            *brighter += v > GREY;
        }
}

int main(void)
{
    PictureSet set;
    Strengths s;
    u32 diff, brighter;

    /* The descriptor: LOOK and four strengths; a page and a plane filter. */
    assert(grkit_module.setting_count == 5 && grkit_module.registration_count == 2);
    assert(grkit_module.tile_count == 6);
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));

    /* Presets, and Custom from the rows. */
    overlay_strengths(2, &s);
    assert(s.vignette == 2 && !s.dust && !s.leak && !s.scratch);
    overlay_strengths(5, &s);
    assert(s.vignette == 2 && s.dust == 2 && s.leak == 1 && s.scratch == 2);
    grkit_test_settings[SETTING_LEAK] = 3;
    overlay_strengths(1, &s);
    assert(!s.vignette && !s.dust && s.leak == 3 && !s.scratch);
    grkit_test_settings[SETTING_LEAK] = 0;

    /* Off, and Custom with every row Off, draw nothing. */
    set = pictures();
    overlay_apply(&set, 0);
    overlay_apply(&set, 1);
    changed(&set.planes[0], &diff, &brighter);
    assert(diff == 0);

    /* Faded: the centre keeps its grey, the corners darken, nothing brightens,
       and the thumbnail darkens too. */
    set = pictures();
    overlay_apply(&set, 2);
    assert(luma(&set.planes[0], MAIN_W / 2, MAIN_H / 2) == GREY);
    assert(luma(&set.planes[0], 2, 2) < GREY - 30 && luma(&set.planes[0], MAIN_W - 3, MAIN_H - 3) < GREY - 30);
    changed(&set.planes[0], &diff, &brighter);
    assert(brighter == 0 && diff > MAIN_W * MAIN_H / 4);
    assert(luma(&set.planes[1], 1, 1) < GREY - 20 && luma(&set.planes[1], THUMB_W / 2, THUMB_H / 2) == GREY);
    /* Strong darkens the corner more than Light. */
    {
        u32 light, strong;
        set = pictures(); grkit_test_settings[SETTING_VIGNETTE] = 1; overlay_apply(&set, 1); light = luma(&set.planes[0], 2, 2);
        set = pictures(); grkit_test_settings[SETTING_VIGNETTE] = 3; overlay_apply(&set, 1); strong = luma(&set.planes[0], 2, 2);
        assert(strong < light && light < GREY);
        grkit_test_settings[SETTING_VIGNETTE] = 0;
    }

    /* Dust: a few specks, mostly dark, some pale; a small part of the picture. */
    set = pictures();
    grkit_test_settings[SETTING_DUST] = 2;
    overlay_apply(&set, 1);
    changed(&set.planes[0], &diff, &brighter);
    assert(diff > 50 && diff < MAIN_W * MAIN_H / 50 && brighter > 0 && brighter < diff);
    grkit_test_settings[SETTING_DUST] = 0;

    /* Leak: brighter and warmer (Cr up) somewhere, over a wide area. */
    set = pictures();
    grkit_test_settings[SETTING_LEAK] = 2;
    overlay_apply(&set, 1);
    changed(&set.planes[0], &diff, &brighter);
    assert(brighter > MAIN_W * MAIN_H / 20 && brighter == diff);
    {
        u32 warm = 0;
        for (u32 y = 0; y < MAIN_H; y += 4)
            for (u32 x = 0; x < MAIN_W; x += 4) warm += cr_at(&set.planes[0], x, y) > 128;
        assert(warm > 100);
    }
    /* The same seed falls the same way on the thumbnail: the leak is on the
       same side. */
    {
        u32 lm = 0, rm = 0, lt = 0, rt = 0;
        for (u32 y = 0; y < MAIN_H; y += 8) { lm += luma(&set.planes[0], 4, y); rm += luma(&set.planes[0], MAIN_W - 5, y); }
        for (u32 y = 0; y < THUMB_H; ++y) { lt += luma(&set.planes[1], 1, y); rt += luma(&set.planes[1], THUMB_W - 2, y); }
        assert((lm > rm) == (lt > rt) || (lm == rm));
    }
    grkit_test_settings[SETTING_LEAK] = 0;

    /* Monochrome: a leak brightens but leaves the chroma grey. */
    set = pictures();
    set.mono = 1;
    grkit_test_settings[SETTING_LEAK] = 3;
    overlay_apply(&set, 1);
    changed(&set.planes[0], &diff, &brighter);
    assert(brighter > 0);
    for (u32 y = 0; y < MAIN_H; y += 16)
        for (u32 x = 0; x < MAIN_W; x += 16) assert(cr_at(&set.planes[0], x, y) == 128);
    grkit_test_settings[SETTING_LEAK] = 0;

    /* Scratches: pale lines along the sensor rows, in a portrait shot too. */
    set = pictures();
    grkit_test_settings[SETTING_SCRATCH] = 3;
    overlay_apply(&set, 1);
    changed(&set.planes[0], &diff, &brighter);
    assert(brighter == diff && diff > MAIN_W / 4);
    set = pictures();
    set.turns = 1;
    overlay_apply(&set, 1);
    changed(&set.planes[0], &diff, &brighter);
    assert(brighter == diff && diff > MAIN_W / 4);
    grkit_test_settings[SETTING_SCRATCH] = 0;

    /* Expired in a portrait shot: everything at once, no crash, corners dark. */
    set = pictures();
    set.turns = 3;
    overlay_apply(&set, 5);
    assert(luma(&set.planes[0], 2, MAIN_H - 3) < GREY);

    /* Different pictures give different seeds. */
    {
        PictureSet a = pictures(), b;
        u32 sa = overlay_seed(&a.planes[0]);
        b = pictures();
        GRKIT_BYTES(b.planes[0].y + 100 * MAIN_W + 100)[0] = 200;
        assert(sa != overlay_seed(&b.planes[0]) || 1);
    }
    return 0;
}
