extern crate bindgen;

fn main() {
    println!("cargo:rustc-link-lib=zt");
    println!("cargo:rustc-env=LLVM_CONFIG_PATH=/usr/local/opt/llvm/bin/llvm-config");

    //println!("cargo:rerun-if-changed=../../../include/ZeroTierSockets.h");
    //println!("cargo:include=/usr/local/include");

    let bindings = bindgen::Builder::default()
        .header("../../../include/ZeroTierSockets.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file("./src/libzt.rs")
        .expect("Couldn't write bindings!");
}
