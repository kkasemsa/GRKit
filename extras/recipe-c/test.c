/* The host test of Recipe C (grkit module test): the code is that of Recipe
   A, whose test covers it in full. This checks the recipes of menu c, and
   that choosing one sets Recipe A and B to Off. */
#include <assert.h>
#include <string.h>
#include "grkit_module.h"

void recipe_event(u32 event, u32 handle, u32 value);
void recipe_badge(const Surface *s, u32 pick);
u32 recipe_state(u32 index);

/* The recipes, in the order of PICK (recipes-gr4.h). */
#define PORTRA800 1U
#define E100 2U
#define V250D 3U
#define DOUBLEX 4U
#define ASTIA 5U
#define INSTAX 6U

static void choose(u32 pick)
{
    grkit_test_settings[SETTING_PICK] = pick;
    recipe_event(GRKIT_EVENT_SETTING_CHANGED, SETTING_PICK, pick);
}

int main(void)
{
    static const u16 full[4] = {0, 0, 720, 480};
    Surface s = grkit_test_surface(full);

    assert(grkit_module.setting_count == 3 && grkit_module.use_count == 6);
    assert(grkit_module.registration_count == 3);
    assert(grkit_module.settings[0].values == INSTAX + 1);
    assert(grkit_module.tile_count == grkit_module.settings[0].values);

    grkit_test_effects[0] = "instax-v1";
    grkit_test_effects[1] = "astia-100f-v1";
    grkit_test_effects[2] = "portra-800-push2-v1";
    grkit_test_effects[3] = "double-x-5222-mono-v1";
    grkit_test_effects[4] = "ektachrome-e100-v1";
    grkit_test_effects[5] = "vision3-250d-2383-v1";
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));

    /* Each recipe of menu c, by effect ID. */
    grkit_test_settings[USED_RECIPE_PICK] = 2;
    grkit_test_settings[USED_RECIPEB_PICK] = 5;
    choose(PORTRA800);
    assert(grkit_test_settings[USED_CORE_TONE] == 3 && grkit_test_settings[USED_CORE_GRAIN] == 3);
    /* Recipe A and B go to Off. */
    assert(grkit_test_settings[USED_RECIPE_PICK] == 0 && grkit_test_settings[USED_RECIPEB_PICK] == 0);
    choose(E100);
    assert(grkit_test_settings[USED_CORE_TONE] == 5 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
    choose(V250D);
    assert(grkit_test_settings[USED_CORE_TONE] == 6 && grkit_test_settings[USED_CORE_GRAIN] == 1);
    choose(DOUBLEX);
    assert(grkit_test_settings[USED_CORE_TONE] == 4 && grkit_test_settings[USED_CORE_GRAIN] == 3);
    choose(ASTIA);
    assert(grkit_test_settings[USED_CORE_TONE] == 2 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
    choose(INSTAX);
    assert(grkit_test_settings[USED_CORE_TONE] == 1 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 3);
    assert(recipe_state(INSTAX - 1) == 0);

    /* Each label fits the surface. */
    for (u32 pick = 1; pick <= INSTAX; ++pick) {
        grkit_test_clear();
        recipe_badge(&s, pick);
    }
    return 0;
}
