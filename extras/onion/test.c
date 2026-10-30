/* The host test of Onion Skin (grkit module test): taking and holding the
   picture, and drawing it into the photo area. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

void onion_apply(PictureSet *set, u32 mode);
void onion_event(u32 event, u32 handle, u32 value);
void onion_paint(const Surface *s, u32 mode);

#define MODE_LAST 1U
#define MODE_HOLD 2U

/* A main picture of 600 x 400 and a thumbnail of 180 x 120 whose left half
   has luma `left` and right half luma `right`. */
static PictureSet shot(u32 left, u32 right)
{
    PictureSet set;
    u32 main_y = GRKIT_TEST_PLANES, main_c = main_y + 600 * 400;
    u32 thumb_y = main_c + 600 * 400, thumb_c = thumb_y + 180 * 120;
    memset(&set, 0, sizeof(set));
    set.planes[0] = (Plane){main_y, main_c, 600, 400, 600, 0, 0, 0, 600, 400};
    set.planes[1] = (Plane){thumb_y, thumb_c, 180, 120, 180, 0, 0, 0, 180, 120};
    set.count = 2;
    for (u32 y = 0; y < 120; ++y)
        for (u32 x = 0; x < 180; ++x) GRKIT_BYTES(thumb_y + y * 180 + x)[0] = (u8)(x < 90 ? left : right);
    /* The main picture differs, so a test sees which picture was taken. */
    for (u32 i = 0; i < 600 * 400; ++i) GRKIT_BYTES(main_y + i)[0] = 7;
    return set;
}
static const u16 full[4] = {0, 0, 720, 480};
static const u16 xpan[4] = {0, 107, 720, 373};
static u32 lit(void)
{
    u32 n = 0;
    for (u32 y = 0; y < 480; ++y)
        for (u32 x = 0; x < 720; ++x) n += grkit_test_pixel(x, y) != GRKIT_TEST_CLEAR;
    return n;
}

int main(void)
{
    Surface s = grkit_test_surface(full);
    PictureSet set;

    /* The descriptor: MODE and STYLE; a page, an event subscriber, a plane
       filter, and a layer. */
    assert(grkit_module.setting_count == 2 && grkit_module.registration_count == 4);
    assert(grkit_module.tile_count == 3);

    /* Before any shot, Last Shot draws nothing. */
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));
    grkit_test_clear();
    onion_paint(&s, MODE_LAST);
    assert(lit() == 0);

    /* Last Shot takes the thumbnail and draws it faint over the photo area,
       without changing the picture. */
    set = shot(40, 200);
    onion_apply(&set, MODE_LAST);
    assert(GRKIT_BYTES(set.planes[1].y)[0] == 40 && GRKIT_BYTES(set.planes[0].y)[0] == 7);
    grkit_test_clear();
    onion_paint(&s, MODE_LAST);
    assert(grkit_test_pixel(10, 240) == (0x50U << 24 | 40U << 16 | 40U << 8 | 40U));
    assert(grkit_test_pixel(710, 240) == (0x50U << 24 | 200U << 16 | 200U << 8 | 200U));
    /* Strong is more opaque. */
    grkit_test_settings[SETTING_STYLE] = 1;
    grkit_test_clear();
    onion_paint(&s, MODE_LAST);
    assert(grkit_test_pixel(10, 240) >> 24 == 0x90);
    /* Edges draws only the edge between the halves. */
    grkit_test_settings[SETTING_STYLE] = 2;
    grkit_test_clear();
    onion_paint(&s, MODE_LAST);
    assert(grkit_test_pixel(360, 240) == 0xe0ffffffU || grkit_test_pixel(358, 240) == 0xe0ffffffU);
    assert(grkit_test_pixel(10, 240) == GRKIT_TEST_CLEAR && grkit_test_pixel(710, 240) == GRKIT_TEST_CLEAR);
    grkit_test_settings[SETTING_STYLE] = 0;
    /* The next shot replaces it. */
    set = shot(90, 90);
    onion_apply(&set, MODE_LAST);
    grkit_test_clear();
    onion_paint(&s, MODE_LAST);
    assert((grkit_test_pixel(10, 240) & 0xff) == 90);
    /* A crop draws only in its photo area. */
    {
        Surface c = grkit_test_surface(xpan);
        grkit_test_clear();
        onion_paint(&c, MODE_LAST);
        assert(grkit_test_pixel(10, 50) == GRKIT_TEST_CLEAR && grkit_test_pixel(10, 240) != GRKIT_TEST_CLEAR);
    }

    /* Hold Reference keeps the first shot after it is chosen. */
    onion_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_MODE, MODE_HOLD);
    set = shot(30, 30);
    onion_apply(&set, MODE_HOLD);
    set = shot(220, 220);
    onion_apply(&set, MODE_HOLD);
    grkit_test_clear();
    onion_paint(&s, MODE_HOLD);
    assert((grkit_test_pixel(10, 240) & 0xff) == 30);
    /* Choosing it again takes a new reference. */
    onion_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_MODE, MODE_HOLD);
    onion_apply(&set, MODE_HOLD);
    grkit_test_clear();
    onion_paint(&s, MODE_HOLD);
    assert((grkit_test_pixel(10, 240) & 0xff) == 220);
    /* Off draws nothing. */
    grkit_test_clear();
    onion_paint(&s, 0);
    assert(lit() == 0);
    return 0;
}
