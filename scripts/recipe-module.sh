#!/bin/sh
# Builds the Film module (FILM.GRM) of one camera family: gr3 (GR III,
# GR IIIx) or gr4 (GR IV), from the page extras/film/all. The module is the
# same for each family but its recipes (recipes/<family>.toml, generated into
# recipes-<family>.h of the page).
#
#     sh scripts/recipe-module.sh gr3 all build/gr3/FILM.GRM [grkit module build options]
set -eu
cd "$(dirname "$0")/.."
usage() { echo "usage: $0 gr3|gr4 all OUT.GRM [options]" >&2; exit 2; }
family=${1:-}
page=${2:-}
out=${3:-}
case "$family" in gr3 | gr4) ;; *) usage ;; esac
case "$page" in all) ;; *) usage ;; esac
[ -n "$out" ] || usage
shift 3
[ -f "extras/film/$page/recipes-$family.h" ] || { echo "no extras/film/$page/recipes-$family.h" >&2; exit 1; }
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
# A page includes the shared code from ../.
cp -R extras/film "$work/film"
printf '#include "recipes-%s.h"\n' "$family" > "$work/film/$page/recipes.h"
mkdir -p "$(dirname "$out")"
grkit module build --dir "$work/film/$page" --out "$out" "$@"
