import qbs

DynamicLibrary {
    name: "plugin.keycode"
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "inputcapture.cpp",
        "inputcapture.h",
        "keycode.cpp",
        "keycode.h",
        "pluginkeycode.cpp",
        "pluginkeycode.h",
        "usage.cpp",
        "usage.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "plugins"
    }
}
