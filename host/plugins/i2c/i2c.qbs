import qbs

DynamicLibrary {
    name: "plugin.i2c"
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "i2c.cpp",
        "i2c.h",
        "plugini2c.cpp",
        "plugini2c.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "plugins"
    }
}
