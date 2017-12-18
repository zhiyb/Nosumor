import qbs

Project {
    minimumQbsVersion: "1.7.1"

    DynamicLibrary {
        Depends {name: "Qt.widgets"}
        Depends {name: "Nosumor"}
        Depends {name: "plugin.widget"}

        files: [
            "keycode.cpp",
            "keycode.h",
            "pluginkeycode.cpp",
            "pluginkeycode.h",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: "plugins"
        }
    }
}
