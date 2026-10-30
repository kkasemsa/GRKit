/* The host test of Recipe B (grkit module test): the code is that of Recipe
   A, whose test covers it in full. This checks the recipes of menu b, and
   that choosing one sets Recipe A to Off. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

void recipe_event(u32 event, u32 handle, u32 value);
void recipe_badge(const Surface *s, u32 pick);
u32 recipe_state(u32 index);

/* The recipes, in the order of PICK (recipes-gr4.h). */
#define HP5 1U
#define TRIX 2U
#define PROVIA 3U
#define ETERNA 4U

static void choose(u32 pick)
{
    grkit_test_settings[SETTING_PICK] = pick;
    recipe_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_PICK, pick);
}

int main(void)
{
    static const u16 full[4] = {0, 0, 720, 480};
    Surface s = grkit_test_surface(full);

    assert(grkit_module.setting_count == 3 && grkit_module.use_count == 5);
    assert(grkit_module.registration_count == 3);
    assert(grkit_module.settings[0].values == ETERNA + 1);
    assert(grkit_module.tile_count == grkit_module.settings[0].values);

    grkit_test_effects[0] = "eterna-500t-3523xd-v1";
    grkit_test_effects[1] = "provia-100f-v1";
    grkit_test_effects[2] = "hp5-plus-400-mono-v1";
    grkit_test_effects[3] = "trix-400-mono-v1";
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));

    /* Each recipe of menu b, by effect ID. */
    grkit_test_settings[USED_RECIPE_PICK] = 3;
    choose(HP5);
    assert(grkit_test_settings[USED_CORE_TONE] == 3 && grkit_test_settings[USED_CORE_GRAIN] == 3);
    /* Recipe A goes to Off. */
    assert(grkit_test_settings[USED_RECIPE_PICK] == 0);
    choose(TRIX);
    assert(grkit_test_settings[USED_CORE_TONE] == 4 && grkit_test_settings[USED_CORE_GRAIN] == 3);
    choose(PROVIA);
    assert(grkit_test_settings[USED_CORE_TONE] == 2);
    assert(grkit_test_settings[USED_CORE_GRAIN] == 1 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
    assert(recipe_state(PROVIA - 1) == 0);
    choose(ETERNA);
    assert(grkit_test_settings[USED_CORE_TONE] == 1 && grkit_test_settings[USED_CORE_GRAIN] == 2);

    /* Each label fits the surface. */
    for (u32 pick = 1; pick <= ETERNA; ++pick) {
        grkit_test_clear();
        recipe_badge(&s, pick);
    }
    return 0;
}
