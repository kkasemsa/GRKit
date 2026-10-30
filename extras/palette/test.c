/* The host test of Palette Card (grkit module test): finding the colours,
   the hex codes, the layouts in each posture, and Roll's film edge. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

typedef struct { u32 y, cb, cr, n; } Color;
u32 palette_count(u32 colors);
u32 palette_find(u8 *scratch, const Plane *p, u32 mono, u32 k, Color *out);
u32 palette_rgb(u32 y, u32 cb, u32 cr);
void palette_draw(const Plane *p, u32 turns, u32 mono, u32 layout, const Color *c, u32 n, u32 with_hex);
void palette_apply(PictureSet *set, u32 layout);

#define MAIN_W 1200U
#define MAIN_H 800U
#define THUMB_W 160U
#define THUMB_H 120U

static u32 luma(const Plane *p, u32 x, u32 y) { return GRKIT_BYTES(p->y + y * p->pitch + x)[0]; }
static u32 cb_at(const Plane *p, u32 x, u32 y) { return GRKIT_BYTES(chroma_row(p, y) + (x & ~1U))[0]; }

/* Paint the picture in vertical bands: 60% of (y0, cb0, cr0), 30% of the
   second, 10% of the third. */
static void bands(const Plane *p, const u32 c[3][3])
{
    for (u32 y = 0; y < p->height; ++y)
        for (u32 x = 0; x < p->width; ++x) {
            u32 k = x < p->width * 6 / 10 ? 0 : x < p->width * 9 / 10 ? 1 : 2;
            GRKIT_BYTES(p->y + y * p->pitch + x)[0] = (u8)c[k][0];
            if (!(x & 1)) {
                GRKIT_BYTES(chroma_row(p, y) + x)[0] = (u8)c[k][1];
                GRKIT_BYTES(chroma_row(p, y) + x + 1)[0] = (u8)c[k][2];
            }
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
    return set;
}

static const u32 scene[3][3] = {{60, 150, 100}, {200, 90, 160}, {120, 128, 128}};

int main(void)
{
    PictureSet set;
    Color c[7];
    u32 n;

    /* The descriptor: LAYOUT, COLORS, HEX, two used settings of Roll; a page
       and a plane filter. */
    assert(grkit_module.setting_count == 3 && grkit_module.registration_count == 2);
    assert(grkit_module.tile_count == 4);
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));
    assert(palette_count(0) == 5 && palette_count(1) == 3 && palette_count(2) == 7);

    /* Hex codes: JFIF YCbCr to sRGB. */
    assert(palette_rgb(0, 128, 128) == 0x000000U && palette_rgb(255, 128, 128) == 0xffffffU);
    assert(palette_rgb(128, 128, 128) == 0x808080U);
    assert(palette_rgb(76, 85, 255) == 0xfe0000U || palette_rgb(76, 85, 255) == 0xff0000U);

    /* Three colours of a picture of three colours, most common first. */
    set = pictures();
    bands(&set.planes[0], scene);
    n = palette_find(set.scratch, &set.planes[0], 0, 3, c);
    assert(n == 3);
    for (u32 i = 0; i < 3; ++i) assert(c[i].y == scene[i][0] && c[i].cb == scene[i][1] && c[i].cr == scene[i][2]);
    assert(c[0].n > c[1].n && c[1].n > c[2].n);
    /* Asking for more colours than the picture has gives what it has. */
    n = palette_find(set.scratch, &set.planes[0], 0, 7, c);
    assert(n == 3);
    /* A flat picture has one colour. */
    {
        static const u32 flat[3][3] = {{90, 128, 128}, {90, 128, 128}, {90, 128, 128}};
        bands(&set.planes[0], flat);
        assert(palette_find(set.scratch, &set.planes[0], 0, 5, c) == 1 && c[0].y == 90);
    }

    /* Bottom Strip: 12% of the height, the swatches across, on each
       picture of the shot. */
    set = pictures();
    bands(&set.planes[0], scene);
    bands(&set.planes[1], scene);
    grkit_test_settings[SETTING_COLORS] = 1;
    palette_apply(&set, 1);
    {
        const Plane *p = &set.planes[0];
        u32 band = MAIN_H * 12 / 100;
        assert(luma(p, 100, MAIN_H - 1) == 60 && luma(p, 600, MAIN_H - 1) == 200 && luma(p, 1100, MAIN_H - 1) == 120);
        assert(cb_at(p, 600, MAIN_H - 1) == 90);
        assert(luma(p, 1100, MAIN_H - band - 2) == 120 && luma(p, 600, MAIN_H - band - 2) == 60);
        assert(luma(&set.planes[1], 150, THUMB_H - 1) == 120 && luma(&set.planes[1], 10, THUMB_H - 1) == 60);
    }
    /* Hex On draws ink in the swatches: light on a dark one. */
    set = pictures();
    bands(&set.planes[0], scene);
    grkit_test_settings[SETTING_HEX] = 1;
    palette_apply(&set, 1);
    {
        const Plane *p = &set.planes[0];
        u32 light = 0, dark = 0;
        for (u32 y = MAIN_H - 96; y < MAIN_H; ++y)
            for (u32 x = 0; x < MAIN_W; ++x) {
                light += luma(p, x, y) == 240;
                dark += luma(p, x, y) == 24;
            }
        assert(light > 100 && dark > 100);
    }
    grkit_test_settings[SETTING_HEX] = 0;

    /* Side Strip on a picture turned once (portrait): the upright right
       side is the sensor bottom. */
    set = pictures();
    bands(&set.planes[0], scene);
    set.turns = 1;
    palette_apply(&set, 2);
    {
        const Plane *p = &set.planes[0];
        /* Upright 800 x 1200: upright u is sensor row 800 - u, so the
           strip, 96 wide on the right, is sensor rows 0 to 95, and its
           swatches run along the sensor rows. */
        assert(luma(p, 10, 2) == 60 && luma(p, 600, 2) == 200 && luma(p, 1190, 2) == 120);
        assert(luma(p, 1190, 200) == 120 && luma(p, 10, 200) == 60);
    }

    /* Dots: round swatches at the bottom left, ringed in white. */
    set = pictures();
    bands(&set.planes[0], scene);
    palette_apply(&set, 3);
    {
        const Plane *p = &set.planes[0];
        u32 d = MAIN_H * 9 / 100, cx = d / 2 + d / 2, cy = MAIN_H - d / 2 - d / 2;
        assert(luma(p, cx, cy) == 60);
        assert(luma(p, cx + d / 2 + 1, cy) == 240);
        assert(luma(p, cx + d + d / 3, cy) == 200);
        /* The corner of the box of a disc stays photo. */
        assert(luma(p, cx - d / 2 + 1, cy - d / 2 + 1) == 60);
    }

    /* Roll's film edge: the strip sits above it, and its colour is not
       sampled. */
    set = pictures();
    bands(&set.planes[0], scene);
    {
        Plane *p = &set.planes[0];
        u32 edge = MAIN_H * 82 / 1000;
        for (u32 y = MAIN_H - edge; y < MAIN_H; ++y)
            for (u32 x = 0; x < MAIN_W; ++x) GRKIT_BYTES(p->y + y * p->pitch + x)[0] = 20;
        grkit_test_settings[USED_ROLL_LENGTH] = 3;
        grkit_test_settings[USED_ROLL_EDGE] = 1;
        palette_apply(&set, 1);
        assert(luma(p, 100, MAIN_H - 1) == 20);
        assert(luma(p, 100, MAIN_H - edge - 1) == 60 && luma(p, 1100, MAIN_H - edge - 1) == 120);
        grkit_test_settings[USED_ROLL_LENGTH] = 0;
        grkit_test_settings[USED_ROLL_EDGE] = 0;
    }

    /* A monochrome picture gets grey swatches. */
    set = pictures();
    bands(&set.planes[0], scene);
    set.mono = 1;
    palette_apply(&set, 1);
    assert(cb_at(&set.planes[0], 600, MAIN_H - 1) == 128);

    /* Off draws nothing. */
    set = pictures();
    bands(&set.planes[0], scene);
    palette_apply(&set, 0);
    assert(luma(&set.planes[0], 1100, MAIN_H - 1) == 120 && luma(&set.planes[0], 100, MAIN_H - 1) == 60);
    return 0;
}
