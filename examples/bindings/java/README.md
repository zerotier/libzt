## ZeroTier with Java via JNI
***

### Example App

- From libzt main directory, build shared library: `make shared_jni_lib`
- Copy the resultant dynamic library (`*.so` or `*.dylib`) from `build/` to this current directory
- Change to this directory and `make example_java_app`
- Run: `java -cp "." ExampleApp`

### JAR file (with embedded C++ dynamic library)

```
make example_java_app
make copy_dynamic_lib
make jar
```

Notes:

Upon execution, it will load the libzt dynamic library via the `loadLibrary` method and begin generating an identity.

***

More resources on JNI usage:

http://journals.ecs.soton.ac.uk/java/tutorial/native1.1/implementing/index.html
