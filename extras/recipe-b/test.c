/* The host test of Recipe B (grkit module test): the code is that of Recipe
   A, whose test covers it in full. This checks the recipes of menu b, and
   that choosing one sets Recipe A and C to Off. */
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
#define KODACHROME 5U
#define VELVIA 6U
#define VISION3 7U

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
    assert(grkit_module.settings[0].values == VISION3 + 1);
    assert(grkit_module.tile_count == grkit_module.settings[0].values);

    grkit_test_effects[0] = "eterna-500t-3523xd-v1";
    grkit_test_effects[1] = "provia-100f-v1";
    grkit_test_effects[2] = "hp5-plus-400-mono-v1";
    grkit_test_effects[3] = "trix-400-mono-v1";
    grkit_test_effects[4] = "vision3-500t-supra-v1";
    grkit_test_effects[5] = "kodachrome-64-v1";
    grkit_test_effects[6] = "velvia-100-v1";
    memset(grkit_test_settings, 0, sizeof(grkit_test_settings));

    /* Each recipe of menu b, by effect ID. */
    grkit_test_settings[USED_RECIPE_PICK] = 3;
    grkit_test_settings[USED_RECIPEC_PICK] = 2;
    choose(HP5);
    assert(grkit_test_settings[USED_CORE_TONE] == 3 && grkit_test_settings[USED_CORE_GRAIN] == 3);
    /* Recipe A and C go to Off. */
    assert(grkit_test_settings[USED_RECIPE_PICK] == 0 && grkit_test_settings[USED_RECIPEC_PICK] == 0);
    choose(TRIX);
    assert(grkit_test_settings[USED_CORE_TONE] == 4 && grkit_test_settings[USED_CORE_GRAIN] == 3);
    choose(PROVIA);
    assert(grkit_test_settings[USED_CORE_TONE] == 2);
    assert(grkit_test_settings[USED_CORE_GRAIN] == 1 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
    assert(recipe_state(PROVIA - 1) == 0);
    choose(ETERNA);
    assert(grkit_test_settings[USED_CORE_TONE] == 1 && grkit_test_settings[USED_CORE_GRAIN] == 2);
    choose(KODACHROME);
    assert(grkit_test_settings[USED_CORE_TONE] == 6 && grkit_test_settings[USED_CORE_GRAIN_SIZE] == 1);
    choose(VELVIA);
    assert(grkit_test_settings[USED_CORE_TONE] == 7 && grkit_test_settings[USED_CORE_GRAIN] == 1);
    choose(VISION3);
    assert(grkit_test_settings[USED_CORE_TONE] == 5 && grkit_test_settings[USED_CORE_GRAIN] == 2);

    /* Each label fits the surface. */
    for (u32 pick = 1; pick <= VISION3; ++pick) {
        grkit_test_clear();
        recipe_badge(&s, pick);
    }
    return 0;
}
