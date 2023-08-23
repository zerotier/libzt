# Java example

## Build the JAR

From the top-level `libzt` directory:

```
./build.sh host-jar
```

Should result in something like:

```
dist
└── linux-x64-jar-release
    └── pkg
        └── libzt-1.8.10.jar
```

Copy the JAR to the working directory:

*NOTE: If you've built multiple variants of the JAR such as debug/release or mac/linux you will need to specify their exact path in the following command. If not, you can use the wildcard form:*


```
cp -f dist/*/pkg/*.jar examples/java/libzt.jar
```

Navigate to the `examples/java` directory and extract the `libzt.so|dylib|dll` dynamic library from the JAR into the working directory:

```
jar xf *.jar libzt.dylib libzt.so libzt.dll
```

## Build the example app

```
javac -cp *.jar Example.java 
```

## Run the example app

```
java -Djava.library.path=. -cp ".:libzt.jar" Example server id_path 0123456789abcdef 9997
java -Djava.library.path=. -cp ".:libzt.jar" Example client id_path 0123456789abcdef ip.ip.ip.ip 9997
```

## Clean

```
rm -rf *.dylib *.so *.jar *.dll *.class
```

## Links

 - Getting Started: [docs.zerotier.com/sockets](https://docs.zerotier.com/sockets/tutorial.html)
 - Java API: [docs.zerotier.com/sockets-java](https://docs.zerotier.com/sockets-java/)
 - Source [src/bindings/java](../../src/bindings/java)
