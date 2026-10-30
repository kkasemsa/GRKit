/* The host test of Load a Roll (grkit module test): the frame counter, a new
   roll, the film edge, and the viewfinder counter. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

void roll_event(u32 event, u32 handle, u32 value);
void roll_apply(PictureSet *set, u32 length);
void roll_counter(const Surface *s, u32 length);
u32 roll_next_frame(u32 count, u32 total);

/* LENGTH values. */
#define ROLL_12 1U
#define ROLL_36 3U

/* A main picture of 1200 x 800 and a thumbnail of 160 x 120, 4:4:4 chroma,
   filled with mid grey. */
#define MAIN_W 1200U
#define MAIN_H 800U
#define THUMB_W 160U
#define THUMB_H 120U
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
    for (u32 i = 0; i < 2 * (MAIN_W * MAIN_H + THUMB_W * THUMB_H); ++i) GRKIT_BYTES(GRKIT_TEST_PLANES + i)[0] = 128;
    return set;
}
static u32 luma(const Plane *p, u32 x, u32 y) { return GRKIT_BYTES(p->y + y * p->pitch + x)[0]; }
static u32 cr(const Plane *p, u32 x, u32 y) { return GRKIT_BYTES(chroma_row(p, y) + (x & ~1U) + 1)[0]; }

/* Count the pixels of a row range with luma v. */
static u32 count_luma(const Plane *p, u32 y0, u32 y1, u32 v)
{
    u32 n = 0;
    for (u32 y = y0; y < y1; ++y)
        for (u32 x = p->left; x < p->right; ++x) n += luma(p, x, y) == v;
    return n;
}

static void shoot(PictureSet *set, u32 length) { roll_apply(set, length); }

/* Whether the counter row has a lit pixel at or right of column x. */
static int drawn_from(u32 x)
{
    for (u32 y = LABEL_TOP + 40; y < LABEL_TOP + 80; ++y)
        for (u32 i = x; i < 720; ++i)
            if (grkit_test_pixel(i, y) == 0xffffffffU) return 1;
    return 0;
}

int main(void)
{
    static const u16 full[4] = {0, 0, 720, 480};
    Surface s = grkit_test_surface(full);
    PictureSet set;

    /* The descriptor: LENGTH, COUNT, EDGE, NEW; the optional recipe; a
       page, an event subscriber, a plane filter, and a layer. */
    assert(grkit_module.setting_count == 4 && grkit_module.use_count == 1);
    assert(grkit_module.registration_count == 4 && grkit_module.tile_count == 4);
    assert(grkit_module.settings[1].values == 37);

    /* The next frame: one more, or 1 after a full roll. */
    assert(roll_next_frame(0, 12) == 1 && roll_next_frame(11, 12) == 12);
    assert(roll_next_frame(12, 12) == 1 && roll_next_frame(36, 24) == 1);

    /* A roll of 12 counts each shot, then the 13th shot loads a new roll. */
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));
    grkit_test_settings[SETTING_LENGTH] = ROLL_12;
    set = pictures();
    for (u32 i = 1; i <= 12; ++i) {
        shoot(&set, ROLL_12);
        assert(grkit_test_settings[SETTING_COUNT] == i);
    }
    shoot(&set, ROLL_12);
    assert(grkit_test_settings[SETTING_COUNT] == 1);

    /* Edge print is Off by default: the pictures stay as they are. */
    assert(count_luma(&set.planes[0], 0, MAIN_H, 128) == MAIN_W * MAIN_H);

    /* A new length, or New Roll, loads an empty roll; New Roll returns to
       Keep. Other events do nothing. */
    grkit_test_settings[SETTING_COUNT] = 7;
    roll_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_LENGTH, ROLL_36);
    assert(grkit_test_settings[SETTING_COUNT] == 0);
    grkit_test_settings[SETTING_COUNT] = 9;
    grkit_test_settings[SETTING_NEW] = 1;
    roll_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_NEW, 1);
    assert(grkit_test_settings[SETTING_COUNT] == 0 && grkit_test_settings[SETTING_NEW] == 0);
    grkit_test_settings[SETTING_COUNT] = 5;
    roll_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_EDGE, 1);
    roll_event(GRKIT_EVENT_SYSTEM_READY, 0, 0);
    assert(grkit_test_settings[SETTING_COUNT] == 5);

    /* Off counts nothing. */
    shoot(&set, 0);
    assert(grkit_test_settings[SETTING_COUNT] == 5);

    /* With Film Edge Print On and Portra 400 chosen: a dark band of 8.2% of
       the height along the bottom, bright sprocket holes in it, orange
       print, and the picture above it unchanged. */
    grkit_test_settings[SETTING_EDGE] = 1;
    grkit_test_settings[USED_RECIPE_PICK] = 1;
    grkit_test_settings[SETTING_COUNT] = 13;
    set = pictures();
    shoot(&set, ROLL_36);
    assert(grkit_test_settings[SETTING_COUNT] == 14);
    {
        const Plane *p = &set.planes[0];
        u32 band = MAIN_H * 82 / 1000, ink = 0;
        assert(count_luma(p, 0, MAIN_H - band, 128) == MAIN_W * (MAIN_H - band));
        assert(luma(p, 0, MAIN_H - 1) == 20 && luma(p, MAIN_W - 1, MAIN_H - band) == 20);
        assert(count_luma(p, MAIN_H - band, MAIN_H, 225) > 0);
        for (u32 y = MAIN_H - band; y < MAIN_H; ++y)
            for (u32 x = 0; x < MAIN_W; ++x)
                if (luma(p, x, y) == 150) { ++ink; assert(cr(p, x, y) == 190); }
        /* The name, "14", and "14A" all print. */
        assert(ink > 0);
        assert(count_luma(p, MAIN_H - band, MAIN_H, 150) > 0);
        /* The thumbnail gets its own, smaller edge. */
        assert(luma(&set.planes[1], 0, THUMB_H - 1) == 20);
        assert(luma(&set.planes[1], 0, 0) == 128);
    }
    /* Without a recipe, the edge prints the frame numbers only: less ink. */
    {
        u32 with_name, without;
        set = pictures();
        grkit_test_settings[SETTING_COUNT] = 13;
        shoot(&set, ROLL_36);
        with_name = count_luma(&set.planes[0], MAIN_H - MAIN_H * 82 / 1000, MAIN_H, 150);
        grkit_test_settings[USED_RECIPE_PICK] = 0;
        set = pictures();
        grkit_test_settings[SETTING_COUNT] = 13;
        shoot(&set, ROLL_36);
        without = count_luma(&set.planes[0], MAIN_H - MAIN_H * 82 / 1000, MAIN_H, 150);
        assert(without > 0 && without < with_name);
    }
    /* A monochrome picture gets light print, without colour. */
    set = pictures();
    set.mono = 1;
    shoot(&set, ROLL_36);
    assert(count_luma(&set.planes[0], MAIN_H - MAIN_H * 82 / 1000, MAIN_H, 200) > 0);
    assert(count_luma(&set.planes[0], 0, MAIN_H, 150) == 0);

    /* The viewfinder counter: "14/36" is 5 characters of 12 pixels. */
    grkit_test_settings[SETTING_COUNT] = 14;
    grkit_test_clear();
    roll_counter(&s, ROLL_36);
    assert(drawn_from(LABEL_LEFT) && !drawn_from(LABEL_LEFT + 5 * 12));
    /* A full roll shows REWIND, 6 characters. */
    grkit_test_settings[SETTING_COUNT] = 36;
    grkit_test_clear();
    roll_counter(&s, ROLL_36);
    assert(drawn_from(LABEL_LEFT + 5 * 12) && !drawn_from(LABEL_LEFT + 6 * 12));
    /* Off draws nothing. */
    grkit_test_clear();
    roll_counter(&s, 0);
    assert(!drawn_from(0));
    return 0;
}
