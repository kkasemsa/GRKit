# Extra modules

Modules that the releases do not build and the site does not offer. Each
folder but `look-workshop` is a module folder, as in `modules/`, and the tests of `modules/` also
build and test each one (`crates/grkit-build/src/tests/modules.rs`).

| Folder | Module | Build |
| --- | --- | --- |
| `look-workshop` | Scripts that render every Film Look on your DNGs for the look workshop page. Not a module. | See its README. |
| `onion` | Onion Skin: a faint picture over the live view, from the last shot or a held reference. Needs module ABI 2.1. | `grkit module build --dir extras/onion --out ONION.GRM` |
| `palette` | Palette Card: the main colours of each photo as swatches over its edge, a bottom strip, a side strip, or dots, with optional hex codes. | `grkit module build --dir extras/palette --out PALETTE.GRM` |
| `recipe` | Recipe A: film recipes, each a Film Look with its grain, chosen from one menu: the colour negatives. Its recipes are menu `a` of the camera family's data (`recipes/gr3.toml`, `recipes/gr4.toml`). Choosing one sets Recipe B to Off. It needs a GRKit with `effect_find` (module ABI 2.1). | `sh scripts/recipe-module.sh gr3\|gr4 a RECIPE.GRM` |
| `recipe-b` | Recipe B: the black-and-white, slide, and cinema recipes (menu `b`), with the code of Recipe A. Choosing one sets Recipe A to Off. | `sh scripts/recipe-module.sh gr3\|gr4 b RECIPEB.GRM` |

To install an extra module, copy its `.GRM` file to `GRKIT/MODULES` on the
card. A module there loads after the package modules (docs/module-reference.md).
