/* Recipe B: the second menu of film recipes, the black-and-white, slide,
   and cinema stocks. The code is that of Recipe A (extras/recipe); choosing a
   recipe here sets Recipe A to Off. The recipes are data of the camera
   family (recipes/<family>.toml, menu "b", generated into
   recipes-<family>.h). */
#include "grkit.h"
#include "grkit_module.h"
#include "../recipe/recipe_type.h"
#include "recipes.h"
#define USED_OTHER_PICK USED_RECIPE_PICK
#include "../recipe/recipe_impl.h"
