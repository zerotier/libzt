{
	"targets": [
		{
			"target_name": "binding",
			"include_dirs": [
				"<(module_root_dir)/libzt/include"
			],
			"includes": [
				"auto.gypi"
			],
			"sources": [
				"binding.cc"
			],
			"conditions":[
				["OS=='linux' and target_arch=='x64'", {
					"libraries": [ "<(module_root_dir)/libzt/dist/linux-x64-host-release/libzt.a" ]
				}],
				["OS=='mac' and target_arch=='x64'", {
					"libraries": [ "<(module_root_dir)/libzt/dist/release/macos-x86_64/libzt.a" ]
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
