#!/usr/bin/env python3
"""Writes the manifest, source, text, and tiles of the Film page
(extras/film/all) from the packs of recipes/gr4.toml (the
same packs in every family) and the groups of PAGES below. The recipe headers
come from the recipes test (GRKIT_UPDATE_GENERATED=1 cargo test -p grkit-core
--test recipes), which reads the same PAGES table from this file's twin in
crates/grkit-core/tests/recipes.rs.

    python3 scripts/film-pages.py
"""
import json
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "effects/sources"))
import canister_icons  # noqa: E402

FILM = ROOT / "extras/film"
LANGUAGES = ["en", "zh-Hans", "zh-Hant", "ja", "ko"]
# folder: module ID, order, groups, module tile letter, title per language.
# One page: the core has 10 page slots (GRKIT_MODULE_PAGES_MAX), and Glow of
# GRKit takes 3 of them.
PAGES = {
    "all": ("FILM", 5, ["negative", "slide", "cine", "bw", "instant"], "F",
            ["Film", "胶片", "底片", "フィルム", "필름"]),
}
# The row prefix of each group, per language.
PREFIX = {
    "negative": ["Neg", "负片", "負片", "ネガ", "네거"],
    "slide": ["Slide", "反转片", "正片", "ポジ", "슬라이드"],
    "cine": ["Cine", "电影", "電影", "シネ", "시네"],
    "bw": ["B&W", "黑白", "黑白", "白黒", "흑백"],
    "instant": ["Instant", "拍立得", "拍立得", "インスタント", "인스턴트"],
}
OFF = ["Off", "关", "關", "オフ", "끄기"]
TILE_MAX = 63


def main():
    packs = tomllib.load(open(ROOT / "recipes/gr4.toml", "rb"))["pack"]
    ids = {folder: spec[0] for folder, spec in PAGES.items()}
    for folder, (module_id, order, groups, letter, titles) in PAGES.items():
        recipes = [p for p in packs if p["group"] in groups]
        assert 1 <= len(recipes) <= TILE_MAX, (folder, len(recipes))
        directory = FILM / folder
        (directory / "tiles").mkdir(parents=True, exist_ok=True)
        (directory / "text").mkdir(exist_ok=True)
        others = [i for f, i in ids.items() if f != folder]
        tiles = ["tiles/0-module.png"] + [f"tiles/{i + 1}-{p['id']}.png" for i, p in enumerate(recipes)]
        uses = ["CORE.TONE", "CORE.GRAIN", "CORE.GRAIN_SIZE", "?CORE.ASPECT"] + [f"?{o}.PICK" for o in others]
        groups_text = " and ".join(groups)
        other = [f"USED_{o}_PICK" for o in others] + ["GRKIT_NO_SETTING"] * 2
        manifest = f"""# {titles[0]}: the Film page of the {groups_text} recipes, each a Film Look with
# its grain and an optional aspect, with the code of extras/film/film_impl.h.
# Choosing a recipe on any Film page sets the other pages to Off. The recipes
# are data of the camera family: the packs of group {groups_text} in
# recipes/<family>.toml, generated into recipes-<family>.h. recipes.h selects
# the family; scripts/recipe-module.sh builds the module of a family.
# scripts/film-pages.py writes this file, film.c, text/, and tiles/.
#
# The page has no native settings rows (a row holds at most 8 values), so
# Fn opens no settings page; the GRKit section of MENU shows Enable and the
# page row.
id = "{module_id}"
version = 1
# The default place in ADJ and in MENU: before Film Look (10).
order = {order}
sources = ["film.c"]
# Tile n shows recipe n. Tile 0 is the module tile, for Off at the root.
tiles = [
{chr(10).join(f'    "{t}",' for t in tiles)}
]
uses = {json.dumps(uses)}

# 0 Off, then recipe n - 1 of recipes.h.
[[settings]]
key = "PICK"
values = {len(recipes) + 1}
saved = true
public = true

[page]
setting = "PICK"
off_row = true

[events]
handle = "film_event"
on = ["setting_changed"]

# The name of the recipe shows with the full display, below the first rows of
# native text.
[[layers]]
paint = "film_badge"
setting = "PICK"
order = 45
full_disp = true
live_only = true
"""
        (directory / "module.toml").write_text(manifest)
        source = f"""/* {titles[0]}: the Film page of the {groups_text} recipes, with the code of
   extras/film/film_impl.h. scripts/film-pages.py writes this file. */
#include "grkit.h"
#include "grkit_module.h"
#include "../recipe_type.h"
#include "recipes.h"
#define USED_OTHER_PICK {other[0]}
#define USED_OTHER_PICK2 {other[1]}
#include "../film_impl.h"
"""
        (directory / "film.c").write_text(source)
        for l, language in enumerate(LANGUAGES):
            rows = [OFF[l]] + [f"{PREFIX[p['group']][l]} - {p['name']}" for p in recipes]
            text = (f"title = {json.dumps(titles[l], ensure_ascii=False)}\n"
                    f"rows = [{', '.join(json.dumps(r, ensure_ascii=False) for r in rows)}]\n")
            (directory / "text" / f"{language}.toml").write_text(text)
        for old in (directory / "tiles").glob("*.png"):
            old.unlink()
        canister_icons.canister((150, 150, 155), (60, 60, 65), letter).save(directory / tiles[0])
        for pack, tile in zip(recipes, tiles[1:]):
            body, band, letter = canister_icons.STOCKS[pack["effect"]]
            canister_icons.canister(body, band, letter).save(directory / tile)
        print(folder, module_id, len(recipes), "recipes")


if __name__ == "__main__":
    main()
