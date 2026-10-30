#!/bin/sh
# Builds a Recipe module of one camera family: gr3 (GR III, GR IIIx) or gr4
# (GR IV), and one menu: a (Recipe A, extras/recipe, RECIPE.GRM) or b
# (Recipe B, extras/recipe-b, RECIPEB.GRM). A module is the same for each
# family but its recipes (menu a or b of recipes/<family>.toml, generated
# into recipes-<family>.h of the module folder).
#
#     sh scripts/recipe-module.sh gr3 a build/gr3/RECIPE.GRM [grkit module build options]
#     sh scripts/recipe-module.sh gr3 b build/gr3/RECIPEB.GRM
set -eu
cd "$(dirname "$0")/.."
usage() { echo "usage: $0 gr3|gr4 a|b OUT.GRM [options]" >&2; exit 2; }
family=${1:-}
menu=${2:-}
out=${3:-}
case "$family" in gr3 | gr4) ;; *) usage ;; esac
case "$menu" in a) folder=recipe ;; b) folder=recipe-b ;; *) usage ;; esac
[ -n "$out" ] || usage
shift 3
[ -f "extras/$folder/recipes-$family.h" ] || { echo "no extras/$folder/recipes-$family.h" >&2; exit 1; }
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
# Recipe B includes the code of Recipe A from ../recipe.
cp -R extras/recipe "$work/recipe"
[ "$folder" = recipe ] || cp -R "extras/$folder" "$work/$folder"
printf '#include "recipes-%s.h"\n' "$family" > "$work/$folder/recipes.h"
mkdir -p "$(dirname "$out")"
grkit module build --dir "$work/$folder" --out "$out" "$@"
