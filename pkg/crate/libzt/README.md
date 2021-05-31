# libzt - Sockets over ZeroTier

`libzt` replicates the functionality of [std::net](https://doc.rust-lang.org/std/net/index.html) but uses [ZeroTier](https://www.zerotier.com) as its transport layer.

Securely connect application instances, physical devices, and virtual devices as if everything is on a single LAN. ZeroTier brings your network into user-space. No root, and no host configuration requirements.

## Usage

Add the following to your `Cargo.toml`:

```toml
[dependencies]
libzt = "0.1.0"
```

## Resources

 - Docs: [docs.zerotier.com](https://docs.zerotier.com/sockets/tutorial.html)
 - Repo: [github.com/zerotier/libzt](https://github.com/zerotier/libzt)
 - Website: [zerotier.com](https://www.zerotier.com/)
