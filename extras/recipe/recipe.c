/* Recipe A: a short menu of film recipes. A recipe is a Film Look, named by
   its stable effect ID, with a grain strength, a grain size, and an optional
   aspect. Choosing a recipe applies it at once and sets Recipe B to Off;
   Reset applies it again after experimenting. The viewfinder shows the name
   of the active recipe, with a mark when a setting differs from it. The
   recipes are data of the camera family (recipes/<family>.toml, menu "a",
   generated into recipes-<family>.h). */
#include "grkit.h"
#include "grkit_module.h"
#include "recipe_type.h"
#include "recipes.h"
#define USED_OTHER_PICK USED_RECIPEB_PICK
#include "recipe_impl.h"
