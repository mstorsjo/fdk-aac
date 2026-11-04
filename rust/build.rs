fn main() {
    // Rust version dependencies
    if rustversion::cfg!(stable) {
        println!("cargo:rustc-cfg=is_stable");
    }

    if rustversion::cfg!(beta) {
        println!("cargo:rustc-cfg=is_beta");
    }

    if rustversion::cfg!(nightly) {
        println!("cargo:rustc-cfg=is_nightly");
    }
}
