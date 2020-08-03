{
	"targets": [
		{
			"include_dirs": [
				"libzt/lib/debug/linux-x86_64",
				"libzt/include"
			],
			"libraries": [
				"<!(pwd)/libzt/lib/release/linux-x86_64/libzt.so"
			],
			"includes": [
				"auto.gypi"
			],
			"sources": [
				"binding.cc"
			]
		}
	],
	"includes": [
		"auto-top.gypi"
	]
}
