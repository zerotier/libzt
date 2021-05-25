# Java example

Build or download the `libzt-${VERSION}.jar`, and copy it to this directory.

```
make
java -cp ".:libzt-${VERSION}.jar" Example server id_path 0123456789abcdef 9997
java -cp ".:libzt-${VERSION}.jar" Example client id_path 0123456789abcdef ip.ip.ip.ip 9997
```

## Links

 - Getting Started: [docs.zerotier.com/sockets](https://docs.zerotier.com/sockets/tutorial.html)
 - Java API: [docs.zerotier.com/sockets-java](https://docs.zerotier.com/sockets-java/)
 - Source [src/bindings/java](../../src/bindings/java)
