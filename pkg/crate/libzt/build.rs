extern crate bindgen;

use std::env;
use std::path::PathBuf;

fn main() {
    if cfg!(windows) {
        let dist_dir = env::var("LIBZT_DIST_DIR").expect(
            "In Windows, please compile libzt and point the LIBZT_DIST_DIR environment variable \
             to the libzt dist dir, such as 'D:/code/libzt/dist/win-x64-host-release'",
        );
        println!(r"cargo:rustc-link-search={}\lib", dist_dir);
        println!("cargo:rustc-link-lib=libzt");
        println!("cargo:rustc-link-lib=shell32");
        println!("cargo:rustc-link-lib=IPHLPAPI");
        println!("cargo:rustc-link-lib=Shlwapi");
    } else {
        println!("cargo:rustc-link-lib=zt");
    }

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
