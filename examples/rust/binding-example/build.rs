fn main() {
	println!("cargo:rustc-flags=-l dylib=c++");
	//println!("cargo:rustc-link-search=.");
}