{
  "targets": [
    {
      "include_dirs": ["libzt/build/darwin", "libzt/include", "libzt/zto/include"],
      "libraries": ["<!(pwd)/libzt/build/darwin/libzt.a"],
      "includes": ["auto.gypi"],
      "sources": ["binding.cc"]
    }
  ],
  "includes": ["auto-top.gypi"]
}
