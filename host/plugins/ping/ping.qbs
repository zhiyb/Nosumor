import qbs

DynamicLibrary {
    name: "plugin.ping"
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "pingwidget.cpp",
        "pingwidget.h",
        "pluginping.cpp",
        "pluginping.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "plugins"
    }
}
