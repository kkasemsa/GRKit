/* The host test of Recipe A (grkit module test): choosing, resetting, the
   other menu going to Off, and the viewfinder label, with the GR IV recipes
   of recipes.h. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

void recipe_event(u32 event, u32 handle, u32 value);
void recipe_badge(const Surface *s, u32 pick);
u32 recipe_state(u32 index);

/* The recipes, in the order of PICK (recipes-gr4.h). */
#define PORTRA 1U
#define EKTAR 2U
#define GOLD 3U
#define ULTRAMAX 4U
#define C200 5U

static void choose(u32 pick)
{
    grkit_test_settings[SETTING_PICK] = pick;
    recipe_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_PICK, pick);
}

static void reset_settings(void)
{
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));
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

    /* The descriptor: PICK, RESET, and NAME; five used settings, the last
       the PICK of Recipe B; a page, an event subscriber, and a layer; Off and
       a tile for each recipe. */
    assert(grkit_module.setting_count == 3 && grkit_module.use_count == 5);
    assert(grkit_module.registration_count == 3);
    assert(grkit_module.settings[0].values == C200 + 1);
    assert(grkit_module.tile_count == grkit_module.settings[0].values);

    /* The installed effects, in an order other than that of the recipes:
       a recipe finds its effect by ID, not by its place. Gold 200 is not
       installed. */
    grkit_test_effects[0] = "velvia-100-v1";
    grkit_test_effects[1] = "hp5-plus-400-mono-v1";
    grkit_test_effects[2] = "portra-400-endura-v1";
    grkit_test_effects[3] = "ektar-100-edge-v1";
    grkit_test_effects[4] = "ultramax-400-premier-v1";
    grkit_test_effects[5] = "c200-crystal-v1";

    /* Choosing a recipe sets its Film Look, grain, and grain size. */
    reset_settings();
    choose(PORTRA);
    assert(grkit_test_settings[USED_CORE_TONE] == 3);
    assert(grkit_test_settings[USED_CORE_GRAIN] == 2 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 0);
    assert(recipe_state(PORTRA - 1) == 0);
    choose(EKTAR);
    assert(grkit_test_settings[USED_CORE_TONE] == 4);
    assert(grkit_test_settings[USED_CORE_GRAIN] == 2 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
    choose(C200);
    assert(grkit_test_settings[USED_CORE_TONE] == 6 && grkit_test_settings[USED_CORE_GRAIN] == 2);
    choose(ULTRAMAX);
    assert(grkit_test_settings[USED_CORE_TONE] == 5 && grkit_test_settings[USED_CORE_GRAIN] == 2);
    choose(EKTAR);
    /* No recipe sets an aspect, so the aspect stays. */
    assert(grkit_test_settings[USED_CORE_ASPECT] == 0);

    /* A recipe whose Film Look is not installed changes nothing and says so. */
    {
        u32 calls = grkit_test_ui_calls;
        grkit_test_settings[USED_RECIPEB_PICK] = 2;
        choose(GOLD);
        assert(grkit_test_settings[USED_CORE_TONE] == 4 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
        assert(grkit_test_ui_calls == calls + 1);
        assert(recipe_state(GOLD - 1) == 2);
        /* ... and leaves Recipe B as it is. */
        assert(grkit_test_settings[USED_RECIPEB_PICK] == 2);
    }

    /* A recipe that applies sets Recipe B to Off. */
    choose(PORTRA);
    assert(grkit_test_settings[USED_RECIPEB_PICK] == 0);

    /* Off applies nothing. */
    choose(PORTRA);
    choose(0);
    assert(grkit_test_settings[USED_CORE_TONE] == 3);

    /* A change after choosing marks the recipe changed; Reset applies it
       again and returns to Keep. */
    choose(PORTRA);
    grkit_test_settings[USED_CORE_GRAIN] = 4;
    assert(recipe_state(PORTRA - 1) == 1);
    grkit_test_settings[USED_CORE_GRAIN] = 2;
    grkit_test_settings[USED_CORE_TONE] = 1;
    assert(recipe_state(PORTRA - 1) == 1);
    grkit_test_settings[SETTING_RESET] = 1;
    recipe_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_RESET, 1);
    assert(grkit_test_settings[USED_CORE_TONE] == 3 && grkit_test_settings[USED_CORE_GRAIN] == 2);
    assert(grkit_test_settings[SETTING_RESET] == 0);
    assert(recipe_state(PORTRA - 1) == 0);
    /* Reset with no recipe returns to Keep and changes nothing. */
    grkit_test_settings[SETTING_PICK] = 0;
    grkit_test_settings[USED_CORE_GRAIN] = 4;
    grkit_test_settings[SETTING_RESET] = 1;
    recipe_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_RESET, 1);
    assert(grkit_test_settings[USED_CORE_GRAIN] == 4 && grkit_test_settings[SETTING_RESET] == 0);
    /* Other events and settings do nothing. */
    grkit_test_settings[SETTING_PICK] = PORTRA;
    recipe_event(GRKIT_EVENT_SYSTEM_READY, 0, 0);
    recipe_event(GRKIT_EVENT_SETTING_CHANGED, USED_CORE_GRAIN, 1);
    assert(grkit_test_settings[USED_CORE_GRAIN] == 4);

    /* The label: the name of the chosen recipe, white, in the third row. */
    reset_settings();
    choose(PORTRA);
    grkit_test_clear();
    recipe_badge(&s, PORTRA);
    assert(drawn_from(LABEL_LEFT));
    assert(grkit_test_pixel(0, 0) == GRKIT_TEST_CLEAR);
    /* "PORTRA 400" is 10 characters of 12 pixels: nothing past them while
       the recipe stands. */
    assert(!drawn_from(LABEL_LEFT + 10 * 12));
    /* A changed setting adds '*' after the name. */
    grkit_test_settings[USED_CORE_GRAIN] = 1;
    grkit_test_clear();
    recipe_badge(&s, PORTRA);
    assert(drawn_from(LABEL_LEFT + 10 * 12));
    /* Hide and Off draw nothing. */
    grkit_test_settings[SETTING_NAME] = 1;
    grkit_test_clear();
    recipe_badge(&s, PORTRA);
    assert(!drawn_from(0));
    grkit_test_settings[SETTING_NAME] = 0;
    grkit_test_clear();
    recipe_badge(&s, 0);
    assert(!drawn_from(0));
    /* Each label fits the surface. */
    for (u32 pick = 1; pick <= ULTRAMAX; ++pick) {
        grkit_test_clear();
        recipe_badge(&s, pick);
    }
    return 0;
}
