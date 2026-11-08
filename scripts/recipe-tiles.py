#!/usr/bin/env python3
"""Draws the tiles of the Recipe modules: the film canister of each recipe's
effect, as effects/sources/canister_icons.py draws it, in the order of the
packs of recipes/gr4.toml (the same in every family).

    python3 scripts/recipe-tiles.py
"""
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "effects/sources"))
import canister_icons  # noqa: E402

MENUS = {"recipe": "a", "recipe-b": "b", "recipe-c": "c"}


def main():
    packs = tomllib.load(open(ROOT / "recipes/gr4.toml", "rb"))["pack"]
    for folder, menu in MENUS.items():
        manifest = tomllib.load(open(ROOT / "extras" / folder / "module.toml", "rb"))
        tiles = manifest["tiles"][1:]
        recipes = [p for p in packs if p["menu"] == menu]
        assert len(recipes) == len(tiles), (folder, len(recipes), len(tiles))
        for pack, tile in zip(recipes, tiles):
            body, band, letter = canister_icons.STOCKS[pack["effect"]]
            canister_icons.canister(body, band, letter).save(ROOT / "extras" / folder / tile)


if __name__ == "__main__":
    main()
