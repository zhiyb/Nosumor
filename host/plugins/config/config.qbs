import qbs

DynamicLibrary {
    name: "plugin.config"
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "config.cpp",
        "config.h",
        "pluginconfig.cpp",
        "pluginconfig.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "plugins"
    }
}
