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

copy_dynamic_lib:
	cp ../../../$(BUILD)/$(SHARED_LIB) .

clean:
	-find . -type f \( -name '*.class' \) -delete	