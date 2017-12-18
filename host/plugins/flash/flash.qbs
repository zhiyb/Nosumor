import qbs

Project {
    minimumQbsVersion: "1.7.1"

    DynamicLibrary {
        Depends {name: "Qt.widgets"}
        Depends {name: "Nosumor"}
        Depends {name: "plugin.widget"}

        files: [
            "flash.cpp",
            "flash.h",
            "pluginflash.cpp",
            "pluginflash.h",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: "plugins"
        }
    }
}
