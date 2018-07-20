OSTYPE=$(shell uname -s | tr '[A-Z]' '[a-z]')
BUILD=build/$(OSTYPE)

ifeq ($(OSTYPE),darwin)
SHARED_LIB=libzt.dylib
endif
ifeq ($(OSTYPE),linux)
SHARED_LIB=libzt.so
endif

example_java_app:
	javac *.java

example_java_app_1.6:
	javac -source 1.6 -bootclasspath /usr/lib/jvm/java-1.6.0-openjdk-1.6.0.41.x86_64/jre/lib/rt.jar -target 1.6 *.java

copy_dynamic_lib:
	cp ../../bin/lib/$(SHARED_LIB) .

jar:
	jar cf libzt.jar libzt.dylib zerotier/ZeroTier.class

clean:
	-rm -rf *.jar *.dylib *.so
	-find . -type f \( -name '*.class' \) -delete	
