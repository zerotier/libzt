{
	"targets": [
		{
			"include_dirs": [
				"libzt/lib/release/linux-x86_64",
				"libzt/include",
			],
			"includes": [
				"auto.gypi"
			],
			"sources": [
				"binding.cc"
			],
			"conditions":[
				["OS=='linux' and target_arch=='x64'", {
					"libraries": [ "<(module_root_dir)/libzt/lib/release/linux-x86_64/libzt.so" ]
				}],
				["OS=='mac' and target_arch=='x64'", {
					"libraries": [ "<(module_root_dir)/libzt/lib/release/macos-x86_64/libzt.a" ]
				}],
				["OS=='win'", {
					
				}]
			]
		}
	],
	"includes": [
		"auto-top.gypi"
	]
}
