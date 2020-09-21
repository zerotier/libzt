### How to compile/use this example:

1. Follow "Building from source" section from README.md in the root of this git repository
(the linking step/example from that section does not apply to this example)

2. Create the java library .jar file:

```
make host_jar_release
```

for other `host_jar` variants see Makefile in the root of this git repository

2. Copy the output .jar to this directory:

```
cp lib/lib/release/linux-x86_64/zt.jar examples/java/simpleExample/
```

3. Now you can compile this example:

```
cd src
javac -cp ../zt.jar ./com/zerotier/libzt/javasimpleexample/*.java 
```
