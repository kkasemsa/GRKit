# Extra modules

Modules that the releases do not build and the site does not offer. Each
folder but `look-workshop` is a module folder, as in `modules/`, and the tests of `modules/` also
build and test each one (`crates/grkit-build/src/tests/modules.rs`).

| Folder | Module | Build |
| --- | --- | --- |
| `look-workshop` | Scripts that render every Film Look on your DNGs for the look workshop page. Not a module. | See its README. |
| `overlay` | Overlay: film artefacts drawn into each picture: a vignette, dust and hairs, a light leak, and scratches, each Off/Light/Medium/Strong, with presets Faded, Dusty, Leaky, and Expired. The dust, leak, and scratches fall differently on every shot. Off by default. | `grkit module build --dir extras/overlay --out OVERLAY.GRM` |
| `palette` | Palette Card: the main colours of each photo as swatches over its edge, a bottom strip, a side strip, or dots, with optional hex codes. | `grkit module build --dir extras/palette --out PALETTE.GRM` |
| `film/all` | Film: one page of the film recipes, each a Film Look with its grain, in groups by type (negative, slide, cine, black-and-white, instant). Its code is `extras/film/film_impl.h`; its recipes are the packs of the camera family's data (`recipes/gr3.toml`, `recipes/gr4.toml`), and `scripts/film-pages.py` writes its manifest, text, and canister tiles. One page, as the core has 10 page slots and Glow takes 3. Needs the Emulsion core, for `effect_find` (module ABI 2.6). | `sh scripts/recipe-module.sh gr3\|gr4 all FILM.GRM` |

To install an extra module, copy its `.GRM` file to `GRKIT/MODULES` on the
card. A module there loads after the package modules (docs/module-reference.md).
