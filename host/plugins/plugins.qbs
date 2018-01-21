import qbs

Project {
    references: [
        "flash",
        "ping",
        "keycode",
        "led",
        "i2c",
    ]

    StaticLibrary {
        name: "plugin.base"
        Depends {name: "Nosumor"}
        cpp.cxxLanguageVersion: "c++11"

        Export {
            Depends {name: "cpp"}
            Depends {name: "Nosumor"}
            cpp.includePaths: ["."]
        }

        files: [
            "plugin.cpp",
            "plugin.h",
        ]
    }

    StaticLibrary {
        name: "plugin.widget"
        Depends {name: "plugin.base"}
        Depends {name: "Qt.widgets"}

        Export {
            Depends {name: "plugin.base"}
        }

        files: [
            "pluginwidget.cpp",
            "pluginwidget.h",
        ]
    }
}
