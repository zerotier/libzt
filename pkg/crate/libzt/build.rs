extern crate bindgen;

use cmake::Config;
use std::env;
use std::path::PathBuf;

fn main() {
    Config::new("src/native").build_target("zt-static").define("ZTS_ENABLE_RUST", "1").out_dir("target").build();

    println!("cargo:rustc-link-search=target/build/lib");
    println!("cargo:rustc-link-lib=static=zt");

    // See here for reasoning: https://flames-of-code.netlify.app/blog/rust-and-cmake-cplusplus/

    let target = env::var("TARGET").unwrap();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=dylib=c++");
    } else if target.contains("linux") {
        println!("cargo:rustc-link-lib=dylib=stdc++");
    } else {
        unimplemented!();
    }

    //println!("cargo:rustc-env=LLVM_CONFIG_PATH=/usr/local/opt/llvm/bin/llvm-config");

    let bindings = bindgen::Builder::default()
        .header("src/native/include/ZeroTierSockets.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
    bindings
        .write_to_file(out_path.join("libzt.rs"))
        .expect("Couldn't write bindings!");
}
