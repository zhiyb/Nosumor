import qbs

DynamicLibrary {
    name: "plugin.motion"
    Depends {name: "Qt.widgets"}
    Depends {name: "Qt.opengl"}
    Depends {name: "Nosumor"}
    Depends {name: "plugin.widget"}

    files: [
        "motionwidget.cpp",
        "motionwidget.h",
        "motionglwidget.cpp",
        "motionglwidget.h",
        "pluginmotion.cpp",
        "pluginmotion.h",
    ]

    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "plugins"
    }
}
