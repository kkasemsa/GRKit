//! The film pack recipes of `recipes/`: one file for each camera family.
//! Both files list the same packs with the same look; only grain may differ.

use std::path::{Path, PathBuf};

const FAMILIES: [&str; 2] = ["gr3", "gr4"];
const GRAINS: [&str; 5] = ["off", "weak", "medium", "strong", "extra-strong"];
const GRAIN_SIZES: [&str; 4] = ["auto", "small", "medium", "large"];
const ASPECTS: [&str; 8] = [
    "native", "xpan", "16:9", "4:3", "2.35:1", "1:1", "5:4", "7:6",
];
const STATUSES: [&str; 4] = ["tested", "grain-unverified", "untested", "needs-effect"];
/// The fields that must be equal in each family: the look itself.
const SHARED: [&str; 6] = ["id", "name", "group", "effect", "white_balance", "aspect"];
/// The recipe groups, in file order.
const GROUPS: [&str; 5] = ["negative", "slide", "cine", "bw", "instant"];
/// The Film pages: the folder under extras/film and the groups of each, as
/// scripts/film-pages.py has them. A page has at most 8 recipes: its tiles,
/// 6400 bytes each, share the 64 KB code segment of the module.
const PAGES: [(&str, &[&str]); 3] = [
    ("neg", &["negative"]),
    ("slide", &["slide", "instant"]),
    ("cine", &["cine", "bw"]),
];
const PAGE_RECIPES: usize = 8;
const LANGUAGES: [&str; 5] = ["en", "zh-Hans", "zh-Hant", "ja", "ko"];

fn root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR")).join("../..")
}

fn packs(family: &str) -> Vec<toml::Table> {
    let path = root().join(format!("recipes/{family}.toml"));
    let text = std::fs::read_to_string(&path).unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    let table: toml::Table = text
        .parse()
        .unwrap_or_else(|e| panic!("{}: {e}", path.display()));
    table["pack"]
        .as_array()
        .expect("a [[pack]] array")
        .iter()
        .map(|pack| pack.as_table().expect("a pack table").clone())
        .collect()
}

fn text<'a>(pack: &'a toml::Table, key: &str) -> Option<&'a str> {
    pack.get(key).map(|value| {
        value
            .as_str()
            .unwrap_or_else(|| panic!("{key} is not a string"))
    })
}

#[test]
fn each_pack_is_valid() {
    let known = [
        "id",
        "name",
        "effect",
        "grain",
        "grain_size",
        "aspect",
        "group",
        "white_balance",
        "status",
    ];
    for family in FAMILIES {
        let packs = packs(family);
        assert!(!packs.is_empty(), "{family}: no packs");
        let mut ids = Vec::new();
        for pack in &packs {
            for key in pack.keys() {
                assert!(known.contains(&key.as_str()), "{family}: unknown key {key}");
            }
            let id = text(pack, "id").expect("id");
            assert!(!ids.contains(&id), "{family}: {id} twice");
            ids.push(id);
            assert!(
                text(pack, "name").is_some_and(|n| !n.is_empty()),
                "{family} {id}: name"
            );
            assert!(
                GRAINS.contains(&text(pack, "grain").expect("grain")),
                "{family} {id}: grain"
            );
            assert!(
                GRAIN_SIZES.contains(&text(pack, "grain_size").expect("grain_size")),
                "{family} {id}: grain_size"
            );
            if let Some(aspect) = text(pack, "aspect") {
                assert!(ASPECTS.contains(&aspect), "{family} {id}: aspect {aspect}");
            }
            let group = text(pack, "group").expect("group");
            assert!(GROUPS.contains(&group), "{family} {id}: group {group}");
            let status = text(pack, "status").expect("status");
            assert!(STATUSES.contains(&status), "{family} {id}: status {status}");
            let effect = text(pack, "effect").expect("effect");
            if effect.is_empty() {
                assert_eq!(
                    status, "needs-effect",
                    "{family} {id}: a pack without an effect needs one"
                );
            } else {
                assert_ne!(status, "needs-effect", "{family} {id}: it has an effect");
                assert!(
                    root()
                        .join("effects")
                        .join(effect)
                        .join("effect.json")
                        .is_file(),
                    "{family} {id}: no effect {effect} in effects/"
                );
            }
        }
    }
}

/// The file lists the groups in GROUPS order, each in one run, so that a
/// page of two groups shows them one after the other.
#[test]
fn the_groups_are_in_file_order() {
    for family in FAMILIES {
        let mut last = 0;
        for pack in packs(family) {
            let group = text(&pack, "group").unwrap();
            let at = GROUPS.iter().position(|g| *g == group).unwrap();
            assert!(
                at >= last,
                "{family} {}: group {group} out of order",
                text(&pack, "id").unwrap()
            );
            last = at;
        }
    }
}

/// Each group is on one page.
#[test]
fn each_group_has_a_page() {
    for group in GROUPS {
        assert_eq!(
            PAGES
                .iter()
                .filter(|(_, groups)| groups.contains(&group))
                .count(),
            1,
            "group {group}"
        );
    }
}

#[test]
fn the_families_share_each_look() {
    let [a, b] = FAMILIES.map(packs);
    assert_eq!(a.len(), b.len(), "the families list different packs");
    for (pa, pb) in a.iter().zip(&b) {
        for key in SHARED {
            assert_eq!(
                pa.get(key),
                pb.get(key),
                "{}: {key} differs between the families",
                text(pa, "id").unwrap_or("?")
            );
        }
        assert_eq!(
            text(pa, "grain_size"),
            text(pb, "grain_size"),
            "{}: grain_size differs",
            text(pa, "id").unwrap_or("?")
        );
    }
}

/// The value of a recipe field in CORE.GRAIN, CORE.GRAIN_SIZE, or CORE.ASPECT.
fn index(values: &[&str], value: &str) -> usize {
    values
        .iter()
        .position(|v| *v == value)
        .expect("a checked value")
}

/// The packs of a page with an effect: the recipes of its module, in file
/// order.
fn module_recipes(family: &str, groups: &[&str]) -> Vec<toml::Table> {
    packs(family)
        .into_iter()
        .filter(|pack| text(pack, "effect").is_some_and(|e| !e.is_empty()))
        .filter(|pack| groups.contains(&text(pack, "group").unwrap()))
        .collect()
}

#[test]
fn each_page_fits_its_module() {
    for family in FAMILIES {
        for (page, groups) in PAGES {
            let count = module_recipes(family, groups).len();
            assert!(
                (1..=PAGE_RECIPES).contains(&count),
                "{family} page {page}: {count} recipes"
            );
        }
    }
}

/// The viewfinder label of a pack: its name in capitals. The module draws
/// capitals, digits, space, '-', '+', and '.'.
fn label(name: &str) -> String {
    let label = name.to_uppercase();
    assert!(
        label
            .chars()
            .all(|c| c.is_ascii_uppercase() || c.is_ascii_digit() || " -+.".contains(c)),
        "{name}: the viewfinder font has no character for it"
    );
    label
}

/// The C header of the recipes of a family on a page
/// (extras/film/<page>/recipes-<family>.h).
fn recipe_header(family: &str, page: &str, groups: &[&str]) -> String {
    let recipes = module_recipes(family, groups);
    let mut out = format!(
        "/* Generated from recipes/{family}.toml (page {page}) by crates/grkit-core/tests/recipes.rs.\n   Do not edit: run GRKIT_UPDATE_GENERATED=1 cargo test -p grkit-core --test recipes. */\n#ifndef RECIPES_{upper}_H\n#define RECIPES_{upper}_H\n#define RECIPE_COUNT {count}U\nstatic const Recipe recipes[RECIPE_COUNT] = {{\n",
        upper = family.to_uppercase(),
        count = recipes.len()
    );
    for pack in &recipes {
        let aspect = text(pack, "aspect").map_or("RECIPE_NO_ASPECT".to_owned(), |a| {
            index(&ASPECTS, a).to_string()
        });
        out.push_str(&format!(
            "    {{\"{}\", \"{}\", {}, {}, {}, 0}},\n",
            text(pack, "effect").unwrap(),
            label(text(pack, "name").unwrap()),
            index(&GRAINS, text(pack, "grain").unwrap()),
            index(&GRAIN_SIZES, text(pack, "grain_size").unwrap()),
            aspect
        ));
    }
    out.push_str("};\n#endif\n");
    out
}

#[test]
fn the_recipe_headers_follow_the_recipes() {
    let update = std::env::var_os("GRKIT_UPDATE_GENERATED").is_some();
    for family in FAMILIES {
        for (page, groups) in PAGES {
            let path = root().join(format!("extras/film/{page}/recipes-{family}.h"));
            let expected = recipe_header(family, page, groups);
            if update {
                std::fs::write(&path, &expected).unwrap();
            }
            let actual = std::fs::read_to_string(&path).unwrap_or_default();
            assert!(
                actual == expected,
                "{} is out of date: run GRKIT_UPDATE_GENERATED=1 cargo test -p grkit-core --test recipes",
                path.display()
            );
        }
    }
}

/// Each Film page lists the recipes of its groups: a PICK value, a menu
/// row, and a tile for each, after Off.
#[test]
fn the_film_pages_list_each_recipe() {
    for (page, groups) in PAGES {
        check_page(page, groups);
    }
}

fn check_page(page: &str, groups: &[&str]) {
    let recipes = module_recipes("gr4", groups);
    let directory = root().join(format!("extras/film/{page}"));
    let manifest: toml::Table = std::fs::read_to_string(directory.join("module.toml"))
        .unwrap()
        .parse()
        .unwrap();
    let pick = manifest["settings"]
        .as_array()
        .unwrap()
        .iter()
        .find(|s| s["key"].as_str() == Some("PICK"))
        .expect("a PICK setting");
    assert_eq!(
        pick["values"].as_integer(),
        Some(recipes.len() as i64 + 1),
        "{page}: PICK values"
    );
    assert_eq!(
        manifest["tiles"].as_array().unwrap().len(),
        recipes.len() + 1,
        "{page}: tiles"
    );
    for language in LANGUAGES {
        let file: toml::Table =
            std::fs::read_to_string(directory.join(format!("text/{language}.toml")))
                .unwrap()
                .parse()
                .unwrap();
        let rows: Vec<&str> = file["rows"]
            .as_array()
            .unwrap()
            .iter()
            .map(|r| r.as_str().unwrap())
            .collect();
        assert_eq!(rows.len(), recipes.len() + 1, "{page} {language}: rows");
        let expected: Vec<&str> = recipes
            .iter()
            .map(|pack| text(pack, "name").unwrap())
            .collect();
        assert_eq!(&rows[1..], expected, "{page} {language}: recipe rows");
    }
}
