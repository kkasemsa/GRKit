/* The recipe of the Recipe menus (extras/recipe, extras/recipe-b). grain,
   size, and aspect are values of CORE.GRAIN, CORE.GRAIN_SIZE, and
   CORE.ASPECT; RECIPE_NO_ASPECT leaves the aspect as it is. label is the
   viewfinder text: capitals, digits, space, '-', '+', '.'. */
#ifndef RECIPE_TYPE_H
#define RECIPE_TYPE_H
typedef struct {
    const char *effect;
    const char *label;
    u8 grain, size, aspect, reserved;
} Recipe;
#define RECIPE_NO_ASPECT 0xffU
#endif
