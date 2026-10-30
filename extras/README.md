# Extra modules

Modules that the releases do not build and the site does not offer. Each
folder is a module folder, as in `modules/`, and the tests of `modules/` also
build and test each one (`crates/grkit-build/src/tests/modules.rs`).

| Folder | Module | Build |
| --- | --- | --- |
| `look-workshop` | Scripts that render every Film Look on your DNGs for the look workshop page. Not a module. | See its README. |
| `roll` | Load a Roll: a roll of 12, 24, or 36 frames of the chosen recipe, a frame counter in the viewfinder, and an optional film edge on each picture. Uses `RECIPE.PICK` when Recipe loads. Needs module ABI 2.1. | `grkit module build --dir extras/roll --out ROLL.GRM` |
| `recipe` | Recipe: film recipes, each a Film Look with its grain, chosen from one menu. Its recipes are data of the camera family (`recipes/gr3.toml`, `recipes/gr4.toml`). It needs a GRKit with `effect_find` (module ABI 2.1). | `sh scripts/recipe-module.sh gr3\|gr4 OUT.GRM` |

To install an extra module, copy its `.GRM` file to `GRKIT/MODULES` on the
card. A module there loads after the package modules (docs/module-reference.md).
