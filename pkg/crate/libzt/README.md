# libzt - Sockets over ZeroTier

`libzt` replicates the functionality of [std::net](https://doc.rust-lang.org/std/net/index.html) but uses [ZeroTier](https://www.zerotier.com) as its P2P transport layer.

Securely connect application instances, physical devices, and virtual devices as if everything is on a single LAN.

## Dependencies

The `libzt` crate is a binding around a C/C++ native library. You must have this library installed on your system in order for this crate to work. Currently the best way to do this is to install from our Homebrew tap:

```
brew install zerotier/tap/libzt
```

*Note: Windows is untested but support is planned.*

## Usage

Add the following to your `Cargo.toml`:

```toml
[dependencies]
libzt = "0.1.2"
```

## Docs

 - See: [docs.zerotier.com/sockets](https://docs.zerotier.com/sockets/tutorial.html)
