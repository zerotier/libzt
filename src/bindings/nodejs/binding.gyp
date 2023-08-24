{
    "targets": [
        {
            "target_name": "zts",
            "cflags!": ["-fno-exceptions"],
            "cflags_cc!": ["-fno-exceptions"],
            "sources": ["src/native/binding.cc"],
            "include_dirs": [
                "<!(node -p \"require('node-addon-api').include_dir\")",
                "<(module_root_dir)/libzt/include",
            ],
            "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
            "conditions": [
                [
                    "OS=='linux' and target_arch=='x64'",
                    {
                        "libraries": [
                            "<(module_root_dir)/libzt/dist/linux-x64-host-release/lib/libzt.a"
                        ]
                    },
                ],
                ["OS=='mac' and target_arch=='x64'", {}],
                ["OS=='win'", {}],
            ],
        }
    ]
}
