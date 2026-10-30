/* Recipe: a short menu of film recipes. A recipe is a Film Look, named by its
   stable effect ID, with a grain strength, a grain size, and an optional
   aspect. Choosing a recipe applies it at once; Reset applies it again after
   experimenting. The viewfinder shows the name of the active recipe, with a
   mark when a setting differs from it. The recipes are data of the camera
   family (recipes/<family>.toml, generated into recipes-<family>.h). */
#include "grkit.h"
#include "grkit_module.h"

/* One recipe. grain, size, and aspect are values of CORE.GRAIN,
   CORE.GRAIN_SIZE, and CORE.ASPECT; RECIPE_NO_ASPECT leaves the aspect as it
   is. label is the viewfinder text: capitals, digits, space, '-', '+', '.'. */
typedef struct {
    const char *effect;
    const char *label;
    u8 grain, size, aspect, reserved;
} Recipe;
#define RECIPE_NO_ASPECT 0xffU
#include "recipes.h"

/* ---- applying a recipe ---------------------------------------------------- */

/* The text of a recipe whose Film Look is not installed, in the menu
   languages: English, Simplified Chinese, Traditional Chinese, Japanese,
   Korean. */
static const u16 missing_en[] = u"Film Look not installed";
static const u16 missing_hans[] = u"未安装该胶片风格";
static const u16 missing_hant[] = u"未安裝該底片風格";
static const u16 missing_ja[] = u"フィルムルックが未インストールです";
static const u16 missing_ko[] = u"필름 룩이 설치되지 않았습니다";
static const u16 *const missing[GRKIT_LANGUAGES] = {missing_en, missing_hans, missing_hant, missing_ja, missing_ko};

/* Apply recipe `index`. A recipe whose Film Look is not installed changes
   nothing: it never falls back to another look. */
static void apply(u32 index)
{
    const Recipe *r = &recipes[index];
    u32 tone = effect_find(r->effect);
    if (!tone) {
        ui_toast(missing, 2000);
        return;
    }
    setting_set(USED_CORE_TONE, tone);
    setting_set(USED_CORE_GRAIN, r->grain);
    setting_set(USED_CORE_GRAIN_SIZE, r->size);
    if (r->aspect != RECIPE_NO_ASPECT) setting_set(USED_CORE_ASPECT, r->aspect);
}

/* A recipe applies when it is chosen. Reset applies the chosen recipe again,
   then returns to Keep. A setting_set here sends no second event. */
void recipe_event(u32 event, u32 handle, u32 value)
{
    u32 pick;
    if (event != GRKIT_EVENT_SETTING_CHANGED) return;
    if (handle == SETTING_PICK) {
        if (value && value <= RECIPE_COUNT) apply(value - 1);
        return;
    }
    if (handle == SETTING_RESET && value) {
        pick = setting_get(SETTING_PICK);
        if (pick && pick <= RECIPE_COUNT) apply(pick - 1);
        setting_set(SETTING_RESET, 0);
    }
}

/* How the settings stand against recipe `index`: RECIPE_SAME, RECIPE_CHANGED
   when a setting differs from the recipe, or RECIPE_MISSING when its Film Look
   is not installed. */
#define RECIPE_SAME 0U
#define RECIPE_CHANGED 1U
#define RECIPE_MISSING 2U
u32 recipe_state(u32 index)
{
    const Recipe *r = &recipes[index];
    u32 tone = effect_find(r->effect);
    if (!tone) return RECIPE_MISSING;
    if (setting_get(USED_CORE_TONE) != tone || setting_get(USED_CORE_GRAIN) != r->grain ||
        setting_get(USED_CORE_GRAIN_SIZE) != r->size)
        return RECIPE_CHANGED;
    if (r->aspect != RECIPE_NO_ASPECT && USED_CORE_ASPECT != GRKIT_NO_SETTING &&
        setting_get(USED_CORE_ASPECT) != r->aspect)
        return RECIPE_CHANGED;
    return RECIPE_SAME;
}

/* ---- the viewfinder label ------------------------------------------------- */

/* 5 x 7 characters, one byte for each row, bit 4 the left column: A to Z,
   0 to 9, then '-', '+', '*', '?', '.'. Other characters are blank. */
static const u8 font[41][7] = {
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
    {0x00,0x04,0x15,0x0e,0x15,0x04,0x00},{0x0e,0x11,0x01,0x02,0x04,0x00,0x04},
    {0x00,0x00,0x00,0x00,0x00,0x0c,0x0c},
};
/* The glyph of a character, or 0 for a blank. */
static const u8 *glyph(char c)
{
    if (c >= 'A' && c <= 'Z') return font[c - 'A'];
    if (c >= '0' && c <= '9') return font[26 + (c - '0')];
    switch (c) {
    case '-': return font[36];
    case '+': return font[37];
    case '*': return font[38];
    case '?': return font[39];
    case '.': return font[40];
    default: return 0;
    }
}
/* The label: 5 x 7 characters at twice their size, white on a dark edge.
   LABEL_TOP of the third row of the native text. */
#define SCALE 2U
#define ADVANCE (6U * SCALE)
#define LABEL_ROW (LABEL_TOP + 24U)
#define WHITE 0xffffffffU
#define EDGE 0xc0000000U
static void put(const Surface *s, u32 x, u32 y, u32 pixel)
{
    if (x < 720 && y < 480) SURFACE_PIXEL(s, x, y) = pixel;
}
/* Draw `c` with its top left at (x, y): first a dark edge one pixel around
   each dot, then the dots. */
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
                        put(s, x + column * SCALE + dx + pass, y + row * SCALE + dy + pass, pass ? WHITE : EDGE);
            }
}
/* The viewfinder label of the chosen recipe, while Name is Show: its name,
   then '*' when a setting differs from it, or '?' when its Film Look is not
   installed. */
void recipe_badge(const Surface *s, u32 pick)
{
    const char *p;
    u32 x = LABEL_LEFT, state;
    if (!pick || pick > RECIPE_COUNT || setting_get(SETTING_NAME)) return;
    for (p = recipes[pick - 1].label; *p; ++p, x += ADVANCE) draw_char(s, *p, x, LABEL_ROW);
    state = recipe_state(pick - 1);
    if (state == RECIPE_CHANGED) draw_char(s, '*', x, LABEL_ROW);
    else if (state == RECIPE_MISSING) draw_char(s, '?', x, LABEL_ROW);
}
