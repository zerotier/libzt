## ZeroTier with Java via JNI
***

### ExampleApp

Copy `zt.jar` file into this directory
Extract shared library from JAR file: `jar xf zt.jar libzt.dylib`
Build ExampleApp: `javac -cp ".:zt.jar" ExampleApp.java`