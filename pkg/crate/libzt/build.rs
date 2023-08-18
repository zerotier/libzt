extern crate bindgen;

use std::env;
use std::path::{Path, PathBuf};

fn main() {
    let dir = env::var("CARGO_MANIFEST_DIR").unwrap();
    let lib_dir = Path::new(&dir).join("../../../dist/native/lib").canonicalize().unwrap();

    println!("cargo:rustc-link-search=native={}", lib_dir.to_string_lossy());
    println!("cargo:rustc-link-lib=static=zt");
    println!("cargo:rustc-link-lib=dylib=c++");

    let bindings = bindgen::Builder::default()
        .header("src/include/ZeroTierSockets.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("libzt.rs"))
        .expect("Couldn't write bindings!");
}
