
#[link(name = "libzt", kind = "dylib")]
extern {
    fn zts_socket(address_family: i32) -> i32;
}

fn main() {
    let x = unsafe { zts_socket(100) };
    println!("zts_socket() = {}", x);
}
