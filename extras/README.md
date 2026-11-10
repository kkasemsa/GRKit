# Extra modules

Modules that the releases do not build and the site does not offer. Each
folder but `look-workshop` is a module folder, as in `modules/`, and the tests of `modules/` also
build and test each one (`crates/grkit-build/src/tests/modules.rs`).

| Folder | Module | Build |
| --- | --- | --- |
| `look-workshop` | Scripts that render every Film Look on your DNGs for the look workshop page. Not a module. | See its README. |
| `overlay` | Overlay: film artefacts drawn into each picture: a vignette, dust and hairs, a light leak, and scratches, each Off/Light/Medium/Strong, with presets Faded, Dusty, Leaky, and Expired. The dust, leak, and scratches fall differently on every shot. Off by default. | `grkit module build --dir extras/overlay --out OVERLAY.GRM` |
| `palette` | Palette Card: the main colours of each photo as swatches over its edge, a bottom strip, a side strip, or dots, with optional hex codes. | `grkit module build --dir extras/palette --out PALETTE.GRM` |
| `film/neg` | Negative: the Film page of the colour negative recipes, each a Film Look with its grain. The three Film pages (`neg`, `slide`, `cine`) share the code of `extras/film/film_impl.h`; their recipes are the packs of their groups in the camera family's data (`recipes/gr3.toml`, `recipes/gr4.toml`), and `scripts/film-pages.py` writes their manifests, text, and canister tiles. Choosing a recipe on one page sets the other pages to Off. A page has at most 8 recipes: its tiles share the 64 KB code segment of the module. Needs the Emulsion core, for `effect_find` (module ABI 2.6). | `sh scripts/recipe-module.sh gr3\|gr4 neg NEGATIVE.GRM` |
| `film/slide` | Slide: the slide and instant recipes. | `sh scripts/recipe-module.sh gr3\|gr4 slide SLIDE.GRM` |
| `film/cine` | Cine / B&W: the cinema and black-and-white recipes. | `sh scripts/recipe-module.sh gr3\|gr4 cine CINEBW.GRM` |

To install an extra module, copy its `.GRM` file to `GRKIT/MODULES` on the
card. A module there loads after the package modules (docs/module-reference.md).
