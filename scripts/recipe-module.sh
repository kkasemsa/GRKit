#!/bin/sh
# Builds the Recipe module of one camera family: gr3 (GR III, GR IIIx) or gr4
# (GR IV). The module is the same for each family but its recipes
# (recipes/<family>.toml, generated into extras/recipe/recipes-<family>.h).
#
#     sh scripts/recipe-module.sh gr3 build/gr3/RECIPE.GRM [grkit module build options]
set -eu
cd "$(dirname "$0")/.."
family=${1:-}
out=${2:-}
case "$family" in
    gr3 | gr4) ;;
    *) echo "usage: $0 gr3|gr4 OUT.GRM [options]" >&2; exit 2 ;;
esac
[ -n "$out" ] || { echo "usage: $0 gr3|gr4 OUT.GRM [options]" >&2; exit 2; }
shift 2
[ -f "extras/recipe/recipes-$family.h" ] || { echo "no extras/recipe/recipes-$family.h" >&2; exit 1; }
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cp -R extras/recipe "$work/recipe"
printf '#include "recipes-%s.h"\n' "$family" > "$work/recipe/recipes.h"
mkdir -p "$(dirname "$out")"
grkit module build --dir "$work/recipe" --out "$out" "$@"
