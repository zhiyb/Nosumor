import qbs

StaticLibrary {
    name: "plugin.info"
    Depends {name: "Qt.widgets"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "plugininfo.cpp",
        "plugininfo.h",
    ]

    Export {
        Depends {name: "cpp"}
        cpp.includePaths: ["."]
    }
}
