## Example usage of libzt in Swift app via native static library
***

If any of the below instructions seem unclear see [HOWTO: Use a C++ Library from Swift](http://www.swiftprogrammer.info/swift_call_cpp.html) or drop us a message.

 - Build (`make static_lib`) or Download: [libzt.a]()
 - Add `libztWrapper.cpp` and `libztWrapper.hpp` and `libztWrapper.swift` to your project
 - Add `include/` (from the cloned libzt repo) to your projects `Header Search Path`
 - Build 