// swift-tools-version:5.3

import PackageDescription

let package = Package(
    name: "zt",
    products: [
        .library(
            name: "zt",
            targets: ["zt"]),
    ],
    targets: [
        .target(
            name: "zt",
            path: "",
            exclude: [
                "build",
                "examples",
                "ext/concurrentqueue",
                "ext/lwip",
                "ext/lwip-contrib",
                "ext/ZeroTierOne/artwork",
                "ext/ZeroTierOne/attic",
                "ext/ZeroTierOne/controller",
                "ext/ZeroTierOne/debian",
                "ext/ZeroTierOne/ext",
                "ext/ZeroTierOne/java",
                "ext/ZeroTierOne/java",
                "ext/ZeroTierOne/macui",
                "ext/ZeroTierOne/osdep",
                "ext/ZeroTierOne/rule-compiler",
                "ext/ZeroTierOne/service",
                "ext/ZeroTierOne/windows",
                "ports",
                "src/java",
            ],
            sources: [
                "src/Controls.cpp",
                "src/Events.cpp",
                "src/NodeService.cpp",
                "src/Sockets.cpp",
                "src/VirtualTap.cpp",
                
                "ext/ZeroTierOne/node"
            ],
            cSettings: [
                .headerSearchPath("src"),
                .headerSearchPath("ext/concurrentqueue"),
                .headerSearchPath("ext/lwip/src/include"),
                .headerSearchPath("ext/lwip-contrib/ports/unix/port/include"),
                .headerSearchPath("ext/ZeroTierOne/node"),
                .headerSearchPath("ext/ZeroTierOne/osdep")
            ],
            cxxSettings: [
                .unsafeFlags([ "-DOMIT_JSON_SUPPORT=1", "-std=c++11" ])
            ])
    ]
)
