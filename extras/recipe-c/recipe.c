/* Recipe C: the third menu of film recipes, the stocks that A and B have no
   room for. The code is that of Recipe A (extras/recipe); choosing a recipe
   here sets Recipe A and B to Off. The recipes are data of the camera
   family (recipes/<family>.toml, menu "c", generated into
   recipes-<family>.h). */
#include "grkit.h"
#include "grkit_module.h"
#include "../recipe/recipe_type.h"
#include "recipes.h"
#define USED_OTHER_PICK USED_RECIPE_PICK
#define USED_OTHER_PICK2 USED_RECIPEB_PICK
#include "../recipe/recipe_impl.h"
