/* The host test of a Film page (grkit module test): choosing each recipe of
   the page, a missing Film Look, the other pages going to Off, and the
   viewfinder label, with the GR IV recipes of the page. The test.c of a page
   includes grkit_module.h, ../recipe_type.h, and its recipes.h (the same
   table as film.c), defines OTHER_PICK and OTHER_PICK2, the used PICKs of
   the other pages, when there are other pages, and includes this. */
#include <assert.h>
#include <string.h>

void film_event(u32 event, u32 handle, u32 value);
void film_badge(const Surface *s, u32 pick);
u32 film_state(u32 index);

static void choose(u32 pick)
{
    grkit_test_settings[SETTING_PICK] = pick;
    film_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_PICK, pick);
}

/* Whether the label row has a lit pixel at or right of column x. */
static int drawn_from(u32 x)
{
    for (u32 y = LABEL_TOP; y < LABEL_TOP + 60; ++y)
        for (u32 i = x; i < 720; ++i)
            if (grkit_test_pixel(i, y) == 0xffffffffU) return 1;
    return 0;
}

int main(void)
{
    static const u16 full[4] = {0, 0, 720, 480};
    Surface s = grkit_test_surface(full);
    u32 i, calls;

    /* The descriptor: PICK alone; four used settings, and the PICKs of the
       other pages after them; a page, an event subscriber, and a layer; Off
       and a tile for each recipe. */
    assert(RECIPE_COUNT >= 2 && RECIPE_COUNT <= 63);
#ifdef OTHER_PICK
    assert(grkit_module.setting_count == 1 && grkit_module.use_count == 6);
#else
    assert(grkit_module.setting_count == 1 && grkit_module.use_count == 4);
#endif
    assert(grkit_module.registration_count == 3);
    assert(grkit_module.settings[0].values == RECIPE_COUNT + 1);
    assert(grkit_module.tile_count == RECIPE_COUNT + 1);

    /* The installed effects, in the reverse order of the recipes, after one
       of no page: a recipe finds its effect by ID, not by its place. The
       last recipe is not installed. */
    grkit_test_effects[0] = "elsewhere-v1";
    for (i = 0; i + 1 < RECIPE_COUNT; ++i) grkit_test_effects[1 + i] = recipes[RECIPE_COUNT - 2 - i].effect;

    /* Choosing a recipe sets its Film Look, grain, and grain size, and sets
       the other pages to Off. */
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));
    for (i = 0; i + 1 < RECIPE_COUNT; ++i) {
        u32 tone = 1 + (RECIPE_COUNT - 2 - i) + 1;
#ifdef OTHER_PICK
    #ifdef OTHER_PICK
    grkit_test_settings[OTHER_PICK] = 2;
#endif
        grkit_test_settings[OTHER_PICK2] = 3;
#endif
        choose(i + 1);
        assert(grkit_test_settings[USED_CORE_TONE] == tone);
        assert(grkit_test_settings[USED_CORE_GRAIN] == recipes[i].grain);
        assert(grkit_test_settings[USED_CORE_GRAIN_SIZE] == recipes[i].size);
#ifdef OTHER_PICK
        assert(grkit_test_settings[OTHER_PICK] == 0 && grkit_test_settings[OTHER_PICK2] == 0);
#endif
        assert(film_state(i) == 0);
    }
    /* No recipe of this page sets an aspect, so the aspect stays. */
    assert(grkit_test_settings[USED_CORE_ASPECT] == 0);

    /* A recipe whose Film Look is not installed changes nothing, says so,
       and leaves the other pages as they are. */
    choose(1);
    calls = grkit_test_ui_calls;
#ifdef OTHER_PICK
    grkit_test_settings[OTHER_PICK] = 2;
#endif
    {
        u32 tone = grkit_test_settings[USED_CORE_TONE], grain = grkit_test_settings[USED_CORE_GRAIN];
        choose(RECIPE_COUNT);
        assert(grkit_test_settings[USED_CORE_TONE] == tone && grkit_test_settings[USED_CORE_GRAIN] == grain);
    }
    assert(grkit_test_ui_calls == calls + 1);
    assert(film_state(RECIPE_COUNT - 1) == 2);
#ifdef OTHER_PICK
    assert(grkit_test_settings[OTHER_PICK] == 2);
    grkit_test_settings[OTHER_PICK] = 0;
#endif

    /* Off applies nothing. */
    choose(1);
    {
        u32 tone = grkit_test_settings[USED_CORE_TONE];
        choose(0);
        assert(grkit_test_settings[USED_CORE_TONE] == tone);
    }

    /* A change after choosing marks the recipe changed. */
    choose(1);
    grkit_test_settings[USED_CORE_GRAIN] = 4;
    assert(film_state(0) == 1);
    grkit_test_settings[USED_CORE_GRAIN] = recipes[0].grain;
    grkit_test_settings[USED_CORE_TONE] = 19;
    assert(film_state(0) == 1);
    /* Other events and settings do nothing. */
    grkit_test_settings[USED_CORE_GRAIN] = 4;
    film_event(GRKIT_EVENT_SYSTEM_READY, 0, 0);
    film_event(GRKIT_EVENT_SETTING_CHANGED, USED_CORE_GRAIN, 1);
#ifdef OTHER_PICK
    film_event(GRKIT_EVENT_SETTING_CHANGED, OTHER_PICK, 1);
#endif
    assert(grkit_test_settings[USED_CORE_GRAIN] == 4 && grkit_test_settings[USED_CORE_TONE] == 19);

    /* The label: the name of the chosen recipe, white, in the third row. */
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));
    choose(1);
    grkit_test_clear();
    film_badge(&s, 1);
    assert(drawn_from(LABEL_LEFT));
    assert(grkit_test_pixel(0, 0) == GRKIT_TEST_CLEAR);
    /* A name is n characters of 12 pixels: nothing past them while the
       recipe stands. */
    assert(!drawn_from(LABEL_LEFT + 12 * (u32)strlen(recipes[0].label)));
    /* A changed setting adds '*' after the name. */
    grkit_test_settings[USED_CORE_GRAIN] = recipes[0].grain + 1;
    grkit_test_clear();
    film_badge(&s, 1);
    assert(drawn_from(LABEL_LEFT + 12 * (u32)strlen(recipes[0].label)));
    /* Off draws nothing. */
    grkit_test_clear();
    film_badge(&s, 0);
    assert(!drawn_from(0));
    /* Each label fits the surface. */
    for (i = 1; i <= RECIPE_COUNT; ++i) {
        grkit_test_clear();
        film_badge(&s, i);
    }
    return 0;
}
