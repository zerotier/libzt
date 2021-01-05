debug:
	cd ../../ && make host_pinvoke_debug
	cp -f ../../lib/debug/linux-x86_64/libzt.so .
	mono-csc -out:example.exe *.cs

release:
	cd ../../ && make host_pinvoke_release
	cp -f ../../lib/release/linux-x86_64/libzt.so .
	mono-csc -out:example.exe *.cs

clean:
	rm -rf libzt.* example.exe