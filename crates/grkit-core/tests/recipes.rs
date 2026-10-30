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
const SHARED: [&str; 5] = ["id", "name", "effect", "white_balance", "aspect"];

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
