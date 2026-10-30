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
const SHARED: [&str; 6] = ["id", "name", "menu", "effect", "white_balance", "aspect"];
/// The Recipe menus: the menu field and the module folder of each. A menu
/// has at most 7 recipes, as PICK, with Off, is a native settings row of at
/// most 8 values.
const MENUS: [(&str, &str); 2] = [("a", "extras/recipe"), ("b", "extras/recipe-b")];
const MENU_RECIPES: usize = 7;

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
        "menu",
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
            let menu = text(pack, "menu").expect("menu");
            assert!(
                MENUS.iter().any(|(m, _)| *m == menu),
                "{family} {id}: menu {menu}"
            );
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

/// The packs of a menu with an effect: the recipes of its module, in file
/// order.
fn module_recipes(family: &str, menu: &str) -> Vec<toml::Table> {
    packs(family)
        .into_iter()
        .filter(|pack| text(pack, "effect").is_some_and(|e| !e.is_empty()))
        .filter(|pack| text(pack, "menu") == Some(menu))
        .collect()
}

#[test]
fn each_menu_fits_a_settings_row() {
    for family in FAMILIES {
        for (menu, _) in MENUS {
            let count = module_recipes(family, menu).len();
            assert!(
                (1..=MENU_RECIPES).contains(&count),
                "{family} menu {menu}: {count} recipes"
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

/// The C header of the recipes of a family in a menu
/// (<module folder>/recipes-<family>.h).
fn recipe_header(family: &str, menu: &str) -> String {
    let recipes = module_recipes(family, menu);
    let mut out = format!(
        "/* Generated from menu {menu} of recipes/{family}.toml by crates/grkit-core/tests/recipes.rs.\n   Do not edit: run GRKIT_UPDATE_GENERATED=1 cargo test -p grkit-core --test recipes. */\n#ifndef RECIPES_{upper}_H\n#define RECIPES_{upper}_H\n#define RECIPE_COUNT {count}U\nstatic const Recipe recipes[RECIPE_COUNT] = {{\n",
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
        for (menu, folder) in MENUS {
            let path = root().join(format!("{folder}/recipes-{family}.h"));
            let expected = recipe_header(family, menu);
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

/// Each Recipe module lists the recipes of its menu: a PICK value, a menu
/// row, and a tile for each, after Off.
#[test]
fn the_recipe_modules_list_each_recipe() {
    for (menu, folder) in MENUS {
        check_module(menu, folder);
    }
}

fn check_module(menu: &str, folder: &str) {
    let names: Vec<String> = module_recipes("gr4", menu)
        .iter()
        .map(|pack| text(pack, "name").unwrap().to_owned())
        .collect();
    let directory = root().join(folder);
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
        Some(names.len() as i64 + 1),
        "PICK values"
    );
    assert_eq!(
        manifest["tiles"].as_array().unwrap().len(),
        names.len() + 1,
        "tiles"
    );
    for language in ["en", "zh-Hans", "zh-Hant", "ja", "ko"] {
        let text: toml::Table =
            std::fs::read_to_string(directory.join(format!("text/{language}.toml")))
                .unwrap()
                .parse()
                .unwrap();
        let rows: Vec<&str> = text["rows"]
            .as_array()
            .unwrap()
            .iter()
            .map(|r| r.as_str().unwrap())
            .collect();
        assert_eq!(rows.len(), names.len() + 1, "{language}: rows");
        assert_eq!(
            &rows[1..],
            names.iter().map(String::as_str).collect::<Vec<_>>(),
            "{language}: recipe names"
        );
    }
}
