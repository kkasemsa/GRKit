# Extra modules

Modules that the releases do not build and the site does not offer. Each
folder is a module folder, as in `modules/`, and the tests of `modules/` also
build and test each one (`crates/grkit-build/src/tests/modules.rs`).

| Folder | Module | Build |
| --- | --- | --- |
| `recipe` | Recipe: film recipes, each a Film Look with its grain, chosen from one menu. Its recipes are data of the camera family (`recipes/gr3.toml`, `recipes/gr4.toml`). It needs a GRKit with `effect_find` (module ABI 2.1). | `sh scripts/recipe-module.sh gr3\|gr4 OUT.GRM` |

To install an extra module, copy its `.GRM` file to `GRKIT/MODULES` on the
card. A module there loads after the package modules (docs/module-reference.md).
