import qbs

DynamicLibrary {
    name: "plugin.led"
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "leds.cpp",
        "leds.h",
        "pluginleds.cpp",
        "pluginleds.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "plugins"
    }
}
