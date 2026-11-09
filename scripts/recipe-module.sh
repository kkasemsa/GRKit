#!/bin/sh
# Builds a Film page module of one camera family: gr3 (GR III, GR IIIx) or
# gr4 (GR IV), and one page: neg (Negative, NEGATIVE.GRM), slide (Slide,
# SLIDE.GRM), or cine (Cine / B&W, CINEBW.GRM), from extras/film/<page>. A
# module is the same for each family but its recipes (the groups of the page
# in recipes/<family>.toml, generated into recipes-<family>.h of the page).
#
#     sh scripts/recipe-module.sh gr3 neg build/gr3/NEGATIVE.GRM [grkit module build options]
set -eu
cd "$(dirname "$0")/.."
usage() { echo "usage: $0 gr3|gr4 neg|slide|cine OUT.GRM [options]" >&2; exit 2; }
family=${1:-}
page=${2:-}
out=${3:-}
case "$family" in gr3 | gr4) ;; *) usage ;; esac
case "$page" in neg | slide | cine) ;; *) usage ;; esac
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
